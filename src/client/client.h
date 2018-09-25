#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <string>
#include <pthread.h>

#include "session.h"
#include "videocap.h"

class client
{
public:
    client(const std::string &sn, session * sess, videocap * vc);
    ~client();

    void run(void);
    videocap * get_videocap(void);

private:
    void send_packet(const void * data, size_t len);
    static void * thread_poll(void * pdata);

private:
    std::string sn_;
    session * sess_;
    videocap * vc_;

    pthread_t tid_;
    bool loop_exit_;
    size_t packet_max_;
};

#endif // SENDER_H
