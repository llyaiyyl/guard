#include "tcp_socket.h"

tcp_socket::tcp_socket()
{
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == m_sockfd) {
        printf("socket open error\n");
    }
}

tcp_socket::~tcp_socket()
{
    if(-1 != m_sockfd) {
        close(m_sockfd);
        m_sockfd = -1;
    }
}


int tcp_socket::bind_edp(endpoint &edp)
{
    m_ledp = edp;
    return bind(m_sockfd, (struct sockaddr *)m_ledp.get_saddr(), sizeof(struct sockaddr_in));
}

int tcp_socket::connect_edp(endpoint &edp)
{
    m_redp = edp;
    return connect(m_sockfd, (struct sockaddr *)m_redp.get_saddr(), sizeof(struct sockaddr_in));
}

int tcp_socket::listen_edp()
{
    return listen(m_sockfd, 5);
}

int tcp_socket::accept_run(endpoint &edp)
{
    int fd;
    socklen_t slen;
    sockaddr_in saddr;
    char ip[INET_ADDRSTRLEN];

    fd = accept(m_sockfd, (struct sockaddr *)&saddr, &slen);
    if(-1 == fd) {
        edp.set_valid(false);
        return fd;
    }

    inet_ntop(AF_INET, &(saddr.sin_addr.s_addr), ip, INET_ADDRSTRLEN);
    endpoint edp_ret(ip, ntohs(saddr.sin_port));
    edp = edp_ret;
}





















