#include "packet.h"

void Default_Header(uint32_t seqNum, char * buffer, uint8_t flag){
    uint32_t seqNum_network = htonl(seqNum);
    memcpy(buffer, &seqNum_network , 4);    // Sequence Number
    memset(buffer + 4, 0, 2);       // Checksum
    buffer[6] = flag;  // Flag
}

uint32_t GetSeqNum(char * buffer){
    uint32_t seqNumNet = 0;
    memcpy(&seqNumNet, buffer, 4);
    return ntohl(seqNumNet);
}

uint8_t sendRRorSrej(char * buffer, uint32_t packetSeqNum, uint8_t flag, uint32_t RRSeqNum){
    uint32_t packetSeqNumNet =  htonl(packetSeqNum);
    uint32_t RRSeqNumNet = htonl(RRSeqNum);

    memcpy(buffer,  &packetSeqNumNet, 4);  // Sequence Number in network order
    memset(buffer + 4, 0, 2);  // checksum to 0 
    buffer[6] = flag;
    memcpy(buffer + 7, &RRSeqNumNet, 4);   // RR Sequence Number in network order
    return 11;     // length of RR packet
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
    printf("from_file_name length: %d\n",from_filename_len);
    memcpy(buffer + 15, argv[1], from_filename_len + 1);
    printf("from_filename: %s\n", buffer + 15);

}

void Insert_Checksum(char * buffer, uint16_t str_len){
    buffer[4] = 0;
    buffer[5] = 0;
    uint16_t checksum = in_cksum((unsigned short *) buffer, str_len);
    printf("DEBUG: Calculated Checksum to send: %d\n", checksum);
    memcpy(buffer + 4, &checksum, 2); // Checksum
}


uint16_t Checksum_Corrupt(char * buffer, int Len){
    uint16_t checksum = in_cksum((unsigned short *) buffer, Len);
    printf("DEBUG: Checksum of whole packet: %d\n", checksum);
    return checksum;
}

