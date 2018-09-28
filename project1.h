#ifndef PROJECT1_H
#define PROJECT1_H

#include <arpa/inet.h>

#define WINDOW_SIZE 5  // SWS = RWS
#define PACKET_DATA_SIZE 1024

// Header for packet information. Place file data in here and sent whole struct variable?
typedef struct {
    int SeqNum;  // sequence number: 0..10 used, can hold [0, 255]
    //int AckNum;  // acknowledgement number: 0..10 (maybe bool)
    int isAck;   // 0 or 1, use seqnum to know which ack it's for
    // packet data here?
}header; 

typedef struct {
    //int fileSize;  // size in bytes
    //header hdr; // place here or with the queues?
    int LAR;  // seqnum of last ack received
    int LFS;  // last frame sent
    int LAF;  // largest acceptable frame
    int LFR;  // last frame received
    int NFE;  // seqnum of next frame expected
    int pos;  // window position (0 through 2*windowsize-1)
    struct sendQ_slot {
        header hdr;  // could check in a loop for seq num matching an ack sent
        int isValid;  // used to prevent resending old data
        char msg[PACKET_DATA_SIZE];
    }sendQ[WINDOW_SIZE];
    struct recvQ_slot {
        header hdr;
        int wasReceived;  // is msg valid?
        char msg[PACKET_DATA_SIZE];
    }recvQ[WINDOW_SIZE];
}swpState;

typedef enum { false, true } bool;

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
		printf("Please enter a valid port number.\n");
        return 0;
	} else {
        return 1;  // valid number
    }
}

/**
 * This function checks if a valid ipv4 address was passed.
 * source:   https://stackoverflow.com/questions/791982
 * accessed: 9/26/18
 */
int isValidIpAddress(char *ipAddress) {
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}

/*
 * This function checks if a sequence number is in the window.
 * Adapted from the text "Computer Networks: A Systems Approach".
 * A range of [0,9] is used, but left open in case of changes in
 * the future.
 */
int swpInWindow(int seqno, int min, int max) {
    int pos, maxpos;
    pos = seqno - min;       // pos should be in range [0..Max)
    maxpos = max - min + 1;  // maxpos is in range [0..Max]
    return pos < maxpos;
}

/*
 * This function creates the header for a packet. The packet is from 0-9
 * for the sequence number and 0 or 1 to say if its an acknowledgement.
 * Two bytes are used to store this information. This header will then be
 * appended to a packet to provide this information.
 */ 
void createHeader(char* hdr, int seqNum, int isAck) {
    char tmp[3];
    if ( seqNum >= 0 && seqNum < 2 * WINDOW_SIZE ) {
        sprintf(hdr, "%d", seqNum);
    } else {
        printf("Invalid range for sequence number.\n");
        exit(1);
    }
    if (isAck == 0 || isAck == 1) {
        sprintf(tmp, "%d", isAck);
        strcat(hdr, tmp);
    } else {
        printf("Invalid range for acknowledgement number.\n");
        exit(1);
    }
    printf("Created header: %s\n", hdr);  // TEST
}

/*
 * This function creates the header for a packet. The packet is from 0-9
 * for the sequence number and 0 or 1 to say if its an acknowledgement.
 * This header will then be appended to a packet to provide this 
 * information. A header structure is assigned its values. A packet
 * structure may use this header structure as one of its fields and
 * directly sent all of this information with the message.
 */ 
void createHeaderStruct(header *hdr, int seqNum, int isAck) {
    char tmp[3];
    if ( seqNum >= 0 && seqNum < 2 * WINDOW_SIZE ) {
        hdr->SeqNum = seqNum;
    } else {
        printf("Invalid range for sequence number.\n");
        exit(1);
    }
    if (isAck == 0 || isAck == 1) {
        hdr->isAck = isAck;
    } else {
        printf("isAck is true (1) or false (0).\n");
        exit(1);
    }
}

/*
 * This function is used to check for curruption of a packet.
 */
// int checksum(char *)

#endif
