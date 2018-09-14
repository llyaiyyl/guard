#include <iostream>
#include <stdint.h>
#include "tcp.h"
#include "server.h"
#include "reactor.h"

using namespace std;

int main(int argc, char * argv[])
{
    int32_t fdsock;
    reactor * r;
    server ser;

    fdsock = tcp::Listen(4000);
    ser.run();
    r = reactor::create(fdsock, 10, ser);
    cout << "server run: 0.0.0.0:4000" << endl;

    r->run();

    return 0;
}
