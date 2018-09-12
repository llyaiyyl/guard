#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"

enum THREAD_TYPE {
    THREAD_TYPE_NET_MANAGE = 1,
    THREAD_TYPE_VCAP_RTSP
};

typedef struct {
    int exitloop;
    int dst_w, dst_h;
    int frame_number;
    char url[256];

    void (* pf_packet)(AVPacket * ptr_packet);
    void (* pf_frame)(AVFrame * ptr_frame);
    void (* pf_frame_rgb)(uint8_t * buff, int32_t linesize, int32_t w, int32_t h, int32_t frame_number);
} thread_videocap_arg_t;

typedef struct {
    enum THREAD_TYPE type;
    pthread_t tid;
    thread_videocap_arg_t targ;
} thread_data_t;

typedef struct {
    list_t * list_thread;
} g_data_t;


static void cb_packet_dump(AVPacket * ptr_packet)
{
    printf("packet: %d bytes, pts: %ld, dts: %ld, duration: %ld\n", ptr_packet->size,
           ptr_packet->pts, ptr_packet->dts,
           ptr_packet->duration);
}
static void cb_frame_dump(AVFrame * ptr_frame)
{
    int32_t i = 0, sum = 0;

    printf("frame: ");
    while(ptr_frame->data[i]) {
        printf("%d ", ptr_frame->linesize[i]);
        sum += ptr_frame->linesize[i];
        i++;
    }
    printf("sum per line: %d bytes\n", sum);
}
static void cb_pgm_save(uint8_t * buff, int32_t linesize, int32_t w, int32_t h, int32_t frame_number)
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



static void * thread_videocap_rtsp(void * pdata)
{
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVCodec *pCodec = NULL;
    AVPacket packet;
    AVFrame *frame;
    struct SwsContext *sws_ctx;
    AVDictionary * opts = NULL;
    uint8_t *dst_data[4];
    int dst_linesize[4];
    int ret, i, videoStream;
    thread_videocap_arg_t * ptr_arg;

    ptr_arg = (thread_videocap_arg_t *)pdata;

    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    if((ret = avformat_open_input(&pFormatCtx, ptr_arg->url, NULL, &opts))) {
        printf("avformat_open_input error %d\n", ret);
        return (void *)0;
    }

    if((ret = avformat_find_stream_info(pFormatCtx, NULL)) < 0) {
        printf("avformat_find_stream_info error %d\n", ret);
        goto err0;
    }
    av_dump_format(pFormatCtx, 0, ptr_arg->url, 0);

    videoStream = -1;
    for(i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    if(videoStream == -1) {
        ret = videoStream;
        goto err0;
    }

    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
    if(NULL == pCodec) {
        printf("avcodec_find_decoder error\n");
        goto err0;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("avcodec_open2 error\n");
        goto err0;
    }
    printf("video: %d x %d, pix_fmt: %d\n", pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt);
    if(ptr_arg->dst_w <= 0 || ptr_arg->dst_h <= 0) {
        ptr_arg->dst_w = pCodecCtx->width;
        ptr_arg->dst_h = pCodecCtx->height;
    }

    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
            ptr_arg->dst_w, ptr_arg->dst_h, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, NULL, NULL, NULL);
    if(NULL == sws_ctx) {
        printf("sws_getContext error\n");
        goto err1;
    }
    ret = av_image_alloc(dst_data, dst_linesize, ptr_arg->dst_w, ptr_arg->dst_h, AV_PIX_FMT_RGB24, 1);
    if(ret < 0) {
        printf("av_image_alloc error\n");
        goto err1;
    }

    frame = av_frame_alloc();
    while(av_read_frame(pFormatCtx, &packet) >= 0 && (0 == ptr_arg->exitloop)) {
        if(packet.stream_index == videoStream) {
            // decode packet to frame
            ret = avcodec_send_packet(pCodecCtx, &packet);
            if(ret < 0) {
                printf("avcodec_send_packet error %d\n", ret);
                goto err1;
            }
            ret = avcodec_receive_frame(pCodecCtx, frame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                continue;
            else if(ret < 0) {
                printf("avcodec_receive_frame error %d\n", ret);
                goto err1;
            }
            ptr_arg->frame_number = pCodecCtx->frame_number;

            // callback function
            if(ptr_arg->pf_packet)
                ptr_arg->pf_packet(&packet);
            if(ptr_arg->pf_frame)
                ptr_arg->pf_frame(frame);
            if(ptr_arg->pf_frame_rgb) {
                ret = sws_scale(sws_ctx, (const uint8_t * const *)frame->data, frame->linesize, 0, frame->height,
                        dst_data, dst_linesize);
                if(ret > 0)
                    ptr_arg->pf_frame_rgb(dst_data[0], dst_linesize[0], ptr_arg->dst_w, ptr_arg->dst_h, ptr_arg->frame_number);
            }
        }
    }

err1:
    av_frame_free(&frame);
    av_freep(&dst_data[0]);
    avcodec_close(pCodecCtx);
err0:
    avformat_close_input(&pFormatCtx);
    return (void *)0;
}

static void * thread_net_manage(void * pdata)
{
    int fd, ret;
    struct sockaddr_in	ser_addr;
    ssize_t recv_n;

    static char recv_buff[1000];
    static char send_buff[1000];

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == fd) {
        printf("can't create socket\n");
        goto exit;
    }
    bzero(&ser_addr, sizeof(struct sockaddr_in));
    ser_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.0.197", &ser_addr.sin_addr.s_addr);
    ser_addr.sin_port = htons(9000);

again:
    do {
        ret = connect(fd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in));
        if(-1 == ret){
            printf("connect error, wait 10s\n");
            sleep(10);
        }
    } while(0 != ret);
    while(1) {
        recv_n = recv(fd, recv_buff, 1000, 0);
        if(-1 == recv_n) {
            if(errno != EINTR) {
                printf("recv data from client error\n");
                sleep(10);
                goto again;
            } else
                continue ;
        } else if(0 == recv_n) {
            printf("client close\n");
            sleep(10);
            goto again;
        }

        recv_buff[recv_n] = 0;
        printf("%s\n", recv_buff);

        if(0 == strcmp(recv_buff, "list")){

        }
        else {

        }
    }

