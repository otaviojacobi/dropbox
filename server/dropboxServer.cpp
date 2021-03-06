#include "dropboxServer.h"

uint32_t current_port = 8132;
uint32_t next_id = 0;

std::map<int, Client> clients;
std::vector<BackupServer> backups;
std::vector<Client> backup_clients;

pthread_mutex_t clients_timesOnline = PTHREAD_MUTEX_INITIALIZER;

uint32_t process_value_election;
int knew_leader_is_dead = false;

int main(int argc, char **argv) {
    
    int leader_port = argc >= 2 ? atoi(argv[1]) : LEADER_DEFAULT_PORT; 
    uint32_t is_backup =  argc >= 3 ? atoi(argv[2]) : false; //set 0 to be leader

    if(!is_backup) {
        printf("I'm leader server\n");
        process_value_election = 0;
        main_leader_server(leader_port);
    }
    else {
        uint32_t backup_id = is_backup;
        int this_server_port = argc >= 4 ? atoi(argv[3]) : BACKUP_DEFAULT_PORT;
        char *server_from_leader =  argc >= 5 ? argv[4] : SERVER_DEFAULT;
        char *server_from_backup =  argc >= 5 ? argv[4] : SERVER_DEFAULT;
        printf("I'm backup server\n");
        main_backup_server(this_server_port, server_from_leader, leader_port, server_from_backup);
    }    

    return 0;
}

int main_leader_server(int port) { 
    int recv_len;
    struct sockaddr_in si_other;
    unsigned int slen = sizeof(si_other);
    Packet packet;
    Ack ack;

    //create a UDP socket
    int socket_id = init_socket_to_receive_packets(port, &si_other);

    //keep listening for data
    printf("Listening on port %d...\n", port);
    while(true) {

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)
            kill("Failed to receive data...\n");

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        
        switch(packet.packet_type) {
            case Client_login_type:
                receive_login_server(packet, socket_id, &si_other, slen);
                break;

            case New_Backup_Server_Type:
                receive_new_backup(packet, socket_id, &si_other, slen);
                break;

            case Is_Leader_Alive_Type:
                ack.packet_type = Ack_type;
                ack.packet_id = packet.packet_id;
                send_ack(&ack, socket_id, &si_other, slen);
                break;

            case Header_type:
                printf("Error: not supposed to be Header_type case on main\n");
                break;
            
            case Data_type:
                printf("Error: not supposed to be Data_type case on main\n");
                break;
            
            case Client_exit_type:
                printf("Error: not supposed to be Client_exit_type case on main\n");
                break;
            
            case Delete_type:
                printf("Error: not supposed to be Delete_type case on main\n");
                break;

            default: printf("The packet type is not supported! on main\n");
        }
    }
 
    close(socket_id);
    return 0;
}

void receive_login_server(Packet packet, int socket_id, struct sockaddr_in *si_other, unsigned int slen) {

    char *host = packet.data;
    int packet_id = packet.packet_id;
    pthread_t logged_client;
    int status = check_login_status(host);
    int *new_socket_id = (int *) malloc (sizeof(int));
    Ack ack;
    ack.packet_type = Ack_type;
    ack.packet_id = packet_id;
    int is_online = check_if_online(host);
    uint32_t port;

    ClientDevice client_device;
    client_device.port = packet.packet_info;
    strcpy(client_device.device, inet_ntoa(si_other->sin_addr));

    if(is_online) {
        port = clients[is_online].portListening;
	    pthread_mutex_lock(&clients_timesOnline);
        clients[is_online].devices.push_back(client_device);
	    pthread_mutex_unlock(&clients_timesOnline);

    } else {
        port = create_user_socket(new_socket_id);
        bind_user_to_server(host, *new_socket_id, port, client_device);
        pthread_create(&logged_client, NULL, handle_user, (void*) new_socket_id); // WATCH OUT : new_socket_id will be freed
    }

    switch(status) {
        case Old_user:
            printf("Logged in!\n");
            ack.util = Old_user;
            break;
        case New_user:
            printf("Creating new user for %s\n", host);
            ack.util = New_user;            
            create_new_user(host);
            break;

        default: printf("Failed to login.\n");
    }
    
    send_packet_to_backups(packet, backups);

    ack.info = port;
    send_ack(&ack, socket_id, si_other, slen);
    printf("User %s Logged in.\n", host);
}

