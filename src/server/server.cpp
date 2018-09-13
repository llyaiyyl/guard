#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "json/json.h"
#include "server.h"

using namespace std;

server_data::server_data(int fd)
{
    ipstr_[0] = 0;
    port_ = 0;

    fd_ = fd;
}

server_data::~server_data()
{
}

void server_data::set_addr(const char *ipstr, uint16_t port)
{
    strcpy(ipstr_, ipstr);
    port_ = port;
}

const char * server_data::get_ipstr()
{
    return ipstr_;
}

uint16_t server_data::get_port()
{
    return port_;
}

int server_data::get_fd()
{
    return fd_;
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

    pthread_create(&tid_, NULL, thread_poll, this);
}

server::~server()
{

}


void server::on_connect(int &fd, void **pdata)
{
    struct sockaddr_in saddr;
    socklen_t slen;
    char ipstr[INET_ADDRSTRLEN];
    uint16_t port;
    server_data * sd;

    slen = sizeof(struct sockaddr_in);
    bzero(&saddr, slen);
    if(0 == getpeername(fd, (struct sockaddr *)&saddr, &slen)) {
        port = ntohs(saddr.sin_port);
        inet_ntop(saddr.sin_family, &(saddr.sin_addr.s_addr), ipstr, INET_ADDRSTRLEN);
    } else {
        ipstr[0] = 0;
        port = 0;
    }

    // save server data to private
    sd = new server_data(fd);
    sd->set_addr(ipstr, port);
    *pdata = sd;

    std::cout << "client connect: " << ipstr << ":" << port << std::endl;
}

void server::on_close(int &fd, void *pdata)
{
    server_data * ptr = (server_data *)pdata;

    if(this->s_recv_) {
        delete this->s_recv_;
        this->s_recv_ = NULL;
        push_num_--;
    }

    if(this->s_send_) {
        delete this->s_send_;
        this->s_send_ = NULL;
        this->haspull = false;
        pull_num_--;
    }

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



















