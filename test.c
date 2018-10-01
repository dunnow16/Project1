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
    uint8_t a =  '1';
    char packet[2];
    packet[0] = 255; 

    printf("%u\n", (unsigned char)a);
    printf("%u\n", (unsigned char)packet[0]);

    if ( (packet[1] - 48) == 1 ) {  // is an ACK
        char* packetNumString;
        sprintf(packetNumString, "%u", packet[0]);
        //swp.LAR = atoi(packetNumString);

        //free(packet);
    }

    return 0;
}