#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <iostream>
#include <list>
#include <iterator>

#include "client.h"

using namespace std;

class schedule
{
public:
    schedule(int fdcon);
    ~schedule();

    void reg(const client &c);
    void run();
private:
    int fdcon_;
    list<client> list_client_;
};

#endif // SCHEDULE_H
