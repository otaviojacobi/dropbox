#include "dropboxFrontEnd.h"

struct sockaddr_in si_me;
int my_port;
int socket_id;

struct sockaddr_in si_leader;
int socket_id_leader;
char *leader_server;
int leader_port;

struct sockaddr_in si_client;
int client_port;
char client_server[MAXNAME];

int main(int argc, char **argv) {
    
    my_port = argc >= 2 ? atoi(argv[1]) : FRONTEND_DEFAULT_PORT; 
    leader_server = argc >= 3 ? argv[2] : SERVER_DEFAULT;
    leader_port = argc >= 3 ? atoi(argv[3]) : LEADER_DEFAULT_PORT; 

    int recv_len;
    char buf[PACKET_SIZE];
    char buf_data[DATA_PACKET_SIZE];
    Packet packet;
    Ack ack;
    int packets_to_receive;

    //create a UDP socket
    socket_id = init_socket_to_receive_packets(my_port, &si_me);
    socket_id_leader = init_socket_to_send_packets(leader_port, leader_server, &si_leader);
    
    unsigned int slen = sizeof(si_me);

    pthread_t thread_to_receive_packets;

    printf("I'm frontend\n");
    printf("Listening on port %d...\n", my_port);
    while(true) {

        if ((recv_len = recvfrom(socket_id, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_me, &slen)) == -1)
            kill("Failed to receive data from client...\n");

        printf("Received packet from %s:%d\n", inet_ntoa(si_me.sin_addr), ntohs(si_me.sin_port));
        
        switch(packet.packet_type) {
            case Client_login_type:
                strcpy(client_server, inet_ntoa(si_me.sin_addr));
                client_port = ntohs(si_me.sin_port);
                socket_id_leader = init_socket_to_send_packets(client_port, client_server, &si_client);

                await_send_packet(&packet, &ack, buf, socket_id_leader, &si_leader, slen);
                
                socket_id_leader = init_socket_to_send_packets(ack.info, leader_server, &si_leader);

                if((uint8_t)buf[0] == Ack_type) {
                    memcpy(&ack, buf, sizeof(ack));
                }
                send_ack(&ack, socket_id, &si_me, slen); 
                break;

            case Download_type:
                await_send_packet(&packet, &ack, buf, socket_id_leader, &si_leader, slen);
                
                if((uint8_t)buf[0] == Ack_type) {
                    memcpy(&ack, buf, sizeof(ack));
                }
                send_ack(&ack, socket_id, &si_me, slen);
                
                packets_to_receive = ceil(ack.util/sizeof(buf_data));
                receive_packets_from_server(packets_to_receive);
                break;

            case List_type:
                await_send_packet(&packet, &ack, buf, socket_id_leader, &si_leader, slen);
                
                if((uint8_t)buf[0] == Ack_type) {
                    memcpy(&ack, buf, sizeof(ack));
                }
                send_ack(&ack, socket_id, &si_me, slen);
                
                packets_to_receive = ack.util;
                receive_packets_from_server(packets_to_receive - 1);
                break;

            default:    
                await_send_packet(&packet, &ack, buf, socket_id_leader, &si_leader, slen);

                if((uint8_t)buf[0] == Ack_type) {
                    memcpy(&ack, buf, sizeof(ack));
                }
                send_ack(&ack, socket_id, &si_me, slen);  

        }
    }

    return 0;
}

void receive_packets_from_server(int total_packets) {
    unsigned int slen = sizeof(si_me);
    int recv_len;
    char buf[PACKET_SIZE];
    Packet packet;
    Ack ack;
    int packets_received = 0;

    //keep listening for data
    printf("Listening server %d...\n", ntohs(si_leader.sin_port));
    while(packets_received <= total_packets) {
            printf("%d de %d\n", packets_received, total_packets);

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id_leader, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_leader, &slen)) == -1)
            kill("Failed to receive data from server leader...\n");
        packets_received++;

        printf("Received packet from %s:%d\n", inet_ntoa(si_leader.sin_addr), ntohs(si_leader.sin_port));
        
        switch(packet.packet_type) {            
            case New_Leader_type://TODO: REVIEW WHEN END THE ELECTION
                socket_id_leader = init_socket_to_send_packets(packet.packet_info, packet.data, &si_leader);
                break;
                
            default:
                await_send_packet(&packet, &ack, buf, socket_id, &si_client, slen);
                
                if((uint8_t)buf[0] == Ack_type) {
                    memcpy(&ack, buf, sizeof(ack));
                }
                send_ack(&ack, socket_id_leader, &si_leader, slen); 
        }
    }
}
