#include <iostream>
#include <stdint.h>
#include "tcp.h"
#include "manage.h"
#include "session.h"

using namespace std;

int main(int argc, char * argv[])
{
    int32_t fdsock;
    uint16_t port;
    char rbuff[1024];
    ssize_t rn;
    string req, rsp;
    manage * mg;

    if(argc != 3) {
        cout << "Usage: guard-manage <ip> <port>" << endl;
        return -1;
    }
    sscanf(argv[2], "%d", &fdsock);
    port = fdsock;
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
            req = "pull";
            tcp::Write(fdsock, req.c_str(), req.size());
            rn = tcp::Read(fdsock, rbuff, 1024);
            rsp = string(rbuff, rn);
            cout << "dest port: " << rsp.c_str() << endl;

            session * sess = session::create(argv[1], (uint16_t)6000, (uint16_t)6002);
            mg = new manage(sess);
            mg->run();

            // wait quit
            cin >> rn;

            mg->quit();
            delete mg;
            delete sess;
        } else {
            cout << "connect server fail." << endl;
        }
    }

    return 0;
}
