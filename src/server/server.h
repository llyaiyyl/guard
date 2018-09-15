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
    enum sess_type {
        sess_unknow = 0,
        sess_push,
        sess_pull
    };

    server_data(int fd);
    server_data(int fd, string node_name);
    ~server_data();

    int get_fd(void) const;
    string get_node_name(void) const;
    void set_session(session * sess);
    session * get_session(void);
    void set_pull_session(session * sess);

    void set_sess_type(server_data::sess_type stype);
    server_data::sess_type get_sess_type(void);

    string get_id(void) const;
    bool operator ==(const server_data &p) const;

    void poll(void);
private:
    char ipstr_[INET_ADDRSTRLEN];
    uint16_t port_;
    int fdsock_;
    string id_;                  // ip:port

    session * sess_;
    session * sess_pull_;
    enum sess_type sess_type_;
    string node_name_;

    bool has_client;
    uint32_t client_ip;
    uint16_t client_port;
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

    void sd_poll(void);
    void sd_reg(const server_data &sd);
    void sd_config(const string &node_name, session * sess, server_data::sess_type stype);

    void sd_set_pullsess(const string &node_name, session *sess);

    /*
     * delete all session of fd
     */
    void sd_del(int fd);

    /*
     * delete session of node_name
     */
    void sd_del(const string &node_name);
    bool sd_exist(const string &node_name);
private:
    static void * thread_poll(void * pdata);

private:
    Json::CharReader * creader_;

    pthread_t tid_;
    std::list<server_data> list_sd_;         // save all connect client

    uint16_t port_base_;
};

#endif // SERVER_H
















