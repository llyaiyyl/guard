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

#include "receiver.h"

using namespace std;
using namespace jrtplib;

class sender
{
public:
    sender(receiver * p, uint16_t port);
    ~sender();

    void run();
    void stop();
private:
    static void * thread_poll(void * pdata);
    static void log_error(int ret);

private:
    RTPSession sess_;
    pthread_t tid_;
    receiver * rece_;
};

#endif // SENDER_H
