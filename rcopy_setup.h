#ifndef _RCOPY_SETUP_H_
#define _RCOPY_SETUP_H_

#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "packet.h"
#include "safeUtil.h"
#include "networks.h"
#include "pollLib.h"


typedef enum{
    RCOPY_START_STATE,
    RCOPY_WAIT_STATE,
    RCOPY_USE_STATE,
    RCOPY_ERROR_STATE
} RCOPY_STATES;


int Rcopy_setup(int socketNum, char * argv[], char * buffer, struct sockaddr_in6 * server, int portNumber,uint8_t nextState);
int Rcopy_setup_start(int socketNum,char * buffer, char * argv[], struct sockaddr_in6 * server);
int Rcopy_setup_wait(int socketNum, char * buffer, char * argv[], struct sockaddr_in6 * server, int portNum, uint8_t nextState);
void Rcopy_setup_error(int socketNum);

#endif