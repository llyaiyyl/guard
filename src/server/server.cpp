#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "json/json.h"
#include "server.h"

using namespace std;

/****************************************
 * push
 ***************************************/
push::push(int fd, session *sess, const string &sn, uint16_t push_port)
{
    fd_ = fd;
    push_port_ = push_port;
    sess_ = sess;
    sn_ = sn;
    pull_num_ = 0;
}

push::~push()
{
    if(sess_) {
        delete sess_;
        sess_ = NULL;
    }
}

string push::get_sn()
{
    return sn_;
}

session * push::get_session()
{
    return sess_;
}

uint16_t push::get_push_port()
{
    return push_port_;
}

int push::get_fd()
{
    return fd_;
}

void push::pull_inc()
{
    pull_num_++;
}

void push::pull_dec()
{
    if(pull_num_)
        pull_num_--;
}

void push::poll()
{
    session * sess = sess_;
    RTPPacket * pack;

    list<RTPPacket *> list_ppk_;
    list<RTPPacket *>::iterator it;

    sess->BeginDataAccess();
    if(sess->GotoFirstSourceWithData()) {
        do {
            while(NULL != (pack = sess->GetNextPacket())) {
#if 0
                cout << sn_ << " " << pack->GetTimestamp()
                     << " " << pack->GetExtensionID()
                     << " " << pack->GetPayloadLength() << endl;
#endif
                list_ppk_.push_back(pack);
            }
        } while(sess->GotoNextSourceWithData());
    }
    sess->EndDataAccess();

    if(pull_num_) {
        for(it = list_ppk_.begin(); it != list_ppk_.end(); it++) {
            if((*it)->GetExtensionID() == 1) {
                sess->SendPacketEx((*it)->GetPayloadData(), (*it)->GetPayloadLength(), 96, false, 10, (*it)->GetExtensionID(), NULL, 0);
            } else {
                sess->SendPacketEx((*it)->GetPayloadData(), (*it)->GetPayloadLength(), 96, false, 0, (*it)->GetExtensionID(), NULL, 0);
            }
            sess->DeletePacket(*it);
        }
        list_ppk_.clear();
    }
}

/****************************************
 * Server
 ***************************************/
server::server(uint16_t port_udp)
    : event_handler()
{
    Json::CharReaderBuilder builder;
    creader_ = builder.newCharReader();

    tid_ = 0;
    tid_udp_ = 0;

    port_base_ = 5000;
    port_udp_ = port_udp;
    loop_exit_ = false;

    pthread_mutex_init(&lock_, NULL);
}

server::~server()
{
    if(0 != tid_) {
        loop_exit_ = true;
        pthread_join(tid_, NULL);
        tid_ = 0;
    }

    if(0 != tid_udp_) {
        pthread_cancel(tid_udp_);
        pthread_join(tid_udp_, NULL);
        tid_udp_ = 0;
    }

    if(creader_) {
        delete creader_;
        creader_ = NULL;
    }

    pthread_mutex_destroy(&lock_);
}

// public function
void server::run()
{
    if(0 == tid_) {
        pthread_create(&tid_, NULL, thread_poll, this);
    }

    if(0 == tid_udp_) {
        pthread_create(&tid_udp_, NULL, thread_echo, &port_udp_);
    }
}

// privata function
void server::on_connect(int &fd, void **pdata)
{
    std::cout << "client connect: " << get_client_addrs(fd) << std::endl;
}

void server::on_close(int &fd, void *pdata)
{
    pull_del(fd);
    push_del(fd);
    cout << "now push client number: " << list_push_.size() << endl;
    cout << "now pull client number: " << list_pull_.size() << endl;

    std::cout << "client close: " << get_client_addrs(fd) << std::endl;
}

void server::on_read_err(int &fd, void *pdata, int err)
{
    errno = err;
    perror("on_read_err");

    pull_del(fd);
    push_del(fd);
    cout << "now push client number: " << list_push_.size() << endl;
    cout << "now pull client number: " << list_pull_.size() << endl;

    std::cout << "client read error: " << get_client_addrs(fd) << std::endl;
}