exit:
    return (void *)0;
}

int main(int argc, char * argv[])
{
    g_data_t g_data;
    thread_data_t * ptr_td;
    int ret, ch;
    list_iterator_t * it;
    list_node_t * node;

    memset(&g_data, 0, sizeof(g_data_t));
    g_data.list_thread = list_new();

    // rtsp vcap thread
    ptr_td = (thread_data_t *)malloc(sizeof(thread_data_t));
    memset(ptr_td, 0, sizeof(thread_data_t));
    ptr_td->type = THREAD_TYPE_VCAP_RTSP;
    strcpy(ptr_td->targ.url, argv[1]);
    ptr_td->targ.pf_packet = cb_packet_dump;
    ret = pthread_create(&(ptr_td->tid), NULL, thread_videocap_rtsp, &(ptr_td->targ));
    if(0 != ret){
        printf("pthread_create error %d\n", ret);
        free(ptr_td);
    } else {
        list_lpush(g_data.list_thread, list_node_new(ptr_td));
    }

    // net manage thread
    ptr_td = (thread_data_t *)malloc(sizeof(thread_data_t));
    memset(ptr_td, 0, sizeof(thread_data_t));
    ptr_td->type = THREAD_TYPE_NET_MANAGE;
    ret = pthread_create(&(ptr_td->tid), NULL, thread_net_manage, NULL);
    if(0 != ret){
        printf("pthread_create error %d\n", ret);
        free(ptr_td);
    } else {
        list_lpush(g_data.list_thread, list_node_new(ptr_td));
    }

    // wait key to exit
    while(1){
        ch = getchar();
        if('q' == ch){
            break;
        }
    }

    // destroy all thread
    it = list_iterator_new(g_data.list_thread, LIST_HEAD);
    while((node = list_iterator_next(it))){
        ptr_td = (thread_data_t *)node->val;

        if(ptr_td->type == THREAD_TYPE_VCAP_RTSP){
            ptr_td->targ.exitloop = 1;
            pthread_join(ptr_td->tid, NULL);

            free(ptr_td);
            ptr_td = NULL;
        }

        if(ptr_td->type == THREAD_TYPE_NET_MANAGE){
            pthread_cancel(ptr_td->tid);
            pthread_join(ptr_td->tid, NULL);

            free(ptr_td);
            ptr_td = NULL;
        }
    }
    list_iterator_destroy(it);
    list_destroy(g_data.list_thread);

    return 0;
}






















