#ifndef SESSION_H
#define SESSION_H

#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtplibraryversion.h"
#include "rtppacket.h"
#include "rtpsourcedata.h"

using namespace jrtplib;

class session : public RTPSession
{
public:
    static session * create(const char * ipstr, uint16_t dst_port, uint16_t src_port);
    ~session();

    static void log_error(int ret);

private:
    session(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port);
};

#endif // SESSION_H