void server::on_read(int &fd, void *pdata, const void *rbuff, size_t rn)
{
    string rsp;
    Json::Value root;

    // parse json data
    rsp = string((char *)rbuff, rn);
    creader_->parse(rsp.c_str(), rsp.c_str() + rsp.size(), &root, NULL);

    string cmd = root["cmd"].asString();
    if(cmd == string("ping")) {
        root.clear();
        root["cmd"] = "pong";
    } else if(cmd == string("push")) {
        cout << root.toStyledString() << endl;

        if(exist(root["sn"].asString()) == false) {
            session * sess = session::create(NULL, 0, port_base_);
            push * p = new push(fd, sess, root["sn"].asString(), port_base_);
            push_reg(p);
            push_set_videoinfo(root["sn"].asString(), root["videoinfo"]);

            root.clear();
            root["port"] = port_base_;
            port_base_ += 2;
        } else {
            root.clear();
            root["port"] = 0;
            root["msg"] = "node name/sn of client exist";
        }
    } else if(cmd == string("pull")) {
        if(exist(root["sn"].asString()) == true) {
            pull * p = new pull(fd, root["sn"].asString(), root["ip"].asUInt(), uint16_t(root["port"].asUInt()));
            pull_reg(p);

            uint16_t port = push_get_port(root["sn"].asString());
            Json::Value v = push_get_videoinfo(root["sn"].asString());

            root.clear();
            root["status"] = 0;
            root["msg"] = "pull ok";
            root["port"] = port;
            root["videoinfo"] = v;
        } else {
            root.clear();
            root["status"] = 1;
            root["msg"] = "can't find node name/sn";
        }
    } else if(cmd == string("list")) {
        root.clear();
        list<push *>::iterator it;
        pthread_mutex_lock(&lock_);
        for(it = list_push_.begin(); it != list_push_.end(); it++) {
            root["cam"].append((*it)->get_sn());
        }
        pthread_mutex_unlock(&lock_);
    } else {
        root.clear();
        root["cmp"] = "unsuppot";
    }

    rsp = root.toStyledString();
    write(fd, rsp.c_str(), rsp.size());

    cout << "push client number: " << list_push_.size() << endl;
    cout << "pull client number: " << list_pull_.size() << endl;
}

bool server::poll(void)
{
    list<push *>::iterator it;

    pthread_mutex_lock(&lock_);
    for(it = list_push_.begin(); it != list_push_.end(); it++) {
        (*it)->poll();
    }
    pthread_mutex_unlock(&lock_);

    return loop_exit_;
}

bool server::exist(const string &sn)
{
    list<push *>::iterator it;

    pthread_mutex_lock(&lock_);
    for(it = list_push_.begin(); it != list_push_.end(); it++) {
        if(sn == (*it)->get_sn()) {
            pthread_mutex_unlock(&lock_);
            return true;
        }
    }
    pthread_mutex_unlock(&lock_);

    return false;
}

void server::push_reg(push *p)
{
    pthread_mutex_lock(&lock_);
    list_push_.push_back(p);
    pthread_mutex_unlock(&lock_);
}

uint16_t server::push_get_port(const string &sn)
{
    list<push *>::iterator it;
    pthread_mutex_lock(&lock_);
    for(it = list_push_.begin(); it != list_push_.end(); it++) {
        if(sn == (*it)->get_sn()) {
            uint16_t port = (*it)->get_push_port();
            pthread_mutex_unlock(&lock_);
            return port;
        }
    }
    pthread_mutex_unlock(&lock_);

    return 0;
}

void server::push_set_videoinfo(const string &sn, const Json::Value &v)
{
    list<push *>::iterator it;
    pthread_mutex_lock(&lock_);
    for(it = list_push_.begin(); it != list_push_.end(); it++) {
        if(sn == (*it)->get_sn()) {
            int tmp;

            tmp = v["fps"].asInt();
            (*it)->set_fps(tmp);

            tmp = v["width"].asInt();
            (*it)->set_width(tmp);

            tmp = v["height"].asInt();
            (*it)->set_height(tmp);

            tmp = v["pix_fmt"].asInt();
            (*it)->set_pix_fmt(tmp);

            tmp = v["codec_id"].asInt();
            (*it)->set_codec_id(tmp);
        }
    }
    pthread_mutex_unlock(&lock_);
}

