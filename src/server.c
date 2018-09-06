#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#define SER_IP          "0.0.0.0"
#define SER_PUSH_PORT   9000
#define SER_PULL_PORT   9001

typedef struct {
	int fd_con;
} thread_arg_t;

static char recv_buff[1000];
static char send_buff[1000];

static void * thread_connect(void * pdata)
{
	int fd_con, ret;
	struct sockaddr_in con_addr;
	socklen_t con_len;
	char con_ipbuff[INET_ADDRSTRLEN];
	ssize_t recv_n;
	thread_arg_t * ptr_arg;

	ptr_arg = (thread_arg_t *)pdata;

	fd_con = ptr_arg->fd_con;
	con_len = sizeof(struct sockaddr_in);
	ret = getpeername(fd_con, (struct sockaddr *)&con_addr, &con_len);
	if(-1 == ret) {
		perror("get peername");
		goto exit;
	}

	inet_ntop(AF_INET, &con_addr.sin_addr.s_addr, con_ipbuff, INET_ADDRSTRLEN);
	printf("client ip: %s:%d\n", con_ipbuff, ntohs(con_addr.sin_port));


	while(1) {
		recv_n = recv(fd_con, recv_buff, 1000, 0);
		if(-1 == recv_n) {
			if(errno != EINTR) {
				printf("recv data from client error\n");
				goto exit;
			} else
				continue ;
		} else if(0 == recv_n) {
			printf("client %s close\n", con_ipbuff);
			goto exit;
		}
		recv_buff[recv_n] = 0;
		printf("%s\n", recv_buff);

		if(0 == strcmp(recv_buff, "ping")) {
			strcpy(send_buff, "pong");
			send(fd_con, send_buff, strlen(send_buff), 0);
		}
		else {
			strcpy(send_buff, "not support");
			send(fd_con, send_buff, strlen(send_buff), 0);
		}
	}

exit:
    free(ptr_arg);
    close(fd_con);
	return (void *)0;
}

static void * thread_push(void * pdata)
{
    int fd_push, fd_con, ret;
    pthread_t tid;

    fd_push = *((int *)pdata);
    while(1) {
        fd_con = accept(fd_push, NULL, NULL);
        if(-1 == fd_con) {
            printf("accept error\n");
            continue ;
        }

        // create a thread to process
        thread_arg_t * ptr_arg;
        ptr_arg = (thread_arg_t *)malloc(sizeof(thread_arg_t));
        ptr_arg->fd_con = fd_con;
        ret = pthread_create(&tid, NULL, thread_connect, ptr_arg);
        if(ret) {
            printf("create thread error\n");
            continue ;
        }
    }
}

static void * thread_pull(void * pdata)
{

}


int main(int argc, char * argv[])
{
    int fd_push, fd_pull, ret;
	struct sockaddr_in	ser_addr;
    pthread_t tid_push, tid_pull;
	void * tret;

    /*
     * create server port to receive frame
     */
    fd_push = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == fd_push) {
		printf("can't create socket\n");
		goto exit;
	}

	bzero(&ser_addr, sizeof(struct sockaddr_in));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr.sin_port = htons(SER_PUSH_PORT);

    ret = bind(fd_push, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in));
	if(-1 == ret) {
		printf("bind sockaddr error\n");
		goto err0;
	}

    ret = listen(fd_push, 5);
	if(-1 == ret) {
		printf("listen error\n");
		goto err0;
	}
    printf("guard-server push listen: %s:%d\n", SER_IP, SER_PUSH_PORT);

    /*
     * create server port to send frame
     */
    fd_pull = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == fd_pull) {
        printf("can't create socket\n");
        goto err0;
    }

    bzero(&ser_addr, sizeof(struct sockaddr_in));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr.sin_port = htons(SER_PULL_PORT);

    ret = bind(fd_pull, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in));
    if(-1 == ret) {
        printf("bind sockaddr error\n");
        goto err1;
    }

    ret = listen(fd_pull, 5);
    if(-1 == ret) {
        printf("listen error\n");
        goto err1;
    }
    printf("guard-server pull listen: %s:%d\n", SER_IP, SER_PULL_PORT);


    /*
     * start thread
     */
    ret = pthread_create(&tid_push, NULL, thread_push, &fd_push);
    if(ret){
        printf("create thread error\n");
        goto err1;
    }
    ret = pthread_create(&tid_pull, NULL, thread_pull, &fd_pull);
    if(ret){
        printf("create thread error\n");
        goto err1;
    }


    while(1){
        if(getchar() == 'q')
            break;
    }
    pthread_cancel(tid_push);
    pthread_cancel(tid_pull);
    pthread_join(tid_push, NULL);
    pthread_join(tid_pull, NULL);

err1:
    close(fd_pull);
err0:
    close(fd_push);
exit:
	return 0;
}

























