#include "dropboxUtil.h"

int init_socket_client(int PORT, char* SERVER, struct sockaddr_in *si_other) {

    int socket_id;

    if ( (socket_id=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
       kill("An error ocurred while creating the socket.\n");

 
    memset((char *) si_other, 0, sizeof(*si_other));
    si_other->sin_family = AF_INET;
    si_other->sin_port = htons(PORT);
     
    if (inet_aton(SERVER , &si_other->sin_addr) == 0) 
        kill("An error ocurred while connecting to the server.\n");
    
    set_socket_timeout(socket_id);
    return socket_id;
}

int init_socket_server(int PORT, struct sockaddr_in *si_me) {

    int socket_id;

    if ((socket_id=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        kill("An error ocurred while creating the socket.\n");
     
    // zero out the structure
    memset((char *) si_me, 0, sizeof(*si_me));
     
    si_me->sin_family = AF_INET;
    si_me->sin_port = htons(PORT);
    si_me->sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(socket_id , (struct sockaddr*) si_me, sizeof(*si_me) ) == -1)
        kill("An error ocurred binding the socket to the port. Make sure nothing else is listening on the same port.\n");
    
    return socket_id;
}

int command_to_action(char *command) {

    string_tolower(command);

    if(strcmp(command, "upload") == 0) {
        return Upload;
    }
    if(strcmp(command, "download") == 0) {
        return Download;
    }
    if(strcmp(command, "list_server") == 0) {
        return List_server;
    }
    if(strcmp(command, "list_client") == 0) {
        return List_client;
    }
    if(strcmp(command, "get_sync_dir") == 0) {
        return Get_sync_dir;
    }
    if(strcmp(command, "exit") == 0) {
        return Exit;
    }
    return -1;
}

void clear_packet(Packet *packet) {
    packet->packet_type = 0;
    packet->packet_id = 0;
    memset(packet->data,'\0', DATA_PACKET_SIZE); //TODO: get rid of magic number 40 which stands for 32 + 8 packet size structure
}

void string_tolower(char *p) {
    for ( ; *p; ++p) *p = tolower(*p);
}

void print_info(char *USER, char *version) {
    
    printf("Arthur Marques Medeiros - XXXXXX\n");
    printf("Flavia de Avila Pereira - 136866\n");
    printf("Otavio Flores Jacobi - 261569\n");
    printf("Priscila Cavalli Rachevsky - 261573\n");
    if(version != NULL)
        printf("Welcome to Dropboxx %s. Version %s\n", USER, version);
    else
        printf("Welcome to Dropboxx %s. Version 1.0.0\n", USER);
}

void create_packet(Packet *packet, uint8_t type, uint32_t id, uint32_t info, char *data) {
    packet->packet_type = type;
    packet->packet_id = id; 
    packet->packet_info = info;
    memcpy(packet->data, data,  DATA_PACKET_SIZE);
}

void create_ack(Ack *ack, uint32_t id, uint32_t util) {
    ack->packet_type = Ack_type;
    ack->packet_id = id;
    ack->util = util;
}

void kill(char *message) {
    printf("%s", message);
    printf("Exiting...\n");
    exit(-1);
}

long get_file_size(FILE *file) {
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    return file_size;
}

void set_socket_timeout(int socket_id) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = ACK_TIME_OUT;
    if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        kill("Error setting socket timeout\n");
    }
}

int match_ack_packet(Ack *ack, Packet *packet) {
    return packet->packet_type == Client_login_type || packet->packet_type == Download_type ?
           (ack->packet_id == packet->packet_id) :
           ((ack->packet_id == packet->packet_id) && (ack->util == packet->packet_info));
}

void receive_file(char *path_file, uint32_t file_size, uint32_t packet_id, int socket_id) {


    FILE *file_opened = fopen(path_file, "w+");
    
    char buf[PACKET_SIZE];
    char buf_data[DATA_PACKET_SIZE];

    uint32_t block_amount = ceil(file_size/sizeof(buf_data));
    
    Ack ack;
    Packet packet;
    unsigned int packets_received = 0;

    struct sockaddr_in si_other;
    unsigned int slen = sizeof(si_other);

    if (file_opened) {
        do {
            receive_packet(buf, socket_id, &si_other, &slen);
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
            send_ack(&ack, socket_id, &si_other, slen);
        } while(packets_received <= block_amount);

        fclose(file_opened);

        if (ferror(file_opened)) {
            kill("Error writing file\n");
        }
    }
}

void send_ack(Ack *ack, int socket_id, struct sockaddr_in *si_other, unsigned int slen) {


    if (sendto(socket_id, ack, sizeof(Ack) , 0 , (struct sockaddr *) si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to send ack...\n");
    }
}

int receive_packet(char *buffer, int socket_id, struct sockaddr_in *si_other, unsigned int *slen) {
    int recv_len;

    //try to receive the ack, this is a blocking call
    if ((recv_len = recvfrom(socket_id, buffer, PACKET_SIZE, 0, (struct sockaddr *) si_other, slen)) == -1)
        kill("Failed to receive ack...\n");

    return recv_len;
}

int send_file(char *file_name, int socket_id, struct sockaddr_in *si_other, unsigned int slen, int packet_id, char destination) {    

    FILE *file = fopen(file_name, "rb");
    if(!file) {
        printf("Please insert a existing file (file_name.extension)\n");
        return socket_id;
    }
    Packet packet;
    Ack ack;
    char buf[PACKET_SIZE];
    char buf_data[DATA_PACKET_SIZE];
    uint32_t file_size = get_file_size(file);
    uint32_t file_pos = 0;
    uint32_t block_amount = ceil(file_size/sizeof(buf_data));

    if(destination == 's') {
        //file header packet
        format_file_name(file_name);
        create_packet(&packet, Header_type, packet_id, file_size, file_name);
        await_send_packet(&packet, &ack, buf, socket_id, si_other, slen);
    }
    //Send all file data in block_amount packets
    while (file_pos <= block_amount) {
        packet_id++;
        fread(buf_data, 1, DATA_PACKET_SIZE, file);
        create_packet(&packet, Data_type, packet_id, file_pos, buf_data);
        await_send_packet(&packet, &ack, buf, socket_id, si_other, slen);
        file_pos++;
    }
    if (ferror(file)) {
        kill("Error reading file\n");
    }
    fclose(file);

    return packet_id;
}

void await_send_packet(Packet *packet, Ack *ack, char *buf, int socket_id, struct sockaddr_in *si_other, unsigned int slen) {
    int recieve_status;
    int is_valid_ack = false;
    
    do {
        send_packet(packet, socket_id, si_other, slen);
        recieve_status = recvfrom(socket_id, buf, PACKET_SIZE, 0, (struct sockaddr *) si_other, &slen);
        if(recieve_status >= 0 && buf[0] == Ack_type) {
            memcpy(ack, buf, sizeof(Ack));
            is_valid_ack = match_ack_packet(ack, packet);
        }
    } while(!is_valid_ack);
}

void send_packet(Packet *packet, int socket_id, struct sockaddr_in *si_other, unsigned int slen) {
    char buf[PACKET_SIZE];
    memcpy(buf, packet, PACKET_SIZE);

    if (sendto(socket_id, buf, PACKET_SIZE , 0 , (struct sockaddr *) si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to send packet...\n");
    }
}
void format_file_name(char *file_name) {
    int i;
    for(i = 0; i < strlen(file_name); i++)
        if ( file_name[i] == '/' ) break;
    strcpy(file_name, &file_name[i+1]);
}

void get_sync_path(char *full_path, char *USER, char *file_name) {
    strcpy(full_path, "sync_dir_");
    strcat(full_path, USER);
    strcat(full_path, "/");
    strcat(full_path, file_name);
}