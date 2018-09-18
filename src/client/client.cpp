#include <unistd.h>
#include <string.h>

#include "client.h"

client::client(string name, session *sess)
{
    sess_ = sess;
    tid_ = 0;
    loop_exit_ = false;
    name_ = name;
}

client::~client()
{
    if(tid_) {
        loop_exit_ = true;
        pthread_join(tid_, NULL);

        tid_ = 0;
        loop_exit_ = false;

        delete sess_;
        sess_ = NULL;
    }
}

void client::run(void)
{
    pthread_create(&tid_, NULL, thread_poll, this);
    cout << "client " << name_ << " has run" << endl;
}

void client::quit(void)
{
    if(tid_) {
        loop_exit_ = true;
        pthread_join(tid_, NULL);

        tid_ = 0;
        loop_exit_ = false;

        delete sess_;
        sess_ = NULL;
    }
}

const char *client::get_name()
{
    return name_.c_str();
}


void * client::thread_poll(void *pdata)
{
    client * ptr = (client *)pdata;
    session * sess = ptr->sess_;
    uint32_t num = 0;
    char sbuff[1024];

    while(ptr->loop_exit_ == false) {
        sprintf(sbuff, "%d: Don't think you are, Know you are", ++num);
        sess->SendPacket(sbuff, strlen(sbuff));
        cout << "client " << sess->GetLocalSSRC() << ": " << sbuff << endl;

        RTPTime::Wait(RTPTime(0, 50 * 1000));
    }

    pthread_exit((void *)0);
}



