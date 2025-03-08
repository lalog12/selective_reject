#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>

#include "checksum.h"
#include "cpe464.h"

#define MAXRECV 600

void Header_File_TransferRS(char * buffer, char * argv[]);
void IncrementSeqNum(char * buffer);
void Insert_Checksum(char * buffer, uint16_t str_len);
uint16_t Checksum_Corrupt(char * buffer, int Len);
void Default_Header(uint32_t seqNum, char * buffer, uint8_t flag);
void Header_File_TransferSR(char * buffer, uint8_t flag);

#endif