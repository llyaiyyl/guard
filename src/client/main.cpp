#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include "tcp.h"
#include "client.h"
#include "schedule.h"
#include "json/json.h"

using namespace std;

int main(int argc, char * argv[])
{
    int32_t fdsock;
    uint16_t port;
    char rbuff[1024];
    ssize_t rn;
    string req, rsp;
    uint32_t dst_port;
    Json::Value root;
    Json::Reader reader;

    // load arg
    if(argc != 3) {
        cout << "Usage: guard-client <ip> <port>" << endl;
        return -1;
    }
    sscanf(argv[2], "%d", &fdsock);
    port = fdsock;
    cout << "Connect to " << argv[1] << ":" << port << endl;

    // connect to server
    fdsock = tcp::Connect(argv[1], port);
    if(-1 == fdsock) {
        cout << "Can't not connect to server." << endl;
        return -1;
    }

    root["cmd"] = "ping";
    req = root.toStyledString();
    tcp::Write(fdsock, req.c_str(), req.size());
    rn = tcp::Read(fdsock, rbuff, 1024);
    rsp = string(rbuff, rn);
    reader.parse(rsp, root);
    if(string("pong") == root["cmd"].asString()) {
        cout << "connect server ok." << endl;

        schedule sch(fdsock);

        // find all video frame, push to server

        while(1) {
            // require a port from server
            root["cmd"] = "push";
            root["sn"] = "cam0";
            req = root.toStyledString();
            tcp::Write(fdsock, req.c_str(), req.size());
            rn = tcp::Read(fdsock, rbuff, 1024);
            rsp = string(rbuff, rn);
            reader.parse(rsp, root);
            dst_port = root["port"].asUInt();
            if(dst_port) {
                // upstream
                session * sess = session::create(argv[1], (uint16_t)dst_port, (uint16_t)(dst_port + 2));
                client * c = new client(string("cam0"), sess);
                c->run();
                cout << "push stream to " << argv[1] << ":" << dst_port << endl;

                sch.reg(c);
            } else {
                cout << "request a port error" << endl;
                return 1;
            }

            break;
        }

        sch.run();
    } else {
        cout << "connect server fail." << endl;
    }

    return 0;
}

