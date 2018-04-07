#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include"../lib/dropboxUtil.h"

int main(int argc, char **argv)
{
    struct sockaddr_in si_other;
    char buf[BUFLEN];
    char message[BUFLEN];
    int slen=sizeof(si_other);
    
    char* USER = argv[1];
    char* SERVER = argc >= 3 ? argv[2] : SERVER_DEFAULT;
    int PORT = argc >= 4 ? atoi(argv[3]) : DEFAULT_PORT;
    
    int socket_id = init_socket_client(PORT, SERVER, &si_other);
 
    while(true) {

        printf("Send message : ");
        scanf("%s", message);

        if (sendto(socket_id, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
            kill("Failed to send data...\n");
         
        //clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', BUFLEN);
    }
 
    close(socket_id);
    return 0;
}