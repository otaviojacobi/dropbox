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

int main(int argc, char **argv) {
    
    char message[BUFLEN];

    char command[BUFLEN];
    char command_parameter[BUFLEN];
    char exit_message[64];

    char* USER = argv[1];
    char* SERVER = argc >= 3 ? argv[2] : SERVER_DEFAULT;
    int PORT = argc >= 4 ? atoi(argv[3]) : DEFAULT_PORT;
    
    socket_id = init_socket_client(PORT, SERVER, &si_other);
    
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
                kill("Not yet implemented\n");
                break;

            case List_server:
                close(socket_id);            
                kill("Not yet implemented\n");            
                break;

            case List_client:
                close(socket_id);            
                kill("Not yet implemented\n");            
                break;

            case Get_sync_dir:
                close(socket_id);            
                kill("Not yet implemented\n");            
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

void send_file(char *file) {

    //TODO: change message to actual file (this must become a while)
    if (sendto(socket_id, file, strlen(file) , 0 , (struct sockaddr *) &si_other, slen) == -1) {
        close(socket_id);        
        kill("Failed to send data...\n");
    }

    memset(buf,'\0', BUFLEN);
}