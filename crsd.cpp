//-------------------------------------------------------------
// CSCE 438: Distributed objects programming
// HW1: A Chat Room Service
// 
// Chat Server
// Vincent Velarde, Kyle Rowland
// January 2017
//
// NOTES:
// 
//-------------------------------------------------------------

#include <cstdlib>
#include <strings.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <signal.h>

//simply structure to store information about a chat room
struct chatRoom {
	int pid, port;	//process ID, port number
	std::string name;	//chat room name
	
	//constructor
	chatRoom(int id, int p, std::string n) {
		pid = id;
		port = p;
		name = n;
	}
};

struct sockaddr_in server, client;	//client and server information
std::vector<chatRoom> rooms;	//stores a list of all active chat rooms
int roomPort = 1876;	//new chatroom ports start at this number

//handle reading/writing from/to client sockets
void msgHandler(int i, fd_set *fds, int sockfd, int max_fds) {
	int n;	//number of chars sent/recieved
	char readBuf[256], writeBuf[256];
	
	//read from socket
	if((n = read(i, readBuf, sizeof(readBuf))) <= 0) {
		if(n == 0) {	//if client has disconnected from the server
			printf("client %d left\n", i);
		}
		else {
			perror("ERROR, msgHandler-read");	//read failed
		}
		close(i);
		FD_CLR(i, fds);
	}
	else {	//write to socket
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

//runs in each of the forked processes for chat rooms
void childProcess(int port) {
	fd_set fds, rfds;	//file descriptor sets
	int max_fds, acceptfd, sockfd;
	socklen_t length;
	struct sockaddr_in server, client;
	
	//create and check socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("ERROR, socket");
		exit(1);
	}
	
	//set sockaddr_in fields for server
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
	
	fflush(stdout);
	FD_ZERO(&fds);
	FD_ZERO(&rfds);
	FD_SET(sockfd, &fds);
	max_fds = sockfd;
	
	//infinitely loop waiting for connections and messages
	while(true) {
		rfds = fds;
		
		//monitor socket file descriptors
		if(select(max_fds+1, &rfds, NULL, NULL, NULL)  < 0) {
			perror("ERROR, select");
			exit(1);
		}
		
		//iterate through file descriptors checking for connections/messages
		for(int i=0; i<=max_fds; i++) {
			if(FD_ISSET(i, &rfds)) {	//checks if filedescriptor is in read set
				if(i == sockfd) {	//if i is an incoming connection, accept it
					acceptfd = 0;
					length = sizeof(struct sockaddr_in);
					
					//accept incoming connection
					acceptfd = accept(sockfd, (struct sockaddr *) &client, &length);
					if(acceptfd < 0){
						perror("ERROR, accept");
						exit(1);
					}
					else {
						FD_SET(acceptfd, &fds);
						if(acceptfd > max_fds) {
							max_fds = acceptfd;
						}
					}
				}
				else msgHandler(i, &fds, sockfd, max_fds);	//if i is an incoming message, call msgHandler
			}
		}
	}
	exit(1);
}

//handle CREATE command
int createRoom(std::string cmd, int n) {
	//Make sure room doesn't already exist
	if(rooms.size() > 0) {
		for(int i=0; i<rooms.size(); i++) {
			if(rooms[i].name == cmd.substr(7, n-7)) {
				return (-1);
			}
		}
	}
	
	int pid;
	pid = fork();	//create process for new chat room
	if (pid == 0) {	//child process
		childProcess(roomPort);
		return 0;
	}
	else {	//parent process
		chatRoom r(pid, roomPort, cmd.substr(7, n-7));
		rooms.push_back(r);	//add this room to the chatRoom vector
		std::cout << "New room: " << cmd.substr(7, n-7);
		roomPort++;
		return 1;
	}
}

//handle DELETE commands
int deleteRoom(std::string cmd, int n) {
	//make sure room exists
	if(rooms.size() > 0) {
		for(int i=0; i<=rooms.size(); i++) {
			if(rooms[i].name == cmd.substr(7, n-7)) {
				kill(rooms[i].pid, SIGKILL);	//kill child process handling this room
				rooms.erase(rooms.begin() + i);	//erase this room from the chatRoom vector
				std::cout << "Deleted: " << cmd.substr(7, n-7);
				return 1;
			}
		}
	}
	return -1;
}

//handle JOIN commands
int joinRoom(std::string cmd, int n, int i) {
	//Make sure room exists
	if(rooms.size() > 0) {
		for(int j=0; j<rooms.size(); j++) {
			if(rooms[j].name == cmd.substr(5, n-5)) {
				std::stringstream jn;
				jn << "JOIN " << rooms[j].port << '\0';	//create join command for client
				if(write(i, jn.str().c_str(), strlen(jn.str().c_str())) <= 0) {	//send join command to client
					perror("ERROR, joinRoom-write");
					exit(1);
				}
				printf("Joined: %s", cmd.substr(5, n-5).c_str());
				return 1;
			}
		}
	}
}

//handle incoming client commands
void cmdHandler(int i, fd_set *fds, int sockfd, int max_fds) {
	int n;	//number of chars sent/recieved
	char readBuf[256];
	
	if((n = read(i, readBuf, sizeof(readBuf))) <= 0) {
		if(n == 0) {	//if client has disconnected from the server
			printf("client %d left\n", i);
		}
		else {	//read failed
			perror("ERROR, cmdHandler-read");
		}
		close(i);
		FD_CLR(i, fds);
	}
	else {
		//iterate through file descriptors for incoming commands
		for(int j=0; j<=max_fds; j++) {
			if(FD_ISSET(j, fds)) {
				if(j != i) {
					std::string cmd(readBuf, n);	//need c++ string to easily get substrings
					//CREATE room command
					if(cmd.substr(0, 6) == "CREATE") {
						if(createRoom(cmd, n) < 0) {
							return;
						}
					}
					//DELETE room command
					else if (cmd.substr(0, 6) == "DELETE") {
						if(deleteRoom(cmd, n) < 0) {
							return;
						}
					}
					//JOIN room command
					else if (cmd.substr(0, 4) == "JOIN") {
						if(joinRoom(cmd, n, i) < 0) {
							return;
						}
					}
					else printf("invalid input entered\n");
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	fd_set fds, rfds;	//file descriptor sets
	int max_fds, port, acceptfd, sockfd;
	socklen_t length;
	rooms.clear();
	
	//User must provide port number as command line argument
	if(argc < 2) {
		fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);	//convert given port from string to int
	
	//server startup message
	printf("--------------------------------------\n");
	printf("Chat Service Master Server\n");
	printf("Vincent Velarde, Kyle Rowland\n");
	printf("January 2017\n");
	printf("Usage: CREATE <string>\n");
	printf("       DELETE <string>\n");
	printf("       JOIN <string>\n");
	printf("--------------------------------------\n\n");
	
	//create and check master socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("ERROR, socket");
		exit(1);
	}
	
	//set sockaddr_in fields for server
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
	printf("listening...\n\n");
	
	fflush(stdout);
	FD_ZERO(&fds);
	FD_ZERO(&rfds);
	FD_SET(sockfd, &fds);
	max_fds = sockfd;
	
	//infinitely loop waiting for connections and messages
	while(true) {
		rfds = fds;
		
		//monitor socket file descriptors
		if(select(max_fds+1, &rfds, NULL, NULL, NULL)  < 0) {
			perror("ERROR, main-select");
			exit(1);
		}
		
		//iterate through file descriptors checking for connections/messages
		for(int i=0; i<=max_fds; i++) {
			if(FD_ISSET(i, &rfds)) {	//checks if filedescriptor is in read set
				if(i == sockfd) {	//if i is an incoming connection, accept it
					acceptfd = 0;
					length = sizeof(struct sockaddr_in);
					
					//accept incoming connection
					acceptfd = accept(sockfd, (struct sockaddr *) &client, &length);
					if(acceptfd < 0){
						perror("ERROR, main-accept");
						exit(1);
					}
					else {
						FD_SET(acceptfd, &fds);
						if(acceptfd > max_fds) {
							max_fds = acceptfd;
						}
						printf("New client connected\n");
					}
				}
				else cmdHandler(i, &fds, sockfd, max_fds);	//if i is an incoming command, call cmdhandler
			}
		}
	}
	
	return 0;
}





