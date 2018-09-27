#include <iostream>

#include "tcp.h"
#include "session.h"
#include "json/json.h"

extern "C" {
#include <stdint.h>
#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavdevice/avdevice.h"
}

using namespace std;

int main(int argc, char * argv[])
{
    int32_t fdsock;
    uint16_t port;
    string rsp;
    Json::Value root;
    Json::CharReaderBuilder builder;
    Json::CharReader * creader = builder.newCharReader();

    int32_t fdudp;
    uint16_t port_udp;
    struct sockaddr_in saddr;
    socklen_t slen;
    int rn;
    char rbuff[1024];

    char * sn_str, * ip_str, * port_str;


    // load arg
    if(argc != 4) {
        cout << "Usage: guard-client <sn> <ip> <port>" << endl;
        return -1;
    }
    sn_str = argv[1];
    ip_str = argv[2];
    port_str = argv[3];

    sscanf(port_str, "%d", &fdsock);
    port = fdsock;
    cout << "Connect to " << ip_str << ":" << port << endl;

    // connect to server
    fdsock = tcp::Connect(ip_str, port);
    if(-1 == fdsock) {
        cout << "Can't not connect to server." << endl;
        return -1;
    }

    root.clear();
    root["cmd"] = "ping";
    tcp::Write(fdsock, root.toStyledString());
    rsp = tcp::Read(fdsock, 1024);
    creader->parse(rsp.c_str(), rsp.c_str() + rsp.size(), &root, NULL);
    if(string("pong") == root["cmd"].asString()) {
        cout << "connect server ok." << endl;

        // get local output address
again:
        fdudp = socket(AF_INET, SOCK_DGRAM, 0);

        bzero(&saddr, sizeof(struct sockaddr_in));
        saddr.sin_family = AF_INET;
        inet_pton(AF_INET, ip_str, &(saddr.sin_addr.s_addr));
        saddr.sin_port = htons(port);

        sendto(fdudp, "ping", 4, 0, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
        rn = recvfrom(fdudp, rbuff, 1024, 0, NULL, NULL);
        rsp = string(rbuff, rn);
        creader->parse(rsp.c_str(), rsp.c_str() + rsp.size(), &root, NULL);
        cout << root.toStyledString() << endl;

        slen = sizeof(struct sockaddr_in);
        bzero(&saddr, sizeof(struct sockaddr_in));
        getsockname(fdudp, (struct sockaddr *)&saddr, &slen);
        port_udp = ntohs(saddr.sin_port);

        close(fdudp);
        if((port_udp % 2) != 0) {
            goto again;
        }

        //
        root["cmd"] = "pull";
        root["sn"] = string(sn_str);
        tcp::Write(fdsock, root.toStyledString());
        rsp = tcp::Read(fdsock, 1024);
        creader->parse(rsp.c_str(), rsp.c_str() + rsp.size(), &root, NULL);
        cout << root.toStyledString() << endl;

        if(root["status"].asUInt() == 0) {
            uint16_t port_test = root["port"].asUInt();
            session * sess = session::create(ip_str, port_test, port_udp);
            bool has_recv = false;

            uint8_t * buff = new uint8_t [300000];
            int index = 0;

            AVCodecContext * ptr_codec_ctx = NULL;
            AVCodec * ptr_codec = NULL;
            AVPacket packet;
            AVFrame * frame = av_frame_alloc();
            int ret;

            ptr_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
            ptr_codec_ctx = avcodec_alloc_context3(ptr_codec);
            avcodec_open2(ptr_codec_ctx, ptr_codec, NULL);
            while(1) {
                if(false == has_recv) {
                    sess->SendRawData("ping", 4, true);
                }

                sess->BeginDataAccess();
                if(sess->GotoFirstSourceWithData()) {
                    do {
                        RTPPacket * pack;
                        while(NULL != (pack = sess->GetNextPacket())) {
                            if(pack->GetExtensionID() == 1) {
                                memcpy(buff + index, pack->GetPayloadData(), pack->GetPayloadLength());
                                index += pack->GetPayloadLength();

                                // process
                                // cout << "recv: " << index << " bytes" << endl;
                                av_init_packet(&packet);
                                packet.data = buff;
                                packet.size = index;

                                avcodec_send_packet(ptr_codec_ctx, &packet);
                                ret = avcodec_receive_frame(ptr_codec_ctx, frame);
                                if(ret == 0) {
                                    // cout << "frame: " << frame->width << " x " << frame->height << endl;
                                }

                                // reset
                                index = 0;
                            } else {
                                memcpy(buff + index, pack->GetPayloadData(), pack->GetPayloadLength());
                                index += pack->GetPayloadLength();
                            }

                            sess->DeletePacket(pack);
                            if(false == has_recv) {
                                has_recv = true;
                                sess->ClearDestinations();
                            }
                        }
                    } while(sess->GotoNextSourceWithData());
                }
                sess->EndDataAccess();
                RTPTime::Wait(RTPTime(0, 10 * 1000));
            }
        } else {
            cout << root["msg"].asString() << endl;
        }
    } else {
        cout << "connect server fail." << endl;
    }

    close(fdsock);
    return 0;
}

