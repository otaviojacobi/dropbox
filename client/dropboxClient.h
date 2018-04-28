#ifndef _DROPBOXCLIENT_H_
#define _DROPBOXCLIENT_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "../lib/dropboxUtil.h"


int login_server(char *host, int port);
void sync_client();
void send_file(char *file_name);
void get_file(char *file);
void delete_file(char *file);
void close_session();
void send_packet(Packet *packet);
void await_send_packet(Packet *packet, Ack *ack, char* buf);
uint32_t get_id();


#endif