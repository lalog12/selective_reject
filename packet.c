#include "packet.h"

void Default_Header(uint32_t seqNum, char * buffer, uint8_t flag){
    uint32_t seqNum_network = htonl(seqNum);
    memcpy(buffer, &seqNum_network , 4);    // Sequence Number
    memset(buffer + 4, 0, 2);       // Checksum
    buffer[6] = flag;  // Flag
}

void IncrementSeqNum(char * buffer){
    uint32_t seqNum = 0;
    memcpy(&seqNum, buffer, 4);
    seqNum = htonl(ntohl(seqNum) + 1);   // incrementing 1
    memcpy(buffer, &seqNum, 4);
}

void Header_File_TransferRS(char * buffer, char * argv[]){
    uint32_t window_size = htonl(atol(argv[3]));
    memcpy(buffer + 7, &window_size, 4); // Window Size

    uint32_t buffer_size = htonl(atol(argv[4]));
    memcpy(buffer + 11, &buffer_size, 4);  // data bytes transmitted per packet


    uint8_t from_filename_len = strlen(argv[1]);
    memcpy(buffer + 15, argv[1], from_filename_len + 1); // File name + null terminator
}

void Insert_Checksum(char * buffer, uint16_t str_len){
    buffer[4] = 0;
    buffer[5] = 0;
    uint16_t checksum = in_cksum((unsigned short *) buffer, str_len);
    printf("DEGBUG: Calculated Checksum to send: %d\n", checksum);
    memcpy(buffer + 4, &checksum, 2); // Checksum
}

void Header_File_TransferSR(char * buffer, uint8_t flag){
    buffer[7] = flag;
}

uint16_t Checksum_Corrupt(char * buffer, int Len){
    uint16_t checksum = in_cksum((unsigned short *) buffer, Len);
    printf("DEBUG: Checksum of whole packet: %d\n", checksum);
    return checksum;
}

