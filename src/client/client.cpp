#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "tcp_socket.h"

using namespace std;


int main(int argc, char *argv)
{
    int sock_fd, ret;
    struct sockaddr_in saddr;
    short port;
    ssize_t recv_n;

    //
    if(3 != argv) {
        printf("Usage: ./guard-client <host> <port>\n");
        return 1;
    }
    sscanf(argv[2], "%d", &sock_fd);
    port = sock_fd;
    endpoint redp(argv[1], port);

    tcp_socket s;

again:
    do {
        ret = s.connect_edp(redp);
        if(-1 == ret){
            printf("connect error, wait 10s\n");
            sleep(10);
        }
    } while(0 != ret);

    // send and recv cmd
    while(1) {
        recv_n = recv(fd, recv_buff, 1000, 0);
        if(-1 == recv_n) {
            if(errno != EINTR) {
                printf("recv data from client error\n");
                sleep(10);
                goto again;
            } else
                continue ;
        } else if(0 == recv_n) {
            printf("client close\n");
            sleep(10);
            goto again;
        }

        recv_buff[recv_n] = 0;
        printf("%s\n", recv_buff);

        if(0 == strcmp(recv_buff, "list")){

        }
        else {

        }
    }

    return 0;
}

