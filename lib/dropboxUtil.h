#ifndef _DROPBOXUTIL_H_
#define _DROPBOXUTIL_H_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFLEN 512  
#define SERVER_DEFAULT "127.0.0.1"
#define DEFAULT_PORT 8888
#define true 1
#define false 0

//actions
enum possible_actions {
    Upload,
    Download,
    List_server,
    List_client,
    Get_sync_dir,
    Exit
} ACTION;

int init_socket_client(int PORT, char *SERVER, struct sockaddr_in *si_other);
int init_socket_server(int PORT, struct sockaddr_in *si_me);
int command_to_action(char *command);
void string_tolower(char *p);
void print_info(char *USER, char *version);
void kill(char *message);

#endif