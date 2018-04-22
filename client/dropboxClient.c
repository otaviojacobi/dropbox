#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "../lib/dropboxUtil.h"
#include "dropboxClient.h"

struct sockaddr_in si_other;
int socket_id;
unsigned int slen;
uint32_t next_id = 0;

uint32_t get_id() {
    next_id++;
    return next_id;
}

int main(int argc, char **argv) {
    
    char command[COMMAND_LENGTH];
    char command_parameter[COMMAND_LENGTH];
    char exit_message[COMMAND_LENGTH];

    char *USER = argv[1];
    char *SERVER = argc >= 3 ? argv[2] : SERVER_DEFAULT;
    int PORT = argc >= 4 ? atoi(argv[3]) : DEFAULT_PORT;
    int action;
    
    slen = sizeof(si_other);

    socket_id = init_socket_client(PORT, SERVER, &si_other);
    login_server(USER, PORT);
    print_info(USER, "1.0.0");

    while(true) {

        printf(">> ");
        scanf("%s", command);

        action = command_to_action(command);

        switch(action) {

            case Upload:
                scanf("%s", command_parameter); // files names are not allowed to have spaces. TODO: fix this
                send_file(command_parameter);
                break;

            case Download:
                printf("Not yet implemented\n");
                break;

            case List_server:            
                printf("Not yet implemented\n");            
                break;

            case List_client:            
                printf("Not yet implemented\n");            
                break;

            case Get_sync_dir:            
                printf("Not yet implemented\n");            
                break;

            case Exit:
                close(socket_id);
                sprintf(exit_message, "Goodbye %s!\n", USER);
                kill(exit_message);
                break;

            default: printf("%s command not found.\n", command);
        }
    }
 
    close(socket_id);
    return 0;
}

int login_server(char *host, int port) {

    char buffer[PACKAGE_SIZE];
    Packet login_packet;
    Ack ack;
    int packet_id = get_id(); //TODO: fix id generation
    
    create_packet(&login_packet, Client_login_type, packet_id, 0, host); //sdds construtor
    send_packet(&login_packet);
    receive_packet(buffer); //TODO : Timelimit

    if((uint8_t)buffer[0] == Ack_type) {
        memcpy(&ack, buffer, sizeof(ack));
        ack.util == New_user ? printf("This is your first time! We're creating your account...\n")
                             : printf("Loggin you in...\n");
    }
    else {
       kill("We failed to log you in. Try again later!\n");
    }
    //Maybe should return the packet id ?
    return 0;
}

void send_file(char *file_name) {

    FILE *file = fopen(file_name, "rb");
    char buf[DATA_PACKAGE_SIZE];
    char buf_received[PACKAGE_SIZE];
    uint32_t file_pos = 0;
    Packet packet;
    Ack ack;
    int recieve_status;
    int isValidAck = false;
    uint32_t file_size = get_file_size(file);

    create_packet(&packet, Header_type, get_id(), file_size, file_name);
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = ACK_TIME_OUT;
    if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        kill("Error setting socket timeout\n");
    }
    do {
        send_packet(&packet);
        recieve_status= recvfrom(socket_id, buf_received, sizeof(buf_received), 0, (struct sockaddr *) &si_other, &slen);

        if(recieve_status >= 0 && (uint8_t) buf_received[0] == Ack_type) {
            memcpy(&ack, buf_received, sizeof(Ack));
            isValidAck = ((ack.packet_id == packet.packet_id) && (ack.util == packet.packet_info));
        }

    }while(!isValidAck);    
    if (file) {
        while (fread(buf, 1, DATA_PACKAGE_SIZE, file) == DATA_PACKAGE_SIZE) {
            isValidAck = false;
            do {
                create_packet(&packet, Data_type, get_id(), file_pos, buf);
                //UM_BOM_PRINT(buf);
                UM_BOM_PRINT(packet.data);
                send_packet(&packet);
                kill("PORQUE\n");


                recieve_status= recvfrom(socket_id, buf_received, sizeof(buf_received), 0, (struct sockaddr *) &si_other, &slen);
                if(recieve_status >= 0 && (uint8_t) buf_received[0] == Ack_type) {
                    memcpy(&ack, buf_received, sizeof(Ack));
                    isValidAck = ((ack.packet_id == packet.packet_id) && (ack.util == packet.packet_info));
                }
            } while(!isValidAck);
            file_pos++;
        }
        if (ferror(file)) {
            kill("Error reading file\n");
        }
        fclose(file);
    }
}

void send_packet(Packet *packet) {

    char buf[PACKAGE_SIZE];

    memcpy(buf, packet, PACKAGE_SIZE);
    
    if (sendto(socket_id, buf, PACKAGE_SIZE , 0 , (struct sockaddr *) &si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to login...\n");
    }

}

void receive_packet(char *buffer) {
    int recv_len;
    //try to receive the ack, this is a blocking call

    if ((recv_len = recvfrom(socket_id, buffer, PACKAGE_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        kill("Failed to receive ack...\n");
}

