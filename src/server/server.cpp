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
    if(0 == getpeername(fd, (struct sockaddr *)&saddr, &slen)) {
        port_ = ntohs(saddr.sin_port);
        inet_ntop(saddr.sin_family, &(saddr.sin_addr.s_addr), ipstr_, INET_ADDRSTRLEN);
    } else {
        ipstr_[0] = 0;
        port_ = 0;
    }

    fd_ = fd;
    status_ = sd_init;

    sprintf(buff, "%s:%d", ipstr_, port_);
    client_addr_ = string(buff);
}

server_data::~server_data()
{
}

const char * server_data::get_ipstr()
{
    return ipstr_;
}

string server_data::get_client_addr()
{
    return client_addr_;
}

uint16_t server_data::get_port()
{
    return port_;
}

server_data * server_data::get_this()
{
    return this;
}






/*
 * client data
 */
client_data::client_data(int fd)
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

    fd_ = fd;
    sprintf(buff, "%s:%d", ipstr_, port_);
    client_addr_ = string(buff);
}

client_data::~client_data()
{

}

const char * client_data::get_ipstr()
{
    return ipstr_;
}

string client_data::get_client_addr()
{
    return client_addr_;
}

uint16_t client_data::get_port()
{
    return port_;
}



/*
 *
 * Server
 *
 */
server::server()
    : event_handler()
{
    push_num_ = 0;
    pull_num_ = 0;

    s_send_ = NULL;
    s_recv_ = NULL;

    haspull = false;
}

server::~server()
{

}

void server::run()
{
    pthread_create(&tid_, NULL, thread_poll, this);
}


void server::on_connect(int &fd, void **pdata)
{
    client_data * cd;


    // save server data to private
    cd = new client_data(fd);
    *pdata = cd;

    // add server data to client list
    list_sd_.push_back(server_data(fd));

    std::cout << "client connect: " << cd->get_ipstr() << ":" << cd->get_port() << std::endl;
}

void server::on_close(int &fd, void *pdata)
{
    client_data * cd = (client_data *)pdata;
    server_data * sd;

    sd = get_sd(cd->get_client_addr());
    if(sd) {
        // delete this client all session
    } else {
        // can find client data
    }

    // remove this client
    // list_sd_.remove(*ptr);

    // delete client private data;

    std::cout << "on_close" << std::endl;
}

void server::on_read_err(int &fd, void *pdata, int err)
{
    errno = err;
    perror("on_read_err");
}

void server::on_read(int &fd, void *pdata, const void *rbuff, size_t rn)
{
    string req, rsp;
    server_data * psd = (server_data *)pdata;
    Json::Value root;
    Json::Reader reader;

    rsp = string((char *)rbuff, rn);
    reader.parse(rsp, root);

    if(string("ping") == root["cmd"].asString()) {
        root.clear();
        root["cmd"] = "pong";
        rsp = root.toStyledString();
        write(fd, rsp.c_str(), rsp.size());
    } else {
        reader.parse(rsp, root);
        string cmd = root["cmd"].asString();
        if(cmd == string("push")) {
            if(push_num_ < 1) {
                // chose a port to receiv data

                // create receiver object
                if(NULL == this->s_recv_) {
                    this->s_recv_ = session::create(NULL, 0, 5000);
                    push_num_++;
                }

                // return the listen port
                root.clear();
                root["port"] = 5000;
                rsp = root.toStyledString();
                write(fd, rsp.c_str(), rsp.size());
            } else {
                // return unvalid listen port
                root.clear();
                root["port"] = 0;
                rsp = root.toStyledString();
                write(fd, rsp.c_str(), rsp.size());
            }
        } else if(cmd == string("pull")) {
            if(pull_num_ < 1) {
                // chose a port to

                if(NULL == this->s_send_) {
                    this->s_send_ = session::create(NULL, 0, 6000);
                    pull_num_++;
                }

                // return the listen port
                root.clear();
                root["port"] = 6000;
                rsp = root.toStyledString();
                write(fd, rsp.c_str(), rsp.size());
            } else {
                // return unvalid listen port
                root.clear();
                root["port"] = 0;
                rsp = root.toStyledString();
                write(fd, rsp.c_str(), rsp.size());
            }
        } else {
            root.clear();
            root["cmp"] = "unsuppot";
            rsp = root.toStyledString();
            write(fd, rsp.c_str(), rsp.size());
            // need to close client
            fd = -1;
        }
    }
}

server_data * server::get_sd(const string &addr)
{
    list<server_data>::iterator it;

    for(it = list_sd_.begin(); it != list_sd_.end(); it++) {
        if(addr == it->get_client_addr()) {
            return it->get_this();
        }
    }

    return NULL;
}

void * server::thread_poll(void *pdata)
{
    server * s = (server *)pdata;
    session * sess;

    while(1) {
        // has recv session
        if(s->s_recv_) {
            sess = s->s_recv_;

            sess->BeginDataAccess();
            if(sess->GotoFirstSourceWithData()) {
                do {
                    RTPPacket * pack;
                    while(NULL != (pack = sess->GetNextPacket())) {
                        cout << pack->GetSSRC() << ": get data " << pack->GetPacketLength() << " bytes "
                             << pack->GetSequenceNumber() << endl;

                        if(s->haspull) {
                            s->s_send_->SendPacket(pack->GetPayloadData(), pack->GetPacketLength());
                        }

                        sess->DeletePacket(pack);
                    }
                } while(sess->GotoNextSourceWithData());
            }
            sess->EndDataAccess();
        }

        // has send session
        if(s->s_send_ && (!s->haspull)) {
            sess = s->s_send_;
            sess->BeginDataAccess();
            if(sess->GotoFirstSourceWithData()) {
                RTPSourceData * dat;
                uint32_t ip;
                uint16_t port;

                do {
                    // get source ip
                    dat = sess->GetCurrentSourceInfo();
                    if (dat->GetRTPDataAddress() != 0) {
                        const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTPDataAddress());
                        ip = addr->GetIP();
                        port = addr->GetPort();
                    }
                    else if (dat->GetRTCPDataAddress() != 0) {
                        const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTCPDataAddress());
                        ip = addr->GetIP();
                        port = addr->GetPort()-1;
                    }
                    else {
                        ip = 0;
                        port = 0;
                    }

                    if(ip != 0 && port != 0) {
                        sess->AddDestination(RTPIPv4Address(ip, port));
                        s->haspull = true;
                    }
                } while(sess->GotoNextSourceWithData());
            }
            sess->EndDataAccess();
        }

        // wait 10ms
        RTPTime::Wait(RTPTime(0, 10 * 1000));
    }
}



















