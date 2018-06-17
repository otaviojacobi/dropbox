#include "../lib/dropboxUtil.h"
#include <sys/select.h> // http://man7.org/linux/man-pages/man2/select.2.html
#include <sys/time.h>
#include "election.h"

extern std::vector<BackupServer> backups;
extern BackupServer thisBackup;
extern BackupServer leaderServer;

struct timeval timeout;
fd_set readfds, masterfds;


void print_backups()
{
    printf("------------------------- BACKUP SERVERS --------------------\n");
    for(int i=0; i < backups.size(); i++) {
        printf("%d: port %d, server %s\n", i, backups[i].port, backups[i].server);
    }
    printf("-------------------------------------------------------------\n");
}


void set_new_leader(BackupServer newLeader)
{
	leaderServer.id = newLeader.id;
	leaderServer.initOwnElection = false;
	leaderServer.isLeader = true;

	leaderServer.port = newLeader.port;
	strcpy(leaderServer.server,newLeader.server);
}

/*
bool explode_timeout(double start_seconds){
	double seconds = clock() / CLOCKS_PER_SEC ; // works for this problem ??
    return (seconds - start_seconds >= TIMEOUT);
}
*/

int wait_for_answer_in_timeout()
{
    struct sockaddr_in si_other;
    unsigned int slen = sizeof(si_other);
    int socket_id = init_socket_to_send_packets(thisBackup.port, thisBackup.server, &si_other);

	// http://forums.codeguru.com/showthread.php?382646-timeout-for-recvfrom()
	// http://alumni.cs.ucr.edu/~jiayu/network/lab8.htm
    FD_ZERO(&masterfds);
    FD_SET(socket_id, &masterfds);
    memcpy(&readfds, &masterfds, sizeof(fd_set));

    timeout.tv_sec = 10;
	timeout.tv_usec = 0;
    
    if (select(socket_id+1, &readfds, NULL, NULL, &timeout) < 0)
    {
        perror("on select");
        return 0;
    }
    else
    {
        if (FD_ISSET(socket_id, &readfds)) // read from the socket
        {
        	Ack ack;
        	int recv_len;
            if ((recv_len = recvfrom(socket_id, &ack, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen)) != -1)
		    {
		    	printf("Receive answer from %d\n", ack.info);
		    	return (int)ack.info;
		    }
        }
        else
        {   
            // the socket timedout
			return 0;
        }
    }
}

void send_check_leader_message(){ 

    Packet packet;
    char buf[PACKET_SIZE];
    struct sockaddr_in si_other;
    unsigned int slen = sizeof(si_other);

    int socket_id = init_socket_to_send_packets(leaderServer.port, leaderServer.server, &si_other);

	create_packet(&packet, Check_Leader_type, 0, thisBackup.id, ""); // id = 0 ???
	send_packet(&packet, socket_id, &si_other, slen);

	int recvAnswerInTimeOut = wait_for_answer_in_timeout();
	close(socket_id);

	if(recvAnswerInTimeOut == LEADER_ALIVE){
		init_election();
	}
	if(recvAnswerInTimeOut == I_AM_NOT_LEADER){
		
		printf("Erro at check leader: msg sended for someone that is not the leader!\n");
		//exit(1); //??
	}
}


void send_election_messages()
{
    Ack ack;
    Packet packet;
    char buf[PACKET_SIZE];
    struct sockaddr_in si_other;
    unsigned int slen = sizeof(si_other);

    int socket_id = init_socket_to_send_packets(thisBackup.port, thisBackup.server, &si_other);

    for(int i = 0; i < backups.size() - 1; i++) 
    {
    	if(thisBackup.id > backups[i].id)
    	{
	        create_packet(&packet, Election_type, i, thisBackup.id, "");

	        await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

	        if((uint8_t)buf[0] != Ack_type) {
	            printf("Fail to send election msg to %s:%d\n", backups[i].server, backups[i].port);
	        }
	        printf("Sent packet election msg to %s:%d\n", backups[i].server, backups[i].port);
        }
    }
    close(socket_id);
}

//int socket_id, struct sockaddr_in *si_other, unsigned int slen
void send_coordinate_messages()
{
    Ack ack;
    Packet packet;
    char buf[PACKET_SIZE];
    struct sockaddr_in si_other;
    unsigned int slen = sizeof(si_other);

    int socket_id = init_socket_to_send_packets(thisBackup.port, thisBackup.server, &si_other);

    for(int i = 0; i < backups.size(); i++) 
    {
    	if(backups[i].id != thisBackup.id)
    	{
		    create_packet(&packet, Leader_type, i, thisBackup.id, "");

		    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

		    if((uint8_t)buf[0] != Ack_type) {
		        printf("Fail to send coordinate msg to %s:%d\n", backups[i].server, backups[i].port);
		    }
		    printf("Sent packet coordinate msg to %s:%d\n", backups[i].server, backups[i].port);
		}
    }
    close(socket_id);
}


void init_election()
{
	if(thisBackup.initOwnElection)
		return;

	thisBackup.initOwnElection = true;

	send_election_messages();

    int recvAnswerInTimeOut = wait_for_answer_in_timeout();
    if(!recvAnswerInTimeOut)
    {
    	printf("I am the new leader!\n");
    	thisBackup.isLeader = true;
    	thisBackup.initOwnElection = false;
    	set_new_leader(thisBackup);
		send_coordinate_messages();
		// avisar front-end?
    }
}



// update the leaderServer and the new leader in vector
void receive_leader_message(int idNewLeader) 
{
	// find new leader in backups to update
	bool find = false;
	int i=-1;
	while(i < backups.size() && !find)
	{
		i++;
		find = (backups[i].id == idNewLeader);
	}

	if(!find){
		printf("Receive leader msg: Not find lider with id = %d\n", idNewLeader); //????
	}
	else{
		set_new_leader(backups[i]);
	}
}


void loop_election() // in a new thread for backupServer
{
    double start_seconds = clock()/CLOCKS_PER_SEC;
    double now_seconds;

    while(!thisBackup.isLeader)
    {
        now_seconds = clock()/CLOCKS_PER_SEC;

        if(now_seconds - start_seconds >= TIME_FOR_CHECK_COORD)
        {
            start_seconds = now_seconds;

            send_check_leader_message(); // if leader dont answer, init election and wait the victory or not!
        }
    }
}




