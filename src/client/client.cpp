#include <unistd.h>
#include <string.h>

#include "client.h"

client::client(string name, session *sess)
{
    sess_ = sess;
    tid_ = 0;
    loop_exit_ = false;
    name_ = name;
    max_ = 423;
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

const char *client::get_name(void)
{
    return name_.c_str();
}

void client::send_packet(const void *data, size_t len)
{
    int seq, index;
    const uint8_t * ptr = (const uint8_t *)data;

    if((len % max_) == 0) {
        seq = len / max_;
    } else {
        seq = (len / max_) + 1;
    }

    index = 0;
    while(len) {
        if(len <= max_) {
            sess_->SendPacketEx((const void *)(ptr + index), len, 96, false, 10, seq, NULL, 0);
            break;
        } else {
            sess_->SendPacketEx((const void *)(ptr + index), max_, 96, false, 0, seq, NULL, 0);
            len -= max_;
            index += max_;
        }

        seq--;
    }
}


void * client::thread_poll(void *pdata)
{
    client * ptr = (client *)pdata;
    char sbuff[1024];

    while(ptr->loop_exit_ == false) {
        ptr->send_packet(sbuff, 1024);
        cout << "send 1024 bytes" << endl;
        RTPTime::Wait(RTPTime(0, 50 * 1000));
    }

    pthread_exit((void *)0);
}



