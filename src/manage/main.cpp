#include <iostream>
#include <stdint.h>
#include <stdio.h>

#include "tcp.h"
#include "session.h"
#include "json/json.h"

using namespace std;


int main(int argc, char * argv[])
{
    int32_t fdsock;
    uint16_t port;
    string rsp;
    uint32_t dst_port;
    Json::Value root;
    Json::CharReaderBuilder builder;
    Json::CharReader * creader = builder.newCharReader();

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

        // require a port from server
        root.clear();
        root["cmd"] = "pull";
        root["sn"] = sn_str;
        tcp::Write(fdsock, root.toStyledString());
        rsp = tcp::Read(fdsock, 1024);
        creader->parse(rsp.c_str(), rsp.c_str() + rsp.size(), &root, NULL);
        dst_port = root["port"].asUInt();
        if(dst_port) {
            // download stream
            cout << root.toStyledString();
            session * sess = session::create(ip_str, (uint16_t)dst_port, (uint16_t)(dst_port + 2000));
            bool isframe = false;
            while(1) {
                if(isframe == false)
                    sess->SendPacket("ping", 4);

                sess->BeginDataAccess();
                if(sess->GotoFirstSourceWithData()) {
                    do {
                        // read packet
                        RTPPacket * pack;
                        while(NULL != (pack = sess->GetNextPacket())) {
                            cout << pack->GetSSRC() << ": get data " << pack->GetPacketLength() << " bytes "
                                 << pack->GetSequenceNumber() << endl;
                            sess->DeletePacket(pack);

                            isframe = true;
                        }
                    } while(sess->GotoNextSourceWithData());
                }
                sess->EndDataAccess();

                RTPTime::Wait(RTPTime(0, 10 * 1000));
            }
        } else {
            cout << "error: " << root["msg"].asString()<< endl;
            return 1;
        }
    } else {
        cout << "connect server fail." << endl;
    }

    return 0;
}

