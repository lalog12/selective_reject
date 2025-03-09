#ifndef _RCOPY_USE_H_
#define _RCOPY_USE_H_

#include <stdint.h>

#include "pollLib.h"
#include "buffer.h"


typedef enum{
    RCOPY_USE_WAIT,
    RCOPY_USE_SEND,
    RCOPY_USE_SELREJ,
    RCOPY_USE_TEARDOWN
} RCOPY_USE_STATES;


void rcopyUseFSM(int socketNum, char * argv[] , char * buffer, struct sockaddr_in6 * server, uint8_t nextState, Buffer * buffer_man);
void rcopyUseWait(struct pollfd * pollfds);
void rcopyUseSend(void);
void rcopyUseSelRej(void);
void rcopyUseTeardown(void);

#endif