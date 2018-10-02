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
#include <limits.h>
#include "project1.h"  // implementation in .h file

int main(int argc, char** argv){
	int portnumber, n;
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
	timeout.tv_sec = 1;   // seconds
	timeout.tv_usec = 20;  // micro sec
	//set options sockfd, option1, option2
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, 
		sizeof(timeout));

	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(portnumber);
	serveraddr.sin_addr.s_addr = inet_addr(ipaddr);

	// Request the file. 
	// TODO Resend request until confirmed.
	int fileRequestDone = 0;
	socklen_t len = sizeof(serveraddr);
	int /*bytesRead,*/ fileSize;
	char filehdr[1];
	while (!fileRequestDone) {
		n = sendto(sockfd, line, strlen(line)+1, 0,
			(struct sockaddr*)&serveraddr, sizeof(serveraddr));
		if (n == -1) {
			printf("Error: file name not sent to server.\n");
		} else {
			// get the file size from the server
			// TODO Try until get this value.
			n = recvfrom(sockfd, packet, PACKET_DATA_SIZE+H_SIZE, 0, 
				(struct sockaddr*)&serveraddr, &len);
			if(n == -1) {
				printf("Did not receive file size in time. Retrying.\n");
			} else {
				fileSize = atoi(packet);
				if (fileSize > 0 && fileSize < INT_MAX) {
					filehdr[0] = 'a';
					printf("Sending ack for file data.\n");
					n = sendto(sockfd, &filehdr[0], 1, 0,
						(struct sockaddr*)&serveraddr, sizeof(serveraddr));
					printf("File data attempted ACK sent: n = %d, data[0] = %u\n", 
						n, filehdr[0]);
					if (n == -1) {
						printf("File data ACK for packet %u not sent.\n", 
							filehdr[0]);
					} else {
						printf("ACK for packet %u sent.\n", filehdr[0]);
						fileRequestDone = 1;
					}
				}
			}
		}
	}
	printf("File request accepted and file size received.\n");
	clearBuffer(line);
	printf("File size to write: %d bytes\n", fileSize);
	clearBuffer(packet);
	int totalPackets = (int)ceil((double)fileSize / PACKET_DATA_SIZE);
	printf("Client will receive %d packets of data.\n", totalPackets);
	
	int transferComplete;
	//int sizeReceived;
	//ssize_t msgSize;  // how to print value?
	int i; 
	int packetsWritten = 0;  // keep going until all packets written
	swpState swp;
	swp.LFR = 0;
	swp.LAF = WINDOW_SIZE;
	swp.NFE = 1;
	u_int8_t sendAck = 0, ackNum = 0;
	//uint8_t client_seqnum = 1;  // use seqnum from server packets only?
	uint8_t freeQSlot = 0;
	int inQ = 0;  // if packet already in the Q
	char hdr[H_SIZE+1];
	// Collect packets from the server until the full file is received.
	outfile = fopen("sentFile", "w");  // account for all file types
	while( !transferComplete ) {  // until all data received
		n = recvfrom(sockfd, packet, PACKET_DATA_SIZE+H_SIZE, 0, 
			(struct sockaddr*)&serveraddr, &len);
		if(n==-1) {
			printf("Error: didn't receive a packet in time.\n");
			continue;
		}
		printf("Header value: %u\n", (uint8_t)packet[0]);  //TEST 
		printf("n = %d\n", n);  //test
		// See if can accept a packet.
		if ( n > 0 && (swp.LAF - swp.LFR <= WINDOW_SIZE) ) {
			printf("LFR: %u, LAF: %u, NFE: %u\n", swp.LFR, swp.LAF, swp.NFE);
			if ( packet[0] <= swp.LFR ||
				 packet[0] > swp.LAF ) {
				printf("Packet %u outside of frame. Not accepted.\n", 
					(uint8_t)packet[0]);
				
				if((uint8_t)packet[0] <= (uint8_t)swp.LFR) {
					sendAck = 1;
					ackNum = packet[0];
				}
			} else {  // packet is within the window
				// First check if it's the NFE. Append data to file if so.
				// Don't add to Q if so, as not needed later.
				if(packet[0] == swp.NFE) {  // next frame in sequence?
					outfile = fopen("sentFile", "a");
					fwrite(&packet[1], 1, n - H_SIZE, outfile);  // needed for binary
					//printf("Received a packet: %d.\n", strlen(packet)+1);
					fclose(outfile);
					packetsWritten++;
					printf("Appended packet %u to file.\n", packet[0]);
					swp.NFE++;
					swp.LFR = (uint8_t)packet[0];
					swp.LAF = (uint8_t)swp.LFR + WINDOW_SIZE;
					
					// Send the ACK. A packet with only the seqnum character.
					sendAck = 1;
					ackNum = packet[0];

					clearBuffer(packet);
				} else {
					// Check if packet already in Q.
					for(i=0; i<WINDOW_SIZE; ++i) {
						if(swp.recvQ[i].isValid) {
							if(packet[0] == swp.recvQ[i].hdr.SeqNum) {
								inQ = 1;
								break;
							}
						}
					}
					if(inQ) {
						printf("Repeated packet %u discarded.\n", 
							(uint8_t)packet[0]);
					} else {
						// add data to Q
						swp.recvQ[freeQSlot].hdr.SeqNum = (uint8_t)packet[0];
						swp.recvQ[freeQSlot].isValid = 1;
						printf("Packet %u in window but out of sequence. Added to queue.\n", 
							(uint8_t)packet[0]);
						freeQSlot = (freeQSlot + 1) % WINDOW_SIZE;
					}

					// Check if have lowest packet to append to file.
					for(i=0; i<WINDOW_SIZE; ++i) {
						if(swp.recvQ[i].isValid && !swp.recvQ[i].wasWritten) {
							if(swp.NFE == swp.recvQ[i].hdr.SeqNum) {
								outfile = fopen("sentFile", "a");
								fwrite(&packet[1], 1, sizeof(packet) - 1, outfile);
								fclose(outfile);
								packetsWritten++;
								swp.recvQ[i].wasWritten = 1;
								printf("Wrote packet %u from the Q.\n", swp.NFE);
								// Send the ACK. A packet with only the seqnum character.
								sendAck = 1;
								ackNum = swp.NFE;
								break;
							}
						}
					}
				}
				if (sendAck) { 
					hdr[0] = ackNum;
					//hdr[1] = '\0';
					printf("Sending ack %u.\n", hdr[0]);
					n = sendto(sockfd, &hdr[0], 1, 0,
						(struct sockaddr*)&serveraddr, sizeof(serveraddr));
					printf("ACK send: n = %d, data[0] = %u\n", n, hdr[0]);
					if (n == -1) {
						printf("ACK for packet %u not sent.\n", 
							hdr[0]);
					} else {
						printf("ACK for packet %u sent.\n", hdr[0]);
					}
				}
			}

			// Check if file write is complete.
			if ( packetsWritten == totalPackets ) {  // TODO, leave until swp done
				transferComplete = 1;
			}

			inQ = 0;
			sendAck = 0;
		}
	}
	printf("Successfully downloaded a file from server.\n");
	printf("Closing client.\n");

	close(sockfd);
	return 0;
}
