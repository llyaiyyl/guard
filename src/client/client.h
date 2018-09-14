#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <pthread.h>
#include "session.h"

using namespace std;

class client
{
public:
    client(string name, session * sess);
    ~client();

    void run(void);
    void quit(void);
    const char * get_name();
private:
    static void * thread_poll(void * pdata);

private:
    session * sess_;
    pthread_t tid_;
    bool loop_exit_;
    string name_;
};

#endif // SENDER_H