void receive_new_backup(Packet packet, int socket_id, struct sockaddr_in *si_other, unsigned int slen) {
    Ack ack;
    ack.packet_type = Ack_type;
    ack.packet_id = packet.packet_id;
    ack.info = backups.size() + 1;
    send_ack(&ack, socket_id, si_other, slen); //<<<put this after

    BackupServer new_backup;
    new_backup.port = packet.packet_info;
    new_backup.value_election = backups.size() + 1;
    strcpy(new_backup.server, inet_ntoa(si_other->sin_addr));

    std::cout << "New backup " << new_backup.server << ":" << new_backup.port <<"\nBackup list size = " << backups.size()+1 << '\n';

    
    packet.packet_id = new_backup.value_election;
    send_packet_to_backups(packet, backups);

    backups.push_back(new_backup);
    send_backups_list(new_backup);
}

void send_backups_list(BackupServer backup) {
    struct sockaddr_in si_other;
    Ack ack;
    Packet packet;
    char buf[PACKET_SIZE];
    unsigned int slen = sizeof(si_other);

    int socket_id = init_socket_to_send_packets(backup.port, backup.server, &si_other);

    for(int i = 0; i < backups.size() - 1; i++) {

        printf("value_election :%d\n", backups[i].value_election); 
        create_packet(&packet, New_Backup_Server_Type, backups[i].value_election, backups[i].port, backups[i].server);
        await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);
        if((uint8_t)buf[0] != Ack_type) {
            printf("Fail to send packet to backup %s:%d\n", backups[i].server, backups[i].port);
        }
        printf("Sent packet to backup %s:%d\n", backups[i].server, backups[i].port);  
    }
    close(socket_id);  
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
    int *point_id = (int *) args;
    int socket_id = *point_id;
    free(point_id);
    int recv_len;
    Packet packet;
    Ack ack;
    struct file_info file_metadata;
    struct sockaddr_in si_client;
    unsigned int slen = sizeof(si_client);
    char path_file[MAXNAME];
    char packet_data[DATA_PACKET_SIZE];
    FILE *file;
    uint32_t packet_id = 0;
    int32_t file_size;
    pthread_mutex_t busy_client;
    struct utimbuf file_times;
    int file_amount;

    while(true) {

        printf("Thread user %s\n", clients[socket_id].user_name);

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_client, &slen)) == -1)
            kill("Failed to receive data...\n");

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_client.sin_addr), ntohs(si_client.sin_port));
        
        //path_file = (char *) malloc (strlen(packet.data) + strlen(clients[socket_id].user_name) + 1);

        get_full_path_file(path_file, packet.data, socket_id);


        switch(packet.packet_type) {
            case Client_login_type:
                printf("Error: You have logged in already !\n");
                break;

            case Header_type:
                memcpy(packet_data, packet.data, sizeof(packet.data));
                create_ack(&ack, packet.packet_id, packet.packet_info);
                
                strcpy(packet.data, path_file);
                send_packet_to_backups(packet, backups);

                create_ack(&ack, packet.packet_id, packet.packet_info);
                send_ack(&ack, socket_id, &si_client, slen);
				
				pthread_mutex_lock(&busy_client);
                receive_file(path_file, packet.packet_info, packet.packet_id, socket_id, true, backups);
                if(!get_file_metadata(&file_metadata, packet_data, socket_id, packet.packet_info))
                    clients[socket_id].info.push_back(file_metadata);
				
				file_times.modtime = file_metadata.times.st_mtime;
				file_times.actime = file_metadata.times.st_atime;
				
				utime(path_file, &file_times);
				pthread_mutex_unlock(&busy_client);
                break;

            case Download_type:
                file = fopen(path_file, "rb");
                file_size = file ? get_file_size(file) : -1;
                create_ack(&ack, packet.packet_id, file_size);
                send_ack(&ack, socket_id, &si_client, slen);
                if(file_size == -1) break;
                fclose(file);
                pthread_mutex_lock(&busy_client);
                packet_id = send_file_chunks(path_file, socket_id, &si_client, slen, packet_id, 'c');
                pthread_mutex_unlock(&busy_client);
                break;

            case List_type:
                file_amount = get_file_amount(socket_id) - 2;
                create_ack(&ack, packet.packet_id, file_amount);
                send_ack(&ack, socket_id, &si_client, slen);
                if(file_amount > 0)
                    packet_id = send_current_files(socket_id, &si_client, slen, packet_id);
                break;

            case Data_type:
                printf("Error: not supposed to be Data_type case THREAD\n");
                break;
            
            case Client_exit_type:
                strcpy(packet.data, inet_ntoa(si_client.sin_addr));
                send_packet_to_backups(packet, backups);

                create_ack(&ack, packet.packet_id, 0);
                send_ack(&ack, socket_id, &si_client, slen);

                log_out_and_close_session(socket_id, packet, &si_client);
                break;
            
            case Delete_type:
                strcpy(packet.data, path_file);
                send_packet_to_backups(packet, backups);

                create_ack(&ack, packet.packet_id, 0);
                send_ack(&ack, socket_id, &si_client, slen);
				pthread_mutex_lock(&busy_client);
                if(remove(path_file) != 0)  {
                    printf("Error: unable to delete the file %s\n", path_file);
                }
                pthread_mutex_unlock(&busy_client);
                break;

            default: printf("The packet type is not supported!\n");
        }
        //free(path_file);
    }
}

