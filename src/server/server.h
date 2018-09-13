#ifndef SERVER_H
#define SERVER_H
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "session.h"
#include "event_handler.h"

class server_data
{
public:
    server_data(int fd);
    ~server_data();

    void set_addr(const char * ipstr, uint16_t port);
    const char * get_ipstr(void);
    uint16_t get_port(void);

    int get_fd(void);

private:
    char ipstr_[INET_ADDRSTRLEN];
    uint16_t port_;
    int fd_;
};



class server : public event_handler
{
public:
    server();
    ~server();

    void on_connect(int &fd, void ** pdata);
    void on_read_err(int &fd, void * pdata, int err);
    void on_read(int &fd, void * pdata, const void * rbuff, size_t rn);
    void on_close(int &fd, void * pdata);

private:
    static void * thread_poll(void * pdata);


private:
    uint32_t push_num_, pull_num_;
    session * s_recv_, * s_send_;
    bool haspull;

    pthread_t tid_;
};

#endif // SERVER_H
















