#ifndef _DROPBOXSERVER_H_
#define _DROPBOXSERVER_H_

#include "../lib/dropboxUtil.h"


void receive_login_server(char *host, int packet_id, int socket_id, struct sockaddr_in *si_other, unsigned int slen );
int check_login_status(char *host);
void create_new_user(char *host);
void bind_user_to_server(char *user_name, int socket_id, uint32_t port );
void get_full_path_file(char *buffer, char *file, int socket_id);
void *handle_user(void *args);
uint32_t create_user_socket(int *id);
char *file_info_serialize(struct file_info info, char *serialized_info, int pos);
int get_file_amount(int socket_id);
int send_current_files(int socket_id, struct sockaddr_in *si_other, unsigned int slen, int packet_id);
int check_if_online(char *host);
#endif