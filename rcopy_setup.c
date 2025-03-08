#include "rcopy_setup.h"


int Rcopy_setup(int socketNum, char * argv[],  char * buffer, struct sockaddr_in6 * server, struct pollfd * pollfds, int portNumber){

    int current_state = RCOPY_START_STATE;

    pollfds[0].fd = socketNum;
    pollfds[0].events = POLLIN;

    while(1){
        switch(current_state){

            case RCOPY_START_STATE:
                current_state = Rcopy_setup_start(socketNum, buffer, argv, server);
                break;

            case RCOPY_USE_STATE:
                return socketNum;

            case RCOPY_ERROR_STATE:
                Rcopy_setup_error(socketNum);
                break;
            
            case RCOPY_WAIT_STATE:
                current_state = Rcopy_setup_wait(socketNum, buffer, argv, server, pollfds, portNumber);
                break;
        }
    }  
} 

void Rcopy_setup_error(int socketNum){

    fprintf(stderr, "Unable to successfully connect to the server\n");
    close(socketNum);
    exit(1);
}

int Rcopy_setup_start(int socketNum, char * buffer, char * argv[], struct sockaddr_in6 * server){  

    Default_Header(1, buffer, 8);    // seqNum = 1, flag = 8
    Header_File_TransferRS(buffer, argv);
    Insert_Checksum(buffer, strlen(argv[1]) + 15);

    int serverAddrLen = sizeof(struct sockaddr_in6);  
    printf("Sending packet: %s", buffer);
    safeSendto(socketNum, buffer, 15 + strlen(argv[1]), 0, (struct sockaddr *) server, serverAddrLen);

    return RCOPY_WAIT_STATE;
}

int Rcopy_setup_wait(int socketNum, char * buffer, char * argv[], struct sockaddr_in6 * server, struct pollfd * pollfds, int portNum){

    int pollReturn = 0;
    int counter = 0;
    int serverAddrLen = sizeof(struct sockaddr_in6);

	while((pollReturn = poll(pollfds, 1, 1000)) <= 0){

		if (pollReturn == 0){
            close(socketNum);
	        socketNum = setupUdpClientToServer(server, argv[6], portNum);
            pollfds[0].fd = socketNum;
            printf("Sending packet: %s", buffer);
            safeSendto(socketNum, buffer, 15 + strlen(argv[1]), 0, (struct sockaddr *) server, serverAddrLen);
            counter++;

            if (counter == 10){
                return RCOPY_ERROR_STATE;
            }
		}

        if(pollReturn < 0){
            fprintf(stderr, "Error with poll call in rcopy_setup.c:54  (Rcopy_setup_wait) \n");
        }
	}

    int len = safeRecvfrom(socketNum, buffer, MAXRECV, 0, (struct sockaddr *) server, &serverAddrLen);

    uint16_t checksum = Checksum_Corrupt(buffer, len);
    // message is intact and packet contains f.n ack, data packet, or resent data packet.
    if( (checksum == 0 && ((buffer[6] == 9 && buffer[7] == 0) || buffer[6] == 16 || buffer[6] == 18))){   // buffer[6] is flag buffer[7] = 0 is ack, is nack
        return RCOPY_USE_STATE;
    }
    else if(checksum == 0 && buffer[6] == 9 && buffer[7] == 1){    // message is intact and packet contains f.n nack
        fprintf(stderr, "Error: file %s not found\n", argv[1]);
        close(socketNum);
        exit(1);
    }

    else{  // message is corrupt. wait for another packet.
        printf("DEBUG: MESSAGE IS CORRUPT\n");
        return RCOPY_WAIT_STATE;
    }

}