#include "receiver.h"

receiver::receiver(int16_t port)
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
    ret = m_sess.Create(sessparams, &transparams);
    log_error(ret);

    // set rtp session
    m_sess.SetDefaultPayloadType(96);
    m_sess.SetDefaultMark(false);
    m_sess.SetDefaultTimestampIncrement(10);

    m_tid = 0;
    loop_exit_ = false;
    sder_ = NULL;
}

receiver::~receiver()
{
}



void receiver::run()
{
    pthread_create(&m_tid, NULL, thread_poll, this);
}

void receiver::stop()
{
    loop_exit_ = true;
    pthread_cancel(m_tid);
    pthread_join(m_tid, NULL);
    m_tid = 0;

    m_sess.BYEDestroy(RTPTime(10, 0), 0, 0);
}

void receiver::set_sender(sender *p)
{
    sder_ = p;
}


void * receiver::thread_poll(void *pdata)
{
    RTPSession * sess = &(((receiver *)pdata)->m_sess);

    while(!(((receiver *)pdata)->loop_exit_)) {
        sess->BeginDataAccess();
        if(sess->GotoFirstSourceWithData()) {
            do {
                RTPPacket * pack;
                while(NULL != (pack = sess->GetNextPacket())) {
                    cout << pack->GetSSRC() << ": get data " << pack->GetPacketLength() << " bytes" << endl;

                    // need to sender
                    if(((receiver *)pdata)->sder_) {
                        ((receiver *)pdata)->sder_->sess_.SendPacket(pack->GetPayloadData(), pack->GetPayloadLength());
                    }
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

void receiver::log_error(int ret)
{
    if(ret < 0) {
        std::cout << "ERROR: " << RTPGetErrorString(ret) << std::endl;
        exit(-1);
    }
}

