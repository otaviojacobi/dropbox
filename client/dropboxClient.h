#ifndef _DROPBOXCLIENT_H_
#define _DROPBOXCLIENT_H_

#define MAXNAME 100
#define MAXFILES 100

struct	file_info	{
    char name[MAXNAME];
    char extension[MAXNAME];
    char last_modified[MAXNAME];
    int size;
};

typedef struct	client	{
    int devices[2];
    char userid[MAXNAME];
    struct	file_info f_info[MAXFILES];
    int logged_in;
} Client;

int login_server(char *host, int port);
void sync_client();
void send_file(char *file);
void get_file(char *file);
void delete_file(char *file);
void close_session();

void send_packet(Packet *packet);
void receive_packet(char *buffer);


#endif