#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include "tcp.h"
#include "client.h"

using namespace std;

int main(int argc, char * argv[])
{
    int32_t fdsock;
    uint16_t port;
    char rbuff[1024];
    ssize_t rn;
    string req, rsp;

    if(argc != 3) {
        cout << "Usage: guard-client <ip> <port>" << endl;
        return -1;
    }
    sscanf(argv[2], "%u", &port);
    cout << "Connect to " << argv[1] << ":" << port << endl;

    fdsock = tcp::Connect(argv[1], port);
    if(-1 == fdsock) {
        cout << "Can't not connect to server." << endl;
        return -1;
    }

    while(1) {
        req = "ping";
        tcp::Write(fdsock, req.c_str(), req.size());
        rn = tcp::Read(fdsock, rbuff, 1024);
        rsp = string(rbuff, rn);
        if(rsp == string("pong")) {
            cout << "connect server ok." << endl;

            // ask server require a port
            req = "push";
            tcp::Write(fdsock, req.c_str(), req.size());
            rn = tcp::Read(fdsock, rbuff, 1024);
            rsp = string(rbuff, rn);
            cout << "dest port: " << rsp.c_str() << endl;

            // upstream
            sender * s = new sender(5000);
            s->run();
            s->wait();
        } else {
            cout << "connect server fail." << endl;
        }

        sleep(3);
    }

    return 0;
}
