#include "dropboxServer.h"

uint32_t current_port = 8132;

//char *clients[100000]; // nao seria melhor uma lista encadeada?
std::map<int, Client> clients;

int main(int argc, char **argv) {
    
    int recv_len;
    struct sockaddr_in si_other;
    unsigned int slen = sizeof(si_other);
    Packet packet;

    int PORT = argc >= 2 ? atoi(argv[1]) : DEFAULT_PORT; 
     
    //create a UDP socket
    struct sockaddr_in si_me;
    int socket_id = init_socket_server(PORT, &si_me);

    //keep listening for data
    printf("Listening on port %d...\n", PORT);
    while(true) {

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)
            kill("Failed to receive data...\n");

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        
        switch(packet.packet_type) {
            case Client_login_type:
                receive_login_server(packet.data, packet.packet_id, socket_id, &si_other, slen);
                break;

            case Header_type:
                printf("Error: not supposed to be Header_type case on main\n");
                break;
            
            case Data_type:
                printf("Error: not supposed to be Data_type case on main\n");
                break;

            default: printf("The packet type is not supported! on main\n");
        }
    }
 
    close(socket_id);
    return 0;
}

void receive_login_server(char *host, int packet_id, int socket_id, struct sockaddr_in *si_other, unsigned int slen ) {

    pthread_t logged_client;
    int status = check_login_status(host);
    int new_socket_id;
    Ack ack;
    ack.packet_type = Ack_type;
    ack.packet_id = packet_id;
    int is_online = check_if_online(host);
    uint32_t port;

    if(is_online) {
        clients[is_online].devices[1] = clients[is_online].devices[0]; // This is the server port listening for them
        port = clients[is_online].devices[0];
    } else {
        port = create_user_socket(&new_socket_id);
        bind_user_to_server( host, new_socket_id, port);
        pthread_create(&logged_client, NULL, handle_user, (void*) new_socket_id);
    }

    switch(status) {
        case Old_user:
            printf("Logged in !\n");
            ack.util = Old_user;
            break;
        case New_user:
            printf("Creating new user for %s\n", host);
            ack.util = New_user;            
            create_new_user(host);
            break;

        default: printf("Failed to login.\n");
    }

    ack.info = port;
    send_ack(&ack, socket_id, si_other, slen);
    printf("User %s Logged in.\n", host);
}

int check_if_online(char *host) {
    for (std::map<int,Client>::iterator it=clients.begin(); it!=clients.end(); ++it) {

        if(strcmp(it->second.user_name, host) == 0) {
            return it->first;
        }
    }
    return 0;
}

void* handle_user(void* args) {
    int socket_id = (int ) args;
    int recv_len;
    Packet packet;
    Ack ack;
    struct file_info file_metadata;
    struct sockaddr_in si_client;
    unsigned int slen = sizeof(si_client);
    char *path_file;
    FILE *file;
    uint32_t packet_id = 0;
    int32_t file_size;
    int file_amount;

    while(true) {

        printf("Thread user %s\n", clients[socket_id].user_name);

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_client, &slen)) == -1)
            kill("Failed to receive data...\n");

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_client.sin_addr), ntohs(si_client.sin_port));
        
        path_file = (char *) malloc (strlen(packet.data) + strlen(clients[socket_id].user_name) + 1);
        get_full_path_file(path_file, packet.data, socket_id);

        printf("%s", path_file);
        switch(packet.packet_type) {
            case Client_login_type:
                printf("Error: You have logged in already !\n");
                break;

            case Header_type:
                create_ack(&ack, packet.packet_id, packet.packet_info);
                send_ack(&ack, socket_id, &si_client, slen);
                receive_file(path_file, packet.packet_info, packet.packet_id, socket_id);
                if(!get_file_metadata(&file_metadata, packet.data, socket_id, packet.packet_info))
                    clients[socket_id].info.push_back(file_metadata);
                break;

            case Download_type:
                file = fopen(path_file, "rb");
                file_size = file ? get_file_size(file) : -1;
                create_ack(&ack, packet.packet_id, file_size);
                send_ack(&ack, socket_id, &si_client, slen);
                if(file_size == -1) break;
                fclose(file);
                packet_id = send_file_chunks(path_file, socket_id, &si_client, slen, packet_id, 'c');
                break;

            case List_type:
                file_amount = get_file_amount(socket_id);
                create_ack(&ack, packet.packet_id, file_amount);
                send_ack(&ack, socket_id, &si_client, slen);
                if(file_amount > 0)
                    packet_id = send_current_files(socket_id, &si_client, slen, packet_id);
                break;

            case Data_type:
                printf("Error: not supposed to be Data_type case THREAD\n");
                break;

            default: printf("The packet type is not supported!\n");
        }
        free(path_file);
    }
}

