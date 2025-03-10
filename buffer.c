#include "buffer.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/**
 * Initialize buffer with specified size
 * 
 * @param buffer_size Size of the buffer (same as window size)
 * @param output_fd File descriptor for output file
 * @return Pointer to initialized Buffer structure or NULL on error
 */
Buffer* buffer_init(uint32_t buffer_size, int output_fd) {
    if (buffer_size == 0) {
        fprintf(stderr, "Error: Buffer size cannot be zero\n");
        return NULL;
    }
    
    if (output_fd < 0) {
        fprintf(stderr, "Error: Invalid file descriptor\n");
        return NULL;
    }
    
    Buffer* buf = (Buffer*)malloc(sizeof(Buffer));
    if (!buf) {
        perror("Failed to allocate memory for Buffer");
        return NULL;
    }
    
    buf->entries = (BufferEntry*)malloc(buffer_size * sizeof(BufferEntry));
    if (!buf->entries) {
        perror("Failed to allocate memory for buffer entries");
        free(buf);
        return NULL;
    }
    
    // Initialize all buffer entries
    for (uint32_t i = 0; i < buffer_size; i++) {
        buf->entries[i].valid = 0;
        buf->entries[i].seq_num = 0;
        buf->entries[i].data_len = 0;
    }
    
    buf->buffer_size = buffer_size;
    buf->expected = 0;        // Start with sequence number 0
    buf->highest = 0;         // No packets received yet
    buf->state = BUFFER_STATE_IN_ORDER;
    buf->fd = output_fd;
    
    return buf;
}

/**
 * Process received packet based on current buffer state
 * 
 * @param buf Pointer to Buffer structure
 * @param seq_num Sequence number of received packet
 * @param data Pointer to packet data payload (excluding header)
 * @param len Length of data
 * @return 1 for RR, 0 for SREJ, -1 on error
 */
int buffer_process_packet(Buffer* buf, uint32_t seq_num, uint8_t* data, uint16_t len) {
    if (!buf || !data) {
        fprintf(stderr, "Error: NULL pointer passed to buffer_process_packet\n");
        return -1;
    }
    
    if (len > MAX_DATA_SIZE) {
        fprintf(stderr, "Error: Data size exceeds maximum allowed (%d > %d)\n", len, MAX_DATA_SIZE);
        return -1;
    }
    
    // Process based on current state
    switch (buf->state) {
        case BUFFER_STATE_IN_ORDER:
            if (seq_num == buf->expected) {
                // In-order packet, write to file directly
                ssize_t written = write(buf->fd, data, len);
                if (written < 0) {
                    perror("Failed to write data to file");
                    return -1;
                }
                if ((size_t)written != len) {
                    fprintf(stderr, "Warning: Only wrote %zd of %d bytes\n", written, len);
                }
                
                // Update expected sequence number
                buf->expected++;
                
                // Return 1 to send RR for next expected packet
                return 1;
            } 
            else if (seq_num > buf->expected) {
                // Out-of-order packet, buffer it
                uint32_t index = seq_num % buf->buffer_size;
                
                // Store data in buffer
                buf->entries[index].seq_num = seq_num;
                memcpy(buf->entries[index].data, data, len);
                buf->entries[index].data_len = len;
                buf->entries[index].valid = 1;
                
                // Update highest received sequence number
                if (seq_num > buf->highest) {
                    buf->highest = seq_num;
                }
                
                // Transition to BUFFERING state
                buf->state = BUFFER_STATE_BUFFERING;
                
                // Return 0 to send SREJ for missing packet
                return 0;
            } 
            else {
                // Duplicate packet (seq_num < expected), ignore it
                fprintf(stderr, "Warning: Received duplicate packet %u, expected %u\n", seq_num, buf->expected);
                return 1; // Still send RR for expected packet
            }
            break;
            
        case BUFFER_STATE_BUFFERING:
            if (seq_num == buf->expected) {
                // Received the expected packet while buffering
                ssize_t written = write(buf->fd, data, len);
                if (written < 0) {
                    perror("Failed to write data to file");
                    return -1;
                }
                
                // Update expected sequence number
                buf->expected++;
                
                // Transition to FLUSHING state to process buffered packets
                buf->state = BUFFER_STATE_FLUSHING;
                
                // Flush buffer and determine next action
                return buffer_flush(buf);
            } 
            else if (seq_num > buf->expected) {
                // Another out-of-order packet
                uint32_t index = seq_num % buf->buffer_size;
                
                // Store data in buffer
                buf->entries[index].seq_num = seq_num;
                memcpy(buf->entries[index].data, data, len);
                buf->entries[index].data_len = len;
                buf->entries[index].valid = 1;
                
                // Update highest received if needed
                if (seq_num > buf->highest) {
                    buf->highest = seq_num;
                }
                
                // Still need the expected packet, send SREJ
                return 0;
            } 
            else {
                // Duplicate packet, ignore it
                fprintf(stderr, "Warning: Received duplicate packet %u while buffering\n", seq_num);
                return 0; // Still need the expected packet, send SREJ
            }
            break;
            
        case BUFFER_STATE_FLUSHING:
            // This should not be called directly during FLUSHING
            // buffer_flush handles the flushing state
            fprintf(stderr, "Warning: buffer_process_packet called during FLUSHING state\n");
            
            // Store packet for later processing
            if (seq_num >= buf->expected) {
                uint32_t index = seq_num % buf->buffer_size;
                buf->entries[index].seq_num = seq_num;
                memcpy(buf->entries[index].data, data, len);
                buf->entries[index].data_len = len;
                buf->entries[index].valid = 1;
                
                if (seq_num > buf->highest) {
                    buf->highest = seq_num;
                }
            }
            
            // Continue flushing
            return buffer_flush(buf);
            
        default:
            fprintf(stderr, "Error: Invalid buffer state\n");
            return -1;
    }
}

