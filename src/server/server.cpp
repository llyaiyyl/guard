#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "json/json.h"
#include "server.h"

using namespace std;


/*
 *
 * server_data
 *
 */
server_data::server_data(int fd, string node_name, uint16_t port_push)
{
    struct sockaddr_in saddr;
    socklen_t slen;
    char buff[24];

    slen = sizeof(struct sockaddr_in);
    bzero(&saddr, slen);
    if(0 == getpeername(fd, (struct sockaddr *)&saddr, &slen)) {
        port_ = ntohs(saddr.sin_port);
        inet_ntop(saddr.sin_family, &(saddr.sin_addr.s_addr), ipstr_, INET_ADDRSTRLEN);
    } else {
        port_ = 0;
        ipstr_[0] = 0;
    }

    fdsock_ = fd;
    sprintf(buff, "%s:%d", ipstr_, port_);
    id_ = string(buff);

    sess_ = NULL;
    node_name_ = node_name;

    pull_num_= 0;

    port_push_ = port_push;
}

server_data::~server_data()
{
    if(sess_) {
        delete sess_;
        sess_ = NULL;
        cout << id_ << "-" << node_name_ << " delete" << endl;
    }
}

int server_data::get_fd() const
{
    return fdsock_;
}

string server_data::get_node_name(void) const
{
    return node_name_;
}

session * server_data::get_session()
{
    return sess_;
}

void server_data::set_session(session * sess)
{
    sess_ = sess;
}

string server_data::get_id(void) const
{
    return id_;
}

bool server_data::operator ==(const server_data &p) const
{
    if(fdsock_ == p.get_fd() && node_name_ == p.get_node_name())
        return true;
    else
        return false;
}

void server_data::poll()
{
    session * sess = sess_;
    RTPPacket * pack;

    list<RTPPacket *> list_ppk_;
    list<RTPPacket *>::iterator it;

    sess->BeginDataAccess();
    if(sess->GotoFirstSourceWithData()) {
        do {
            while(NULL != (pack = sess->GetNextPacket())) {
                cout << id_ << "-" << node_name_ << " " << pack->GetTimestamp()
                     << " " << pack->GetExtensionID()
                     << " " << pack->GetPayloadLength() << endl;
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

void server_data::pull_inc()
{
    pull_num_++;
}

void server_data::pull_dec()
{
    if(pull_num_)
        pull_num_--;
}

void server_data::get_port_push(uint16_t *port)
{
    *port = port_push_;
}



/*
 *
 * Server
 *
 */
server::server(uint16_t port_udp)
    : event_handler()
{
    Json::CharReaderBuilder builder;
    creader_ = builder.newCharReader();

    port_base_ = 5000;
    loop_exit_ = false;
    tid_ = 0;
    tid_udp_ = 0;
    port_udp_ = port_udp;

    pthread_mutex_init(&lock_, NULL);
}

server::~server()
{
    if(0 != tid_) {
        loop_exit_ = true;
        pthread_join(tid_, NULL);
    }

    if(0 != tid_udp_) {
        pthread_cancel(tid_udp_);
        pthread_join(tid_udp_, NULL);
    }

    delete creader_;
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
    std::cout << "client connect: " << get_id(fd) << std::endl;
}

void server::on_close(int &fd, void *pdata)
{
    cout << "push client number: " << list_sd_.size() << endl;
    cout << "pull client number: " << list_pd_.size() << endl;
    sd_del(fd);
    sd_del_addr(fd);
    cout << "now push client number: " << list_sd_.size() << endl;
    cout << "now pull client number: " << list_pd_.size() << endl;

    std::cout << "client close: " << get_id(fd) << std::endl;
}

void server::on_read_err(int &fd, void *pdata, int err)
{
    errno = err;
    perror("on_read_err");

    cout << "push client number: " << list_sd_.size() << endl;
    cout << "pull client number: " << list_pd_.size() << endl;
    sd_del(fd);
    sd_del_addr(fd);
    cout << "now push client number: " << list_sd_.size() << endl;
    cout << "now pull client number: " << list_pd_.size() << endl;

    std::cout << "client read error: " << get_id(fd) << std::endl;
}

void server::on_read(int &fd, void *pdata, const void *rbuff, size_t rn)
{
    string rsp;
    Json::Value root;

    // parse json data
    rsp = string((char *)rbuff, rn);
    creader_->parse(rsp.c_str(), rsp.c_str() + rsp.size(), &root, NULL);

    if(string("ping") == root["cmd"].asString()) {
        root.clear();
        root["cmd"] = "pong";
        rsp = root.toStyledString();
        write(fd, rsp.c_str(), rsp.size());
    } else {
        string cmd = root["cmd"].asString();
        if(cmd == string("push")) {
            // check the sess has exist
            if(sd_exist(root["sn"].asString()) == false) {
                // register push sess
                session * sess = session::create(NULL, 0, port_base_);
                sd_reg(server_data(fd, root["sn"].asString(), port_base_));
                sd_set_sess(root["sn"].asString(), sess);

                root.clear();
                root["port"] = port_base_;
                rsp = root.toStyledString();
                write(fd, rsp.c_str(), rsp.size());

                port_base_ += 2;

                cout << "push client number: " << list_sd_.size() << endl;
            } else {
                root.clear();
                root["port"] = 0;
                root["msg"] = "node name/sn of client exist";
                rsp = root.toStyledString();
                write(fd, rsp.c_str(), rsp.size());
            }
        } else if(cmd == string("pull")) {
            if(sd_exist(root["sn"].asString()) == true) {
                uint16_t port;

                sd_add_addr(fd, root["sn"].asString(), root["ip"].asUInt(), uint16_t(root["port"].asUInt()));
                sd_get_port(fd, root["sn"].asString(), &port);

                root.clear();
                root["status"] = 0;
                root["msg"] = "pull ok";
                root["port"] = port;

                cout << "pull client number: " << list_pd_.size() << endl;
            } else {
                root.clear();
                root["status"] = 1;
                root["msg"] = "can't find node name/sn";
            }
            rsp = root.toStyledString();
            write(fd, rsp.c_str(), rsp.size());
        } else if(cmd == string("list")) {
            // return all camera
            root.clear();
            list<server_data>::iterator it;
            pthread_mutex_lock(&lock_);
            for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
                root["cam"].append(it->get_node_name());
            }
            pthread_mutex_unlock(&lock_);

            rsp = root.toStyledString();
            write(fd, rsp.c_str(), rsp.size());
        } else {
            root.clear();
            root["cmp"] = "unsuppot";
            rsp = root.toStyledString();
            write(fd, rsp.c_str(), rsp.size());

            // need to close client
            sd_del(fd);
            fd = -1;
        }
    }
}

bool server::sd_poll(void)
{
    list<server_data>::iterator it;

    pthread_mutex_lock(&lock_);
    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        it->poll();
    }
    pthread_mutex_unlock(&lock_);

    return loop_exit_;
}

void server::sd_reg(const server_data &sd)
{
    pthread_mutex_lock(&lock_);
    list_sd_.push_back(sd);
    pthread_mutex_unlock(&lock_);
}

void server::sd_set_sess(const string &node_name, session *sess)
{
    list<server_data>::iterator it;

    pthread_mutex_lock(&lock_);
    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(node_name == it->get_node_name()) {
            it->set_session(sess);
            break;
        }
    }
    pthread_mutex_unlock(&lock_);
}

void server::sd_del(int fd)
{
    list<server_data>::iterator it;

    pthread_mutex_lock(&lock_);
again:
    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(fd == it->get_fd()) {
            list_sd_.remove(*it);
            goto again;
        }
    }
    pthread_mutex_unlock(&lock_);
}