int get_file_amount(int socket_id) {
    clients[socket_id].info.size();
}
 
int send_current_files(int socket_id, struct sockaddr_in *si_other, unsigned int slen, int packet_id) {
    Packet packet;
    Ack ack;
    char buf[PACKET_SIZE];
    char serialized_info[DATA_PACKET_SIZE];
    int i = 0;

    for (std::list<struct file_info>::const_iterator iterator = clients[socket_id].info.begin(), 
                end = clients[socket_id].info.end(); iterator != end; ++iterator) {
        
        file_info_serialize(*iterator, serialized_info, i);
        create_packet(&packet, Data_type, packet_id, 0, serialized_info);
        await_send_packet(&packet, &ack, buf, socket_id, si_other, slen);
        i++;
    }


    return packet_id;
}

char *file_info_serialize(struct file_info info, char *serialized_info, int pos) {

    char buf[15];
    int i = 0;
    sprintf(buf, "%d", info.size);

    if(pos == 0) {
        strcpy(serialized_info, "Name\t\tLast modified on\tSize(Bytes)\tCreated\n");
        strcat(serialized_info, "--------------------------------------------------------------\n");
        strcat(serialized_info, info.name);
    } else {
        strcpy(serialized_info, info.name);
    }
    
    strcat(serialized_info, "\t");
    if(strlen(info.name) < 7)
        strcat(serialized_info, "\t");       
    strcat(serialized_info, info.last_modified);
    strcat(serialized_info, "\t");
    strcat(serialized_info, buf);
    strcat(serialized_info, "\t\t");
    strcat(serialized_info, info.created);
    strcat(serialized_info, "\n");    
} 

int check_login_status(char *host) {

    DIR* dir = opendir(host);

    if (dir) { // Directory exists.
        closedir(dir);
        return Old_user;
    }
    if (ENOENT == errno) {  // Directory does not exist. 
        return New_user;
    }
    else { // opendir() failed for some other reason.PACKET_SIZE
        return -1;
    }
}

void create_new_user(char *host) {
    mkdir(host, 0700);
}

void bind_user_to_server(char *user_name, int socket_id, uint32_t port ) {

    Client client;

    client.devices[0] = port;
    client.devices[1] = -1;
    strcpy(client.user_name, user_name);
    clients.insert(std::pair<int, Client>(socket_id, client));
    printf("Binded %d to %s\n", socket_id, clients[socket_id].user_name);
}

void get_full_path_file(char *buffer, char *file, int socket_id) {
    buffer[0] = '\0';
    strcat(buffer, clients[socket_id].user_name);
    strcat(buffer, "/");
    strcat(buffer, file);
}

uint32_t create_user_socket(int *id) {

    int new_socket_id;
    struct sockaddr_in s_in;

    if ( (new_socket_id=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
       kill("An error ocurred while creating the socket.\n");

    *id = new_socket_id;

    memset((char *) &s_in, 0, sizeof(s_in));
     
    //TODO: is the order important ?
    s_in.sin_family = AF_INET;
    s_in.sin_addr.s_addr = htonl(INADDR_ANY);

    do {
        current_port++;
        s_in.sin_port = htons(current_port);
    } while (bind(new_socket_id, (struct sockaddr *)&s_in, sizeof(struct sockaddr_in)) == -1);


    return current_port;
}

/*Sorry, this has many, many side effects.*/
int get_file_metadata(struct file_info *file, char* file_name, int socket_id, int file_size) {

    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];
    int exists = false;

    time (&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer,sizeof(buffer),"%d-%m-%Y %I:%M:%S",timeinfo);

    for (std::list<struct file_info>::iterator iterator = clients[socket_id].info.begin(), 
            end = clients[socket_id].info.end(); iterator != end; ++iterator) {
        if(strcmp((*iterator).name, file_name) == 0) {
            strcpy((*iterator).last_modified, buffer);    
            exists = true;
        }
    }

    strcpy(file->name, file_name);
    file->size = file_size;
    strcpy(file->last_modified, buffer);
    if(!exists)
        strcpy(file->created, buffer);

    return exists;
}
