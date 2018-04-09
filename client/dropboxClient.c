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
int slen = sizeof(si_other);
char buf[BUFLEN];
char *SERVER;

int main(int argc, char **argv) {
    
    char command[BUFLEN];
    char command_parameter[BUFLEN];
    char exit_message[64];

    char* USER = argv[1];
    SERVER = argc >= 3 ? argv[2] : SERVER_DEFAULT;
    int PORT = argc >= 4 ? atoi(argv[3]) : DEFAULT_PORT;
    
    if(login_server(USER, PORT) == -1)
        kill("Failed to conect to server");    
    print_info(USER, "1.0.0");

    int action;

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
                close(socket_id);
                printf("Not yet implemented\n");
                break;

            case List_server:
                close(socket_id);            
                printf("Not yet implemented\n");            
                break;

            case List_client:
                close(socket_id);            
                printf("Not yet implemented\n");            
                break;

            case Get_sync_dir:
                close(socket_id);            
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
    int packet_id = 0; //TODO: fix id generation
    //int recv_len;
    
    socket_id = init_socket_client(port, SERVER, &si_other);

    login_packet.packet_type = Client_login;
    login_packet.packet_id = packet_id; //TODO: fix id generation
    strcpy(login_packet.data, host); 

    if (sendto(socket_id, &login_packet, PACKAGE_SIZE , 0 , (struct sockaddr *) &si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to login...\n");
    }

    //clear_packet(&login_packet);


    // TODO: for some reason this doesn't work. I think the best way to move foward is to serialize or packet structure.
    //try to receive the ack, this is a blocking call
    // if ((recv_len = recvfrom(socket_id, &login_packet, PACKAGE_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)
    //     kill("Failed to receive ack...\n");

    // if(check_ack(&login_packet, packet_id) == false )
    //     kill("Some packet got lost in its way...\n");

    // if(login_packet.data[0] == 'n')
    //     printf("New user created !\n");
    
    return 0;
}

void send_file(char *file) {

    //TODO: change message to actual file (this must become a while and should use the packets)

    Packet packet;

    packet.packet_type = Data;
    packet.packet_id = 1;
    strcpy(packet.data, file);

    if (sendto(socket_id, &packet, PACKAGE_SIZE , 0 , (struct sockaddr *) &si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to send data...\n");
    }

}