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
    memset(packet->data,'\0', DATA_PACKAGE_SIZE); //TODO: get rid of magic number 40 which stands for 32 + 8 packet size structure
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

    memcpy(packet->data, data,  DATA_PACKAGE_SIZE);
    
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

void UM_BOM_PRINT(char *UMA_BOA_STRING) {
    //printf("(len = %d)%s_\n\n", strlen(UMA_BOA_STRING), UMA_BOA_STRING);
    int debug;
    for(debug = 0; debug < DATA_PACKAGE_SIZE; debug++) printf("%c", UMA_BOA_STRING[debug]);
    printf("_\n");

}