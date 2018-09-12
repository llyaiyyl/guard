#include "sender.h"

sender::sender(receiver *p, uint16_t port)
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
    rece_ = p;
}

sender::~sender()
{

}

void sender::run()
{
    pthread_create(&tid_, NULL, thread_poll, this);
}

void sender::stop()
{
    m_sess.BYEDestroy(RTPTime(10, 0), 0, 0);
}

void * sender::thread_poll(void *pdata)
{
    RTPSession * sess = &(((sender *)pdata)->sess_);

    while(1) {
        sess->BeginDataAccess();
        if(sess->GotoFirstSourceWithData()) {
            RTPSourceData * dat;
            uint32_t ip;
            uint16_t port;

            do {
                // get source ip
                dat = sess->GetCurrentSourceInfo();
                if (dat->GetRTPDataAddress() != 0) {
                    const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTPDataAddress());
                    ip = addr->GetIP();
                    port = addr->GetPort();
                }
                else if (dat->GetRTCPDataAddress() != 0) {
                    const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTCPDataAddress());
                    ip = addr->GetIP();
                    port = addr->GetPort()-1;
                }
                else {
                    ip = 0;
                    port = 0;
                }

                if(ip != 0 && port != 0) {
                    sess.AddDestination(RTPIPv4Address(ip, port));
                    goto exit;
                }
            } while(sess->GotoNextSourceWithData());
        }
        sess->EndDataAccess();

        // wait 10ms
        RTPTime::Wait(RTPTime(0, 10 * 1000));
    }

exit:
    sess->EndDataAccess();
    ((sender *)pdata)->rece_->set_sender(pdata);
    pthread_exit((void *)0);
}

void sender::log_error(int ret)
{
    if(ret < 0) {
        std::cout << "ERROR: " << RTPGetErrorString(ret) << std::endl;
        exit(-1);
    }
}



