void log_out_and_close_session(int socket_id, Packet packet, struct sockaddr_in *si_other) {
    char exit_message[COMMAND_LENGTH];

	pthread_mutex_lock(&clients_timesOnline);

    for(int i = 0; i < clients[socket_id].devices.size(); i++)
    {
        if(strcmp(packet.data, clients[socket_id].devices[i].device) == 0 && packet.packet_info == clients[socket_id].devices[i].port) {

            clients[socket_id].devices.erase(clients[socket_id].devices.begin() + i);    
        }   
    }  
    if(clients[socket_id].devices.size() < 1) {
        close(socket_id);
        printf("All %s were disconnect.\n", clients[socket_id].user_name);
        pthread_mutex_unlock(&clients_timesOnline);
        pthread_exit(NULL);
    }
    pthread_mutex_unlock(&clients_timesOnline);
    printf("One of %s was disconnect, but still have %ud connected\n", clients[socket_id].user_name, clients[socket_id].devices.size());
}

int get_file_amount(int socket_id) {

    DIR *dir;
    struct dirent *ent;
    int i = 0;
    if ((dir = opendir (clients[socket_id].user_name)) != NULL) {
    /* print all the files and directories within directory */
    while ((ent = readdir (dir)) != NULL) {
        i++;
    }
    closedir (dir);
    } else {
        perror ("");
        return EXIT_FAILURE;
    }

    return i;
}
 
int send_current_files(int socket_id, struct sockaddr_in *si_other, unsigned int slen, int packet_id) {
    Packet packet;
    Ack ack;
    ServerItem cur_item;
    char buf[PACKET_SIZE];
    char serialized_info[DATA_PACKET_SIZE];
    DIR *dir;
    struct dirent *ent;
    char full_path[80];
    char file_name[80];
    int i = 0;
    struct stat file_stat;
    dir = opendir(clients[socket_id].user_name);

    if (dir) {
        
        while ((ent = readdir(dir)) != NULL) {
            strcpy(file_name, ent->d_name);
            if(strcmp(file_name, "..") != 0 && strcmp(file_name, ".") != 0) {
                strcpy(full_path, clients[socket_id].user_name);
                strcat(full_path, "/");
                strcat(full_path, file_name);

                if(lstat(full_path, &file_stat) < 0)    
                    printf("Error: cannot stat file <%s>\n", full_path);
                    
                cur_item.atime = file_stat.st_atime;
                cur_item.mtime = file_stat.st_mtime;
                cur_item.ctime = file_stat.st_ctime;
                cur_item.size = file_stat.st_size;
                strcpy(cur_item.name, file_name);
                
                
               // file_info_serialize(file_stat, serialized_info, file_name, i);
                memcpy(serialized_info, &cur_item, sizeof(ServerItem));
                create_packet(&packet, Data_type, packet_id, 0, serialized_info);
                await_send_packet(&packet, &ack, buf, socket_id, si_other, slen);
                i++;
                packet_id++;
            }
        }
        closedir(dir);

    }
    else {
        printf("Error: cannot open diretory %s\n", full_path);
    }


    return packet_id;
}

