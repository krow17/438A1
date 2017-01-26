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
#define SERVER_NAME "ServerHostName"
#define NETDB_MAX_HOST_NAME_LENGTH 250

int main(int argc, char *argv[])
{
	int sd = -1, rc, bytesReceived;
	char buffer[BUFFER_LENGTH];
	char server[NETDB_MAX_HOST_NAME_LENGTH];
	struct sockaddr_in serveraddr;
	struct hostent *hostp;

	do
	{
		sd = socket(AF_INET, SOCK_STREAM, 0);
		if(sd < 0)
		{
			printf("Error: sd not properly assigned. Line 28.\n");
		}

		if(argc > 1)
			strcpy(server, argv[1]);
		else
			strcpy(server, SERVER_NAME);

		memset(&serveraddr, 0, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(SERVER_PORT);
		serveraddr.sin_addr.s_addr = inet_addr(server);

		if(serveraddr.sin_addr.s_addr == (unsigned long)INADDR_NONE)
		{
			hostp = gethostbyname(server);
			if(hostp ==(struct hostent *)NULL)
			{
				printf("Host not found-->");
				break;
			}
			memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
		}

		rc = connect(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
		if(rc < 0)
		{
			printf("Error: rc failed to connect. Line 55. \n");
		}


		memset(buffer, 'a', sizeof(buffer));
		rc = send(sd, buffer, sizeof(buffer), 0);
		if(rc < 0)
		{
			printf("Error: rc failed to send. Line 63.\n");
		}

		bytesReceived = 0;
		while(bytesReceived < BUFFER_LENGTH)
		{
			rc = recv(sd, &buffer[bytesReceived], BUFFER_LENGTH - bytesReceived, 0);
			bytesReceived += rc;
		}
	}
	while(FALSE);

	if(sd != -1)
		close(sd);
}