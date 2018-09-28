#include "client.h"

#include "opencv2/opencv.hpp"

extern "C" {
#include <stdio.h>
#include <string.h>
#include <unistd.h>
}

using namespace std;
using namespace cv;

/*
 * packet
 */
video_packet::video_packet(uint8_t *p, int size)
{
    data_ = new uint8_t [size];
    memcpy(data_, p, size);
}
video_packet::~video_packet()
{
    delete [] data_;
}


/*
 * client
 */
client::client(const std::string &sn, session *sess, videocap *vc)
{
    sn_ = sn;
    sess_ = sess;
    vc_ = vc;

    tid_ = 0;
    loop_exit_ = false;
    packet_max_ = 1200;

    pthread_mutex_init(&lock_, NULL);
}

client::~client()
{
    if(tid_) {
        loop_exit_ = true;
        pthread_join(tid_, NULL);
        tid_ = 0;
        loop_exit_ = false;
    }

    if(sess_) {
        delete sess_;
        sess_ = NULL;
    }

    if(vc_) {
        delete vc_;
        vc_ = NULL;
    }

    while(queue_video_packet_.size()) {
        video_packet * p = queue_video_packet_.front();
        delete p;
        queue_video_packet_.pop();
    }

    pthread_mutex_destroy(&lock_);
}

void client::run(void)
{
    namedWindow("video", WINDOW_NORMAL);
    resizeWindow("video", 320, 240);

    pthread_create(&tid_, NULL, thread_poll, this);
    cout << "client " << sn_ << " has run" << endl;
}

videocap * client::get_videocap()
{
    return vc_;
}


void client::send_packet(const void *data, size_t len)
{
    int seq, index;
    const uint8_t * ptr = (const uint8_t *)data;

    if((len % packet_max_) == 0) {
        seq = len / packet_max_;
    } else {
        seq = (len / packet_max_) + 1;
    }

    index = 0;
    while(len) {
        if(len <= packet_max_) {
            sess_->SendPacketEx((const void *)(ptr + index), len, 96, false, 10, seq, NULL, 0);
            break;
        } else {
            sess_->SendPacketEx((const void *)(ptr + index), packet_max_, 96, false, 0, seq, NULL, 0);
            len -= packet_max_;
            index += packet_max_;
        }

        seq--;
    }
}

void client::push_packet(const AVPacket *pkt)
{
    pthread_mutex_lock(&lock_);
    if(queue_video_packet_.size() > (5 * vc_->get_fps())) {
        video_packet * pvp =  queue_video_packet_.front();
        delete pvp;
        queue_video_packet_.pop();
    }
    queue_video_packet_.push(new video_packet(pkt->data, pkt->size));
    pthread_mutex_unlock(&lock_);
}

void client::process_packet(const AVPacket * pkt)
{
    static int save_delay_num = 0;
    int ret;

    // send packet to server
    send_packet(pkt->data, pkt->size);

    // decode video packet and analyse it
    uint8_t * data;
    int linesize;
    ret = vc_->read_rgbpic(pkt, &data, &linesize);
    if(-1 != ret) {
        // pgm_save(data, linesize, vc_->get_width(), vc_->get_height(), ret);
        Mat img(vc_->get_height(), vc_->get_width(), CV_8UC3, data);
        imshow("video", img);
    }

    while(save_delay_num) {
        save_delay_num--;
    }
}

void client::pgm_save(uint8_t * buff, int32_t linesize, int32_t w, int32_t h, int32_t frame_number)
{
    FILE *f;
    int i;
    char filename[128];

    snprintf(filename, sizeof(filename), "%s-%d.ppm", "pic", frame_number);

    f = fopen(filename,"w");
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (i = 0; i < h; i++)
        fwrite(buff + i * linesize, 1, w * 3, f);
    fclose(f);

    printf("saving frame %s\n", filename);
    fflush(stdout);
}

void * client::thread_poll(void *pdata)
{
    client * ptr = (client *)pdata;

    AVPacket packet;
    videocap * vc = ptr->get_videocap();
    int video_stream, audio_stream, ret;

    video_stream = vc->get_video_stream();
    audio_stream = vc->get_audio_stream();
    videocap::url_type type = vc->get_url_type();
    while(ptr->loop_exit_ == false) {
        ret = vc->read_packet(&packet);
        if(0 == ret) {
            if(packet.stream_index == video_stream) {
                // add video packet into queue, buffer 5s data
                ptr->push_packet(&packet);

                // process packet
                ptr->process_packet(&packet);
            } else if (packet.stream_index == audio_stream) {
            }

            av_packet_unref(&packet);
        } else
            cout << "read packet error: " << ret << endl;

        if(videocap::url_type_file == type) {
            // usleep((1000.0 / vc->get_fps()) * 1000);
            waitKey(15);
        } else {
            waitKey(1);
        }
    }

    pthread_exit((void *)0);
}










