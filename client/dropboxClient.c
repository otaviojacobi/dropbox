#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../lib/dropboxUtil.h"
#include "dropboxClient.h"

struct sockaddr_in si_other;
int socket_id;
unsigned int slen;
char *SERVER;

int main(int argc, char **argv) {
    
    char command[COMMAND_LENGTH];
    char command_parameter[COMMAND_LENGTH];
    char exit_message[COMMAND_LENGTH];

    char* USER = argv[1];
    SERVER = argc >= 3 ? argv[2] : SERVER_DEFAULT;
    int PORT = argc >= 4 ? atoi(argv[3]) : DEFAULT_PORT;
    int action;
    
    slen = sizeof(si_other);

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

    Packet login_packet;
    Ack ack;
    int packet_id = 0; //TODO: fix id generation
    
    socket_id = init_socket_client(port, SERVER, &si_other);

    create_packet(&login_packet, Client_login, packet_id, host); //sdds construtor
    send_packet(&login_packet);
    receive_ack(&ack);

    if(ack.packet_id == packet_id)
       ack.ack_type == New_user ? printf("This is your first time! We're creating your account...\n")
                                : printf("Loggin you in...\n");
    else
       kill("We failed to log you in. Try again later!\n");
    //Maybe should return the packet id ?
    return 0;
}

void send_file(char *file) {

    //TODO: change message to actual file (this must become a while and should use the packets)
    Packet packet;

    create_packet(&packet, Data, 1, file);

    if (sendto(socket_id, &packet, PACKAGE_SIZE , 0 , (struct sockaddr *) &si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to send data...\n");
    }
}

void send_packet(Packet *packet) {
    
    if (sendto(socket_id, packet, PACKAGE_SIZE , 0 , (struct sockaddr *) &si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to login...\n");
    }
}

void receive_ack(Ack *ack) {

    int recv_len;
    //try to receive the ack, this is a blocking call
    if ((recv_len = recvfrom(socket_id, ack, sizeof(Ack), 0, (struct sockaddr *) &si_other, &slen)) == -1)
        kill("Failed to receive ack...\n");
}

