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
    loop_exit_ = true;
    pthread_join(tid_, NULL);

    tid_ = 0;
    loop_exit_ = false;

    delete sess_;
    sess_ = NULL;
}

void client::run(void)
{
    pthread_create(&tid_, NULL, thread_poll, this);

    cout << "client " << name_ << " has run" << endl;
}

const char *client::get_name()
{
    return name_.c_str();
}


void * client::thread_poll(void *pdata)
{
    char msg[] = "Don't think you are, Know you are";
    client * ptr = (client *)pdata;
    session * sess = ptr->sess_;
    uint32_t num;
    int ret;

    while(ptr->loop_exit_ == false) {
        // send video frame
        ret = sess->SendPacket(msg, strlen(msg));
        sess->log_error(ret);
        cout << "client " << sess->GetLocalSSRC() << ": sending " << ++num << " packet" << endl;

        RTPTime::Wait(RTPTime(0, 50 * 1000));
    }

    pthread_exit((void *)0);
}



