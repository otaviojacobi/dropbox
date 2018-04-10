#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../lib/dropboxUtil.h"
#include "dropboxServer.h"
 
int socket_id;
struct sockaddr_in si_me, si_other;
unsigned int slen;

int main(int argc, char **argv) {
    
    int recv_len;
    slen = sizeof(si_other);
    Packet packet;

    int PORT = argc >= 2 ? atoi(argv[1]) : DEFAULT_PORT; 
     
    //create a UDP socket
    socket_id = init_socket_server(PORT, &si_me);

    //keep listening for data
    printf("Listening on port %d...\n", PORT);
    while(true) {

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id, &packet, PACKAGE_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)
            kill("Failed to receive data...\n");

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        
        switch(packet.packet_type) {
            case Client_login:
                receive_login_server(packet.data, packet.packet_id);
                break;
            case Data:
                receive_file(packet.data);
                break;
            default: printf("The packet type is not supported!\n");
        }

	    clear_packet(&packet);
    }
 
    close(socket_id);
    return 0;
}

//TODO: send the real file.
void receive_file(char *file) {
    printf("%s\n", file);
}

void receive_login_server(char *host, int packet_id) {

    int status = check_login_status(host);
    Ack ack;

    switch(status) {
        case 0:
            printf("Logged in !\n");
            ack.ack_type = Old_user;
            break;
        case 1:
            printf("Creating new user for %s\n", host);
            create_new_user(host);
            ack.ack_type = New_user;
            break;

        default: printf("Failed to login.\n");
    }


    if (sendto(socket_id, &ack, sizeof(Ack) , 0 , (struct sockaddr *) &si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to send ack...\n");
    }
}

int check_login_status(char *host) {

    DIR* dir = opendir(host);

    if (dir) { // Directory exists.
        closedir(dir);
        return 0;
    }
    if (ENOENT == errno) {  // Directory does not exist. 
        return 1;
    }
    else { // opendir() failed for some other reason.
        return -1;
    }
}

void create_new_user(char *host) {
    mkdir(host, 0700);
}