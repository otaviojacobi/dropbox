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
    int packet_id = get_id();
    
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

    //Send the header packet
    Packet packet;
    Ack ack;
    FILE *file = fopen(file_name, "rb");
    uint32_t id = get_id();
    uint32_t file_size = get_file_size(file);
    int recieve_status;
    int isValidAck = false;
    char buf[PACKAGE_SIZE];

    create_packet(&packet, Header_type, id, file_size, file_name);
    do {
        send_packet(&packet);
        recieve_status = recvfrom(socket_id, buf, sizeof(buf), 0, (struct sockaddr *) &si_other, &slen);

        if(recieve_status >= 0 && (uint8_t) buf[0] == Ack_type) {
            memcpy(&ack, buf, sizeof(Ack));
            isValidAck = match_ack_packet(&ack,&packet);
        }
    } while(!isValidAck);


    //Send all data in block_amount packets
    char bufData[DATA_PACKAGE_SIZE];
    uint32_t file_pos = 0;
    uint32_t block_amount = ceil(file_size/sizeof(bufData));

    if (file) {
        while (file_pos <= block_amount) {
            fread(bufData, 1, DATA_PACKAGE_SIZE, file);
            isValidAck = false;
            do {
                create_packet(&packet, Data_type, id, file_pos, bufData);
                send_packet(&packet);


                recieve_status= recvfrom(socket_id, buf, sizeof(buf), 0, (struct sockaddr *) &si_other, &slen);
                if(recieve_status >= 0 && (uint8_t) buf[0] == Ack_type) {
                    memcpy(&ack, buf, sizeof(Ack));
                    isValidAck = match_ack_packet(&ack,&packet);
                }
            }while(!isValidAck);
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

