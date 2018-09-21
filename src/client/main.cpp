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

again:
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

        // create a schedule, to monitor all client
        schedule sch(fdsock);
        // find all video frame, push to server
        while(1) {
            // require a port from server
            root.clear();
            root["cmd"] = "push";
            root["sn"] = sn_str;
            tcp::Write(fdsock, root.toStyledString());
            rsp = tcp::Read(fdsock, 1024);
            creader->parse(rsp.c_str(), rsp.c_str() + rsp.size(), &root, NULL);
            dst_port = root["port"].asUInt();
            if(dst_port) {
                // upstream
                session * sess = session::create(ip_str, (uint16_t)dst_port, (uint16_t)(dst_port + 1000));
                sch.reg(new client(string(sn_str), sess));

                cout << "register client: " << sn_str << endl;
            } else {
                cout << "error: " << root["msg"].asString()<< endl;
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

