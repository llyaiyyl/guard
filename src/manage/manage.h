#ifndef MANAGE_H
#define MANAGE_H

#include <iostream>
#include <pthread.h>

#include "session.h"

using namespace std;
using namespace jrtplib;

class manage
{
public:
    /*
     * init with local port
     */
    manage(session * sess);
    ~manage();

    void run();
    void quit();

private:
    static void * thread_poll(void * pdata);

private:
    session * sess_;
    pthread_t tid_;
    bool isrecv_;
    bool loop_exit_;
};

#endif // RECEIVER_H


