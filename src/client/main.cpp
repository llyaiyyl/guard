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

again:
    // connect to server
    fdsock = tcp::Connect(argv[1], port);
    if(-1 == fdsock) {
        cout << "Can't not connect to server." << endl;
        return -1;
    }

    root.clear();
    root["cmd"] = "ping";
    tcp::Write(fdsock, root.toStyledString());
    rsp = tcp::Read(fdsock, 1024);
    reader.parse(rsp, root);
    if(string("pong") == root["cmd"].asString()) {
        cout << "connect server ok." << endl;

        // create a schedule, to monitor all client
        schedule sch(fdsock);
        // find all video frame, push to server
        while(1) {
            // require a port from server
            root.clear();
            root["cmd"] = "push";
            root["sn"] = "000000";
            tcp::Write(fdsock, root.toStyledString());
            rsp = tcp::Read(fdsock, 1024);
            reader.parse(rsp, root);
            dst_port = root["port"].asUInt();
            if(dst_port) {
                // upstream
                session * sess = session::create(argv[1], (uint16_t)dst_port, (uint16_t)(dst_port + 2));
                sch.reg(client(string("000000"), sess));

                cout << "register client: 000000" << endl;
            } else {
                cout << "request a port error" << endl;
                return 1;
            }
            break;
        }
        sch.run();

        cout << "connect server error, 10s later restart" << endl;
        sleep(10);
        close(fdsock);
        goto again;
    } else {
        cout << "connect server fail." << endl;
    }

    return 0;
}

