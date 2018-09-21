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
    const char * get_name(void);

    void send_packet(const void * data, size_t len);
private:
    static void * thread_poll(void * pdata);

private:
    session * sess_;
    pthread_t tid_;
    bool loop_exit_;
    string name_;
    size_t max_;
};

#endif // SENDER_H
