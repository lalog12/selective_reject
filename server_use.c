#include "server_use.h"

void serverUseFSM(int socketNum, FILE* fp, struct sockaddr* client, int* clientAddrLen) {
    // printf("serverUseStateFSM: Function\n");
    SERVER_USE_STATES current_state = SERVER_USE_SEND_STATE;

    Window* win = window_init(serverWindowLength);
    uint32_t server_seq_num = 0;
    uint32_t retry_count = 0;
    int eof_reached = 0;
    char buffer[MAXBUF];
    int len = 0;
    
    if (!win) {
        fprintf(stderr, "Error: Failed to initialize window\n");
        serverUseTeardownState(socketNum);
        return;
    }
    // printf("Entering Server Use State FSM\n");
    // Main state machine loop
    while (current_state != SERVER_USE_TEARDOWN_STATE) {
        switch (current_state) {
            
            case SERVER_USE_SEND_STATE:
                printf("SERVER_USE_SEND_STATE \n");
                current_state = serverUseSendState(socketNum, buffer, fp, client, clientAddrLen, win, &server_seq_num, &eof_reached);

                break;
                
            case SERVER_USE_WINDOW_OPEN_STATE:
                printf("SERVER_USE_WINDOW_OPEN_STATE \n");
                current_state = serverUseWindowOpenState(socketNum, buffer, client, clientAddrLen, win);
               
                break;
                
            case SERVER_USE_WINDOW_CLOSED_STATE:
                printf("SERVER_USE_WINDOW_CLOSE_STATE \n");
                current_state = serverUseWindowClosedState(socketNum, buffer, client, clientAddrLen, win, &retry_count);
                
                break;
                
            case SERVER_USE_PROCESS_RR_STATE:
                printf("SERVER_USE_PROCESS_RR_STATE \n");
                current_state = serverUseProcessRRState(socketNum, buffer, len, client, clientAddrLen, win);
                
                break;
                
            case SERVER_USE_PROCESS_SREJ_STATE:
                printf("SERVER_USE_PROCESS_SREJ_STATE \n");
                current_state = serverUseProcessSREJState(socketNum, buffer, len, client, clientAddrLen, win);
                
                break;
                
            case SERVER_USE_EOF_STATE:
                printf("SERVER_USE_EOF_STATE \n");
                current_state = serverUseEOFState(socketNum, buffer, client, clientAddrLen, win, server_seq_num, &retry_count);
                
                break;
                
            case SERVER_USE_TEARDOWN_STATE:
                printf("SERVER_USE_TEARDOWN_STATE \n");
                serverUseTeardownState(socketNum);
                
                break;
                
            default:
                fprintf(stderr, "Error: Invalid state in serverUseFSM\n");
                current_state = SERVER_USE_TEARDOWN_STATE;
                break;
        }
    }
    
    window_free(win);
}

int serverUseSendState(int socketNum, char* buffer, FILE* fp, struct sockaddr* client, int* clientAddrLen, Window* win, uint32_t* server_seq_num, int* eof_reached) {
    // Check if window is open
    fflush(stdout);
    if (!window_is_open(win)) {
        return SERVER_USE_WINDOW_CLOSED_STATE;
    }
   // printf("window is %d (1 open) (0 closed)\n", window_is_open(win));
    // Check if we've reached EOF
    if (*eof_reached) {
        return SERVER_USE_EOF_STATE;
    }
    
    fflush(stdout);
    // Read data from file
    memset(buffer, 0, MAXBUF);

    fflush(stdout);
    int data_len = fread(buffer + 7, 1, rcopyBufferLength, fp);
    //printf("Reading data from %s of length %d\n", filenameString, data_len);

    fflush(stdout);
    // Check for EOF
    uint8_t flag = 16; // Regular data packet
    
    if (data_len < rcopyBufferLength) {
        if (feof(fp)) {
            *eof_reached = 1;
            flag = 10; // EOF flag
        } else if (ferror(fp)) {
            fprintf(stderr, "Error reading from file\n");
            return SERVER_USE_TEARDOWN_STATE;
        }
    }
    fflush(stdout);
    // If we read 0 bytes but didn't reach EOF, it's an error
    if (data_len == 0 && !(*eof_reached)) {
        fprintf(stderr, "Error: Read 0 bytes but not at EOF\n");
        return SERVER_USE_TEARDOWN_STATE;
    }
    // Create packet header
    Default_Header(win->current, buffer, flag);
    Insert_Checksum(buffer, data_len + 7);
    
    // Add PDU to window
    if (window_add_pdu(win, win->current, (uint8_t*)buffer, data_len + 7) < 0) {
        fprintf(stderr, "Error adding PDU to window\n");
        return SERVER_USE_TEARDOWN_STATE;
    }
    
    // Send the packet
    safeSendto(socketNum, buffer, data_len + 7, 0, client, *clientAddrLen);
    //printf("Sent data to socket number %d, of length %d, with sequence number %d\n",socketNum ,data_len , *server_seq_num);
    (*server_seq_num)++;
    fflush(stdout);
    // Check for any available RRs/SREJs (non-blocking)
    return SERVER_USE_WINDOW_OPEN_STATE;
}

