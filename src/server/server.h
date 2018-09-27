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

class push
{
public:
    push(int fd, session * sess, const string &sn, uint16_t push_port);
    ~push();

    string get_sn(void);
    session * get_session(void);
    uint16_t get_push_port(void);
    int get_fd(void);
    void pull_inc(void);
    void pull_dec(void);
    void poll();

    void set_fps(int m) { fps_ = m; }
    int get_fps(void) { return fps_; }

    void set_width(int m) { width_ = m; }
    int get_width(void) { return width_; }

    void set_height(int m) { height_ = m; }
    int get_height(void) { return height_; }

    void set_pix_fmt(int m) { pix_fmt_ = m; }
    int get_pix_fmt(void) { return pix_fmt_; }

    void set_codec_id(int m) { codec_id_ = m; }
    int get_codec_id(void) { return codec_id_; }

private:
    int fd_;
    uint16_t push_port_;
    session * sess_;
    string sn_;
    uint32_t pull_num_;

    int fps_;
    int width_, height_;
    int pix_fmt_, codec_id_;
};

class pull
{
public:
    pull(int fd, const string &sn, uint32_t ip, uint16_t port)
    {
        fd_ = fd;
        sn_ = sn;
        ip_ = ip;
        port_ = port;
    }

    int fd_;
    string sn_;
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

    bool poll(void);
    bool exist(const string &sn);

    void push_reg(push * p);
    void push_del(int fd);
    uint16_t push_get_port(const string &sn);
    void push_set_videoinfo(const string &sn, const Json::Value &v);
    Json::Value push_get_videoinfo(const string &sn);

    void pull_reg(pull * p);
    void pull_del(int fd);

    static void * thread_poll(void * pdata);
    static void * thread_echo(void * pdata);
    static string get_client_addrs(int fd);
private:
    Json::CharReader * creader_;

    pthread_t tid_, tid_udp_;
    std::list<push *> list_push_;
    std::list<pull *> list_pull_;

    uint16_t port_base_;
    uint16_t port_udp_;
    bool loop_exit_;
    pthread_mutex_t lock_;
};

#endif // SERVER_H
















