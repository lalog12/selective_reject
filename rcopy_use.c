#include "rcopy_use.h"
#include "rcopy_setup.h"
void rcopyUseFSM(int socketNum, char *argv[], char * buffer, struct sockaddr_in6 * server, uint8_t *nextState, Buffer * buffer_man){

    uint8_t current_state = *nextState;
    int buffer_len = 0;
    int rcopySeqNum = 0;

    while(1){
        switch(current_state){
            case RCOPY_USE_WAIT:
                current_state = rcopyUseWait(socketNum, buffer, server, buffer_len);
                break;

            case RCOPY_USE_SEND:
                current_state = rcopyUseSend(socketNum, buffer, server, buffer_man, &rcopySeqNum, nextState);
                break;

            case RCOPY_USE_TEARDOWN:
                return;
                break;

            case RCOPY_USE_SELREJ:
                current_state = rcopyUseSelRej(buffer_man, &rcopySeqNum, buffer, server, socketNum);
                break;
        }
    }
}

int rcopyUseSelRej(Buffer * buffer_man, int *rcopySeqNum, char * buffer, struct sockaddr_in6 * server, int socketNum){
    uint8_t counter = 0;
    int pollReturn = 0;
    int serverAddrLen = sizeof(struct sockaddr_in6);
    int length = 0;

    while(1){

        while((pollReturn = pollCall(1000)) < 0 ){

            int buffer_len = sendRRorSrej(buffer, (*rcopySeqNum)++, 6, buffer_man->expected);   // flag = 6 (selective reject)
            safeSendto(socketNum, buffer, buffer_len, 0,(struct sockaddr *) server, serverAddrLen);
            counter++;
            if(counter == 10){
                return RCOPY_USE_TEARDOWN;
            }

        }

        counter = 0;
        uint16_t packet_length = safeRecvfrom(pollReturn, buffer, MAXRECV, 0,(struct sockaddr *) server, &serverAddrLen);
        uint16_t checksum = Checksum_Corrupt(buffer, packet_length);

        if(checksum != 0){
            continue;
        }

        uint32_t seqNumHost = GetSeqNum(buffer);

        if (buffer[6] == 10){
            buffer_process_packet(buffer_man, seqNumHost,(uint8_t *) buffer + 7, packet_length - 7);
            length = sendRRorSrej(buffer, (*rcopySeqNum)++, 5, buffer_man -> expected);
            safeSendto(socketNum, buffer, length, 0,(struct sockaddr *) server, serverAddrLen);
            return RCOPY_USE_TEARDOWN;
        }

        int result = buffer_process_packet(buffer_man, seqNumHost,(uint8_t *) buffer + 7, packet_length - 7);

        if(result == 1){         // In RR state
            int length = sendRRorSrej(buffer, (*rcopySeqNum)++, 5, buffer_man->expected);
            safeSendto(pollReturn, buffer, length, 0,(struct sockaddr *) server, serverAddrLen);
            return RCOPY_USE_WAIT;
        }

        else if(result == 0){    // Still in Srej state
            length = sendRRorSrej(buffer, (*rcopySeqNum)++, 6, buffer_man -> expected);
            safeSendto(pollReturn, buffer, length, 0,(struct sockaddr *) server, serverAddrLen);
        }
    }
}

int rcopyUseSend(int socketNum, char * buffer, struct sockaddr_in6 * server, Buffer * buffer_man, int * rcopySeqNum, uint8_t * nextState){
    static uint8_t first_flag = 0;
    int buffer_len = 0;

    if(*nextState == RCOPY_USE_STATE && first_flag ==  0){   // first packet is already in buffer
        buffer_len = FirstDataPacketRecvRcopyLen;
    }

    int serverAddrLen = sizeof(struct sockaddr_in6);

    if(*nextState == RCOPY_USE_STATE && first_flag !=  0){   // first packet was processed . now returning to normal operations.
    buffer_len = safeRecvfrom(socketNum, buffer, MAXRECV, 0, (struct sockaddr *) server, &serverAddrLen);
    }
    uint16_t checksum = Checksum_Corrupt(buffer, buffer_len);
    uint32_t seqNumHost = GetSeqNum(buffer);
    int length = 0;
    first_flag = 1;
    if (checksum != 0){
        return RCOPY_USE_WAIT;
    }

    else if(buffer[6] == 10){
        buffer_process_packet(buffer_man, seqNumHost,(uint8_t *) buffer + 7, buffer_len - 7);
        length = sendRRorSrej(buffer, (*rcopySeqNum)++, 5, buffer_man -> expected);
        safeSendto(socketNum, buffer, length, 0,(struct sockaddr *) server, serverAddrLen);
        return RCOPY_USE_TEARDOWN;
    }

    else if ( (buffer[6] == 16 || buffer[6] == 18) ){  // Regular Data Packet (16) or Resent Data Packet (18)
        uint8_t RRorSrej = buffer_process_packet(buffer_man, seqNumHost, (uint8_t *) buffer + 7, buffer_len - 7);
        
        if(RRorSrej == 1){    // RR expected
            // function that sends rr with buffer_man -> expected
            length = sendRRorSrej(buffer, (*rcopySeqNum)++, 5, buffer_man -> expected);  // flag 5 = RR Packet
            safeSendto(socketNum, buffer, length, 0,(struct sockaddr *) server, serverAddrLen);
            
            return RCOPY_USE_WAIT;
        }

        else{  // Srej expected
            length = sendRRorSrej(buffer, (*rcopySeqNum)++, 6, buffer_man -> expected);  // flag 6 == Srej
            // function that sends srej with buffer_man -> expected
            safeSendto(socketNum, buffer, length, 0,(struct sockaddr *) server, serverAddrLen);

            return RCOPY_USE_SELREJ;
        }
    }
    return RCOPY_USE_WAIT;
}

int rcopyUseWait(){
    printf("rcopyUseWait function!!!\n");
    int pollReturn = 0;
    uint8_t counter = 0;


    while((pollReturn = pollCall(1000)) < 0){
        counter++;
        printf("counter: %d\n", counter);
        if(counter == 10){
            return RCOPY_USE_TEARDOWN;
        }
    }
    return RCOPY_USE_SEND;
}