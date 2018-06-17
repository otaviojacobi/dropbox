#ifndef _ELECTION_H_
#define _ELECTION_H_

#define TIME_FOR_CHECK_COORD 5 // seconds


void init_election();

void receive_leader_message(int idNewLeader);


#endif