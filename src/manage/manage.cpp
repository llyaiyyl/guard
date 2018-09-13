#include "manage.h"

manage::manage(session *sess)
{
    sess_ = sess;
    tid_ = 0;
    isrecv_ = false;
    loop_exit_ = false;
}

manage::~manage()
{
}


void manage::run()
{
    pthread_create(&tid_, NULL, thread_poll, this);
}

void manage::quit()
{
    loop_exit_ = true;
    pthread_join(tid_, NULL);

    tid_ = 0;
    isrecv_ = false;
    loop_exit_ = false;
}

void * manage::thread_poll(void *pdata)
{
    manage * mg = (manage *)pdata;
    session * sess = mg->sess_;

    while(!mg->loop_exit_) {
        if(mg->isrecv_ == false) {
            sess->SendPacket("ping", 4);
        }

        sess->BeginDataAccess();
        if(sess->GotoFirstSourceWithData()) {
            do {
                RTPPacket * pack;
                while(NULL != (pack = sess->GetNextPacket())) {
                    mg->isrecv_ = true;

                    cout << "manage " << pack->GetSSRC() << ": get data " << pack->GetPacketLength() << " bytes "
                         << pack->GetSequenceNumber() << endl;
                    sess->DeletePacket(pack);
                }

            } while(sess->GotoNextSourceWithData());
        }
        sess->EndDataAccess();

        // wait 10ms
        RTPTime::Wait(RTPTime(0, 10 * 1000));
    }

    pthread_exit((void *)0);
}

