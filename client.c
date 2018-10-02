/*
 * Michael Weir, Owen Dunn, Anthony Nguyen
 * Programming Project 1: Reliable File Transfer over UDP
 * Cis 457
 * Fall 2018
 * 
 * Client:
 * When started, the client asks for the IP address and a port number.
 * If valid data is given, the client will then use this information
 * to send data. he client then prompts the user for a file name to
 * request from the server (or file path). If the file is found in the
 * server it will be sent to the client over several packets with data
 * size of 1024 bytes each.
 * 
 * Run: ./exename portnumber ipaddress filename
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <math.h>
#include "project1.h"  // implementation in .h file

int main(int argc, char** argv){
	int portnumber;
	char ipaddr[100];
	char line[1024];
	char packet[PACKET_DATA_SIZE+H_SIZE];
	FILE *outfile;

	printf("--------------UDP file transfer client--------------\n");
	printf("Files are created in the current directory.\n");
	printf("Files must be in the current directory of the server.\n");
	printf("Run: ./exename portnumber ipaddress filename\n");

	// Allow command line input. (save time testing)
	if (argc == 4) {
		// Read the port number.
		portnumber = atoi(argv[1]);
		if (!isValidPort(portnumber)) {
			printf("Invalid port number.\n");
			return -1;
		}
		printf("Using port: %d\n", portnumber);

		// Read the ip address.
		strcpy(ipaddr, argv[2]);
		// error test for ipaddr
		if ( !isValidIpAddress(ipaddr) ) {
			printf("Invalid ipv4 address.\n");
			return -1;
		}
		printf("Using ip address: %s\n", ipaddr);

		// Read the file name.
		strcpy(line, argv[3]);
		// add error test for filename?
		printf("Asking for file: %s\n", line);
	} else {
		printf("Invalid command line input.\n");
		printf("Run: ./exename portnumber ipaddress filename\n");
		return -1;
	}
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // using UDP
	if(sockfd < 0) {
		printf("Error creating socket\n");
		return -1;
	}

	struct timeval timeout;
	timeout.tv_sec = 5;   // seconds
	timeout.tv_usec = 0;  // micro sec

	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(portnumber);
	serveraddr.sin_addr.s_addr = inet_addr(ipaddr);

	// Request the file. 
	// TODO Resend request until confirmed.
	socklen_t len = sizeof(serveraddr);
	sendto(sockfd, line, strlen(line)+1, 0,
		(struct sockaddr*)&serveraddr, sizeof(serveraddr));
	clearBuffer(line);

	// get the file size from the server
	// TODO Try until get this value.
	int /*bytesRead,*/ fileSize;
	recvfrom(sockfd, packet, 1024, 0, 
        (struct sockaddr*)&serveraddr, &len);
	fileSize = atoi(packet);
	printf("File size to write: %d bytes\n", fileSize);
	clearBuffer(packet);
	int totalPackets = (int)ceil((double)fileSize / PACKET_DATA_SIZE);
	printf("Client will receive %d packets of data.\n", totalPackets);
	
	int transferComplete;
	int sizeReceived;
	ssize_t msgSize;  // how to print value?
	int i; 
	int packetsWritten = 0;  // keep going until all packets written
	swpState swp;
	swp.LFR = 0;
	swp.LAF = WINDOW_SIZE;
	swp.NFE = 1;
	//uint8_t client_seqnum = 1;  // use seqnum from server packets only?
	uint8_t freeQSlot = 0;
	int inQ = 0;  // if packet already in the Q
	// Collect packets from the server until the full file is received.
	outfile = fopen("sentFile", "w");  // account for all file types
	while( !transferComplete ) {  // until all data received
		// See if can accept a packet.
		if ( swp.LAF - swp.LFR < WINDOW_SIZE ) {
			// msgSize not correct value? (not getting 1024)
			msgSize = recvfrom(sockfd, packet, 1024, 0, 
				(struct sockaddr*)&serveraddr, &len);
			if ( (unsigned int)packet[0] <= swp.LFR ||
				 (unsigned int)packet[0] > swp.LAF ) {
				printf("Packet %u outside of frame. Not accepted.\n", 
					(unsigned int)packet[0]);
			} else {  // packet is within the window
				// First check if it's the NFE. Append data to file if so.
				// Don't add to Q if so, as not needed later.
				if((unsigned int)packet[0] == swp.NFE) {  // next frame in sequence?
					outfile = fopen("sentFile", "a");
					fwrite(packet[1], 1, msgSize - 1, outfile);  // needed for binary
					//printf("Received a packet: %d.\n", strlen(packet)+1);
					clearBuffer(packet);
					fclose(outfile);
					packetsWritten++;
					printf("Appended packet %u to file.\n", 
						(unsigned int)packet[0]);
					swp.NFE++;
					swp.LFR = (unsigned int)packet[0];
					swp.LAF = swp.LFR + WINDOW_SIZE;
					//inQ = 1;
				} else {
					// Check if packet already in Q.
					for(i=0; i<WINDOW_SIZE; ++i) {
						if(swp.recvQ[i].isValid) {
							if((unsigned int)packet[0] == swp.recvQ[i].hdr.SeqNum) {
								inQ = 1;
								break;
							}
						}
					}
					if(inQ) {
						printf("Repeated packet %u discarded,\n", 
							(unsigned int)packet[0]);
					} else {
						// add data to Q
						swp.recvQ[freeQSlot].hdr.SeqNum = (unsigned int)packet[0];
						swp.recvQ[freeQSlot].isValid = 1;
						printf("Packet %u in window and added to queue.\n", 
							(unsigned int)packet[0]);
						freeQSlot = (freeQSlot + 1) % WINDOW_SIZE;
					}

					// Check if have lowest packet to append to file.
					for(i=0; i<WINDOW_SIZE; ++i) {
						if(swp.recvQ[i].isValid && !swp.recvQ[i].wasWritten) {
							if(swp.NFE == swp.recvQ[i].hdr.SeqNum) {
								outfile = fopen("sentFile", "a");
								fwrite(packet[1], 1, msgSize - 1, outfile);
								fclose(outfile);
								packetsWritten++;
								swp.recvQ[i].wasWritten = 1;
								break;
							}
						}
					}
				}
			}

			// Check if file write is complete.
			if ( packetsWritten == totalPackets ) {  // TODO, leave until swp done
				transferComplete = 1;
			}
			
			inQ = 0;
		}
	}
	printf("Successfully download a file from server.\n");
	printf("Closing client.\n");

	//fclose(outfile);
	close(sockfd);
	return 0;
}
