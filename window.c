#include "window.h"



/**
 * Initialize a new window with the specified size
 * 
 * @param window_size Size of the window (number of packets)
 * @return Pointer to initialized Window structure or NULL on error
 */
Window* window_init(uint32_t window_size) {
    if (window_size == 0) {
        fprintf(stderr, "Error: Window size cannot be zero\n");
        return NULL;
    }
    
    Window* win = (Window*)malloc(sizeof(Window));
    if (!win) {
        perror("Failed to allocate memory for Window");
        return NULL;
    }
    
    win->buffer = (WindowPDU*)malloc(window_size * sizeof(WindowPDU));
    if (!win->buffer) {
        perror("Failed to allocate memory for window buffer");
        free(win);
        return NULL;
    }
    
    // Initialize all buffer entries
    for (uint32_t i = 0; i < window_size; i++) {
        win->buffer[i].valid = 0;
        win->buffer[i].attempts = 0;
    }
    
    win->window_size = window_size;
    win->lower = 0;      // Start sequence number
    win->current = 0;    // Next sequence number to send
    win->upper = window_size; // Upper bound (lower + window_size)
    
    return win;
}


/**
 * Check if the window is open (can send more packets)
 * 
 * @param win Pointer to Window structure
 * @return 1 if window is open, 0 if window is closed, -1 on error
 */
int window_is_open(Window* win) {
    if (!win) {
        fprintf(stderr, "Error: NULL window pointer\n");
        return -1;
    }
    
    return (win->current < win->upper) ? 1 : 0;
}

/**
 * Store a PDU in the window buffer
 * 
 * @param win Pointer to Window structure
 * @param seq_num Sequence number of the PDU
 * @param data Pointer to PDU data (including header)
 * @param len Length of PDU data
 * @return 0 on success, -1 on error
 */
int window_add_pdu(Window* win, uint32_t seq_num, uint8_t* data, uint16_t len) {
    if (!win || !data) {
        fprintf(stderr, "Error: NULL pointer passed to window_add_pdu\n");
        return -1;
    }
    
    if (len > MAX_PDU_SIZE) {
        fprintf(stderr, "Error: PDU size exceeds maximum allowed (%d > %d)\n", len, MAX_PDU_SIZE);
        return -1;
    }
    
    if (seq_num != win->current) {
        fprintf(stderr, "Error: Can only add PDU at current sequence number (%u != %u)\n", seq_num, win->current);
        return -1;
    }
    
    if (!window_is_open(win)) {
        fprintf(stderr, "Error: Cannot add PDU when window is closed\n");
        return -1;
    }
    
    // Get buffer index using circular buffer pattern
    uint32_t index = seq_num % win->window_size;
    
    // Store PDU data in buffer
    win->buffer[index].seq_num = seq_num;
    memcpy(win->buffer[index].data, data, len);
    win->buffer[index].pdu_len = len;
    win->buffer[index].valid = 1;
    win->buffer[index].attempts = 1; // Initial transmission
    
    // Advance current pointer
    win->current++;
    
    return 0;
}


/**
 * Retrieve a PDU from the window buffer by sequence number
 * 
 * @param win Pointer to Window structure
 * @param seq_num Sequence number of the PDU to retrieve
 * @return Pointer to the WindowPDU or NULL if not found/invalid
 */
WindowPDU* window_get_pdu(Window* win, uint32_t seq_num) {
    if (!win) {
        fprintf(stderr, "Error: NULL window pointer\n");
        return NULL;
    }
    
    // Check if sequence number is in the current window range
    if (seq_num < win->lower || seq_num >= win->upper) {
        fprintf(stderr, "Error: Sequence number %u outside window range [%u, %u)\n", 
                seq_num, win->lower, win->upper);
        return NULL;
    }
    
    uint32_t index = seq_num % win->window_size;
    
    // Check if the PDU at this index is valid and has the correct sequence number
    if (!win->buffer[index].valid || win->buffer[index].seq_num != seq_num) {
        return NULL;
    }
    
    return &win->buffer[index];
}



