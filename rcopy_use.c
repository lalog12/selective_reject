#include "rcopy_use.h"
#include "rcopy_setup.h"
void rcopyUseFSM(int socketNum, char *argv[], char * buffer, struct sockaddr_in6 * server, struct pollfd * pollfds, uint8_t nextState, Buffer * buffer_man){

    uint8_t current_state = nextState;

    while(1){
        switch(current_state){
            case RCOPY_USE_WAIT:
                rcopyUseWait(pollfds);
                break;

            case RCOPY_USE_SEND:
                rcopyUseSend();
                break;

            case RCOPY_USE_TEARDOWN:
                rcopyUseSelRej();
                break;

            case RCOPY_USE_SELREJ:
                rcopyUseTeardown();
                break;
        }
    }
}

void rcopyUseWait(int socketNum, char * buffer, struct sockaddr_in6 * server, struct pollfd * pollfds){

    int pollReturn = poll(pollfds, -1, 1000);
    safeRecvfrom(socketNum, buffer, MAXRECV, 0, (struct sockaddr *) server, &serverAddrLen);
    // if(pollReturn == )

}