#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../lib/dropboxUtil.h"
 
int main(int argc, char **argv) {
    
    struct sockaddr_in si_me, si_other;
    char buf[BUFLEN];
    int recv_len;
    unsigned int slen = sizeof(si_other);

    int PORT = argc >= 2 ? atoi(argv[1]) : DEFAULT_PORT; 
     
    //create a UDP socket
    int socket_id = init_socket_server(PORT, &si_me);

    //keep listening for data
    printf("Listening on port %d...\n", PORT);
    while(true) {

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
            kill("Failed to receive data...\n");

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("Data: %s\n" , buf);
        
	    memset(buf,'\0', BUFLEN);
    }
 
    close(socket_id);
    return 0;
}
