// buffer.h
#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <stdlib.h>

#define MAX_DATA_SIZE 1400  // Max data payload size

// Buffer states as described in the slides
typedef enum {
    BUFFER_STATE_IN_ORDER,   // Receiving packets in sequence
    BUFFER_STATE_BUFFERING,  // Buffering out-of-order packets
    BUFFER_STATE_FLUSHING    // Flushing buffer after receiving missing packet
} BufferState;

// Structure for buffer entries
typedef struct {
    uint32_t seq_num;            // Sequence number
    uint8_t data[MAX_DATA_SIZE]; // Data payload (excluding header)
    uint16_t data_len;           // Length of data
    uint8_t valid;               // Flag indicating if entry contains valid data
} BufferEntry;

// Buffer management structure
typedef struct {
    BufferEntry *entries;    // Circular buffer for out-of-order packets
    uint32_t buffer_size;    // Size of buffer (same as window size)
    uint32_t expected;       // Next expected sequence number
    uint32_t highest;        // Highest received sequence number
    BufferState state;       // Current buffer state
    int fd;                  // File descriptor for writing data
} Buffer;

// Initialize buffer with specified size
Buffer* buffer_init(uint32_t buffer_size, int output_fd);

// Process received packet
// Returns action to take (RR or SREJ)
int buffer_process_packet(Buffer* buf, uint32_t seq_num, uint8_t* data, uint16_t len);

// Handle flushing state - process buffered packets in sequence
// Returns 1 if flushing complete, 0 if another gap found
int buffer_flush(Buffer* buf);

// Get sequence number to RR or SREJ
uint32_t buffer_get_response_seq(Buffer* buf);

// Check if buffer should send SREJ for this sequence number
int buffer_should_srej(Buffer* buf, uint32_t seq_num);

// Free buffer resources
void buffer_free(Buffer* buf);

#endif