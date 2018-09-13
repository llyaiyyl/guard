#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "schedule.h"

using namespace std;

schedule::schedule(int fdcon)
{
    fdcon_ = fdcon;
    pc_num_ = 0;
}

schedule::~schedule()
{

}

void schedule::reg(client *c)
{
    pc_arry_[pc_num_] = c;
    pc_num_++;
}

void schedule::run()
{
    ssize_t n;
    char rbuff[1024];

    cout << "schedule runing..." << endl;
    while(1) {
        n = read(fdcon_, rbuff, 1024);
        if(n < 0) {
            perror("read");
            continue ;
        } else if(n == 0) {
            printf("server close\n");
            close(fdcon_);

            // close all client and exit
            for(int i = 0; i < pc_num_; i++) {
                printf("%s will exit\n", pc_arry_[i]->get_name());
                delete pc_arry_[i];
            }

            return ;
        } else {
            rbuff[n] = 0;
            cout << rbuff << endl;
        }
    }
}


