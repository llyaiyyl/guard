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
server_data::server_data(int fd)
{
    struct sockaddr_in saddr;
    socklen_t slen;
    char buff[24];

    slen = sizeof(struct sockaddr_in);
    bzero(&saddr, slen);
    getpeername(fd, (struct sockaddr *)&saddr, &slen);
    port_ = ntohs(saddr.sin_port);
    inet_ntop(saddr.sin_family, &(saddr.sin_addr.s_addr), ipstr_, INET_ADDRSTRLEN);

    sprintf(buff, "%s:%d", ipstr_, port_);
    id_ = string(buff);

    fdsock_ = fd;
    node_name_ = string("unknow");
    sess_ = NULL;
    sess_pull_ = NULL;
    sess_type_ = sess_unknow;

    has_client = false;
}

server_data::server_data(int fd, string node_name)
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
        ipstr_[0] = 0;
        port_ = 0;
    }

    sprintf(buff, "%s:%d", ipstr_, port_);
    id_ = string(buff);

    fdsock_ = fd;
    node_name_ = node_name;
    sess_ = NULL;
    sess_pull_ = NULL;
    sess_type_ = sess_unknow;

    has_client = false;
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

void server_data::set_pull_session(session *sess)
{
    sess_pull_ = sess;
}

void server_data::set_session(session * sess)
{
    sess_ = sess;
}

void server_data::set_sess_type(server_data::sess_type stype)
{
    sess_type_ = stype;
}

server_data::sess_type server_data::get_sess_type()
{
    return sess_type_;
}

string server_data::get_id(void) const
{
    return id_;
}

bool server_data::operator ==(const server_data &p) const
{
    if(id_ == p.get_id())
        return true;
    else
        return false;
}

void server_data::poll()
{
    server_data::sess_type stype = sess_type_;
    session * sess = sess_;

    if(stype == sess_push) {
        sess->BeginDataAccess();
        if(sess->GotoFirstSourceWithData()) {
            do {
                // get remote client address
                if(false == has_client) {
                    RTPSourceData * dat;

                    // get source ip
                    dat = sess->GetCurrentSourceInfo();
                    if (dat->GetRTPDataAddress() != 0) {
                        const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTPDataAddress());
                        client_ip = addr->GetIP();
                        client_port = addr->GetPort();
                        has_client = true;
                    }
                    else if (dat->GetRTCPDataAddress() != 0) {
                        const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTCPDataAddress());
                        client_ip = addr->GetIP();
                        client_port = addr->GetPort() - 1;
                        has_client = true;
                    }
                }

                // read packet
                RTPPacket * pack;
                while(NULL != (pack = sess->GetNextPacket())) {
                    cout << pack->GetSSRC() << ": get data " << pack->GetPacketLength() << " bytes "
                         << pack->GetSequenceNumber() << endl;

                    if(sess_pull_) {
                        sess_pull_->SendPacket(pack->GetPayloadData(), pack->GetPayloadLength());
                        cout << "send to client" << endl;
                    }

                    sess->DeletePacket(pack);
                }
            } while(sess->GotoNextSourceWithData());
        }
        sess->EndDataAccess();
    }

    if(stype == sess_pull) {
        sess->BeginDataAccess();
        if(sess->GotoFirstSourceWithData()) {
            do {
                // get remote client address
                if(false == has_client) {
                    RTPSourceData * dat;

                    // get source ip
                    dat = sess->GetCurrentSourceInfo();
                    if (dat->GetRTPDataAddress() != 0) {
                        const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTPDataAddress());
                        client_ip = addr->GetIP();
                        client_port = addr->GetPort();
                        has_client = true;
                    }
                    else if (dat->GetRTCPDataAddress() != 0) {
                        const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTCPDataAddress());
                        client_ip = addr->GetIP();
                        client_port = addr->GetPort() - 1;
                        has_client = true;
                    }

                    if(has_client == true) {
                        sess->AddDestination(RTPIPv4Address(client_ip, client_port));
                    }
                }
            } while(sess->GotoNextSourceWithData());
        }
        sess->EndDataAccess();
    }
}



