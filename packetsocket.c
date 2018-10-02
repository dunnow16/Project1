#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <net/if.h>  // network interfaces
/*
 * see ether_aton(3) man
 */
int main(int argc, char** argv) {
    // low level socket: can see hdr on incoming packets and set
    // them on outgoing packets
    int packet_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(packet_socket<0){
        perror("socket");
        return 1;
    }

    struct sockaddr_ll serveraddr, clientaddr;  // see man packet
    serveraddr.sll_family=AF_PACKET;
    // corresponds to type feild of ethernet header
    serveraddr.sll_protocol=htons(ETH_P_ALL);  // constant for any type to receive
    // identifier for the interface (usually know name for interface)
    serveraddr.sll_ifindex=if_nametoindex("h2-eth0"); // hard coded name on mininet

    int e = bind(packet_socket, (struct sockaddr*)&serveraddr,
        sizeof(serveraddr));
    if(e<0){
        perror("bind");
        return 2;
    }

    while(1){
        char buf[1514];
        int len = sizeof(clientaddr);
        // gives entire packet, not just data portion (can see hdrs)
        // see every packet, not just incoming: must filter if don't
        // want them
        int n = recvfrom(packet_socket, buf, 1514, 0, 
            (struct sockaddr*)&clientaddr, &len);
        // ignore all outgoing traffic
        if(clientaddr.sll_pkttype==PACKET_OUTGOING) {
            continue;
        }
        // hh - 1 byte, x - in hex
        // first part is part of the destination addr (see if matches in ifconfig: prints - HWaddr: ...)
        // f's is a broadcast packet.
        printf("Got a %d byte packet, first byte is %hhx\n",n,buf[0]);
    }

}
