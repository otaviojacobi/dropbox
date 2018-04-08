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

void string_tolower(char *p) {
    for ( ; *p; ++p) *p = tolower(*p);
}

void print_info(char *USER, char *version) {
    
    printf("Arthur Marques Medeiros - XXXXXX\n");
    printf("Flavia de Avila Pereira - XXXXXX\n");
    printf("Otavio Flores Jacobi - 261569\n");
    printf("Priscila Cavalli Rachevsky - XXXXXX\n");
    if(version != NULL)
        printf("Welcome to Dropboxx %s. Version %s\n", USER, version);
    else
        printf("Welcome to Dropboxx %s. Version 1.0.0\n", USER);
}

void kill(char *message) {
    printf("%s", message);
    printf("Exiting...\n");
    exit(-1);
}