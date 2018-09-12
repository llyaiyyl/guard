#include "client.h"

sender::sender(uint16_t port)
{
    int32_t ret;
    RTPUDPv4TransmissionParams transparams;
    RTPSessionParams sessparams;

    // create rtp session
    sessparams.SetOwnTimestampUnit(1.0 / 9000.0);
    sessparams.SetMaximumPacketSize(1500);
    sessparams.SetAcceptOwnPackets(false);
    sessparams.SetUsePollThread(true);

    transparams.SetPortbase(port + 2);
    ret = m_sess.Create(sessparams, &transparams);
    log_error(ret);

    // set rtp session
    m_sess.SetDefaultPayloadType(96);
    m_sess.SetDefaultMark(false);
    m_sess.SetDefaultTimestampIncrement(10);
    ret = m_sess.AddDestination(RTPIPv4Address((uint32_t)0, port));
    log_error(ret);
}

sender::~sender()
{

}

void sender::run(void)
{
    pthread_create(&m_tid, NULL, thread_poll, this);
}

void sender::wait(void)
{
    pthread_join(m_tid, NULL);
}

void sender::quit(void)
{
    pthread_cancel(m_tid);
    this->wait();
}

void * sender::thread_poll(void *pdata)
{
    char msg[] = "We hold these truths to be self-evident, that all men are created equal, \n"
                 "that they are endowed by their Creator with certain unalienable rights, \n"
                 "that they are among these are life, liberty and the pursuit of happiness. \n"
                 "That to secure these rights, governments are instituted among them, deriving \n"
                 "their just power from the consent of the governed. That whenever any form of \n"
                 "government becomes destructive of these ends, it is the right of the people to \n"
                 "alter or to abolish it, and to institute new government, laying its foundation \n"
                 "on such principles and organizing its powers in such form, as to them shall \n"
                 "seem most likely to effect their safety and happiness. Prudence, indeed, \n"
                 "will dictate that governments long established should not be changed for \n"
                 "light and transient causes; and accordingly all experience hath shown that \n"
                 "mankind are more disposed to suffer, while evils are sufferable, than t right \n"
                 "themselves by abolishing the forms to which they are accustomed. But when a long \n"
                 "train of abuses and usurpations, pursuing invariably the same object evinces a \n"
                 "design to reduce them under absolute despotism, it is their right, it is their duty, \n"
                 "to throw off such government, and to provide new guards for their future security. \n"
                 "Such has been the patient sufferance of these Colonies; and such is now the necessity, \n"
                 "which constrains them to alter their former systems of government.";
    RTPSession * sess = (RTPSession *)pdata;
    uint32_t num;
    int ret;

    while(1) {
        // send video frame
        ret = sess->SendPacket(msg, strlen(msg));
        log_error(ret);
        cout << sess->GetLocalSSRC() << ": sending " << ++num << " packet" << endl;
        RTPTime::Wait(RTPTime(0, 50 * 1000));
    }
}

void sender::log_error(int ret)
{
    if(ret < 0) {
        std::cout << "ERROR: " << RTPGetErrorString(ret) << std::endl;
        exit(-1);
    }
}



