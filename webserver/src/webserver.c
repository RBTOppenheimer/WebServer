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
#define SERVER_ROOT "/home/wj/workspace/FreelanceProjects/WebServer/webserver/Default"


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

void respondToRequest(int n)
{
	char request[99999];
	char dataToSend[1024], path[1024];
	char * token = NULL;
	int rcvd, fd, noBytes;

	memset( (void*)request, (int)'\0', 99999 );
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
		printf(request);
		token = strtok (request, " ");
		int token_counter = 0;
	    while (token != NULL)
	    {
	        printf ("%s\n",token);
	        if (token_counter == 0)
	        {
	            if (strcmp(token, "GET") == 0)
	            {
            		printf("Success");

	            	if ( (fd=open(path, O_RDONLY))!=-1 )    //FILE FOUND
					{
	            		printf("Success");
						send(clients[n], "HTTP/1.0 200 OK\n\n", 17, 0);
						while ((noBytes = read(fd, dataToSend, 1024)) > 0)
							write (clients[n], dataToSend, noBytes);
					}
					else
						write(clients[n], "HTTP/1.0 404 Not Found\n", 23); //FILE NOT FOUND
	            }
	            else
	            {
	            	printf("SUC");
	    			write(clients[n],"HTTP/1.0 501 Not Implemented\n",30);
	            }
	        }
	        else if (token_counter == 1)
	        {
	        	strcpy(path, SERVER_ROOT);
				strcpy(&path[strlen(SERVER_ROOT)],token);
				printf("file: %s\n", path);
	        }
	        token = strtok(NULL, " ");
	        token_counter++;
	    }
	}
}

int main(int argc, char * argv [])
{

	struct sockaddr_in clientaddr;

	char portNum [6];
	// Set the default port number to 10000
	strcpy(portNum,"10025");

	/* Parsing the command line arguments */
	char arg;
	while ((arg = getopt (argc, argv, "p:")) != -1)
	{
		switch (arg)
		{
			case 'p':
				strcpy(portNum,optarg);
				break;
		}
	}

	// Set all the client fds' to 0 to indicate empty clients
	for (int i=0; i<MAX_CONNECTIONS; i++)
			clients[i]=-1;

	// Start the server
	int listenSocket = startServer(portNum);
	printf("Server started");


	// To indicate the current slot in the queue
	int currentSlot=0;
	socklen_t addrlen;
	while (1)
	{
		addrlen = sizeof(clientaddr);
		clients[currentSlot] = accept (listenSocket, (struct sockaddr *) &clientaddr, &addrlen);

		if (clients[currentSlot]<0)
			error ("accept() error");
		else
		{
			respondToRequest(currentSlot);
//			if ( fork()==0 )
//			{
//				respondToRequest(currentSlot);
//				exit(0);
//			}
		}

		// The currentSlot increment shouldn't increase more than the maximum allowed connections
		while (clients[currentSlot]!=-1)
			currentSlot = (currentSlot+1)%MAX_CONNECTIONS;
	}

	return 0;
}
