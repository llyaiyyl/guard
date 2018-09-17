#include <iostream>
#include <arpa/inet.h>

#include "session.h"

session::session(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, bool usepoll)
    : RTPSession()
{
    int32_t ret;
    RTPUDPv4TransmissionParams transparams;
    RTPSessionParams sessparams;

    // create rtp session
    sessparams.SetOwnTimestampUnit(1.0 / 9000.0);
    sessparams.SetMaximumPacketSize(1500);
    sessparams.SetAcceptOwnPackets(false);
    sessparams.SetUsePollThread(usepoll);

    transparams.SetBindIP(0);
    transparams.SetPortbase(src_port);

    ret = this->Create(sessparams, &transparams);
    log_error(ret);

    // set rtp session
    this->SetDefaultPayloadType(96);
    this->SetDefaultMark(false);
    this->SetDefaultTimestampIncrement(10);

    if(dst_ip) {
        ret = this->AddDestination(RTPIPv4Address(dst_ip, dst_port));
        log_error(ret);
    }
}

session::~session()
{
    this->BYEDestroy(RTPTime(10, 0), 0, 0);
}


session * session::create(const char *ipstr, uint16_t dst_port, uint16_t src_port, bool usepoll)
{
    uint32_t dst_ip;

    if(ipstr) {
        inet_pton(AF_INET, ipstr, &dst_ip);
        return new session(ntohl(dst_ip), dst_port, src_port, usepoll);
    } else {
        return new session(0, 0, src_port, usepoll);
    }
}

void session::log_error(int ret)
{
    if(ret < 0) {
        std::cout << "ERROR: " << RTPGetErrorString(ret) << std::endl;
        exit(-1);
    }
}
















