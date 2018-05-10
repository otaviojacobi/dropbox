#pragma once
#ifndef _DROPBOXCLIENT_H_
#define _DROPBOXCLIENT_H_

#include "../lib/dropboxUtil.h"

int login_server(char *host, int port);
void sync_client();
void get_file(char *file);
void delete_file(char *file);
void close_session();
uint32_t get_id();


#endif