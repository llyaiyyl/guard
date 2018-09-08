#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#include <iostream>
#include <queue>

#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtplibraryversion.h"
#include "rtppacket.h"
#include "rtpsourcedata.h"

#include "list.h"

using namespace std;
using namespace jrtplib;

enum CLIENT_TYPE {
    CLI_TYPE_UNKNOW = 0,
    CLI_TYPE_NEW,
    CLI_TYPE_SEND,
    CLI_TYPE_RECV
};

class Frame
{
public:
    Frame(uint8_t * data, size_t len)
    {
        m_len = len;
        m_data = new uint8_t [m_len];
        memcpy(m_data, data, m_len);
    }
    ~Frame()
    {
        delete [] m_data;
    }

    uint8_t * get_data()
    {
        return m_data;
    }

    size_t get_len()
    {
        return m_len;
    }
private:
    size_t m_len;
    uint8_t * m_data;
};


typedef struct {
    uint32_t ip;        // host order
    uint16_t port;      // host order
    char ip_str[INET_ADDRSTRLEN];
    enum CLIENT_TYPE type;

    bool is_dst_list;
} client_data_t;

typedef struct {
    pthread_t tid;
    list_t * list_client;
    queue <Frame> vf;
    RTPSession * psess;
} g_data_t;





static void log_error(int ret)
{
    if(ret < 0) {
        std::cout << "ERROR: " << RTPGetErrorString(ret) << std::endl;
        exit(-1);
    }
}

static void get_client_addr(const RTPSourceData * dat, uint32_t *ip, uint16_t *port)
{
    if (dat->GetRTPDataAddress() != 0) {
        const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTPDataAddress());
        *ip = addr->GetIP();
        *port = addr->GetPort();
    }
    else if (dat->GetRTCPDataAddress() != 0) {
        const RTPIPv4Address *addr = (const RTPIPv4Address *)(dat->GetRTCPDataAddress());
        *ip = addr->GetIP();
        *port = addr->GetPort()-1;
    }
    else {
        *ip = 0;
        *port = 0;
    }
}

static client_data_t * insert_client_addr(list_t *list, uint32_t ip, uint16_t port)
{
    list_iterator_t * it;
    list_node_t * node;
    client_data_t * ptr_cd, * ptr_cd_ret;

    ptr_cd_ret = NULL;
    it = list_iterator_new(list, LIST_HEAD);
    while((node = list_iterator_next(it))) {
        ptr_cd = (client_data_t *)(node->val);

        if(ptr_cd->ip == ip && ptr_cd->port == port) {
            ptr_cd_ret = ptr_cd;
            break ;
        }
    }
    list_iterator_destroy(it);

    // not found, add it
    if(NULL == ptr_cd_ret) {
        ptr_cd_ret = (client_data_t *)malloc(sizeof(client_data_t));

        ptr_cd_ret->ip = ip;
        ptr_cd_ret->port = port;
        ptr_cd_ret->type = CLI_TYPE_NEW;
        ptr_cd_ret->is_dst_list = false;
        ip = htonl(ip);
        inet_ntop(AF_INET, &ip, ptr_cd_ret->ip_str, INET_ADDRSTRLEN);

        list_lpush(list, list_node_new(ptr_cd_ret));
    }

    return ptr_cd_ret;
}

static void * thread_send(void * pdata)
{
    list_iterator_t * it;
    list_node_t * node;
    client_data_t * ptr_cd;

    g_data_t * pgd;
    uint8_t * data;
    int ret;
    bool has_recv;

    pgd = (g_data_t *)pdata;
    while(1) {
        usleep(10 * 1000);

        // make sure queue <= 100
        while(pgd->vf.size() > 100)
            pgd->vf.pop();

        // if has recv devices, send all video frame
        has_recv = false;
        it = list_iterator_new(pgd->list_client, LIST_HEAD);
        while((node = list_iterator_next(it))) {
            ptr_cd = (client_data_t *)node->val;
            if(ptr_cd->type == CLI_TYPE_RECV) {
                has_recv = true;
                break;
            }
        }
        list_iterator_destroy(it);

        //
        while(!pgd->vf.empty() && has_recv) {
            pgd->psess->SendPacket(pgd->vf.front().get_data(), pgd->vf.front().get_len());
            pgd->vf.pop();
        }
    }
}

