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
#include "project1.h"  // implementation in .h file

int main(int argc, char** argv){
	int portnumber;
	char ipaddr[100];
	char line[5000];
	char line2[5000];
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
			return -1;
		}
		printf("Using port: %d\n", portnumber);

		// Read the ip address.
		strcpy(ipaddr, argv[2]);
// add error test for ipaddr?

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
	if(sockfd < 0){
		printf("Error creating socket\n");
		return -1;
	}

	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(portnumber);
	serveraddr.sin_addr.s_addr=inet_addr(ipaddr);

	int len = sizeof(serveraddr);
	sendto(sockfd, line, strlen(line)+1, 0,
		(struct sockaddr*)&serveraddr, sizeof(serveraddr));

	// receive file contents
	int n = recvfrom(sockfd, line2, 5000, 0,
		(struct sockaddr*)&serveraddr, &len);
	//printf("Got file from server: %s\n", line2);  // should only run for text files
	
	// process file contents: clear contents of file with same name or create file
	outfile = fopen("sentFile", "w");  // account for all file types

	// Write the buffer to the file.
	// -wait until get all of file to do this (connect all packets in order?)
	fprintf(outfile, "%s", line2);  // write data to file

	fclose(outfile);
	close(sockfd);
	return 0;
}
