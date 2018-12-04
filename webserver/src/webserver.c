/*
 * webserver.c
 *
 *  Created on: Nov 10, 2018
 *      Author: wj
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include <unistd.h>
#include <fcntl.h>


#define MAX_CONNECTIONS 1024


int clients[MAX_CONNECTIONS];


int startServer (char *  port)
{
	int listenSocket;
	struct addrinfo address;
	struct addrinfo *reserve, *current;

	// getaddrinfo for the resolution of the Host Address
	memset (&address, 0, sizeof(address));
	address.ai_family = AF_INET;
	address.ai_socktype = SOCK_STREAM;
	address.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &address, &reserve) != 0)
	{
		perror ("getaddrinfo() error");
		exit(1);
	}
	// sCreate a socket and bind it
	for (current = reserve; current != NULL; current = current->ai_next)
	{
		listenSocket = socket (current->ai_family, current->ai_socktype, 0);
		if (listenSocket == -1)
			continue;
		if (bind(listenSocket, current->ai_addr, current->ai_addrlen) == 0)
			break;
	}
	if (current==NULL)
	{
		perror ("socket() or bind()");
		exit(1);
	}

	freeaddrinfo(reserve);

	if(listen(listenSocket,9999999) != 0)
	{
		perror("listen() error");
		exit(EXIT_FAILURE);
	}

	return listenSocket;
}

void printHelp()
{
    printf("--------------------------------------------------------------------\n");
    printf("-h Print help text.\n");
    printf("-p port Listen to port number port.\n");
    printf("--------------------------------------------------------------------\n");
    exit(2);
}


void respondToRequest(int n)
{
	char request[99999];
	char dataToSend[1024], path[1024];
	int rcvd, fd, noBytes;
	char method[1024];


	rcvd=recv(clients[n], request, 99999, 0);

	// Recieve Error
	if (rcvd<0)
		fprintf(stderr,("recv() error\n"));
	// Receiving Socket Closed
	else if (rcvd==0)
		fprintf(stderr,"Client Disconnected.\n");

	// Request received
	else
	{
		for(int i=0;i<strlen(request);i++)  /* remove CF and LF characters */
			if(request[i] == '\r' || request[i] == '\n')
				request[i]='*';

		printf("%s \n\r",request);

		if (!strncmp(&request[0],"HEAD",4) || !strncmp(&request[0],"head",4)) /* convert no filename to index file */
			strcpy(method, "HEAD");
		else if (!strncmp(&request[0],"GET",3) || !strncmp(&request[0],"get",3)) /* convert no filename to index file */
			strcpy(method, "GET");

		if ( !strncmp(method, "GET",3) || !strncmp(method,"HEAD",4) )
		{
			/* Checking the correct HTTP version */
			int versionFlag = 0;
			for (int j=5; j<strlen(request); j++)
				if(request[j] == ' ')
					if(!strncmp(&request[j+1],"HTTP/1.1", 8) || !strncmp(&request[j+1], "HTTP/1.0", 8))
					{
						versionFlag = 1;
						break;
					}


			/* Null terminate after the second space to ignore extra stuff */
			for(int i=4;i<strlen(request);i++)
			{
				if(request[i] == ' ')
				{ /* The request string is "GET/HEAD URL " +lots of other stuff */
					request[i] = 0;
					break;
				}
			}

			/* URL Validation  (Checking for illegal parent directory use) */
			for(int j=0; j<strlen(request); j++)
			{
				if(request[j] == '.' && request[j+1] == '.')
				{
					printf("Illegal Parent Directory Usage \n");
					write(clients[n], "HTTP/1.0 403 Forbidden	\r\n", 25);
					exit(-1);
				}
			}


			if (!strncmp(&request[0],"GET /\0",6) || !strncmp(&request[0],"get /\0",6)) /* convert no filename to index file */
				strcpy(request,"GET /index.html");
			else if (!strncmp(&request[0],"HEAD /\0",7) || !strncmp(&request[0],"head /\0",7)) /* convert no filename to index file */
				strcpy(request,"HEAD /index.html");

			strcpy(request,"GET /index.html");


			if (!versionFlag)
			{
				write(clients[n], "HTTP/1.0 400 Bad Request	\t\r\n", 25);
			}
			else
			{

				char* totalPath = realpath("../www", NULL);
				strcpy(path, totalPath);
				strcpy(&path[strlen(totalPath)],&request[4]);

				printf("file: %s\n", path);

				if ((fd=open(path, O_RDONLY))!= -1 )
				{
					// FILE FOUND
					int long len = (long)lseek(fd, (off_t)0, SEEK_END); /* lseek to the file end to find the length */
					lseek(fd, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading */
					sprintf(dataToSend,"HTTP/1.0 200 OK\nConnection: close\nContent-Type: text/html\nContent-Length: %d\n\n", len); /* Header + a blank line */

					// Send the initial header

					write(clients[n],dataToSend,strlen(dataToSend));


					if(!strncmp(method,"GET",3))
					{
						// Send the file in 1kB blocks
						while ((noBytes=read(fd, dataToSend, 1024)) > 0)
						{
							write(clients[n], dataToSend, noBytes);
						}
						sleep(1);
					}
				}
				// FILE NOT FOUND
				else
					write(clients[n], "HTTP/1.0 404 Not Found\n", 23);
			}
		}
		else
			write(clients[n],"HTTP/1.0 501 Not Implemented\n",30);
	}
	shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
	close(clients[n]);
	clients[n]=-1;
}


int main(int argc, char * argv [])
{
	struct sockaddr_in clientaddr;

	char portNum [6];
	// Set the default port number to 10000
	strcpy(portNum,"10000	");

	// Parsing the command line arguments
	char arg;
	while ((arg = getopt (argc, argv, "p:")) != -1)
	{
		switch (arg)
		{
			case 'p':
				strcpy(portNum,optarg);
				break;
			case 'h':
				printHelp();
				break;
			default:
				printHelp();
				break;
		}
	}

	// Set all the client fds' to 0 to indicate empty clients
	for (int i=0; i<MAX_CONNECTIONS; i++)
			clients[i]=-1;

	// Start the server
	int listenSocket = startServer(portNum);
	printf("Server started\n");


	// To indicate the current slot in the queue
	int currentSlot=0;
	socklen_t addrlen;
	while (1)
	{
		addrlen = sizeof(clientaddr);
		clients[currentSlot] = accept (listenSocket, (struct sockaddr *) &clientaddr, &addrlen);

		if (clients[currentSlot]<0)
			perror("accept() error");
		else
		{
			if ( fork()==0 )
			{
				respondToRequest(currentSlot);
				exit(0);
			}
			else
			{
				signal(SIGCLD, SIG_IGN);
			}
		}
		// The currentSlot increment shouldn't increase more than the maximum allowed connections
		while (clients[currentSlot]!=-1)
			currentSlot = (currentSlot+1)%MAX_CONNECTIONS;
	}
	return 0;
}
