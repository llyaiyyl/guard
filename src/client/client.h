#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <string>
#include <list>
#include <queue>

extern "C" {
#include <pthread.h>
}

#include "session.h"
#include "videocap.h"


class video_packet
{
public:
    video_packet(uint8_t * p, int size);
    ~video_packet();

    uint8_t * data_;
    int size_;
};


class client
{
public:
    client(const std::string &sn, session * sess, videocap * vc);
    ~client();

    void run(void);
    videocap * get_videocap(void);

private:
    void send_packet(const void * data, size_t len);
    void push_packet(const AVPacket * pkt);
    void process_packet(const AVPacket * pkt);
    void pgm_save(uint8_t * buff, int32_t linesize, int32_t w, int32_t h, int32_t frame_number);

    static void * thread_poll(void * pdata);

private:
    std::string sn_;
    session * sess_;
    videocap * vc_;

    pthread_t tid_;
    bool loop_exit_;
    size_t packet_max_;

    std::queue<video_packet *> queue_video_packet_, queue_video_buff_;
    pthread_mutex_t lock_;
};

#endif // SENDER_H
