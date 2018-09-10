#include "endpoint.h"

endpoint::endpoint(const char *ip, short port)
{
    bzero(&m_saddr, sizeof(struct sockaddr_in));
    m_saddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &(m_saddr.sin_addr.s_addr));
    m_saddr.sin_port = htons(port);

    strcpy(m_ipstr, ip);
    m_valid = true;
}


const struct sockaddr_in * endpoint::get_saddr()
{
    return &m_saddr;
}

const char * endpoint::get_ipstr()
{
    return m_ipstr;
}

short endpoint::get_port()
{
    return ntohs(m_saddr.sin_port);
}

void endpoint::set_valid(bool valid)
{
    m_valid = valid;
}

bool endpoint::get_valid()
{
    return m_valid;
}
