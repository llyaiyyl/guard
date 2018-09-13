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

    void reg(client * c);
    void run();
private:
    int fdcon_;
    client * pc_arry_[10];
    int pc_num_;
};

#endif // SCHEDULE_H
