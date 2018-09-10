#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include <unistd.h>
#include "endpoint.h"


class tcp_socket
{
public:
    tcp_socket();
    ~tcp_socket();

    int bind_edp(endpoint &edp);
    int connect_edp(endpoint &edp);
    int listen_edp();
    int accept_run(endpoint &edp);
private:
    int m_sockfd;

    endpoint m_ledp;
    endpoint m_redp;
};

#endif // TCP_SOCKET_H
