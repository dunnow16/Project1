/*
 * Michael Weir, Owen Dunn, Anthony Nguyen
 * Programming Project 1: Reliable File Transfer over UDP
 * Cis 457
 * Fall 2018
 *
 * Server:
 * When started, the server asks for a port number. The server then listens
 * for connections on that port. The sliding window protocol is implemented
 * to allow reliable file transfer with UDP. A window size of 5 is used.
 * 
 * Run: ./exename portnumber 
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include "project1.h"  // implementation in .h file

int createUDPSocket(int portnumber, struct sockaddr_in serveraddr);

int main(int argc, char** argv) {
	printf("--------------UDP file transfer server--------------\n");
	printf("Sent file requests must be in the current directory.\n");
	printf("Run: ./exename portnumber\n");

	int portnumber;
	swpState swp;  // has variables to handle the sliding window protocol
	// Allow command line input. (save time testing)
	if (argc == 2) {
		portnumber = atoi(argv[1]);
		if (!isValidPort(portnumber)) {
			return -1;
		}
		printf("Using port: %d\n", portnumber);
	} else {
		printf("Invalid command line input.\n");
		printf("Run: ./exename portnumber \n");
	}

	FILE *fp;	
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("There was an error creating the socket.\n");
		return 1;
	}

	struct timeval timeout;
	timeout.tv_sec = 1;   // seconds
	timeout.tv_usec = 0;  // micro sec

	//set options sockfd, option1, option2
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, 
		sizeof(timeout));

	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(portnumber);
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	int b = bind(sockfd,(struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (b < 0) {
		printf("failed to bind\n");
		return 2;
	}

	long fileSize = 0;
	struct stat fileStats;
	int elementsRead = 0;
	int packetsSent = 0;
	int n;  	  // wait timing variable
	swp.LAR = 0;  // seqnum of last ack received
	swp.LFS = 0;  // seqnum of last frame (packet) sent
	//int send_i; // how many frames sent and not ack yet
	uint8_t server_seqnum = 1;  // start at 1, (max of 255) assumed unlimited range (doesn't loop back)
	// char hdr[3];
	// createHeader(hdr, 1, 1);
	// printf("%s\n", hdr);

	int transferComplete;  // flag to mark end of file transfer
	int sizeReceived;      // flag to know if file size received by client
	int acksReceived;	   // acknowledgements received
	while(1) {
		socklen_t  len = sizeof(clientaddr);
		char line[PACKET_DATA_SIZE];
		char buffer[PACKET_DATA_SIZE];
		// Stop and wait for a file request. 
		// (timer used to quit waiting after 5s)
		n = recvfrom(sockfd, line, WINDOW_SIZE * PACKET_DATA_SIZE, 0, 
			(struct sockaddr*)&clientaddr, &len);
		if(n == -1) {  // waited 5s
			printf("Time out on receive (5 seconds) waiting for file request.\n");
		}
		else {
			printf("Got file request from client: %s\n", line);
			if((fp = fopen(line, "r")) == NULL){  // returns file pointer (fp)
				printf("File does not exist\n");
			}	
			else{
				// Get the file size.
				int status = stat(line, &fileStats);
				if (status == 0) {
					fileSize = fileStats.st_size;
					printf("File \"%s\" size: %ld bytes\n", line, fileSize);
				} else {
					printf("Failed to read file size.\n");
					return -1;
				}
				// Send file size to client.
				sprintf(buffer, "%ld", fileSize);
				//printf("file size: %s\n", buffer);
				sizeReceived = 0;
				while ( !sizeReceived ) {
					n = sendto(sockfd, buffer, strlen(buffer)+1, 0, 
						(struct sockaddr*)&clientaddr, sizeof(clientaddr));
					
					// TODO Wait for an ack for the file size msg, resend if missed.
					sizeReceived = 1;  // TODO, leave 1 for now 
				}
				clearBuffer(buffer);
				// Read and send the file: packet by packet. 
				// (how many data packets)
				int totalPackets = 
					(int)ceil((double)fileSize / PACKET_DATA_SIZE);

				// Read and send all the file data 1024 bytes at a time.
				// Add header necessary data to handle error and use queues to
				// resend data when necessary (loop over q looking for needed
				// packet).
				transferComplete = 0;  // true when last ACK received
				swp.LFS = 1;
				swp.LAR = 0;  // initialize to 0 (shouldn't effect logic?)
				uint8_t freeQSlot = 0;  // used to keep track of slots in Q that are old data or unwritten
				//uint8_t unAckFrames = 0;  // how many unacknowledged frames
				char* packet;  // contains header and data
				char hdr[H_SIZE];  // has no null terminator!
				while ( !transferComplete ) { // ack received for all packets
					if ( swp.LFS - swp.LAR < WINDOW_SIZE ) {  // maintain window size
						if ( !feof(fp) ) {  // if haven't read all of file yet
							// If read at least one element.
							if ((elementsRead = 
								fread(buffer, 1, PACKET_DATA_SIZE, fp)) > 0 ) {
								printf("Read %d bytes from a file.\n", elementsRead);
								packetsSent++;  // TODO remove once complete logic done
								swp.LFS = (uint8_t)server_seqnum;  // last frame sent
								// add data to the sendQ
								strcpy(swp.sendQ[freeQSlot].msg, buffer);
								// add the header to the sendQ packet
								swp.sendQ[freeQSlot].hdr.SeqNum = server_seqnum;
								swp.sendQ[freeQSlot].isValid = 1;  // not empty data
								//swp.sendQ[freeQSlot].hdr.isAck = 0;

								// create a header of 2 char bits to append the front of data packet
								//createHeaderStructure(&swp.sendQ[send_i].hdr, server_seqnum, 0);
								
								createHeader(hdr, (uint8_t)server_seqnum);  // TODO
	
								// TODO Create packet with header and data. Send it.
								// TODO add checksum char(s)?
								packet = (char*)malloc(sizeof(char)*(elementsRead + H_SIZE));
								createPacket(packet, hdr, buffer);

								n = sendto(sockfd, packet, elementsRead+H_SIZE, 0, 
									(struct sockaddr*)&clientaddr, sizeof(clientaddr));
								printf("%d\n", n);
								if(n>0) {
									// should only need 5 spaces in Q, otherwise outside of window
									freeQSlot = (freeQSlot + 1) % WINDOW_SIZE;  // cycle about Q as write data
									//server_seqnum = (server_seqnum + 1) % (2 * WINDOW_SIZE);  // move server seqnum up one
									server_seqnum++;  // assumed unlimited range
									free(packet);   // allow to be a dif size for last packet (if >1024 bytes)
									printf("Sent packet %u with data and header.\n", 
										swp.LFS);
								} else {
									printf("Error: failed to send packet %u\n", server_seqnum);
								}
							} else {
								printf("File completely read at server.\n");
								printf("Waiting for last acknowledgements from client.\n");
							}
						}
					} 

					/*
					TODO replace strcpy:
					char buffer[345];
					set header (bit 0)
					memcpy(after header addr, data, elementsRead)
					*/

					// Check for an acknowledgement. TODO
					// Wait for 5s and then resend oldest packet? or all?
					// use same socket?
					packet = (char*)malloc(sizeof(char)*(H_SIZE));
					n = recvfrom(sockfd, packet, H_SIZE+1, 0, 
						(struct sockaddr*)&clientaddr, &len);
					printf("Ack n = %d\n", n);  //test
					printf("Ack packet: %u\n", packet[0]);  //test
					if(n == -1) {  // waited 5s
						printf("Time out on receive (5 seconds) waiting for ACK.\n");
					} else if ( n > 0 ) {  // validate packet contains data first TODO 
							swp.LAR = (uint8_t)packet[0];  // TODO works?
							acksReceived++;
							printf("Got acknowledgement for packet %d.\n", swp.LAR);
							free(packet);
							// Mark the stored packet as acknowledged.
							int i;
							for(i = 0; i < WINDOW_SIZE; ++i) {
								if(swp.sendQ[i].hdr.SeqNum == packet[0]) {
									swp.sendQ[i].ackRecv = 1;
									break;
								}
							}
					} else {
						printf("ERROR: packet has no data to read.\n");
					}

					if ( acksReceived == totalPackets ) {
						transferComplete = 1;
						break;
					}

					// TODO Resend packet(s) not received.
					int lowest_i, i;  // find lowest packet seqNum to send
					int atLeastOneUnsent = 0;
					for(i = 0; i < WINDOW_SIZE; ++i) {
						if ( swp.sendQ[i].isValid && !swp.sendQ[i].ackRecv ) {
							if ( atLeastOneUnsent ) {
								if ( swp.sendQ[i].hdr.SeqNum < swp.sendQ[lowest_i].hdr.SeqNum ) {
									lowest_i = i;
								}			
							} else {
								lowest_i = i;  // save index of lowest packet
								atLeastOneUnsent = 1;
							}
						}
					}
					if (atLeastOneUnsent) {
						createHeader(hdr, swp.sendQ[lowest_i].hdr.SeqNum);
						packet = (char*)malloc(sizeof(char)*(PACKET_DATA_SIZE + H_SIZE));
						createPacket(packet, hdr, swp.sendQ[lowest_i].msg);
						n = sendto(sockfd, packet, elementsRead+H_SIZE, 0, 
							(struct sockaddr*)&clientaddr, sizeof(clientaddr));
						printf("Resent packet %u with data and header.\n", 
							swp.sendQ[lowest_i].hdr.SeqNum); 
						free(packet);
						atLeastOneUnsent = 0;
						lowest_i = 0;
					}
					clearBuffer(buffer);
				}
				//printf("Sent %d packets\n", packetsSent);
				packetsSent = 0;  // TODO remove once not needed
				server_seqnum = 1;  // ready for next file

				fclose(fp);  // close the file
			}	
		}
	}

	return 0;
}

int createUDPSocket(int portnumber, struct sockaddr_in serveraddr) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("There was an error creating the socket.\n");
		exit(1);
	}
	struct timeval timeout;
	timeout.tv_sec = 5;   // seconds
	timeout.tv_usec = 0;  // micro sec

	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, 
			   sizeof(timeout));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(portnumber);
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	int b = bind(sockfd,(struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (b < 0) {
		printf("failed to bind\n");
		exit(1);
	}

	return sockfd;
}
