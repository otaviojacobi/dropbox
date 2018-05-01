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

    char buf[PACKET_SIZE];
    Packet login_packet;
    Ack ack;
    
    create_packet(&login_packet, Client_login_type, get_id(), 0, host); //sdds construtor
    await_send_packet(&login_packet, &ack, buf);

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

void send_file(char *file_name) {    

    FILE *file = fopen(file_name, "rb");
    if(!file) {
        printf("Please insert a existing file (file_name.extension)\n");
        return;
    }
    Packet packet;
    Ack ack;
    char buf[PACKET_SIZE];
    char buf_data[DATA_PACKET_SIZE];
    uint32_t file_size = get_file_size(file);
    uint32_t file_pos = 0;
    uint32_t block_amount = ceil(file_size/sizeof(buf_data));

    //file header packet
    create_packet(&packet, Header_type, get_id(), file_size, file_name);
    await_send_packet(&packet, &ack, buf);
    
    //Send all file data in block_amount packets
    while (file_pos <= block_amount) {
        fread(buf_data, 1, DATA_PACKET_SIZE, file);
        create_packet(&packet, Data_type, get_id(), file_pos, buf_data);
        await_send_packet(&packet, &ack, buf);
        file_pos++;
    }
    if (ferror(file)) {
        kill("Error reading file\n");
    }
    fclose(file);
}

void await_send_packet(Packet *packet, Ack *ack, char *buf) {
    int recieve_status;
    int is_valid_ack = false;
    
    do {
        send_packet(packet);
        recieve_status = recvfrom(socket_id, buf, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen);
        if(recieve_status >= 0 && buf[0] == Ack_type) {
            memcpy(ack, buf, sizeof(Ack));
            is_valid_ack = match_ack_packet(ack, packet);
        }
    } while(!is_valid_ack);
}

void send_packet(Packet *packet) {
    char buf[PACKET_SIZE];
    memcpy(buf, packet, PACKET_SIZE);

    printf("Sending to %d\n", socket_id);
    
    if (sendto(socket_id, buf, PACKET_SIZE , 0 , (struct sockaddr *) &si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to send packet...\n");
    }
}

uint32_t get_id() {
    return ++next_id;
}