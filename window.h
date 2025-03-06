#ifndef WINDOW_H
#define WiNDOW_H

#include <stdint.h>


typedef enum {
    IN_ORDER_STATE,
    BUFFERING_STATE,
    FLUSHING_STATE
} BufferState;  // Enum for the buffer state

typedef struct {
    BufferState current_state;
    uint32_t seq_num;
    uint8_t data[MAX_PDU];
    struct BufferEntry *buffer;
    uint16_t dataLen;
} bManager;  // Keeps track of the buffer state

#endif