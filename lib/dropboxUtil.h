#ifndef _DROPBOXUTIL_H_
#define _DROPBOXUTIL_H_


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#define BUFLEN 512  
#define SERVER_DEFAULT "127.0.0.1"
#define DEFAULT_PORT 8888
#define true 1
#define false 0

int init_socket_client(int PORT, char* SERVER, struct sockaddr_in *si_other);
int init_socket_server(int PORT, struct sockaddr_in *si_me);
void kill(char *message);

#endif