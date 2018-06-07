#include "dropboxServer.h"
#include "dropboxBackupServer.h"

int main_backup_server(int this_server_port, char* server_from_leader, int leader_port, char* server_from_backup) { 
    tell_leader_that_backup_exists(this_server_port, leader_port, server_from_leader, server_from_backup);

    int recv_len;
    struct sockaddr_in si_other;
    unsigned int slen = sizeof(si_other);
    Packet packet;

    //create a UDP socket
    struct sockaddr_in si_me;
    int socket_id = init_socket_server(this_server_port, &si_me);

    //keep listening for data
    printf("Listening on port %d...\n", this_server_port);
    while(true) {

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)
            kill("Failed to receive data...\n");

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        
        switch(packet.packet_type) {
            default: printf("Not implemented yet\n");
        }
    }
 
    close(socket_id);
    return 0;
}

void tell_leader_that_backup_exists(int this_server_port, int leader_port, char* server_from_leader, char* server_from_backup) {

    struct sockaddr_in si_other;
    unsigned int slen;    
    slen = sizeof(si_other);
    int socket_id;

    socket_id = init_socket_client(leader_port, server_from_leader, &si_other);

    char buf[PACKET_SIZE];
    Packet packet;
    Ack ack;

    //TODO: review id on create_packet function
    create_packet(&packet, New_Backup_Server_Type, this_server_port + leader_port, this_server_port, server_from_backup); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

    if((uint8_t)buf[0] == Ack_type) {
        printf("The leader server knows that this server exist.\n");
    }
    else {
       kill("We failed to tell leader server that this backup server exist. Try again later!\n");
    }
}