void server::sd_del(int fd, const string &node_name)
{
    list<server_data>::iterator it;

    pthread_mutex_lock(&lock_);
again:
    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(fd == it->get_fd() && node_name == it->get_node_name()) {
            list_sd_.remove(*it);
            goto again;
        }
    }
    pthread_mutex_unlock(&lock_);
}

bool server::sd_exist(const string &node_name)
{
    list<server_data>::iterator it;

    pthread_mutex_lock(&lock_);
    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(node_name == it->get_node_name()) {
            pthread_mutex_unlock(&lock_);
            return true;
        }
    }
    pthread_mutex_unlock(&lock_);

    return false;
}

void server::sd_add_addr(int fd, const string &node_name, uint32_t ip, uint16_t port)
{
    list<server_data>::iterator it;

    pthread_mutex_lock(&lock_);
    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(node_name == it->get_node_name()) {
            session * sess = it->get_session();
            sess->AddDestination(RTPIPv4Address(ip, port));
            it->pull_inc();
            cout << "add: " << ip << "-" << port << endl;
        }
    }

    list_pd_.push_back(pull_data(fd, node_name, ip, (uint16_t)port));
    pthread_mutex_unlock(&lock_);
}

void server::sd_del_addr(int fd)
{
    list<pull_data>::iterator it;
    list<server_data>::iterator it_sub;

    pthread_mutex_lock(&lock_);
again:
    for(it = list_pd_.begin(); it != list_pd_.end(); it++) {
        if(fd == it->fdsock_) {
            for(it_sub = list_sd_.begin(); it_sub != list_sd_.end(); it_sub++) {
                if(it->node_name_ == it_sub->get_node_name()) {
                    session * sess = it_sub->get_session();
                    sess->DeleteDestination(RTPIPv4Address(it->ip_, it->port_));
                    it_sub->pull_dec();
                }
            }

            list_pd_.remove(*it);
            goto again;
        }
    }
    pthread_mutex_unlock(&lock_);
}

void server::sd_get_port(int fd, const string &node_name, uint16_t *port)
{
    list<server_data>::iterator it;

    pthread_mutex_lock(&lock_);
    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(node_name == it->get_node_name()) {
            it->get_port_push(port);
            break;
        }
    }
    pthread_mutex_unlock(&lock_);
}

void * server::thread_poll(void *pdata)
{
    server * s = (server *)pdata;
    bool exit = false;
    while(!exit) {
        exit = s->sd_poll();

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

string server::get_id(int fd)
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



















