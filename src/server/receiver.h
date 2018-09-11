#ifndef RECEIVER_H
#define RECEIVER_H

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

class receiver
{
public:
    receiver(int16_t port);
    ~receiver();

    void run();

private:
    static void * thread_poll(void * pdata);
    static void log_error(int ret);

private:
    RTPSession m_sess;
    pthread_t m_tid;
};

#endif // RECEIVER_H