/**
 * Handle flushing state - process buffered packets in sequence
 * 
 * @param buf Pointer to Buffer structure
 * @return 1 if flushing complete (send RR), 0 if another gap found (send SREJ), -1 on error
 */
int buffer_flush(Buffer* buf) {
    if (!buf) {
        fprintf(stderr, "Error: NULL buffer pointer\n");
        return -1;
    }
    
    // Flush all consecutive packets from buffer
    while (buf->expected <= buf->highest) {
        uint32_t index = buf->expected % buf->buffer_size;
        
        // Check if this packet is in the buffer
        if (!buf->entries[index].valid || buf->entries[index].seq_num != buf->expected) {
            // Missing packet, exit flushing and send SREJ
            buf->state = BUFFER_STATE_BUFFERING;
            return 0; // Send SREJ for this packet
        }
        
        // Write packet data to file
        ssize_t written = write(buf->fd, buf->entries[index].data, buf->entries[index].data_len);
        if (written < 0) {
            perror("Failed to write buffered data to file");
            return -1;
        }
        
        // Mark entry as invalid (free for reuse)
        buf->entries[index].valid = 0;
        
        // Move to next expected packet
        buf->expected++;
    }
    
    // If we get here, all buffered packets have been processed
    buf->state = BUFFER_STATE_IN_ORDER;
    return 1; // Send RR for next expected packet
}

/**
 * Get sequence number to include in RR or SREJ response
 * 
 * @param buf Pointer to Buffer structure
 * @return Sequence number to use in response
 */
uint32_t buffer_get_response_seq(Buffer* buf) {
    if (!buf) {
        fprintf(stderr, "Error: NULL buffer pointer\n");
        return 0;
    }
    
    // Return the expected sequence number
    return buf->expected;
}

/**                                                                 
 * Check if buffer should send SREJ for this sequence number
 * 
 * @param buf Pointer to Buffer structure
 * @param seq_num Sequence number to check
 * @return 1 if should send SREJ, 0 otherwise
 */
int buffer_should_srej(Buffer* buf, uint32_t seq_num) {
    if (!buf) {
        fprintf(stderr, "Error: NULL buffer pointer\n");
        return 0;
    }
    
    // If we're in BUFFERING state and this is the expected sequence number,
    // we should send a SREJ for it
    if (buf->state == BUFFER_STATE_BUFFERING && seq_num == buf->expected) {
        return 1;
    }
    
    // If we're in any state and we find a gap in the sequence,
    // we should send a SREJ
    if (seq_num > buf->expected && seq_num <= buf->highest) {
        uint32_t index = seq_num % buf->buffer_size;
        if (!buf->entries[index].valid) {
            return 1;
        }
    }
    
    return 0;
}

/**
 * Free buffer resources
 * 
 * @param buf Pointer to Buffer structure
 */
void buffer_free(Buffer* buf) {
    if (!buf) {
        return;
    }
    
    if (buf->entries) {
        free(buf->entries);
    }
    
    // Note: We don't close the file descriptor as it should be managed
    // by the caller
    
    free(buf);
}