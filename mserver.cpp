#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 3005
#define BUFFER_LENGTH 250
#define FALSE 0

int main()
{
	int sd = -1, sd2 = -1;
	int rc, length, on = 1;
	char buffer[BUFFER_LENGTH];
	fd_set read_fd;
	struct timeval timeout;
	struct sockaddr_in serveraddr;

	do
	{
		sd = socket(AF_INET, SOCK_STREAM, 0);
		if(sd < 0)
		{
			printf("Error: server descriptor not found\n");
		}

		memset(&serveraddr, 0, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(SERVER_PORT);
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

		rc = bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
		if(rc<0)
		{
			printf("Error: server descriptor did not bind properly\n");
		}

		rc = listen(sd, 10);
		if(rc<0)
		{
			printf("Error: rc failed to listen. Pinche pendejo.\n");
		}
		printf("Ready for client to connect...\n");


		sd2 = accept(sd, NULL, NULL);
		if(sd2 < 0)
		{
			printf("Error: second server descriptor failed to accept.\n");
		}


		timeout.tv_sec = 30;
		timeout.tv_usec = 0;
		FD_ZERO(&read_fd);
		FD_SET(sd2, &read_fd);

		rc = select(sd2+1, &read_fd, NULL, NULL, &timeout);
		if(rc < 0)
		{
			printf("Error: rc failed at select.\n");
		}

		length = BUFFER_LENGTH;
		rc = recv(sd2, buffer, sizeof(buffer), 0);

		rc = send(sd2, buffer, sizeof(buffer), 0);
	}

	while(FALSE);
	
		if(sd != -1)
			close(sd);
		if(sd2 != -1)
			close(sd2);
	
}