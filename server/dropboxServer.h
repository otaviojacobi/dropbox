#ifndef _DROPBOXSERVER_H_
#define _DROPBOXSERVER_H_

#include "../lib/dropboxUtil.h"


void receive_login_server(Packet packet, int socket_id, struct sockaddr_in *si_other, unsigned int slen );
int check_login_status(char *host);
void create_new_user(char *host);
void bind_user_to_server(char *user_name, int socket_id, uint32_t port );
void get_full_path_file(char *buffer, char *file, int socket_id);
void *handle_user(void *args);
uint32_t create_user_socket(int *id);
char *file_info_serialize(struct stat info, char *serialized_info, char* file_name, int pos);
void time_to_string(char *time_buf, long int stat_time);
int get_file_amount(int socket_id);
int send_current_files(int socket_id, struct sockaddr_in *si_other, unsigned int slen, int packet_id);
int check_if_online(char *host);
int get_file_metadata(struct file_info *file, char* file_name, int socket_id, int file_size);
void log_out_and_close_session(int socket_id);
int main_leader_server(int client_port);
int main_backup_server(int this_server_port, char* server_from_leader, int leader_port, char* server_from_backup);

//--------------------------------BACKUP CODE -------------------------------------------------------------------------------
void receive_new_backup(char *server_from_backup, int backup_port, int packet_id, int socket_id, struct sockaddr_in *si_other, unsigned int slen);
void tell_leader_that_backup_exists(int this_server_port, int leader_port, char* server_from_leader, char* server_from_backup);
void backup_dealing_login(Packet packet, int socket_id, struct sockaddr_in *si_other, unsigned int slen);
void add_client_to_backup_vector(char *user_name, uint32_t port);
void sub_client_to_backup_vector(uint32_t port);

#endif