#ifndef _DROPBOXSERVER_H_
#define _DROPBOXSERVER_H_

void receive_file(char *file);

void receive_login_server(char *host, int packet_id);
int check_login_status(char *host);
void create_new_user(char *host);

#endif