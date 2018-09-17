#ifndef SERVER_H
#define SERVER_H
#include <iostream>
#include <list>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "json/json.h"
#include "session.h"
#include "event_handler.h"

using namespace std;

class server_data
{
public:
    server_data(int fd, string node_name);
    ~server_data();

    int get_fd(void) const;
    string get_node_name(void) const;
    void set_session(session * sess);
    session * get_session(void);

    string get_id(void) const;
    bool operator ==(const server_data &p) const;

    void poll(void);

    void pull_inc(void);
    void pull_dec(void);
private:
    char ipstr_[INET_ADDRSTRLEN];
    uint16_t port_;
    int fdsock_;
    string id_;                  // ip:port

    session * sess_;
    string node_name_;

    uint32_t pull_num_;
};

class pull_data
{
public:
    pull_data(int fdsock, const string &node_name, uint32_t ip, uint16_t port)
    {
        fdsock_ = fdsock;
        node_name_ = node_name;
        ip_ = ip;
        port_ = port;
    }

    bool operator ==(const pull_data &p) const
    {
        if(fdsock_ == p.fdsock_ && node_name_ == p.node_name_)
            return true;
        else
            return true;
    }

    int fdsock_;
    string node_name_;
    uint32_t ip_;
    uint16_t port_;
};


class server : public event_handler
{
public:
    server(uint16_t port_udp);
    ~server();

    void run(void);

private:
    void on_connect(int &fd, void ** pdata);
    void on_read_err(int &fd, void * pdata, int err);
    void on_read(int &fd, void * pdata, const void * rbuff, size_t rn);
    void on_close(int &fd, void * pdata);

    bool sd_poll(void);
    void sd_reg(const server_data &sd);
    void sd_set_sess(const string &node_name, session * sess);
    void sd_del(int fd);
    void sd_del(int fd, const string &node_name);
    bool sd_exist(const string &node_name);

    void sd_add_addr(int fd, const string &node_name, uint32_t ip, uint32_t port);
    void sd_del_addr(int fd);

    static void * thread_poll(void * pdata);
    static void * thread_echo(void * pdata);
    static string get_id(int fd);
private:
    Json::CharReader * creader_;

    pthread_t tid_, tid_udp_;
    std::list<server_data> list_sd_;
    std::list<pull_data> list_pd_;

    uint16_t port_base_;
    pthread_mutex_t lock_;
    bool loop_exit_;

    uint16_t port_udp_;
};

#endif // SERVER_H
















