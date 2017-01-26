//-------------------------------------------------------------
// CSCE 438: Distributed objects programming
// HW1: A Chat Room Service
// 
// Chat Room
// Vincent Velarde, Kyle Rowland
// January 2017
//
// NOTES:
//  I still need to clean up my code, polish up some 
//    functionality, and add more comments.
//
//-------------------------------------------------------------

#include <cstdlib>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

//write message to all client sockets
void broadcast(int max_fds, int sockfd, char *msg) {
	for(int i=0; i<max_fds; i++) {
		if(i != sockfd) {
			if(write(i, msg, strlen(msg)) < 0) {
				perror("ERROR, broadcast-write");
			}
		}
	}
}

//handle reading/writing from/to client sockets
void msgHandler(int i, fd_set *fds, int sockfd, int max_fds) {
	int n;	//number of chars sent/recieved
	char readBuf[256], writeBuf[256];
	
	//read
	if((n = read(i, readBuf, sizeof(readBuf))) <= 0) {
		if(n == 0) {
			printf("client %d left\n", i);
		}
		else {
			perror("ERROR, msgHandler-read");
		}
		close(i);
		FD_CLR(i, fds);
	}
	else {	//write
		for(int j=0; j<=max_fds; j++) {
			if(FD_ISSET(j, fds)) {
				if(j != sockfd && j != i) {
					if(write(j, readBuf, n)  < 0) {
						perror("ERROR, msgHandler-write");
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	fd_set fds, rfds;
	int max_fds, port, acceptfd;
	int sockfd = 0;
	socklen_t length;
	struct sockaddr_in server, client;
	
	if(argc < 2) {	//make sure user provided a port number
		fprintf(stderr, "ERROR, no port\n");
		exit(1);
	}
	port = atoi(argv[1]);	//convert given port from string to int
	
	//server startup message
	printf("-------------------------------\n");
	printf("Chat Room Server\n");
	printf("Vincent Velarde, Kyle Rowland\n");
	printf("January 2017\n");
	printf("-------------------------------\n\n");
	
	//create and check socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("ERROR, socket");
		exit(1);
	}
	
	//set fields in "sockaddr_in server"
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = INADDR_ANY;	//INADDR_ANY gets the IP of server machine
	memset(server.sin_zero, '\0', sizeof(server.sin_zero));
	
	//make address reuseable
	int yes = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("ERROR, setsockopt");
		exit(1);
	}
	
	//bind socket to address
	if(bind(sockfd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
		perror("ERROR, bind");
		exit(1);
	}
	
	//listen for client connections
	listen(sockfd, 5);	//second argument is queue size
	printf("listening...\n");
	
	fflush(stdout);
	
	FD_ZERO(&fds);
	FD_ZERO(&rfds);
	FD_SET(sockfd, &fds);
	max_fds = sockfd;
	
	while(true) {
		rfds = fds;
		
		if(select(max_fds+1, &rfds, NULL, NULL, NULL)  < 0) {
			perror("ERROR, select");
			exit(1);
		}
		
		for(int i=0; i<=max_fds; i++) {
			if(FD_ISSET(i, &rfds)) {
				if(i == sockfd) {
					acceptfd = 0;
					length = sizeof(struct sockaddr_in);
					
					if((acceptfd = accept(sockfd, (struct sockaddr *) &client, &length)) == -1) {
						perror("ERROR, accept");
						exit(1);
					}
					else {
						FD_SET(acceptfd, &fds);
						if(acceptfd > max_fds) {
							max_fds = acceptfd;
						}
						broadcast(max_fds, sockfd, (char *) "SERVER: New client connected.\n");
					}
				}
				else msgHandler(i, &fds, sockfd, max_fds);
			}
		}
	}
	
	return 0;
}




