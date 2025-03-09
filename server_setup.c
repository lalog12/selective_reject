#include "server_setup.h"



void ServerSetupFSM(int socketNum, char * buffer, FILE * fp, struct sockaddr * client, int * clientAddrLen, uint16_t dataLen){

    int current_state = SERVER_START_STATE;

    while (1){
        switch(current_state){

            case SERVER_START_STATE:
                current_state = ServerSetupStart(socketNum, buffer, fp, dataLen, client, clientAddrLen);
                break;

            case SERVER_USE_STATE:
                ServerSetupUse(socketNum, buffer, client, clientAddrLen);
                return;

            case SERVER_TEARDOWN_STATE:
                ServerSetupTeardown(socketNum, buffer, client, clientAddrLen);
                break;

            default:
                fprintf(stderr, "Error: Invalid state in server_setup.c \n");
                exit(1);
        }
    }
}

void ServerSetupTeardown(int socketNum, char * buffer, struct sockaddr * client, int * clientAddrLen){
    IncrementSeqNum(buffer);
    buffer[6] = 9;      // response to filename packet
    buffer[7] = 1;      // error
    Insert_Checksum(buffer, 8);     // checksum buffer of length 8
    printf("Sending data: %s", buffer);
    safeSendto(socketNum, buffer, 8, 0, client, *clientAddrLen);
    close(socketNum);
    exit(1);
}

void ServerSetupUse(int socketNum, char * buffer, struct sockaddr * client, int * clientAddrLen){
    IncrementSeqNum(buffer);
    buffer[6] = 9;
    buffer[7] = 0;
    Insert_Checksum(buffer, 8);     // checksum buffer of length 8
    printf("Sending data: %s", buffer);
    safeSendto(socketNum, buffer, 8, 0, client, *clientAddrLen);
    return;
}

int ServerSetupStart(int socketNum, char * buffer, FILE * fp, uint16_t dataLen, struct sockaddr * client, int * clientAddrLen){


    pollCall(-1);
    dataLen = safeRecvfrom(socketNum, buffer, MAXBUF, 0, (struct sockaddr *) client, clientAddrLen);

    uint16_t checksum = Checksum_Corrupt(buffer, dataLen);

    if (checksum != 0){
        printf("checksum != 0 in server_setup.c in ServerSetupStart function.");
        return SERVER_START_STATE;
    }

    fp = fopen(buffer + 15, "rb");    // read characters until NULL character
    if (fp == NULL){
        fprintf(stderr, "Error: file %s not found\n", buffer + 15);
        return SERVER_TEARDOWN_STATE;
    }
    else{
        return SERVER_USE_STATE;    // IMPLEMENT USE STATE EVERYTHING SHOULD BE GOOD.
    }

}