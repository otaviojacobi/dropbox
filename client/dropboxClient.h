#pragma once
#ifndef _DROPBOXCLIENT_H_
#define _DROPBOXCLIENT_H_

#include "../lib/dropboxUtil.h"

int login_server(char *host, int port); //done
void send_file(char *file); //done
void sync_client(); //TODO
void get_file(char *file); //done
void delete_file(char *file); //TODO
void close_session(); //done I think ?

void list_server(); 

uint32_t get_id();

#endif