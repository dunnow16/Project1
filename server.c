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
#include "project1.h"  // implementation in .h file

int main(int argc, char** argv) {
	printf("--------------UDP file transfer server--------------\n");
	printf("Sent file requests must be in the current directory.\n");
	printf("Run: ./exename portnumber\n");

	int portnumber;
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
	timeout.tv_sec = 5;   // seconds
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
	while(1){
		int len = sizeof(clientaddr);
		char line[WINDOW_SIZE * PACKET_DATA_SIZE];
		char buffer[WINDOW_SIZE * PACKET_DATA_SIZE];
		int n = recvfrom(sockfd, line, WINDOW_SIZE * PACKET_DATA_SIZE, 0, 
			(struct sockaddr*)&clientaddr, &len);
		if(n == -1){
			printf("time out on receive\n");
		}
		else{
			printf("Got file request from client: %s\n", line);
		
			if((fp = fopen(line, "r")) == NULL){  // returns file pointer (fp)
				printf("File does not exist\n");
			}	
			else{
				// Get the file size.
				int status = stat(line, &fileStats);
				if (status == 0) {
					fileSize = fileStats.st_size;
					printf("File \"%s\" size: %d bytes\n", line, fileSize);
				} else {
					printf("Failed to read file size.\n");
					return -1;
				}
				
				// Read and send the file packet by packet.
				while ( fread(buffer, PACKET_DATA_SIZE, 1, fp) > 0 ) {  // data read
					
				}

				
				//printf("%s\n", buffer);  //  should only run for text files

				// Append a header to the front of the file? (first x bits)

				sendto(sockfd, buffer, strlen(buffer)+1, 0, 
					(struct sockaddr*)&clientaddr, sizeof(clientaddr));
				fclose(fp);
			}	
		}
	}

	return 0;
}

