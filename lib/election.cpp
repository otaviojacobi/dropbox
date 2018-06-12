#include "dropboxUtil.h"
#include <sys/select.h> // http://man7.org/linux/man-pages/man2/select.2.html

#define MAX_PROCESSES 10 // ??
#define MAX_TRANSMISSION_TIME 2.5f // ??
#define MAX_PROCESSING_TIME 2.5f // ??
#define TIMEOUT (2*MAX_TRANSMISSION_TIME + MAX_PROCESSING_TIME)

std::map<int, Process> allProcesses; // key is the pid's process
std::map<int, Process>::iterator iteratorProc;
Process* coordProcess = NULL;

//-----------------------------------------
// soh pra compilar!!! Onde estarão? Veremos ... :D
int socket_id;
struct sockaddr_in si_other;
unsigned int slen = sizeof(si_other);

//----------------------------------------------------
struct timeval timeout;
timeout.tv_sec = 10;
timeout.tv_usec = 0;
fd_set readfds, masterfds;
//-----------------------------------------

void init_election_system() {
	// ...
}

void create_process(Process *process, int pid, pthread_t *thread){
	process->pid = pid;
	process->isCoord = false;
	process->initOwnElection = false;
	process->thread = thread;
	allProcesses.insert(std::pair<int, Process>(pid, *process));
}

void remove_process(int pid){ // util?
	allProcesses.erase(pid);
}

bool explode_timeout(double start_seconds){
	double seconds = clock() / CLOCKS_PER_SEC ; // works for this problem ??
    return (seconds - start_seconds >= TIMEOUT);
}

void send_election_message(int toPid){

	int myPid = 0;
	Packet packet;
	create_packet(&packet, Election_type, 0, myPid, ""); // id = 0 ???
	send_packet(&packet, socket_id, &si_other, slen);
}

// http://forums.codeguru.com/showthread.php?382646-timeout-for-recvfrom()
// http://alumni.cs.ucr.edu/~jiayu/network/lab8.htm
int wait_for_some_packet(Packet *packet)
{
    FD_ZERO(&masterfds);
    FD_SET(socket_id, &masterfds);
    memcpy(&readfds, &masterfds, sizeof(fd_set));
    
    if (select(socket_id+1, &readfds, NULL, NULL, &timeout) < 0)
    {
        perror("on select");
        return -1;
    }
    else
    {
        if (FD_ISSET(socket_id, &readfds))
        {
            // read from the socket
            if ((recv_len = recvfrom(socket_id, packet, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen)) != -1)
		    {
		        printf("pkt received\n");
		        return 1;
		    }
        }
        else
        {   
            // the socket timedout
			return 0;
        }
    }
}


void init_election(int candidatePid){

	int anotherPid;
	for (iteratorProc=allProcesses.begin(); iteratorProc!=allProcesses.end(); ++iteratorProc) {

		anotherPid = iteratorProc->first;
        if(anotherPid > candidatePid){
        	send_election_message(anotherPid);
        }
    }
    allProcesses[candidatePid].initOwnElection = true;

    // wait for any answer
    Packet packet;
	double start_seconds = clock() / CLOCKS_PER_SEC;
	bool stop = false;
	while(!stop){

		if (recvfrom(socket_id, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen) > -1){
			stop = true;
		}
		else if(explode_timeout(start_seconds)){
			stop = true;
			// candidate is new coord
			allProcesses[candidatePid].isCoord = true;
			coordProcess->isCoord = false;
			*coordProcess = allProcesses[candidatePid];
		}
	}
}


void send_check_coord_message(int pid){ // random pid != coordinate pid

	Packet packet;
	create_packet(&packet, CheckCoord_type, 0, pid, ""); // id = 0 ???
	send_packet(&packet, socket_id, &si_other, slen);

	// wait for answer
	double start_seconds = clock() / CLOCKS_PER_SEC;
	bool stop = false;
	while(!stop){

		if (recvfrom(socket_id, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen) > -1){
			stop = true;
		}
		else if(explode_timeout(start_seconds)){
			stop = true;
			if( allProcesses[pid].initOwnElection == false ){
				init_election(pid);
			}
		}
	}

	// wait again for coord answer and do what?
}




void send_answer_message(int toPid){
	Ack ack;
	create_ack(&ack, 0, 0); // id = 0 ???
	ack.info = toPid;
	send_ack(&ack, socket_id, &si_other, slen);
}

void send_coordinate_message(int toPid){
	// for all processes

}

void receive_election_message(int fromPid){
	// send answer to sender
	send_answer_message(fromPid);

	// init my own election
	//init_election(myPid);
}

void receive_coordinate_message(){


}
