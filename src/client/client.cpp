#include <unistd.h>
#include <string.h>

#include "client.h"

using namespace std;

client::client(const std::string &sn, session *sess, videocap *vc)
{
    sn_ = sn;
    sess_ = sess;
    vc_ = vc;

    tid_ = 0;
    loop_exit_ = false;
    packet_max_ = 1200;
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
}

void client::run(void)
{
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

void * client::thread_poll(void *pdata)
{
    AVPacket packet;
    client * ptr = (client *)pdata;
    videocap * vc = ptr->get_videocap();
    int video_stream, audio_stream, ret;

    video_stream = vc->get_video_stream();
    audio_stream = vc->get_audio_stream();
    while(ptr->loop_exit_ == false) {
        ret = vc->read_packet(&packet);
        if(0 == ret) {
            if(packet.stream_index == video_stream) {
                ptr->send_packet(packet.data, packet.size);
                // cout << "send packet: " << packet.size << endl;
            } else if (packet.stream_index == audio_stream) {
                // cout << "get audio frame" << endl;
            }

            av_packet_unref(&packet);
        } else
            cout << "read packet error: " << ret << endl;
    }

    pthread_exit((void *)0);
}



