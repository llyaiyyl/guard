#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <iostream>
#include <string.h>

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
             "which constrains them to alter their former systems of government. The history of the \n"
             "present King of Great Britain is usurpations, all having in direct object tyranny over \n"
             "these States. To prove this, let facts be submitted to a candid world.";

int main(int argc, char * argv[])
{
    RTPSession sess;
    RTPUDPv4TransmissionParams transparams;
    RTPSessionParams sessparams;
    uint32_t ser_ip;
    uint16_t ser_port, cli_port;
    int32_t mode;
    int ret;

    // init and load arg
    if(argc != 5) {
        printf("usage:\n\t./guard-client 0 192.168.0.0 4000 5000\n");
        return 0;
    }
    sscanf(argv[1], "%d", &mode);
    sscanf(argv[3], "%u", &ser_ip);
    ser_port = ser_ip;
    sscanf(argv[4], "%u", &ser_ip);
    cli_port = ser_ip;
    inet_pton(AF_INET, argv[2], &ser_ip);
    ser_ip = ntohl(ser_ip);

    sessparams.SetOwnTimestampUnit(1.0/10.0);
    sessparams.SetAcceptOwnPackets(true);
    transparams.SetPortbase(cli_port);
    ret = sess.Create(sessparams,&transparams);
    log_error(ret);
    ret = sess.SetMaximumPacketSize(50000);
    log_error(ret);

    ret = sess.AddDestination(RTPIPv4Address(ser_ip, ser_port));
    log_error(ret);
    if(mode) {
        while(1) {
            // send video frame
            printf("Sending packet...\n");
            ret = sess.SendPacket(msg, strlen(msg), 0, false, 10);
            log_error(ret);

            RTPTime::Wait(RTPTime(0, 50 * 1000));
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
                        printf("recv video frame: %ld bytes %u\n", pack->GetPayloadLength(), pack->GetTimestamp());
                        // printf("%s\n", data);
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
