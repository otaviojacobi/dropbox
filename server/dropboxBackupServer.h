#ifndef _DROPBOXBACKUPSERVER_H_
#define _DROPBOXBACKUPSERVER_H_

#include "../lib/dropboxUtil.h"

void receive_new_backup(char *server_from_backup, int backup_port, int packet_id, int socket_id, struct sockaddr_in *si_other, unsigned int slen);
void tell_leader_that_backup_exists(int this_server_port, int leader_port, char* server_from_leader, char* server_from_backup);

#endif