int main(int argc, char * argv[])
{
    g_data_t g_data;

    RTPSession sess;
    RTPUDPv4TransmissionParams transparams;
    RTPSessionParams sessparams;
    int ret;
    uint32_t ser_ip;
    uint16_t ser_port;

    // check arg and load
    if(argc != 3) {
        printf("usage:\n\t./guard-server ip port\n");
        return 0;
    }
    sscanf(argv[2], "%u", &ser_ip);
    ser_port = ser_ip;
    inet_pton(AF_INET, argv[1], &ser_ip);
    ser_ip = ntohl(ser_ip);


    g_data.list_client = list_new();

    // init and create rtp session
    sessparams.SetOwnTimestampUnit(1.0 / 9000.0);
    sessparams.SetMaximumPacketSize(1500);
    sessparams.SetAcceptOwnPackets(false);
    sessparams.SetUsePollThread(true);

    transparams.SetBindIP(ser_ip);
    transparams.SetPortbase(ser_port);
    ret = sess.Create(sessparams, &transparams);
    log_error(ret);

    // set rtp session
    sess.SetDefaultPayloadType(96);
    sess.SetDefaultMark(false);
    sess.SetDefaultTimestampIncrement(10);

    g_data.psess = &sess;

    // create send thread
    ret = pthread_create(&g_data.tid, NULL, thread_send, &g_data);
    if(0 != ret) {
        printf("pthread_create error %d\n", ret);
    }

    printf("server run: 0.0.0.0:%d\n", transparams.GetPortbase());
    while(1) {
        sess.BeginDataAccess();
        if(sess.GotoFirstSourceWithData()) {
            do {
                uint32_t cli_ip;
                uint16_t cli_port;
                RTPPacket *pack;
                client_data_t * ptr_cd = NULL;

                get_client_addr(sess.GetCurrentSourceInfo(), &cli_ip, &cli_port);
                if(0 != cli_ip && 0 != cli_port) {
                    ptr_cd = insert_client_addr(g_data.list_client, cli_ip, cli_port);
                    if(ptr_cd->type == CLI_TYPE_NEW) {
                        printf("found new client: %s:%d\n", ptr_cd->ip_str, ptr_cd->port);
                    }
                }

                while ((pack = sess.GetNextPacket()) != NULL) {
                    if(ptr_cd) {
                        if(pack->GetPayloadLength() == 4) {
                            uint8_t *data;
                            data = pack->GetPayloadData();
                            data[4] = 0;
                            if(0 == strcmp((const char *)data, "pull")) {
                                if(ptr_cd->type == CLI_TYPE_NEW || ptr_cd->type == CLI_TYPE_UNKNOW) {
                                    ret = sess.AddDestination(RTPIPv4Address(ptr_cd->ip, ptr_cd->port));
                                    log_error(ret);
                                    printf("add client: %s:%d\n", ptr_cd->ip_str, ptr_cd->port);
                                    ptr_cd->type = CLI_TYPE_RECV;
                                }
                            } else {
                                ptr_cd->type = CLI_TYPE_UNKNOW;
                            }
                        } else {
                            ptr_cd->type = CLI_TYPE_SEND;

                            // client push video frame
                            printf("%u: get video frame: %u %lu bytes, seqnum: %u\n", pack->GetSSRC(),
                                   pack->GetTimestamp(),
                                   pack->GetPayloadLength(),
                                   pack->GetSequenceNumber());

                            g_data.vf.push(*(new Frame(pack->GetPayloadData(), pack->GetPayloadLength())));
                        }
                    }
                    sess.DeletePacket(pack);
                }
            } while (sess.GotoNextSourceWithData());
        }
        sess.EndDataAccess();

        // wait 10ms
        RTPTime::Wait(RTPTime(0, 10 * 1000));
    }
    sess.BYEDestroy(RTPTime(10,0),0,0);

    list_destroy(g_data.list_client);
    return 0;
}





