#ifndef PROJECT1_H
#define PROJECT1_H

#define WINDOW_SIZE 5  // SWS = RWS
#define PACKET_DATA_SIZE 1024

// Header for packet information. Place file data in here and sent whole struct variable?
typedef struct {
    uint8_t SeqNum;  // sequence number: 0..10 used, can hold [0, 255]
    uint8_t AckNum;  // acknowledgement number: 0..10
    // packet data here?
}header; 

typedef struct {
    header hdr;
    uint8_t LAR;  // seqnum of last ack received
    uint8_t LFS;  // last frame sent
    uint8_t NFE;  // seqnum of next frame expected
    struct sendQ_slot {
        char[PACKET_DATA_SIZE] msg;
    }sendQ[WINDOW_SIZE];
    struct recvQ_slot {
        int wasReceived;  // is msg valid?
        char[PACKET_DATA_SIZE] msg;
    }recvQ[WINDOW_SIZE];
}swpState;

/**
 * This function sets all of a string's characters to the terminator to
 * prevent old data from effecting the next use of the string.
 */
void clearBuffer(char* b) {
    memset(b, '\0', strlen(b));
}

/**
 * This function clears the input buffer of all characters. If this
 * is run when the buffer is empty, this will pause the program and wait
 * for a character input. Intented to use when input functins do not 
 * consume the newline character.
 */
void clearInputBuffer() {
    while (getchar() != '\n')
        ;  // clear the buffer
}

/** 
 * This function checks if the port number provided is valid.
 */
int isValidPort(int port) {
	if (port < 0 || port > 65535) {
		printf("Please enter a valid port number.");
        return 0;
	} else {
        return 1;  // valid number
    }
}

// This function checks if a sequence number is in the window.
int swpInWindow()

#endif
