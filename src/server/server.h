#ifndef SERVER_H
#define SERVER_H
#include <iostream>

#include "endpoint.h"
#include "receiver.h"

class server : public endpoint
{
public:
    server(int32_t fd);
    ~server();

protected:
    void on_recv_data(const void * data, size_t len);
    void on_recv_error(void);
    void on_connect(int32_t fd);
    void on_close(void);

private:
    receiver * m_recv;
};

#endif // SERVER_H
