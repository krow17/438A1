//-------------------------------------------------------------
// CSCE 438: Distributed objects programming
// HW1: A Chat Room Service
// 
// Chat Room Client
// Vincent Velarde, Kyle Rowland
// January 2017
//
// NOTES:
//  I still need to polish some things up and add more comments
//
//-------------------------------------------------------------

#include <cstdlib>
#include <strings.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <iostream>

void msgHandler(int i, int sockfd) {
	char writeBuf[256];
	char readBuf[256];
	int n;
	
	if (i == 0){
		fgets(writeBuf, sizeof(writeBuf), stdin);
		send(sockfd, writeBuf, strlen(writeBuf), 0);
	}
	else {
		n = recv(sockfd, readBuf, sizeof(readBuf), 0);
		readBuf[n] = '\0';
		printf("%s\n" , readBuf);
		fflush(stdout);
	}
}

int main(int argc, char *argv[]) {
	int sockfd, max_fds, port;
	struct sockaddr_in server;
	struct hostent *serverDef;	//pointer to struct defining host box
	fd_set fds, rfds;
	
	if(argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[2]);
	
	serverDef = gethostbyname(argv[1]);
	if(serverDef == NULL) {
		fprintf(stderr, "ERROR, server\n");
		exit(1);
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("ERROR, socket");
		exit(1);
	}
	
	memset((char *) &server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	bcopy((char *)serverDef->h_addr, (char *)&server.sin_addr.s_addr, serverDef->h_length);
	memset(server.sin_zero, '\0', sizeof server.sin_zero);
	
	if(connect(sockfd, (struct sockaddr *) &server, sizeof(struct sockaddr)) < 0) {
		perror("ERROR, connect");
		exit(1);
	}
	
	FD_ZERO(&fds);
	FD_ZERO(&rfds);
	FD_SET(0, &fds);
	FD_SET(sockfd, &fds);
	max_fds = sockfd;
	
	while(true) {
		rfds = fds;
		if(select(max_fds+1, &rfds, NULL, NULL, NULL) == -1){
			perror("ERROR, select");
			exit(1);
		}
		
		for(int i=0; i <= max_fds; i++) {
			if(FD_ISSET(i, &rfds)) msgHandler(i, sockfd);
		}
	}
	printf("client left\n");
	close(sockfd);
	
	return 0;
}




