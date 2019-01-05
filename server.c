#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <pthread.h>

// TODO: assumes sockaddr_in6
struct thread_data {
	int recv_socket, send_socket;
	struct sockaddr_in6 recv_addr, send_addr;
};

//pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// TODO: both p1 and p2 send 4 ints

void *passthrough(void *ptr) {
	struct thread_data *td;
	int buf[4];
	int n;
	int recvlen;
	int sendlen;
	td = (struct thread_data *) ptr;
	// send connection granted
	// connection granted: server sends [-1, -1, -1, -1]
	for(int i = 0; i < 4; i++) {
		buf[i] = htonl(-1);
	}
	recvlen = sizeof(struct sockaddr_in6);
	n = sendto(td->recv_socket, buf, sizeof(int) * 4, 0,
		(struct sockaddr *) &(td->recv_addr), recvlen);
	
	// TODO: error checking with n
	// while p1 is asking to connect, send connection granted
	// asking to connect: player sends [-1, -1, -1, -1]
	while(1) {
		n = recvfrom(td->recv_socket, buf, sizeof(int) * 4, 0,
			(struct sockaddr *) &(td->recv_addr), &recvlen);
		if(ntohl(buf[0]) != -1)
			break;
		n = sendto(td->recv_socket, buf, sizeof(int) * 4, 0,
			(struct sockaddr *) &(td->recv_addr), recvlen);
	}

	// send match data to player 2
	sendlen = sizeof(struct sockaddr_in6);
	n = sendto(td->send_socket, buf, sizeof(int) * 4, 0,
		(struct sockaddr *) &(td->send_addr), sendlen);
	while(1) {
		n = recvfrom(td->recv_socket, buf, sizeof(int) * 4, 0,
			(struct sockaddr *) &(td->recv_addr), &recvlen);
		n = sendto(td->send_socket, buf, sizeof(int) * 4, 0,
			(struct sockaddr *) &(td->send_addr), sendlen);
	}

	// done
	pthread_exit(NULL);
}

void error(char *msg){
	perror(msg);
	exit(0);
}

int main(int argc,char *argv[]){
	int n;
	int p1_socket, p2_socket;
	int p1_client_len, p2_client_len;
	struct sockaddr_in6 p1_client, p2_client;
	struct sockaddr_in6 p1_server, p2_server;

	// set up sockets
	p1_socket = socket(AF_INET6, SOCK_DGRAM, 0);
	if(p1_socket < 0) {
		error("error occured while opening p1_socket\n");
	}
	p2_socket = socket(AF_INET6, SOCK_DGRAM, 0);
	if(p2_socket < 0) {
		error("error occured while opening p2_socket\n");
	}

	// set up player 1 server information
	bzero(&p1_server, sizeof(p1_server)); // zero out
	p1_server.sin6_family = AF_INET6;
	p1_server.sin6_port = htons(atoi("9001"));
	p1_server.sin6_addr = in6addr_any;
	// set up player 2 server information
	bzero(&p2_server, sizeof(p2_server)); // zero out
	p2_server.sin6_family = AF_INET6;
	p2_server.sin6_port = htons(atoi("9002"));
	p2_server.sin6_addr = in6addr_any;

	// bind server information to socket
	if(bind(p1_socket, (struct sockaddr *) &p1_server,
		sizeof(p1_server)) < 0){
		error("error occured while binding socket.\n");
	}
	if(bind(p2_socket, (struct sockaddr *) &p2_server,
		sizeof(p2_server)) < 0){
		error("error occured while binding socket2.\n");
	}

	int p1buf[4];
	int p2buf[2];
	// wait for two players
	// get first message from both players
	p1_client_len = sizeof(struct sockaddr_in6);
	n = recvfrom(p1_socket, p1buf, sizeof(int) * 4, 0,
		(struct sockaddr *) &p1_client, &p1_client_len);
	if(n < 0)
		error("recv p1_socket");
	p2_client_len = sizeof(struct sockaddr_in6);
	n = recvfrom(p2_socket, p2buf, sizeof(int) * 2, 0,
		(struct sockaddr *) &p2_client, &p2_client_len);
	if(n < 0)
		error("recv p2_socket");
	
	// now done waiting for two players

	// set up thread information
	pthread_t thread1, thread2;
	int ret1, ret2;
	struct thread_data td1, td2;
	
	td1.recv_socket = p1_socket;
	td1.send_socket = p2_socket;
	td1.recv_addr = (struct sockaddr_in6) p1_client;
	td1.send_addr = (struct sockaddr_in6) p2_client;

	td2.recv_socket = p2_socket;
	td2.send_socket = p1_socket;
	td2.recv_addr = (struct sockaddr_in6) p2_client;
	td2.send_addr = (struct sockaddr_in6) p1_client;

	ret1 = pthread_create(&thread1, NULL, passthrough, (void *) &td1);
	ret2 = pthread_create(&thread2, NULL, passthrough, (void *) &td2);

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);

	return 0;
}
