//-------------------------------------------------------------
// CSCE 438: Distributed objects programming
// HW1: A Chat Room Service
// 
// Chat Room Client
// Vincent Velarde, Kyle Rowland
// January 2017
//
// NOTES:
// 
//-------------------------------------------------------------

#include <cstdlib>
#include <strings.h>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <iostream>

//handle reading/writing from/to client sockets
int msgHandler(int i, int sockfd) {
	char writeBuf[256];
	char readBuf[256];
	int n;	//number of chars sent/recieved
	
	if (i == 0){	//write message to socket
		fgets(writeBuf, sizeof(writeBuf), stdin);
		write(sockfd, writeBuf, strlen(writeBuf));
		return 0;
	}
	else {	//read message from socket
		if((n = read(sockfd, readBuf, sizeof(readBuf))) == 0) {	//if server has closed
			printf("chat room closed\n");
			exit(0);
		}
		readBuf[n] = '\0';
		std::string cmd(readBuf, n);	//need this in a c++ string to easily get sub strings
		if(cmd.substr(0, 4) == "JOIN") {	//if message is a join command
			printf(">Connected\n");
			return atoi(cmd.substr(5, n-5).c_str());
		}
		printf(">%s\n" , readBuf);	//print recieved message
		fflush(stdout);
	}
}

//infinitely loop waiting for messages
int loop(fd_set fds, fd_set rfds, int max_fds, int sockfd) {
	int port = 0;
	
	while(true) {
		rfds = fds;
		
		//monitor socket file descriptors
		if(select(max_fds+1, &rfds, NULL, NULL, NULL) == -1){
			perror("ERROR, select");
			exit(1);
		}
		
		//iterate through file descriptors checking for messages
		for(int i=0; i <= max_fds; i++) {
			if(FD_ISSET(i, &rfds)) {	//checks if filedescriptor is in read set
				port = msgHandler(i, sockfd);
				if(port > 0) {	//if msghandler returned a port number from a join command
					return port;
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	int sockfd, max_fds, port;
	struct sockaddr_in server;
	struct hostent *serverDef;	//pointer to struct defining host box
	fd_set fds, rfds;	//file descriptor sets
	
	//User must provide host name and port number as command line arguments
	if(argc < 3) {
		fprintf(stderr, "USAGE: %s <hostname> <port>\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[2]);	//convert given port from string to int
	
	//get server information from hostname
	serverDef = gethostbyname(argv[1]);
	if(serverDef == NULL) {
		fprintf(stderr, "ERROR, server\n");
		exit(1);
	}
	
	//create and check socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("ERROR, socket");
		exit(1);
	}
	
	//set sockaddr_in fields for server
	memset((char *) &server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	bcopy((char *)serverDef->h_addr, (char *)&server.sin_addr.s_addr, serverDef->h_length);
	memset(server.sin_zero, '\0', sizeof server.sin_zero);
	
	//connect to server socket
	if(connect(sockfd, (struct sockaddr *) &server, sizeof(struct sockaddr)) < 0) {
		perror("ERROR, connect");
		exit(1);
	}
	
	FD_ZERO(&fds);
	FD_ZERO(&rfds);
	FD_SET(0, &fds);
	FD_SET(sockfd, &fds);
	max_fds = sockfd;
	
	port = loop(fds, rfds, max_fds, sockfd);
	if(port > 0) {	//if a join command was recieved this connects to the chat room
		close(sockfd);	//close master server connection
		server.sin_port = htons(port);
		
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		//connect to chat room socket
		if(connect(sockfd, (struct sockaddr *) &server, sizeof(struct sockaddr)) < 0) {
			perror("ERROR, connect");
			exit(1);
		}
		
		FD_ZERO(&fds);
		FD_ZERO(&rfds);
		FD_SET(0, &fds);
		FD_SET(sockfd, &fds);
		max_fds = sockfd;
		
		loop(fds, rfds, max_fds, sockfd);
	}
	
	close(sockfd);
	
	return 0;
}




