#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <iostream>

#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtplibraryversion.h"
#include "rtppacket.h"

using namespace jrtplib;


static void log_error(int ret)
{
    if(ret < 0) {
        std::cout << "ERROR: " << RTPGetErrorString(ret) << std::endl;
        exit(-1);
    }
}

int main(int argc, char * argv[])
{
    RTPSession sess;
    RTPUDPv4TransmissionParams transparams;
    RTPSessionParams sessparams;
    uint32_t ser_ip;
    uint16_t ser_port;
    int32_t mode;
    int ret;

    // init and load arg
    if(argc != 4) {
        printf("usage:\n\t./guard-client 0 192.168.0.0 4000\n");
        return 0;
    }
    sscanf(argv[1], "%d", &mode);
    inet_pton(AF_INET, argv[2], &ser_ip);
    ser_ip = ntohl(ser_ip);
    sscanf(argv[3], "%u", (uint32_t *)&ser_port);

    sessparams.SetOwnTimestampUnit(1.0/10.0);
    sessparams.SetAcceptOwnPackets(true);

    ret = sess.Create(sessparams,&transparams);
    log_error(ret);

    ret = sess.AddDestination(RTPIPv4Address(ser_ip, ser_port));
    log_error(ret);
    if(mode) {
        while(1) {
            // send video frame
            printf("Sending packet...\n");
            ret = sess.SendPacket((void *)"1234567890", 10, 0, false, 10);
            log_error(ret);

            RTPTime::Wait(RTPTime(0, 500 * 1000));
        }
    } else {
        ret = sess.SendPacket((void *)"pull", 4, 10, false,10);
        log_error(ret);
        while(1) {
            // recv video frame
            ret = sess.Poll();
            log_error(ret);

            sess.BeginDataAccess();
            if(sess.GotoFirstSourceWithData()) {
                do {
                    RTPPacket *pack;
                    while ((pack = sess.GetNextPacket()) != NULL) {
                        // You can examine the data here
                        uint8_t * data;
                        data = pack->GetPayloadData();
                        data[pack->GetPayloadLength()] = 0;
                        printf("recv video frame: %s\n", data);
                        sess.DeletePacket(pack);
                    }
                } while (sess.GotoNextSourceWithData());
            }
            sess.EndDataAccess();

            // wati 10ms
            RTPTime::Wait(RTPTime(0, 10 * 1000));
        }
    }

    sess.BYEDestroy(RTPTime(10,0),0,0);

    return 0;
}
