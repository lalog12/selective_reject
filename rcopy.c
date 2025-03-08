// Client side - UDP Code				    
// By Hugh Smith	4/1/2017		

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "cpe464.h"
#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "rcopy_setup.h"

#define MAXBUF 80

void talkToServer(int socketNum, struct sockaddr_in6 * server, char * argv[], int portNumber);
int readFromStdin(char * buffer);
int checkArgs(int argc, char * argv[]);


int main (int argc, char *argv[])
 {
	int socketNum = 0;				
	struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	socketNum = setupUdpClientToServer(&server, argv[6], portNumber);

	sendErr_init(atof(argv[5]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);  

	talkToServer(socketNum, &server, argv, portNumber);
	
	close(socketNum);

	return 0;
}

void talkToServer(int socketNum, struct sockaddr_in6 * server, char * argv[], int portNumber)
{
	// int serverAddrLen = sizeof(struct sockaddr_in6);
	// char * ipString = NULL;
	// int dataLen = 0; 
	char buffer[MAXBUF+1];

	// adding server connected socket to poll.
	struct pollfd pollfds[1];

	socketNum = Rcopy_setup(socketNum, argv, buffer, server, pollfds, portNumber);

	// buffer[0] = '\0';
	// while (buffer[0] != '.')
	// { 
	// 	dataLen = readFromStdin(buffer);

	// 	printf("Sending: %s with len: %d\n", buffer,dataLen);
	
	// 	safeSendto(socketNum, buffer, dataLen, 0, (struct sockaddr *) server, serverAddrLen);
		
	// 	safeRecvfrom(socketNum, buffer, MAXBUF, 0, (struct sockaddr *) server, &serverAddrLen);
		
	// 	// print out bytes received
	// 	ipString = ipAddressToString(server);
	// 	printf("Server with ip: %s and port %d said it received %s\n", ipString, ntohs(server->sin6_port), buffer);
	      
	// }
}

int readFromStdin(char * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	printf("Enter data: ");
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

int checkArgs(int argc, char * argv[])
{

    int portNumber = 0;
	
        /* check command line arguments  */
	if (argc != 8 )
	{
		fprintf(stderr, "usage: %s from-filename to-filename window-size buffer-size error-rate remote-machine remote-port \n", argv[0]);
		exit(1);
	}
	if(strlen(argv[1]) > 100){
		fprintf(stderr, "%s from-filename is greater than 100 characters.\n", argv[1]);
		exit(1);
	}
	if(strlen(argv[2]) > 100){
		fprintf(stderr, "%s to-filename is greater than 100 characters.\n", argv[2]);	
		exit(1);	
	}
	portNumber = atoi(argv[7]);		// convert port number to integer
	
	return portNumber;
}