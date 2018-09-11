#include "server.h"
using namespace std;

server::server(int32_t fd) : endpoint(fd)
{
    m_recv = NULL;
}
server::~server()
{

}



// virtual function
void server::on_connect(int32_t fd)
{
    cout << "client: " << this->get_remote_ip() << ":"
         << this->get_remote_port() << " connect." << endl;
}

void server::on_close()
{
    cout << "client: " << this->get_remote_ip() << ":"
         << this->get_remote_port() << " close." << endl;
}

void server::on_recv_data(const void *data, size_t len)
{
    string req, rsp;

    rsp = string((const char *)data, len);
    cout << "recv: " << rsp << endl;

    if(rsp == string("ping")) {
        this->send_data("pong", 4);
    } else if(rsp == string("request")) {
        // chose a port to receiv data

        // create receiver object
        if(NULL == m_recv) {
            m_recv = new receiver(5000);
            m_recv->run();
        }

        // return the listen port
        rsp = string("5000");
        this->send_data(rsp.c_str(), rsp.size());
    }
    else {
        this->send_data("unsuppot", 8);
    }
}

void server::on_recv_error()
{
    cout << "on_recv_error" << endl;
}



