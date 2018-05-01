#include "dropboxServer.h"

uint32_t current_port = 8132;
struct sockaddr_in si_me, si_other;
unsigned int slen;
//char *clients[100000]; // nao seria melhor uma lista encadeada?
std::map<int, Client> clients;

int main(int argc, char **argv) {
    
    int recv_len;
    slen = sizeof(si_other);
    Packet packet;

    int PORT = argc >= 2 ? atoi(argv[1]) : DEFAULT_PORT; 
     
    //create a UDP socket
    int socket_id = init_socket_server(PORT, &si_me);

    //keep listening for data
    printf("Listening on port %d...\n", PORT);
    while(true) {

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)
            kill("Failed to receive data...\n");

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        
        switch(packet.packet_type) {
            case Client_login_type:
                receive_login_server(packet.data, packet.packet_id, socket_id);
                break;

            case Header_type:
                printf("Error: not supposed to be Header_type case on main\n");
                break;
            
            case Data_type:
                printf("Error: not supposed to be Data_type case on main\n");
                break;

            default: printf("The packet type is not supported! on main\n");
        }
    }
 
    close(socket_id);
    return 0;
}

void receive_file(char *file, uint32_t file_size, uint32_t packet_id, int socket_id) {

    char path_file[strlen(file) + strlen(clients[socket_id].user_name) + 1];
    get_full_path_file(path_file, file, socket_id);
    FILE *file_opened = fopen(path_file, "w+");
    
    char buf[PACKET_SIZE];
    char buf_data[DATA_PACKET_SIZE];

    uint32_t block_amount = ceil(file_size/sizeof(buf_data));
    
    Ack ack;
    Packet packet;
    unsigned int packets_received = 0;

    if (file_opened) {
        do {
            receive_packet(buf, socket_id);
            memcpy(&packet, buf, PACKET_SIZE);
            create_ack(&ack, packet.packet_id, packet.packet_info);
            if(packets_received != block_amount) {
                if(buf[0] == Data_type) {
                    fwrite(packet.data, 1, DATA_PACKET_SIZE, file_opened);
                    packets_received++;
                } else if(buf[0] == Header_type) {
                    create_ack(&ack, packet_id, file_size);
                } else kill("Unexpected packet type when receiving file");
            } else {
                fwrite(packet.data, 1, file_size-((block_amount) * sizeof(buf_data)), file_opened);
                packets_received++;
            }
            send_ack(&ack, socket_id);
        } while(packets_received <= block_amount);

        fclose(file_opened);

        if (ferror(file_opened)) {
            kill("Error writing file\n");
        }
    }
}

void send_ack(Ack *ack, int socket_id) {
    
    if (sendto(socket_id, ack, sizeof(Ack) , 0 , (struct sockaddr *) &si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to send ack...\n");
    }
}

void receive_login_server(char *host, int packet_id, int socket_id) {

    pthread_t logged_client;
    int status = check_login_status(host);
    int new_socket_id;
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

    uint32_t port = create_user_socket(&new_socket_id);
    bind_user_to_server( host, new_socket_id, port);
    pthread_create(&logged_client, NULL, handle_user, (void*) &new_socket_id);
    ack.info = port;

    send_ack(&ack, socket_id);
    printf("User %s Logged in.\n", host);
}

void* handle_user(void* args) {
    int *point_id = (int *) args;
    int socket_id = *point_id;
    int recv_len;
    Packet packet;
    Ack ack;
    struct sockaddr_in si_client;

    while(true) {

        printf("THREAD RODANDO ANTES %d\n", socket_id);

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_client, &slen)) == -1)
            kill("Failed to receive data...\n");

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_client.sin_addr), ntohs(si_client.sin_port));
        
        switch(packet.packet_type) {
            case Client_login_type:
                printf("Error: You have logged in already !\n");
                break;

            case Header_type:
                ack.packet_type = Ack_type;
                ack.packet_id = packet.packet_id;
                ack.util = packet.packet_info;
                send_ack(&ack, socket_id);
                receive_file(packet.data, packet.packet_info, packet.packet_id, socket_id);
                break;
            
            case Data_type:
                printf("Error: not supposed to be Data_type case\n");
                break;

            default: printf("The packet type is not supported!\n");
        }
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
    else { // opendir() failed for some other reason.PACKET_SIZE
        return -1;
    }
}

void create_new_user(char *host) {
    mkdir(host, 0700);
}

void bind_user_to_server(char *user_name, int socket_id, uint32_t port) {

    Client client;

    client.devices[0] = port;
    strcpy(client.user_name, user_name);
    clients.insert(std::pair<int, Client>(socket_id, client));
    printf("Binded %d to %s\n", socket_id, clients[socket_id].user_name);
}

int receive_packet(char *buffer, int socket_id) {
    int recv_len;

    //try to receive the ack, this is a blocking call
    if ((recv_len = recvfrom(socket_id, buffer, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        kill("Failed to receive ack...\n");

    return recv_len;
}

void get_full_path_file(char *buffer, char *file, int socket_id) {
    buffer[0] = '\0';
    strcat(buffer, clients[socket_id].user_name);
    strcat(buffer, "/");
    strcat(buffer, file);
}

uint32_t create_user_socket(int *id) {

    int new_socket_id;
    struct sockaddr_in s_in;

    if ( (new_socket_id=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
       kill("An error ocurred while creating the socket.\n");

    *id = new_socket_id;

    memset((char *) &s_in, 0, sizeof(s_in));
     
    //TODO: is the order important ?
    s_in.sin_family = AF_INET;
    s_in.sin_addr.s_addr = htonl(INADDR_ANY);

    do {
        current_port++;
        s_in.sin_port = htons(current_port);
    } while (bind(new_socket_id, (struct sockaddr *)&s_in, sizeof(struct sockaddr_in)) == -1);


    return current_port;
}