/*
 *
 * Server
 *
 */
server::server()
    : event_handler()
{
    Json::CharReaderBuilder builder;
    creader_ = builder.newCharReader();

    port_base_ = 5000;
}

server::~server()
{
    delete creader_;
}

void server::run()
{
    pthread_create(&tid_, NULL, thread_poll, this);
}



void server::on_connect(int &fd, void **pdata)
{
    server_data sd(fd);
    std::cout << "client connect: " << sd.get_id() << std::endl;
}

void server::on_close(int &fd, void *pdata)
{
    sd_del(fd);

    server_data sd(fd);
    std::cout << "client close: " << sd.get_id() << std::endl;
}

void server::on_read_err(int &fd, void *pdata, int err)
{
    errno = err;
    perror("on_read_err");

    sd_del(fd);

    server_data sd(fd);
    std::cout << "client read error: " << sd.get_id() << std::endl;
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
                sd_reg(server_data(fd, root["sn"].asString()));
                sd_config(root["sn"].asString(), sess, server_data::sess_push);

                root.clear();
                root["port"] = port_base_;
                rsp = root.toStyledString();
                write(fd, rsp.c_str(), rsp.size());

                port_base_ += 2;
            } else {
                root.clear();
                root["port"] = 0;
                root["msg"] = "session node name has exist";
                rsp = root.toStyledString();
                write(fd, rsp.c_str(), rsp.size());
            }
        } else if(cmd == string("pull")) {
            if(sd_exist(root["sn"].asString()) == true) {
                session * sess = session::create(NULL, 0, port_base_);
                sd_reg(server_data(fd, root["sn"].asString()));
                sd_config(root["sn"].asString(), sess, server_data::sess_pull);
                sd_set_pullsess(root["sn"].asString(), sess);

                root.clear();
                root["port"] = port_base_;
                rsp = root.toStyledString();
                write(fd, rsp.c_str(), rsp.size());

                port_base_ += 2;
            } else {
                root.clear();
                root["port"] = 0;
                root["msg"] = "can't find sn";
                rsp = root.toStyledString();
                write(fd, rsp.c_str(), rsp.size());
            }
        } else if(cmd == string("list")) {
            // return

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

void server::sd_poll()
{
    list<server_data>::iterator it;

    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        it->poll();
    }
}

void server::sd_reg(const server_data &sd)
{
    list_sd_.push_back(sd);
}

void server::sd_config(const string &node_name, session *sess, server_data::sess_type stype)
{
    list<server_data>::iterator it;

    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(node_name == it->get_node_name() && it->get_sess_type() == server_data::sess_unknow) {
            it->set_session(sess);
            it->set_sess_type(stype);
            break;
        }
    }
}

void server::sd_set_pullsess(const string &node_name, session *sess)
{
    list<server_data>::iterator it;

    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(it->get_node_name() == node_name && it->get_sess_type() == server_data::sess_push) {
            it->set_pull_session(sess);
        }
    }
}

void server::sd_del(int fd)
{
    list<server_data>::iterator it;

    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(fd == it->get_fd()) {
            list_sd_.remove(*it);
        }
    }
}

void server::sd_del(const string &node_name)
{
    list<server_data>::iterator it;

    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(node_name == it->get_node_name()) {
            list_sd_.remove(*it);
            break;
        }
    }
}

bool server::sd_exist(const string &node_name)
{
    list<server_data>::iterator it;

    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(node_name == it->get_node_name()) {
            return true;
        }
    }

    return false;
}




// privata function
void * server::thread_poll(void *pdata)
{
    server * s = (server *)pdata;
    while(1) {
        s->sd_poll();

        // wait 10ms
        RTPTime::Wait(RTPTime(0, 10 * 1000));
    }
}



















