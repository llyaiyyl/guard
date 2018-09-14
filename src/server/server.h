#ifndef SERVER_H
#define SERVER_H
#include <iostream>
#include <list>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "session.h"
#include "event_handler.h"

using namespace std;

class server_data
{
public:
    enum sd_status {
        sd_init = 0,
    };

    server_data(int fd);
    ~server_data();

    const char * get_ipstr(void);
    string get_client_addr(void);
    uint16_t get_port(void);

    server_data * get_this(void);
private:
    char ipstr_[INET_ADDRSTRLEN];
    uint16_t port_;
    int fd_;

    enum sd_status status_;
    string client_addr_;
};

class client_data
{
public:
    client_data(int fd);
    ~client_data();

    const char * get_ipstr(void);
    string get_client_addr(void);
    uint16_t get_port();

private:
    char ipstr_[INET_ADDRSTRLEN];
    uint16_t port_;
    int fd_;
    string client_addr_;
};



class server : public event_handler
{
public:
    server();
    ~server();

    void run(void);

    void on_connect(int &fd, void ** pdata);
    void on_read_err(int &fd, void * pdata, int err);
    void on_read(int &fd, void * pdata, const void * rbuff, size_t rn);
    void on_close(int &fd, void * pdata);

private:
    static void * thread_poll(void * pdata);
    server_data * get_sd(const string &addr);

private:
    uint32_t push_num_, pull_num_;
    session * s_recv_, * s_send_;
    bool haspull;

    pthread_t tid_;
    std::list<server_data> list_sd_;         // save all connect client
};

#endif // SERVER_H
















