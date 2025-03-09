// window.h
#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_PDU_SIZE 1407  // 7 bytes header + 1400 max data

// Structure for PDU buffer entries
typedef struct {
    uint32_t seq_num;            // Sequence number
    uint8_t data[MAX_PDU_SIZE];  // Packet data (including header)
    uint16_t pdu_len;            // Length of PDU
    uint8_t valid;               // Flag indicating if this slot contains valid data
    uint8_t attempts;            // Number of transmission attempts (for timeout)
} WindowPDU;

// Window management structure
typedef struct {
    WindowPDU *buffer;      // Circular buffer for storing PDUs
    uint32_t window_size;   // Size of the window (from rcopy)
    uint32_t lower;         // Lowest unacknowledged sequence number
    uint32_t current;       // Next sequence number to send
    uint32_t upper;         // Upper window bound (lower + window_size)
} Window;

// Initialize window with specified size
Window* window_init(uint32_t window_size);

// Check if window is open (can send more packets)
// Returns 1 if open, 0 if closed
int window_is_open(Window* win);

// Store PDU in window buffer
int window_add_pdu(Window* win, uint32_t seq_num, uint8_t* data, uint16_t len);

// Retrieve PDU by sequence number
WindowPDU* window_get_pdu(Window* win, uint32_t seq_num);

// Process received RR (slide window)
int window_process_rr(Window* win, uint32_t rr_seq_num);

// Process received SREJ (mark for retransmission)
int window_process_srej(Window* win, uint32_t srej_seq_num);

// Get lowest unacknowledged PDU (for timeout recovery)
WindowPDU* window_get_lowest(Window* win);

// Free window resources
void window_free(Window* win);




// typedef enum {
//     IN_ORDER_STATE,
//     BUFFERING_STATE,
//     FLUSHING_STATE
// } BufferState;  // Enum for the buffer state

// typedef struct {
//     BufferState current_state;
//     uint32_t seq_num;
//     struct BufferEntry *buffer;
//     uint16_t dataLen;
// } bManager;  // Keeps track of the buffer state

// inorderStateActive(manager);

// bufferingStateActive(manager);

// flushingStateActive(manager);

#endif