#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

class endpoint
{
public:
    endpoint(const char *ip, short port);

    const struct sockaddr_in * get_saddr();
    const char * get_ipstr();
    short get_port();

    void set_valid(bool valid);
    bool get_valid();
private:
    char *m_ipstr[INET_ADDRSTRLEN];
    struct sockaddr_in m_saddr;
    bool m_valid;
};

#endif // ENDPOINT_H
