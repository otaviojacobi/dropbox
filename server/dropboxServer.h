#pragma once
#ifndef _DROPBOXSERVER_H_
#define _DROPBOXSERVER_H_

#include "../lib/dropboxUtil.h"

void receive_file(char *file, uint32_t file_size, uint32_t id);

void receive_login_server(char *host, int packet_id);
int check_login_status(char *host);
void create_new_user(char *host);
void bind_user_to_server(char *user_name);
int receive_packet(char *buffer);
void get_full_path_file(char *buffer, char *file);
void send_ack(Ack *ack);

#endif