char *file_info_serialize(struct stat info, char *serialized_info, char* file_name, int pos) {

    char buf[15];
    char time_buf[80];
    int i = 0;

    sprintf(buf, "%li", info.st_size);

    if(pos == 0) {
        strcpy(serialized_info, "Name\t\tmtime\t\t\t\tatime\t\t\t\tctime\t\t\t\tSize(Bytes)\n");
        strcat(serialized_info, "--------------------------------------------------------------------------------------------------------------------------------\n");
        strcat(serialized_info, file_name);
    } else {
        strcpy(serialized_info, file_name);
    }
    
    strcat(serialized_info, "\t");
    if(strlen(file_name) < 7)
        strcat(serialized_info, "\t");

    stat("alphabet",&info);
    strcat(serialized_info, ctime(&info.st_mtime));
    serialized_info[strlen(serialized_info) -1 ] = '\t';
    strcat(serialized_info, ctime(&info.st_atime));
    serialized_info[strlen(serialized_info) -1 ] = '\t';    
    strcat(serialized_info, ctime(&info.st_ctime));
    serialized_info[strlen(serialized_info) -1 ] = '\t';
    strcat(serialized_info, buf);
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

void bind_user_to_server(char *user_name, int socket_id, uint32_t port, ClientDevice client_device) {

    Client client;

    client.portListening = port;
    strcpy(client.user_name, user_name);
    client.devices.push_back(client_device);
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
    struct stat stat_buffer;
    struct tm *timeinfo;
    char buffer[80];
    int exists = false;
    
     int end_position = strlen(file_name) + 1;

	memcpy(&(file->times), file_name + end_position,  sizeof(struct stat));

    time (&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer,sizeof(buffer),"%d-%m-%Y %I:%M:%S",timeinfo);

	printf("%s\n", file_name);
    for (std::list<struct file_info>::iterator iterator = clients[socket_id].info.begin(), 
            end = clients[socket_id].info.end(); iterator != end; ++iterator) {
	printf("%saa\n", ctime(&((*iterator).times.st_mtime)));
        if(strcmp((*iterator).name, file_name) == 0) {
            memcpy(&((*iterator).times), file_name + end_position,  sizeof(struct stat));   
            exists = true;
        }
    }

    strcpy(file->name, file_name);
    file->size = file_size;

    return exists;
}

//---------------------------------------------------------------------------------------------------------------------------
//--------------------------------BACKUP CODE -------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
int backup_sokcet_id_handle_leader;
int main_backup_server(int this_server_port, char* server_from_leader, int leader_port, char* server_from_backup) { 
    tell_leader_that_backup_exists(this_server_port, leader_port, server_from_leader, server_from_backup);

    pthread_t thread_to_handle_leader;
    pthread_create(&thread_to_handle_leader, NULL, backup_handle_leader, (void*) &this_server_port);

    struct sockaddr_in si_other;
    unsigned int slen;    
    slen = sizeof(si_other);
    int socket_id;

    socket_id = init_socket_to_send_packets(leader_port, server_from_leader, &si_other);

    char buf[PACKET_SIZE];
    Packet packet;
    Ack ack;

    int leader_exist = true;
    int id = 0;
    while(leader_exist) {
        sleep(6);
        create_packet(&packet, Is_Leader_Alive_Type, id, 0, buf); //sdds construtor
        leader_exist = try_to_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);
        id++;
    }

    if (knew_leader_is_dead) {
        printf("Someone told me the leader is death!!!\n");
    }
    else {
        knew_leader_is_dead = true;
        printf("I discover that the leader is death!!!\n");
        close(backup_sokcet_id_handle_leader);
        pthread_cancel(thread_to_handle_leader);

        packet.packet_type = Leader_Is_Dead_Type;
        packet.packet_id = 1;
        printf("I will tell to the rest %ud!!!\n", backups.size());
        send_packet_to_backups(packet, backups);
    }
    
    set_new_leader(this_server_port, server_from_backup);
}