Json::Value server::push_get_videoinfo(const string &sn)
{
    Json::Value v;
    list<push *>::iterator it;

    pthread_mutex_lock(&lock_);
    for(it = list_push_.begin(); it != list_push_.end(); it++) {
        if(sn == (*it)->get_sn()) {
            v["fps"] = (*it)->get_fps();
            v["width"] = (*it)->get_width();
            v["height"] = (*it)->get_height();
            v["pix_fmt"] = (*it)->get_pix_fmt();
            v["codec_id"] = (*it)->get_codec_id();
            v["status"] = 0;
            pthread_mutex_unlock(&lock_);
            return v;
        }
    }
    pthread_mutex_unlock(&lock_);

    v["status"] = -1;
    return v;
}

void server::push_del(int fd)
{
    list<push *>::iterator it;

    pthread_mutex_lock(&lock_);
again:
    for(it = list_push_.begin(); it != list_push_.end(); it++) {
        if(fd == (*it)->get_fd()) {
            list_push_.remove(*it);
            goto again;
        }
    }
    pthread_mutex_unlock(&lock_);
}

void server::pull_reg(pull *p)
{
    list<push *>::iterator it;

    pthread_mutex_lock(&lock_);
    for(it = list_push_.begin(); it != list_push_.end(); it++) {
        if((*it)->get_sn() == p->sn_) {
            session * sess = (*it)->get_session();
            sess->AddDestination(RTPIPv4Address(p->ip_, p->port_));
            (*it)->pull_inc();
            break;
        }
    }

    list_pull_.push_back(p);

    pthread_mutex_unlock(&lock_);
}

void server::pull_del(int fd)
{
    list<pull *>::iterator it;
    list<push *>::iterator it_sub;

    pthread_mutex_lock(&lock_);
again:
    for(it = list_pull_.begin(); it != list_pull_.end(); it++) {
        if(fd == (*it)->fd_) {
            for(it_sub = list_push_.begin(); it_sub != list_push_.end(); it_sub++) {
                if((*it)->sn_ == (*it_sub)->get_sn()) {
                    session * sess = (*it_sub)->get_session();
                    sess->DeleteDestination(RTPIPv4Address((*it)->ip_, (*it)->port_));
                    (*it_sub)->pull_dec();
                    break;
                }
            }

            list_pull_.remove(*it);
            goto again;
        }
    }
    pthread_mutex_unlock(&lock_);
}

void * server::thread_poll(void *pdata)
{
    server * s = (server *)pdata;
    bool exit = false;
    while(!exit) {
        exit = s->poll();

        // wait 10ms
        RTPTime::Wait(RTPTime(0, 10 * 1000));
    }
}

void * server::thread_echo(void *pdata)
{
    int fd;
    struct sockaddr_in saddr;
    socklen_t slen;
    ssize_t rn;
    string rsp;
    char rbuff[1024], ipstr[INET_ADDRSTRLEN];
    Json::Value root;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    bzero(&saddr, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(*((uint16_t *)pdata));

    bind(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    while(1) {
        slen = sizeof(struct sockaddr_in);
        bzero(&saddr, slen);
        rn = recvfrom(fd, rbuff, 1024, 0, (struct sockaddr *)&saddr, &slen);
        rsp = string(rbuff, rn);
        if(rsp == string("ping")) {
            root["ip"] = ntohl(saddr.sin_addr.s_addr);
            root["port"] = ntohs(saddr.sin_port);

            inet_ntop(saddr.sin_family, &(saddr.sin_addr.s_addr), ipstr, INET_ADDRSTRLEN);
            sprintf(rbuff, "%s:%d", ipstr, ntohs(saddr.sin_port));
            root["addr"] = string(rbuff);
            rsp = root.toStyledString();

            cout << rsp << endl;
            sendto(fd, rsp.c_str(), rsp.size(), 0, (struct sockaddr *)&saddr, slen);
        }
    }
}

string server::get_client_addrs(int fd)
{
    struct sockaddr_in saddr;
    socklen_t slen;
    char str[24], ipstr[INET_ADDRSTRLEN];

    slen = sizeof(struct sockaddr_in);
    bzero(&saddr, slen);
    getpeername(fd, (struct sockaddr *)&saddr, &slen);
    inet_ntop(saddr.sin_family, &(saddr.sin_addr.s_addr), ipstr, INET_ADDRSTRLEN);

    sprintf(str, "%s:%d", ipstr, ntohs(saddr.sin_port));
    return string(str);
}



