int serverUseWindowOpenState(int socketNum, char* buffer, struct sockaddr* client, int* clientAddrLen, Window* win) {
    int poll_result = pollCall(0); // Non-blocking poll
    
    if (poll_result <= 0) {
        // No data available, return to send state
      //  printf("non-blocking poll_result returned %d\n", poll_result);
        return SERVER_USE_SEND_STATE;
    }
    
    // Data is available, process it
    int len = safeRecvfrom(socketNum, buffer, MAXBUF, 0, client, clientAddrLen);
   // printf("Packet received of length: %d\n", len);
    // Check checksum
    uint16_t checksum = Checksum_Corrupt(buffer, len);
    if (checksum != 0) {
        // Corrupt packet, ignore and continue
        return SERVER_USE_WINDOW_OPEN_STATE;
    }
    
    // Process based on flag
    uint8_t flag = buffer[6];
    
    if (flag == 5) { // RR packet
        return SERVER_USE_PROCESS_RR_STATE;
    } else if (flag == 6) { // SREJ packet
        return SERVER_USE_PROCESS_SREJ_STATE;
    }
  //  printf("PACKET RECIEVED HAS BEEN IGNORED with flag: %d\n", flag);
    // Unrecognized packet, ignore
    return SERVER_USE_WINDOW_OPEN_STATE;
}

int serverUseWindowClosedState(int socketNum, char* buffer, struct sockaddr* client, int* clientAddrLen, Window* win, uint32_t* retry_count) {
    int poll_result = pollCall(1000); // 1 second timeout
    
    if (poll_result <= 0) {
        // Timeout occurred, resend lowest packet
        WindowPDU* pdu = window_get_lowest(win);

        if (!pdu) {
  //          printf("All pdus have been acknowledged and we can't send data. window is closed.\n");
            // No unacknowledged PDUs, should not happen
            return SERVER_USE_SEND_STATE;
        }
        
        // Increment retry count
        (*retry_count)++;
        
        // If too many retries, teardown
        if (*retry_count >= 10) {
            fprintf(stderr, "Error: Too many retries, client not responding\n");
            return SERVER_USE_TEARDOWN_STATE;
        }
        
        // Change flag to indicate timeout resent
        uint8_t* data = pdu->data;
        data[6] = 18; // Timeout resent flag
        
        // Recalculate checksum
        Insert_Checksum((char*)data, pdu->pdu_len);
        
        // Resend lowest packet
        safeSendto(socketNum, data, pdu->pdu_len, 0, client, *clientAddrLen);
        printf("Sending packet with socket number: %d, data: %s, and pdu length: %d\n", socketNum, data, pdu -> pdu_len);
        return SERVER_USE_WINDOW_CLOSED_STATE;
    }
    
    // Data is available, process it
    int len = safeRecvfrom(socketNum, buffer, MAXBUF, 0, client, clientAddrLen);
    printf("Processing data to open window\n");
    // Reset retry count when we receive a valid packet
    *retry_count = 0;
    
    // Check checksum
    uint16_t checksum = Checksum_Corrupt(buffer, len);
    if (checksum != 0) {
        // Corrupt packet, ignore and continue waiting
        return SERVER_USE_WINDOW_CLOSED_STATE;
    }
    
    // Process based on flag
    uint8_t flag = buffer[6];
    
    if (flag == 5) { // RR packet
        return SERVER_USE_PROCESS_RR_STATE;
    } else if (flag == 6) { // SREJ packet
        return SERVER_USE_PROCESS_SREJ_STATE;
    }
    
    // Unrecognized packet, ignore
    return SERVER_USE_WINDOW_CLOSED_STATE;
}

