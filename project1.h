#ifndef PROJECT1_H
#define PROJECT1_H

#define WINDOW_SIZE 5  // SWS = RWS
#define PACKET_DATA_SIZE 1024

// Header for packet information. Place file data in here and sent whole struct variable?
typedef struct {
    uint8_t SeqNum;  // sequence number: 0..10 used, can hold [0, 255]
    uint8_t AckNum;  // acknowledgement number: 0..10 (maybe bool)
    uint8_t isAck;   // 0 or 1, use seqnum to know which ack it's for
    // packet data here?
}header; 

typedef struct {
    //header hdr; // place here or with the queues?
    uint8_t LAR;  // seqnum of last ack received
    uint8_t LFS;  // last frame sent
    uint8_t NFE;  // seqnum of next frame expected
    struct sendQ_slot {
        header hdr;  // could check in a loop for seq num matching an ack sent
        char msg[PACKET_DATA_SIZE];
    }sendQ[WINDOW_SIZE];
    struct recvQ_slot {
        header hdr;
        int wasReceived;  // is msg valid?
        char msg[PACKET_DATA_SIZE];
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
//int swpInWindow()

/*
 * This function creates the header for a packet. The packet is from 0-9
 * for the sequence number and 0 or 1 to say if its an acknowledgement.
 */ 
// char* createHeader(uint8_t seqNum, uint8_t isAck) {
//     char hdr[3];
//     if (seqNum > 0 && seqNum < 10) {
//         strcat(hdr, itoa(seqNum));
//     } else {
//         printf("Invalid range for sequence number.\n");
//         exit(1);
//     }
//     if (isAck == 0 || isAck == 1) {
//         strcat(hdr, itoa(isAck));
//     } else {
//         printf("Invalid range for acknowledgement number.\n");
//         exit(1);
//     }

//     return hdr;
// }

#endif
