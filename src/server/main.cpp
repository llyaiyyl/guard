#include <iostream>
#include <stdint.h>
#include "endpoint.h"
#include "tcp.h"
#include "server.h"

using namespace std;

int main(int argc, char * argv[])
{
    int32_t fdsock;
    tcp t;

    t.Bind(4000);
    cout << "server run: 0.0.0.0:4000" << endl;

    server * s;
    while(1) {
        fdsock = t.Accept();
        s = new server(fdsock);
        s->run();
    }

    return 0;
}