int serverUseProcessRRState(int socketNum, char* buffer, int len, struct sockaddr* client, int* clientAddrLen, Window* win) {
    uint32_t rr_seq_num = 0;
    
    // Extract RR sequence number
    memcpy(&rr_seq_num, buffer + 7, 4);
    rr_seq_num = ntohl(rr_seq_num);
    
    // Process RR to slide window
    window_process_rr(win, rr_seq_num);
    
    // Continue based on window state
    return window_is_open(win) ? SERVER_USE_SEND_STATE : SERVER_USE_WINDOW_CLOSED_STATE;
}

int serverUseProcessSREJState(int socketNum, char* buffer, int len, struct sockaddr* client, int* clientAddrLen, Window* win) {
    uint32_t srej_seq_num = 0;
    
    // Extract SREJ sequence number
    memcpy(&srej_seq_num, buffer + 7, 4);
    srej_seq_num = ntohl(srej_seq_num);
    
    // Process SREJ
    if (window_process_srej(win, srej_seq_num) == 0) {
        // Get the requested PDU and resend it
        WindowPDU* pdu = window_get_pdu(win, srej_seq_num);
        if (pdu) {
            // Change flag to indicate resent packet
            uint8_t* data = pdu->data;
            data[6] = 17; // SREJ resent flag
            
            // Recalculate checksum
            Insert_Checksum((char*)data, pdu->pdu_len);
            
            // Resend packet
            printf("Resending packet with %u in response to SREJ\n", srej_seq_num);
            safeSendto(socketNum, data, pdu->pdu_len, 0, client, *clientAddrLen);
            
        }
    }
    
    // Continue based on window state
    return window_is_open(win) ? SERVER_USE_SEND_STATE : SERVER_USE_WINDOW_CLOSED_STATE;
}

int serverUseEOFState(int socketNum, char* buffer, struct sockaddr* client, int* clientAddrLen, Window* win, uint32_t server_seq_num, uint32_t* retry_count) {
    // Check if we still have unacknowledged packets
    if (win->lower < win->current) {
        printf("Still waiting for ACKs: lower=%u, current=%u\n", win->lower, win->current);
        // We still have unacknowledged packets, wait for RRs
        return SERVER_USE_WINDOW_CLOSED_STATE;
    }
    printf("All packets acknowkedged, waiting for final confirmation\n");
    // Per the assignment, rcopy should end first after file transfer is completed
    // Wait for final acknowledgment (1 second timeout)
    int poll_result = pollCall(1000);
    
    if (poll_result > 0) {
        int len = safeRecvfrom(socketNum, buffer, MAXBUF, 0, client, clientAddrLen);
        
        // Check checksum
        uint16_t checksum = Checksum_Corrupt(buffer, len);
        if (checksum == 0 && buffer[6] == 5) {
            // Got final RR, file transfer complete - exit cleanly
            printf("Received final RR, transfer complete\n");
            return SERVER_USE_TEARDOWN_STATE;
        }
    }
    
    // Increment retry count
    (*retry_count)++;
    
    // If too many retries, teardown
    if (*retry_count >= 10) {
        fprintf(stderr, "Warning: Could not confirm client received EOF packet\n");
        return SERVER_USE_TEARDOWN_STATE;
    }
    
    // Otherwise, continue waiting
    return SERVER_USE_EOF_STATE;
}

void serverUseTeardownState(int socketNum) {
    // Clean up and exit

    removeFromPollSet(socketNum);
    exit(1);
}