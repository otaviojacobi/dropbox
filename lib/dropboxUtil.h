#ifndef _DROPBOXUTIL_H_
#define _DROPBOXUTIL_H_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PACKAGE_SIZE 1024
#define COMMAND_LENGTH 64
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

//packet types
enum packet_types {
    Client_login,
    Data,
} PACKET_TYPE;

enum ack_types {
    New_user,
    Old_user
} ACK_TYPE;

typedef struct packet {
    uint8_t packet_type;
    uint32_t packet_id; // TODO: How we generate this guy concurrently ?
    char data[PACKAGE_SIZE - 40];
} Packet; //8bytes on type + 32bytes on id + 984bytes on the actual data = 1kb packet each time


typedef struct ack {
    unsigned int ack_type;
    uint32_t packet_id;
} Ack;

int init_socket_client(int PORT, char *SERVER, struct sockaddr_in *si_other);
int init_socket_server(int PORT, struct sockaddr_in *si_me);
int command_to_action(char *command);
void create_packet(Packet *packet, uint8_t type, uint32_t id, char *data);
void clear_packet(Packet *packet);
void string_tolower(char *p);
void print_info(char *USER, char *version);
void kill(char *message);

#endif