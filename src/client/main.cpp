#include <iostream>
#include <stdint.h>
#include "endpoint.h"
#include "tcp.h"
#include "sender.h"

using namespace std;

int main(int argc, char * argv[])
{
    int32_t fdsock;
    tcp t;

    fdsock = t.Connect("0.0.0.0", 4000);
    if(-1 == fdsock) {
        cout << "Can't not connect to server." << endl;
        return -1;
    }

    endpoint * edp = new endpoint(fdsock);
    char rbuff[1024];
    ssize_t rn;
    string req, rsp;
    while(1) {
        req = "ping";
        edp->send_data(req.c_str(), req.size());
        rn = edp->recv_data(rbuff, 1024);
        rsp = string(rbuff, rn);
        if(rsp == string("pong")) {
            cout << "connect server ok." << endl;

            // ask server require a port
            req = "request";
            edp->send_data(req.c_str(), req.size());
            rn = edp->recv_data(rbuff, 1024);
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
