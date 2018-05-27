#ifndef _DROPBOXCLIENT_H_
#define _DROPBOXCLIENT_H_

#include "../lib/dropboxUtil.h"

int login_server(char *host, int port); //done
void send_file(char *file); //done
void sync_client(); //TODO
void get_file(char *file, int local); //done
void delete_file(char *file); //TODO
void close_session(); //done I think ?
void print_time(long int stat_time);
void stop_watch (char* folder_path);
void start_watch (char* folder_path);
void log_out_and_close_session();
void delete_file(char *file);

void* sync_server (void *args);
void* sync_daemon (void *args);

void list_server(); //done
void print_item_info (ServerItem item);
void list_client();

uint32_t get_id();

#endif
