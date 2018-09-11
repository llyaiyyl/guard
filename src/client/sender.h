#ifndef SENDER_H
#define SENDER_H

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

class sender
{
public:
    sender(uint16_t port);
    ~sender();

    void run(void);
    void wait(void);
    void quit(void);
private:
    static void * thread_poll(void * pdata);
    static void log_error(int ret);

private:
    RTPSession m_sess;
    pthread_t m_tid;
};

#endif // SENDER_H
