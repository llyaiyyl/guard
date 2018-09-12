#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "server.h"

using namespace std;

server_data::server_data(int fd)
{
    ipstr_[0] = 0;
    port_ = 0;
    recv_ = NULL;
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

void server_data::set_recv(receiver *ptr)
{
    recv_ = ptr;
}

receiver * server_data::get_recv()
{
    return recv_;
}

void server_data::set_send(sender *ptr)
{
    sder_ = ptr;
}

sender * server_data::get_send()
{
    return sder_;
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
void server::on_connect(int fd, void **pdata)
{
    struct sockaddr_in saddr;
    socklen_t slen;
    char ipstr[INET_ADDRSTRLEN];
    uint16_t port;

    slen = sizeof(struct sockaddr_in);
    bzero(&saddr, slen);
    if(0 == getpeername(fd, (struct sockaddr *)&saddr, &slen)) {
        port = ntohs(saddr.sin_port);
        inet_ntop(saddr.sin_family, &(saddr.sin_addr.s_addr), ipstr, INET_ADDRSTRLEN);
    } else {
        ipstr[0] = 0;
        port = 0;
    }

    *pdata = (new server_data(fd));
    std::cout << "client connect: " << ipstr << ":" << port << std::endl;
}

void server::on_close(int fd, void *pdata)
{
    server_data * ptr = (server_data *)pdata;

    if(ptr->get_recv()) {
        ptr->get_recv()->stop();

        delete ptr->get_recv();
        ptr->set_recv(NULL);

        if(recv_num_)
            recv_num_--;
    }

    std::cout << "on_close" << std::endl;
}

void server::on_read_err(int fd, void *pdata, int err)
{
    errno = err;
    perror("on_read_err");
}

void server::on_read(int fd, void *pdata, const void *rbuff, size_t rn)
{
    string req, rsp;
    server_data * ptr = (server_data *)pdata;

    rsp = string((char *)rbuff, rn);
    cout << "recv: " << rsp << endl;

    if(rsp == string("ping")) {
        write(fd, "pong", 4);
    } else if(rsp == string("push")) {
        if(recv_num_ < 1) {
            // chose a port to receiv data

            // create receiver object
            if(ptr->get_recv() == NULL) {
                ptr->set_recv(new receiver(5000));
                ptr->get_recv()->run();
                recv_num_++;
            }

            // return the listen port
            rsp = string("5000");
            write(fd, rsp.c_str(), rsp.size());
        }
    } else if(rsp == string("pull")) {
        if(pull_num_ < 1) {
            // chose a port to

            if(ptr->get_send() == NULL) {
                ptr->set_send(new sender(ptr->get_recv(), 6000));
                ptr->get_send()->run();
                pull_num_++;
            }

            // return the listen port
            rsp = string("6000");
            write(fd, rsp.c_str(), rsp.size());
        }
    }
    else {
        write(fd, "unsuppot", 8);
    }
}



