#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "json/json.h"
#include "schedule.h"

using namespace std;

schedule::schedule(int fdcon)
{
    fdcon_ = fdcon;
}

schedule::~schedule()
{

}

void schedule::reg(const client &c)
{
    list_client_.push_back(c);
}

void schedule::run()
{
    ssize_t n;
    char rbuff[1024];
    list<client>::iterator it;
    Json::Value root;
    Json::CharReaderBuilder builder;
    Json::CharReader * creader = builder.newCharReader();

    // start all client
    for(it = list_client_.begin(); it != list_client_.end(); it++) {
        it->run();
    }

    cout << "schedule runing..." << endl;
    while(1) {
        n = read(fdcon_, rbuff, 1024);
        if(n < 0) {
            perror("read");
            continue ;
        } else if(n == 0) {
            printf("server close\n");
            close(fdcon_);

            // close all client
            for(it = list_client_.begin(); it != list_client_.end(); it++) {
                it->quit();
                cout << it->get_name() << " quit" << endl;
            }
            list_client_.clear();

            return ;
        } else {
            creader->parse(rbuff, rbuff + n, &root, NULL);
            cout << root.toStyledString() << endl;
        }
    }
}


