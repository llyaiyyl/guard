#include "manage.h"

manage::manage(int16_t port)
{
    int32_t ret;
    RTPUDPv4TransmissionParams transparams;
    RTPSessionParams sessparams;

    // init and create rtp session
    sessparams.SetOwnTimestampUnit(1.0 / 9000.0);
    sessparams.SetMaximumPacketSize(1500);
    sessparams.SetAcceptOwnPackets(false);
    sessparams.SetUsePollThread(true);

    transparams.SetBindIP(0);
    transparams.SetPortbase(port);
    ret = sess_.Create(sessparams, &transparams);
    log_error(ret);

    // set rtp session
    sess_.SetDefaultPayloadType(96);
    sess_.SetDefaultMark(false);
    sess_.SetDefaultTimestampIncrement(10);

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

void manage::stop()
{
    loop_exit_ = true;
    pthread_cancel(tid_);
    pthread_join(tid_, NULL);
    tid_ = 0;
    isrecv_ = false;

    sess_.BYEDestroy(RTPTime(10, 0), 0, 0);
}

void * manage::thread_poll(void *pdata)
{
    manage * mg = (manage *)pdata;
    RTPSession * sess = &(mg->sess_);

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

void manage::log_error(int ret)
{
    if(ret < 0) {
        std::cout << "ERROR: " << RTPGetErrorString(ret) << std::endl;
        exit(-1);
    }
}

