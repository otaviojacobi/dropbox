#ifndef _DROPBOXFRONTEND_H_
#define _DROPBOXFRONTEND_H_

#include "../lib/dropboxUtil.h"

void receive_packets_from_server(int total_packets);
void resolve_send_packet(Packet *packet, Ack *ack, char *buf, int socket_id, struct sockaddr_in *si_other, unsigned int slen);

#endif