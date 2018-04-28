#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../lib/dropboxUtil.h"
#include "dropboxServer.h"

int socket_id;
struct sockaddr_in si_me, si_other;
unsigned int slen;
char *clients[100000]; // nao seria melhor uma lista encadeada?

int main(int argc, char **argv) {
    
    int recv_len;
    slen = sizeof(si_other);
    Packet packet;

    int PORT = argc >= 2 ? atoi(argv[1]) : DEFAULT_PORT; 
     
    //create a UDP socket
    socket_id = init_socket_server(PORT, &si_me);
    Ack ack;

    //keep listening for data
    printf("Listening on port %d...\n", PORT);
    while(true) {

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id, &packet, PACKAGE_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)
            kill("Failed to receive data...\n");

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        
        switch(packet.packet_type) {
            case Client_login_type:
                bind_user_to_server(packet.data);
                receive_login_server(packet.data, packet.packet_id);
                break;
            case Header_type:
                ack.packet_type = Ack_type;
                ack.packet_id = packet.packet_id;
                ack.util = packet.packet_info;
                send_ack(&ack);
                receive_file(packet.data, packet.packet_info, packet.packet_id);
                break;
            
            case Data_type:
                printf("Error: not supposed to be Data_type case\n");
                break;
            default: printf("The packet type is not supported!\n");
        }

    }
 
    close(socket_id);
    return 0;
}

//TODO: receive the real file.
void receive_file(char *file, uint32_t file_size, uint32_t id) {

    char *path_file = (char *) malloc(strlen(file) + strlen(clients[htons(si_other.sin_port)]) + 1);
    path_file[0] = '\0';
    strcat(path_file, clients[htons(si_other.sin_port)]);
    strcat(path_file, "/");
    strcat(path_file, file);
    
    FILE *file_opened = fopen(path_file, "w+");
    
    char buf[PACKAGE_SIZE];
    char bufData[DATA_PACKAGE_SIZE];

    uint32_t block_amount = ceil(file_size/sizeof(bufData));
    
    Ack ack;
    Packet packet;
    int packets_received = 0;
    
    if (file_opened) {
        do {
            receive_packet(buf);

            if(packets_received != block_amount) {
                ack.packet_type = Ack_type;
                if((uint8_t)buf[0] == Data_type) {
                    memcpy(&packet, buf, PACKAGE_SIZE);
                    fwrite(packet.data,1 , DATA_PACKAGE_SIZE, file_opened);

                    ack.packet_id = packet.packet_id;
                    ack.util = packet.packet_info;
                    send_ack(&ack);
                    packets_received++;
                } else if((uint8_t)buf[0] == Header_type) {
                    ack.packet_id = id;
                    ack.util = file_size;
                    send_ack(&ack);
                }
            } 
            //last block
            else {
                memcpy(&packet, buf, PACKAGE_SIZE);
                fwrite(packet.data, 1, file_size-((block_amount) * sizeof(bufData)), file_opened);

                ack.packet_id = packet.packet_id;
                ack.util = packet.packet_info;
                send_ack(&ack);
                packets_received++;
            }
        } while(packets_received <= block_amount);

        fclose(file_opened);

        if (ferror(file_opened)) {
            kill("Error writing file\n");
        }
    }
}

void send_ack(Ack *ack) {
    
    if (sendto(socket_id, ack, sizeof(Ack) , 0 , (struct sockaddr *) &si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to send ack...\n");
    }
}

void receive_login_server(char *host, int packet_id) {

    int status = check_login_status(host);
    Ack ack;
    ack.packet_type = Ack_type;
    ack.packet_id = packet_id;

    switch(status) {
        case Old_user:
            printf("Logged in !\n");
            ack.util = Old_user;
            break;
        case New_user:
            printf("Creating new user for %s\n", host);
            ack.util = New_user;            
            create_new_user(host);
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
        return Old_user;
    }
    if (ENOENT == errno) {  // Directory does not exist. 
        return New_user;
    }
    else { // opendir() failed for some other reason.PACKAGE_SIZE
        return -1;
    }
}

void create_new_user(char *host) {
    mkdir(host, 0700);
}

void bind_user_to_server(char *user_name) {

    printf("%d\n", ntohs(si_other.sin_port));
    clients[ntohs(si_other.sin_port)] =  strdup(user_name);
    printf("Binded %d to %s\n", ntohs(si_other.sin_port), user_name);
}

int receive_packet(char *buffer) {
    int recv_len;

    //try to receive the ack, this is a blocking call
    if ((recv_len = recvfrom(socket_id, buffer, PACKAGE_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        kill("Failed to receive ack...\n");

    return recv_len;
    
}
