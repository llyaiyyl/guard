#ifndef SERVER_H
#define SERVER_H
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "event_handler.h"
#include "receiver.h"
#include "sender.h"


class server_data
{
public:
    server_data(int fd);
    ~server_data();

    void set_addr(const char * ipstr, uint16_t port);
    const char * get_ipstr(void);
    uint16_t get_port(void);

    void set_recv(receiver * ptr);
    receiver *get_recv(void);

    void set_send(sender * ptr);
    sender *get_send(void);

    int get_fd(void);

private:
    char ipstr_[INET_ADDRSTRLEN];
    uint16_t port_;
    receiver * recv_;
    sender * sder_;
    int fd_;
};



class server : public event_handler
{
public:
    server()
    {
        recv_num_ = 0;
        pull_num_ = 0;
    }
    ~server() {}

    void on_connect(int fd, void ** pdata);
    void on_read_err(int fd, void * pdata, int err);
    void on_read(int fd, void * pdata, const void * rbuff, size_t rn);
    void on_close(int fd, void * pdata);

private:
    uint32_t recv_num_, pull_num_;
};

#endif // SERVER_H
