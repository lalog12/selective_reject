/* Server side - UDP Code				    */
/* By Hugh Smith	4/1/2017	*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server_use.h" 
#include "server_setup.h"
#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "cpe464.h"
#include "packet.h"
#include "pollLib.h"


void processClient(int socketNum);
int checkArgs(int argc, char *argv[]);

int main ( int argc, char *argv[]  )
{ 
	int socketNum = 0;				
	int portNumber = 0;
	sendtoErr_init(atof(argv[1]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
	portNumber = checkArgs(argc, argv);

	socketNum = udpServerSetup(portNumber);

	processClient(socketNum);

	close(socketNum);
	
	return 0;
}

void processClient(int socketNum)
{
	int dataLen = 0; 
	char buffer[MAXBUF + 1];	  
	struct sockaddr_in6 client;		
	int clientAddrLen = sizeof(client);	
	setupPollSet();
	addToPollSet(socketNum);


	
	FILE * fp = NULL;

	// while (1)
	// {

	ServerSetupFSM(socketNum, buffer, fp, (struct sockaddr *) &client, &clientAddrLen, dataLen);
	printf("After ServerSetupFSM\n");

	strcpy(filenameString, buffer + 15);
	fflush(stdout);
	printf("File name string %s\n", filenameString);
	fflush(stdout);
	fp = fopen(filenameString, "rb");

	
	serverUseFSM(socketNum, fp, (struct sockaddr *) &client, &clientAddrLen);


		// dataLen = safeRecvfrom(socketNum, buffer, MAXBUF, 0, (struct sockaddr *) &client, &clientAddrLen)
		// printf("Received message from client with ");
		// printIPInfo(&client);
		// printf(" Len: %d \'%s\'\n", dataLen, buffer);

		// // just for fun send back to client number of bytes received
		// sprintf(buffer, "bytes: %d", dataLen);
		// safeSendto(socketNum, buffer, strlen(buffer)+1, 0, (struct sockaddr *) & client, clientAddrLen);

	// }
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if ((argc > 3) || (argc < 2) )
	{
		fprintf(stderr, "Usage %s error-rate [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 3)
	{
		portNumber = atoi(argv[2]);
	}
	
	return portNumber;
}