void* backup_handle_leader(void *args) {
    knew_leader_is_dead = false;
    int *my_port = (int*) args;
    int this_server_port = *my_port;

    int recv_len;
    struct sockaddr_in si_other;
    unsigned int slen = sizeof(si_other);
    Packet packet;
    Ack ack;
    char path_file[MAXNAME];
    struct file_info file_metadata;
    struct utimbuf file_times;

    //create a UDP socket
    //struct sockaddr_in si_me;
    int socket_id = init_socket_to_receive_packets(this_server_port, &si_other);
    backup_sokcet_id_handle_leader = socket_id;
    //keep listening for data
    printf("Listening on port %d...\n", this_server_port);
    while(!knew_leader_is_dead) {
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(socket_id, &packet, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)

        //print details of the client/peer and the data received
        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

        
        strcpy(path_file, packet.data);
        switch(packet.packet_type) {
            case Client_login_type:
                backup_dealing_login(packet, socket_id, &si_other, slen);
                break;

            case New_Backup_Server_Type:
                add_new_backup(packet.data, packet.packet_info, packet.packet_id, socket_id, &si_other, slen);
                break;

            case Header_type:
                create_ack(&ack, packet.packet_id, packet.packet_info);
                send_ack(&ack, socket_id, &si_other, slen);
                receive_file(path_file, packet.packet_info, packet.packet_id, socket_id, false, backups);

                if(!get_file_metadata(&file_metadata, packet.data, socket_id, packet.packet_info))
                    clients[socket_id].info.push_back(file_metadata);
				
				file_times.modtime = file_metadata.times.st_mtime;
				file_times.actime = file_metadata.times.st_atime;
				
				utime(path_file, &file_times);
                break;
            
            case Client_exit_type:
                create_ack(&ack, packet.packet_id, 0);
                send_ack(&ack, socket_id, &si_other, slen);

                sub_client_to_backup_vector(packet.packet_info, packet.data);
                break;
            
            case Delete_type:
                create_ack(&ack, packet.packet_id, 0);
                send_ack(&ack, socket_id, &si_other, slen);

                if(remove(path_file) != 0)  {
                    printf("Error: unable to delete the file %s\n", path_file);
                }
                break;
            
            case Leader_Is_Dead_Type:            
                create_ack(&ack, packet.packet_id, 0);
                send_ack(&ack, socket_id, &si_other, slen);

                knew_leader_is_dead = true;
                close(socket_id);
                
                break;

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

    socket_id = init_socket_to_send_packets(leader_port, server_from_leader, &si_other);

    char buf[PACKET_SIZE];
    Packet packet;
    Ack ack;

    //TODO: review id on create_packet function
    create_packet(&packet, New_Backup_Server_Type, this_server_port + leader_port, this_server_port, server_from_backup); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

    if((uint8_t)buf[0] == Ack_type) {
        printf("The leader server knows that this server exist. My id is %d\n", ack.info);
    }
    else {
       kill("We failed to tell leader server that this backup server exist. Try again later!\n");
    }
    process_value_election = ack.info;
    close(socket_id);
}

void backup_dealing_login(Packet packet, int socket_id, struct sockaddr_in *si_other, unsigned int slen) {
 
    char host[MAXNAME];
    strcpy(host, packet.data);

    int packet_id = packet.packet_id;
    int status = check_login_status(host);
    pthread_t logged_client;
    int *new_socket_id = (int *) malloc (sizeof(int));
    Ack ack;
    ack.packet_type = Ack_type;
    ack.packet_id = packet_id;
    int is_online = backup_check_if_online(host);
    ClientDevice client_device;
    client_device.port = packet.packet_info;
    strcpy(client_device.device, packet.data);

    if(is_online) {
        backup_clients[is_online-1].devices.push_back(client_device);

    } else {
        add_client_to_backup_vector(host, client_device);
    }

    switch(status) {
        case Old_user:
            printf("Logged in!\n");
            break;
        case New_user:
            printf("Creating new user for %s\n", host);
            create_new_user(host);
            break;

        default: printf("Failed to login.\n");
    }
    send_ack(&ack, socket_id, si_other, slen);
    std::cout << "Client "<< host <<" logged in. So there are "<< backup_clients[is_online-1].devices.size() <<" devices of this user online.\n"
              << "Backup knows " << backup_clients.size() <<" diff connected users.\n" ;
}

int backup_check_if_online(char *host) {
    for (int i = 0; i < backup_clients.size(); i++) {
        if(strcmp(backup_clients[i].user_name, host) == 0) {
            return i+1;
        }
    }
    return 0;
}

void add_client_to_backup_vector(char *user_name, ClientDevice client_device) {

    Client client;

    client.portListening = 0; //not important
    strcpy(client.user_name, user_name);
    client.devices.push_back(client_device);
    backup_clients.push_back(client);
}

void sub_client_to_backup_vector(int client_port, char* client_device) {
    int is_found = false;
    for (int i = 0; i < backup_clients.size() && !is_found; i++) {
        for(int j = 0; j < backup_clients[i].devices.size() && !is_found; j++)
        {
            if(strcmp(client_device, backup_clients[i].devices[j].device) == 0 && client_port == backup_clients[i].devices[j].port) {

                backup_clients[i].devices.erase(backup_clients[i].devices.begin() + j); 
                j--;
                is_found = true;
                std::cout << "Client "<< backup_clients[i].user_name <<" logged out. So there are "<< backup_clients[i].devices.size() <<" devices of this user online.\n"
                        << "Backup knows " << backup_clients.size() <<" diff connected users.\n" ;
            }      
        } 
    }
}

void add_new_backup(char *server_from_backup, int backup_port, uint32_t packet_id, int socket_id, struct sockaddr_in *si_other, unsigned int slen) {

    Ack ack;
    ack.packet_type = Ack_type;
    ack.packet_id = packet_id;

    BackupServer new_backup;
    new_backup.port = backup_port;
    new_backup.value_election = packet_id; //<<<<<<<this is the diff between func
    strcpy(new_backup.server, server_from_backup);
    backups.push_back(new_backup);

    std::cout << "New backup (id= " << packet_id << ") " << server_from_backup << ":" << backup_port <<"\nBackup list size = " << backups.size() << '\n';

    send_ack(&ack, socket_id, si_other, slen);
}

void set_new_leader(int this_server_port, char* server_from_backup) {
    BackupServer greater_backup = greatest_backup_server();
    if (greater_backup.value_election == process_value_election) {
        printf("I'm the new leader!\n");
        create_all_clients_threads(this_server_port, server_from_backup);
        backups.clear();
        main_leader_server(this_server_port);
    }
    else {
        printf("I am NOT the new leader!\n");
        backups.clear();
        main_backup_server(this_server_port, greater_backup.server, greater_backup.port, server_from_backup);
    }
}

BackupServer greatest_backup_server() { 

    BackupServer greater;
    greater.value_election = process_value_election;
    for(int i = 0; i < backups.size(); i++)
    {
        if(greater.value_election < backups[i].value_election)
            greater = backups[i];       
    }   
        printf("I am = %d and the greater is = %d!\n", process_value_election, greater.value_election);
    return greater; 
}

void create_all_clients_threads(int this_server_port, char* device) {
    
    Packet packet;
    Ack ack;
    struct file_info file_metadata;
    struct sockaddr_in si_client;
    unsigned int slen = sizeof(si_client);
    char buf[PACKET_SIZE];

    for(int i = 0; i < backup_clients.size(); i++) {

        for(int j = 0; j < backup_clients[i].devices.size(); j++) {
            int client_device_socket_id = init_socket_to_send_packets(backup_clients[i].devices[j].port, backup_clients[i].devices[j].device, &si_client);
            create_packet(&packet, New_Leader_type, i, this_server_port, device);
            await_send_packet(&packet, &ack, buf, client_device_socket_id, &si_client, slen);
        }
    }

}
