#ifndef _SERVER_USE_H_
#define _SERVER_USE_H_

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "globals.h"
#include "packet.h"
#include "safeUtil.h"
#include "pollLib.h"
#include "window.h"
#include "cpe464.h"

#ifndef MAXBUF
#define MAXBUF 1407
#endif

typedef enum {
    SERVER_USE_SEND_STATE,
    SERVER_USE_WINDOW_OPEN_STATE,
    SERVER_USE_WINDOW_CLOSED_STATE,
    SERVER_USE_PROCESS_RR_STATE,
    SERVER_USE_PROCESS_SREJ_STATE,
    SERVER_USE_EOF_STATE,
    SERVER_USE_TEARDOWN_STATE
} SERVER_USE_STATES;

void serverUseFSM(int socketNum, FILE* fp, struct sockaddr* client, int* clientAddrLen);

// State handler functions
int serverUseSendState(int socketNum, char* buffer, FILE* fp, struct sockaddr* client, int* clientAddrLen, Window* win, uint32_t* server_seq_num, int* eof_reached);
int serverUseWindowOpenState(int socketNum, char* buffer, struct sockaddr* client, int* clientAddrLen, Window* win);
int serverUseWindowClosedState(int socketNum, char* buffer, struct sockaddr* client, int* clientAddrLen, Window* win, uint32_t* retry_count);
int serverUseProcessRRState(int socketNum, char* buffer, int len, struct sockaddr* client, int* clientAddrLen, Window* win);
int serverUseProcessSREJState(int socketNum, char* buffer, int len, struct sockaddr* client, int* clientAddrLen, Window* win);
int serverUseEOFState(int socketNum, char* buffer, struct sockaddr* client, int* clientAddrLen, Window* win, uint32_t server_seq_num, uint32_t* retry_count);
void serverUseTeardownState(int socketNum);

#endif