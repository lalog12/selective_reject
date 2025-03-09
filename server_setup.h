#ifndef _SERVER_SETUP_H_
#define _SERVER_SETUP_H_
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>

#include "packet.h"
#include "safeUtil.h"
#include "pollLib.h"


#define MAXBUF 1407

typedef enum{
    SERVER_START_STATE,
    SERVER_USE_STATE,
    SERVER_TEARDOWN_STATE
} ServerState;

void ServerSetupFSM(int socketNum, char * buffer, FILE * fp, struct sockaddr * client, int * clientAddrLen, uint16_t dataLen);
int ServerSetupStart(int socketNum, char * buffer, FILE * fp, uint16_t dataLen, struct sockaddr * client, int * clientAddrLen);
// int ServerSetupFilename(int socketNum, char * buffer);
void ServerSetupUse(int socketNum, char * buffer, struct sockaddr * client, int * clientAddrLen);
void ServerSetupTeardown(int socketNum, char * buffer, struct sockaddr * client, int * clientAddrLen);

#endif