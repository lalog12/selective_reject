#ifndef _RCOPY_USE_H_
#define _RCOPY_USE_H_

#include <stdint.h>
#include <stdio.h>
#include <netinet/in.h>

#include "globals.h"
#include "safeUtil.h"
#include "pollLib.h"
#include "buffer.h"

typedef enum{
    RCOPY_USE_WAIT,
    RCOPY_USE_SEND,
    RCOPY_USE_SELREJ,
    RCOPY_USE_TEARDOWN
} RCOPY_USE_STATES;

void rcopyUseFSM(int socketNum, char * argv[] , char * buffer, struct sockaddr_in6 * server, uint8_t * nextState, Buffer * buffer_man);
int rcopyUseWait();
int rcopyUseSend(int socketNum, char * buffer, struct sockaddr_in6 * server, Buffer * buffer_man, int *rcopySeqNum, uint8_t * nextState);
int rcopyUseSelRej(Buffer * buffer_man, int *rcopySeqNum, char * buffer, struct sockaddr_in6 * server, int socketNum);
void rcopyUseTeardown(void);

#endif