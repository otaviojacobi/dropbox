#include "dropboxClient.h"

struct sockaddr_in si_other;
int socket_id;
unsigned int slen;
uint32_t next_id = 0;


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
                next_id = send_file(command_parameter, socket_id, &si_other, slen, get_id(), 's');
                break;

            case Download:
                scanf("%s", command_parameter);
                receive_client(command_parameter);
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


void receive_client(char *file_name) {
    char buf[PACKET_SIZE];
    Packet packet;
    Ack ack;
    
    create_packet(&packet, Download_type, get_id(), 0, file_name); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

    if((uint8_t)buf[0] == Ack_type) {
        memcpy(&ack, buf, sizeof(ack));
        printf("%d\n", ack.util);
    }

    receive_file(file_name, ack.util, 0, socket_id);
}

int login_server(char *host, int port) {

    char buf[PACKET_SIZE];
    Packet login_packet;
    Ack ack;
    
    create_packet(&login_packet, Client_login_type, get_id(), 0, host); //sdds construtor
    await_send_packet(&login_packet, &ack, buf, socket_id, &si_other, slen);

    if((uint8_t)buf[0] == Ack_type) {
        memcpy(&ack, buf, sizeof(ack));
        ack.util == New_user ? printf("This is your first time! We're creating your account...\n")
                             : printf("Loggin you in...\n");
    }
    else {
       kill("We failed to log you in. Try again later!\n");
    }

    socket_id = init_socket_client(ack.info, SERVER_DEFAULT, &si_other);
    //Maybe should return the packet id ?
    return 0;
}

uint32_t get_id() {
    return ++next_id;
}