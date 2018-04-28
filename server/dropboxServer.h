#ifndef _DROPBOXSERVER_H_
#define _DROPBOXSERVER_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

void receive_file(char *file, uint32_t file_size, uint32_t id);

void receive_login_server(char *host, int packet_id);
int check_login_status(char *host);
void create_new_user(char *host);
void bind_user_to_server(char *user_name);
int receive_packet(char *buffer);
void get_full_path_file(char *buffer, char *file);
void send_ack(Ack *ack);

#endif