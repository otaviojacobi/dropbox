#include "dropboxClient.h"

struct sockaddr_in si_other;
int socket_id;
unsigned int slen;
uint32_t next_id = 0;
char *USER;

int main(int argc, char **argv) {
    
    char command[COMMAND_LENGTH];
    char command_parameter[COMMAND_LENGTH];

    USER = argv[1];
    char *SERVER = argc >= 3 ? argv[2] : SERVER_DEFAULT;
    int PORT = argc >= 4 ? atoi(argv[3]) : DEFAULT_PORT;
    int action;
    char full_path[MAXNAME+10];
    
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
                scanf("%s", command_parameter);
                get_file(command_parameter);
                break;

            case List_server:            
                list_server();            
                break;

            case List_client:            
                printf("Not yet implemented\n");            
                break;

            case Get_sync_dir:            
                printf("Not yet implemented\n");            
                break;

            case Exit:
                close_session();
                break;

            default: printf("%s command not found.\n", command);
        }
    }
 
    close(socket_id);
    return 0;
}

void list_server(void) {
    char buf[PACKET_SIZE];
    Packet packet;
    Ack ack;
    int packets_received = 0;
    int packets_to_receive;


    create_packet(&packet, List_type, get_id(), 0, ""); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

    packets_to_receive = ack.util;

    if(packets_to_receive == 0) {
        printf("Your remote sync_dir is empty, upload your first file !\n");
        return;
    }

    do {
        receive_packet(buf, socket_id, &si_other, &slen);
        memcpy(&packet, buf, PACKET_SIZE);
        create_ack(&ack, packet.packet_id, packet.packet_info);
        if(buf[0] == Data_type) {
            printf("%s", packet.data);
            packets_received++;
        }
        send_ack(&ack, socket_id, &si_other, slen);
    } while(packets_received < packets_to_receive);

}

int login_server(char *host, int port) {

    char buf[PACKET_SIZE];
    Packet login_packet;
    Ack ack;
    char full_path[MAXNAME+10];

    strcpy(full_path, "sync_dir_");
    strcat(full_path, host);
    DIR* dir = opendir(full_path);

    if (dir)
        closedir(dir);
    else // Directory does not exist. 
        mkdir(full_path, 0700);

    
    create_packet(&login_packet, Client_login_type, get_id(), 0, host); //sdds construtor
    await_send_packet(&login_packet, &ack, buf, socket_id, &si_other, slen);

    if((uint8_t)buf[0] == Ack_type) {
        memcpy(&ack, buf, sizeof(ack));
        if ( ack.util == New_user ) {
            printf("This is your first time! We're creating your account...\n");

        } else {
            printf("Loggin you in...\n");
        }
    }
    else {
       kill("We failed to log you in. Try again later!\n");
    }

    socket_id = init_socket_client(ack.info, SERVER_DEFAULT, &si_other);
    //Maybe should return the packet id ?
    return 0;
}

void send_file(char *file) {
    next_id = send_file_chunks(file, socket_id, &si_other, slen, get_id(), 's');
}

void get_file(char *file) {
    char buf[PACKET_SIZE];
    char full_path[MAXNAME+10];
    Packet packet;
    Ack ack;
    
    create_packet(&packet, Download_type, get_id(), 0, file); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

    if((uint8_t)buf[0] == Ack_type) {
        memcpy(&ack, buf, sizeof(ack));
    }
    
    get_sync_path(full_path, USER, file);

    if(ack.util >= 0)
        receive_file(full_path, ack.util, 0, socket_id);
    else
        printf("This file does not exists!\n");
}

void close_session() {
    char exit_message[COMMAND_LENGTH];
    
    close(socket_id);
    sprintf(exit_message, "Goodbye %s!\n", USER);
    kill(exit_message);
}

uint32_t get_id() {
    return ++next_id;
}