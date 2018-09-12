#ifndef MANAGE_H
#define MANAGE_H

#include <iostream>
#include <pthread.h>

#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtplibraryversion.h"
#include "rtppacket.h"
#include "rtpsourcedata.h"

using namespace std;
using namespace jrtplib;

class manage
{
public:
    /*
     * init with local port
     */
    manage(int16_t port);
    ~manage();

    void run();
    void stop();

private:
    static void * thread_poll(void * pdata);
    static void log_error(int ret);

private:
    RTPSession sess_;
    pthread_t tid_;
    bool isrecv_;
    bool loop_exit_;
};

#endif // RECEIVER_H


