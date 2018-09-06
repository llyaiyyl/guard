#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#define SER_IP		"0.0.0.0"
#define SER_PORT	9000

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
	return (void *)0;
}


int main(int argc, char * argv[])
{
	int fd_ser, fd_con, ret;
	struct sockaddr_in	ser_addr;
	pthread_t tid;
	void * tret;

	// create server and listen
	fd_ser = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == fd_ser) {
		printf("can't create socket\n");
		goto exit;
	}

	bzero(&ser_addr, sizeof(struct sockaddr_in));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ser_addr.sin_port = htons(SER_PORT);

	ret = bind(fd_ser, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in));
	if(-1 == ret) {
		printf("bind sockaddr error\n");
		goto err0;
	}

	ret = listen(fd_ser, 5);
	if(-1 == ret) {
		printf("listen error\n");
		goto err0;
	}
	printf("guard-server listen: %s:%d\n", SER_IP, SER_PORT);

	while(1) {
		fd_con = accept(fd_ser, NULL, NULL);
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
		pthread_join(tid, &tret);
		close(fd_con);
		free(ptr_arg);
	}

err0:
	close(fd_ser);
exit:
	return 0;
}

























