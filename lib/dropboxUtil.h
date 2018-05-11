#pragma once
#ifndef _DROPBOXUTIL_H_
#define _DROPBOXUTIL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <map>

#define PACKET_SIZE 1024
#define PACKET_HEADER_SIZE 12 //lowest value to have sizeof(struct packet) = 1024 bytes
#define DATA_PACKET_SIZE 1024-12 //PACKET_SIZE-PACKET_HEADER_SIZE

#define MAXNAME 100
#define MAXFILES 100
#define ACK_TIME_OUT 200000

#define COMMAND_LENGTH 64
#define DEFAULT_PORT 8888
#define true 1
#define false 0

static char *SERVER_DEFAULT = "127.0.0.1";
struct	file_info	{
    char name[MAXNAME];
    char extension[MAXNAME];
    char last_modified[MAXNAME];
    int size;
};

typedef struct	client	{
    int devices[2];
    char user_name[MAXNAME];
    int logged_in;
} Client;

//actions
enum possible_actions {
    Upload,
    Download,
    List_server,
    List_client,
    Get_sync_dir,
    Exit
};

//packet types
enum packet_types {
    Client_login_type,
    Download_type,
    Data_type,
    Header_type,
    Ack_type
};

enum login_types {
    Old_user,
    New_user
};

typedef struct packet {
    uint8_t packet_type;
    uint32_t packet_id;
    uint32_t packet_info; //For header means size and for data means file order
    char data[DATA_PACKET_SIZE];
} Packet;


typedef struct ack {
    uint8_t packet_type;
    int32_t util;
    uint32_t packet_id;
    uint32_t info;
} Ack;

int init_socket_client(int PORT, char *SERVER, struct sockaddr_in *si_other);
int init_socket_server(int PORT, struct sockaddr_in *si_me);
int command_to_action(char *command);
void create_packet(Packet *packet, uint8_t type, uint32_t id, uint32_t info, char *data);
void create_ack(Ack *ack, uint32_t id, uint32_t util);
void clear_packet(Packet *packet);
void string_tolower(char *p);
void print_info(char *USER, char *version);
void kill(char *message);
long get_file_size(FILE *file);
void set_socket_timeout(int socket_id);
int match_ack_packet(Ack *ack, Packet *packet);
void send_ack(Ack *ack, int socket_id, struct sockaddr_in *si_other, unsigned int slen);
int receive_packet(char *buffer, int socket_id, struct sockaddr_in *si_other, unsigned int *slen);
void receive_file(char *file, uint32_t file_size, uint32_t packet_id, int socket_id);
void send_packet(Packet *packet, int socket_id, struct sockaddr_in *si_other, unsigned int slen);
void await_send_packet(Packet *packet, Ack *ack, char *buf, int socket_id, struct sockaddr_in *si_other, unsigned int slen);
int send_file(char *file_name, int socket_id, struct sockaddr_in *si_other, unsigned int slen, int packet_id, char destination);

#endif