/**
 * Process a Receiver Ready (RR) to slide the window
 * 
 * @param win Pointer to Window structure
 * @param rr_seq_num Sequence number from the RR packet (acknowledges all packets before it)
 * @return Number of packets acknowledged, -1 on error
 */
int window_process_rr(Window* win, uint32_t rr_seq_num) {
    if (!win) {
        fprintf(stderr, "Error: NULL window pointer\n");
        return -1;
    }
    
    // Invalid RR sequence number
    if (rr_seq_num <= win->lower) {
        // RR is for packets already acknowledged
        return 0;
    }
    
    if (rr_seq_num > win->current) {
        // RR is for future packets (not sent yet) - this is an error
        fprintf(stderr, "Error: RR sequence number %u beyond current %u\n", rr_seq_num, win->current);
        return -1;
    }
    
    // Calculate how many packets this RR acknowledges
    uint32_t acked_count = rr_seq_num - win->lower;
    
    // Mark acknowledged packets as invalid (can be reused)
    for (uint32_t seq = win->lower; seq < rr_seq_num; seq++) {
        uint32_t index = seq % win->window_size;
        win->buffer[index].valid = 0;
    }
    
    // Update lower bound and upper bound
    win->lower = rr_seq_num;
    win->upper = win->lower + win->window_size;
    
    return acked_count;
}


/**
 * Process a Selective Reject (SREJ) to mark a packet for retransmission
 * 
 * @param win Pointer to Window structure
 * @param srej_seq_num Sequence number of the packet to retransmit
 * @return 0 on success, -1 on error
 */
int window_process_srej(Window* win, uint32_t srej_seq_num) {
    if (!win) {
        fprintf(stderr, "Error: NULL window pointer\n");
        return -1;
    }
    
    // Check if SREJ sequence number is in valid range
    if (srej_seq_num < win->lower || srej_seq_num >= win->current) {
        fprintf(stderr, "Error: SREJ sequence number %u outside valid range [%u, %u)\n", 
                srej_seq_num, win->lower, win->current);
        return -1;
    }
    
    // Get the PDU from the buffer
    WindowPDU* pdu = window_get_pdu(win, srej_seq_num);
    if (!pdu) {
        fprintf(stderr, "Error: PDU with sequence number %u not found in window\n", srej_seq_num);
        return -1;
    }
    
    // Mark for retransmission by incrementing attempts
    pdu->attempts++;
    
    return 0;
}


/**
 * Get the lowest unacknowledged PDU (for timeout handling)
 * 
 * @param win Pointer to Window structure
 * @return Pointer to the lowest unacknowledged PDU, or NULL if none exists
 */
WindowPDU* window_get_lowest(Window* win) {
    if (!win) {
        fprintf(stderr, "Error: NULL window pointer\n");
        return NULL;
    }
    
    // If window is empty
    if (win->lower == win->current) {
        return NULL;
    }
    
    uint32_t index = win->lower % win->window_size;
    
    // Check if the PDU at this index is valid
    if (!win->buffer[index].valid) {
        fprintf(stderr, "Error: Lowest PDU at index %u is invalid\n", index);
        return NULL;
    }
    
    return &win->buffer[index];
}


/**
 * Free window resources
 * 
 * @param win Pointer to Window structure
 */
void window_free(Window* win) {
    if (!win) {
        return;
    }
    
    if (win->buffer) {
        free(win->buffer);
    }
    
    free(win);
}

// int process_packet_buffer(bManager *manager)
// {
//     int current_state = 0;

//     switch (manager -> current_state)
//     {
//     case IN_ORDER_STATE:
//         current_state = inorderStateActive(manager);
//         break;
    
//     case BUFFERING_STATE:
//         current_state = bufferingStateActive(manager);
//         break;

//     case FLUSHING_STATE:
//         current_state = flushingStateActive(manager);
//         break;
//     }
// }


// void inorderStateActive(bManager * manager){
//     return;
// }

// void bufferingStateActive(bManager * manager){
//     return;
// }

// void flushingStateActive(bManager * manager){
//     return;
// }