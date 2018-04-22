#ifndef _DROPBOXCLIENT_H_
#define _DROPBOXCLIENT_H_


int login_server(char *host, int port);
void sync_client();
void send_file(char *file_name);
void get_file(char *file);
void delete_file(char *file);
void close_session();

void send_packet(Packet *packet);
void receive_packet(char *buffer);


#endif