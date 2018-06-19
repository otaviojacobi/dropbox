#include "dropboxClient.h"

struct sockaddr_in si_other;
int socket_id;
int inotify_f;
int inotify_wd;
unsigned int slen;
uint32_t next_id = 0;
char *USER;
char *SERVER;
pthread_mutex_t busy_client = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t syncing_client = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {
    
    char command[COMMAND_LENGTH];
    char command_parameter[COMMAND_LENGTH];

    USER = argv[1];
    SERVER = argc >= 3 ? argv[2] : SERVER_DEFAULT;
    int PORT = argc >= 4 ? atoi(argv[3]) : FRONTEND_DEFAULT_PORT;
    int action;
    char full_path[MAXNAME+10];
    
    slen = sizeof(si_other);

    socket_id = init_socket_to_send_packets(PORT, SERVER, &si_other);
    printf("received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        
    login_server(USER);
    print_info(USER, "1.0.0");

    while(true) {

        printf(">> ");
        scanf("%s", command);

        action = command_to_action(command);

        switch(action) {

            case Upload:
                scanf("%s", command_parameter); // files names are not allowed to have spaces. TODO: fix this
                send_file(command_parameter);
                break;

            case Download:
                scanf("%s", command_parameter);
                get_file(command_parameter, 0);
                break;

            case List_server:            
                list_server();            
                break;

            case List_client:            
                list_client();           
                break;

            case Get_sync_dir:            
                sync_client();       
                break;

            case Exit:
                log_out_and_close_session();
                break;

            case Delete:
                scanf("%s", command_parameter);
                delete_file(command_parameter);
                break;

            default: printf("%s command not found.\n", command);
        }
    }
 
    close(socket_id);
    return 0;
}

void list_client() {
    DIR *dir;
    struct dirent *dirStruct;
    char full_path[80];
    char file_name[80];
    struct stat file_stat;

    strcpy(full_path, "sync_dir_");
    strcat(full_path, USER);
    dir = opendir(full_path);
    char buf[15];


    if (dir) {
        printf("Name\t\tmtime\t\t\t\tatime\t\t\t\tctime\t\t\t\tSize(Bytes)\n");
        printf("--------------------------------------------------------------------------------------------------------------------------------\n");
        
        while ((dirStruct = readdir(dir)) != NULL) {
            strcpy(file_name, dirStruct->d_name);
            strcpy(full_path, "sync_dir_");
            strcat(full_path, USER);
            strcat(full_path, "/");
            strcat(full_path, file_name);
            if(lstat(full_path,&file_stat) < 0)    
                printf("Error: cannot stat file <%s>\n", full_path);
            else if(strcmp(file_name, "..") != 0 && strcmp(file_name, ".") != 0){
                printf("%s\t", file_name);
                if(strlen(file_name) < 7) printf("\t");
                print_time(file_stat.st_mtime);
                printf("\t");
                print_time(file_stat.st_atime);
                printf("\t");
                print_time(file_stat.st_ctime);
                printf("\t");
                sprintf(buf, "%li", file_stat.st_size);
                printf("%s\n", buf);
            }
        }
        closedir(dir);

    }
    else {
        printf("Error: cannot open diretory %s\n", full_path);
    }
}

void sync_client() {
    char buf[PACKET_SIZE];
    Packet packet;
    Ack ack;
    ServerItem cur_item;
    int packets_received = 0;
    int packets_to_receive;
    char file_names[MAXFILES][MAXNAME];
    char file_name[MAXNAME];
    int cur_char, cur_split = 0, cur_file = 0;

	pthread_mutex_lock(&syncing_client);
	
    create_packet(&packet, List_type, get_id(), 0, ""); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

    packets_to_receive = ack.util;

    if(packets_to_receive == 0) {
        printf("Your remote sync_dir is empty, upload your first file !\n");
        pthread_mutex_unlock(&syncing_client);
        return;
    }


    do {
        receive_packet(buf, socket_id, &si_other, &slen);
        memcpy(&packet, buf, PACKET_SIZE);
        create_ack(&ack, packet.packet_id, packet.packet_info);
        if(buf[0] != Data_type) 
			kill("Something really wrong happened.n");
	
		memcpy(&cur_item, packet.data, sizeof(ServerItem));
        strcpy(file_name, cur_item.name);
            
        strcpy(file_names[packets_received], file_name);
        packets_received++;
        send_ack(&ack, socket_id, &si_other, slen);
    } while(packets_received < packets_to_receive);
    
    for(cur_file = 0; cur_file < packets_received; cur_file++) {
        get_file(file_names[cur_file], 1);
    }
    
    pthread_mutex_unlock(&syncing_client);
}

void list_server(void) {
    char buf[PACKET_SIZE];
    Packet packet;
    Ack ack;
    ServerItem cur_item;
    int packets_received = 0;
    int packets_to_receive;

    create_packet(&packet, List_type, get_id(), 0, ""); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

    packets_to_receive = ack.util;

    if(packets_to_receive == 0) {
        printf("Your remote sync_dir is empty, upload your first file !\n");
        return;
    }

	printf("Name\t\tmtime\t\t\t\tatime\t\t\t\tctime\t\t\t\tSize(Bytes)\n");
    printf("--------------------------------------------------------------------------------------------------------------------------------\n");
    
    do {
        receive_packet(buf, socket_id, &si_other, &slen);
        memcpy(&packet, buf, PACKET_SIZE);
        create_ack(&ack, packet.packet_id, packet.packet_info);
        if(buf[0] == Data_type) {
            memcpy(&cur_item, packet.data, sizeof(ServerItem));
			
			print_item_info(cur_item);
			
            packets_received++;
        }
        send_ack(&ack, socket_id, &si_other, slen);
    } while(packets_received < packets_to_receive);

}

void print_item_info (ServerItem item) {
	char m[50];
	char a[50];
	char c[50];
	printf("%s", item.name);
	printf("\t");
    if(strlen(item.name) < 7)
        printf("\t");

	strcpy(m, ctime(&item.mtime));
	strcpy(a, ctime(&item.atime));
	strcpy(c, ctime(&item.ctime));
	
	m[strlen(m) - 1] = 0;
	a[strlen(a) - 1] = 0;
	c[strlen(c) - 1] = 0;
	
    printf("%s\t", m);
    printf("%s\t", a); 
    printf("%s\t", c);
    
    printf("%li", item.size);
    printf("\n");
}

int login_server(char *host) {

    char buf[PACKET_SIZE];
    Packet login_packet;
    Ack ack;
    char *full_path = (char*) malloc(sizeof(char) * MAXNAME+10);
    pthread_t daemon, syncserver;

    get_sync_path(full_path, host, "");
    
    DIR* dir = opendir(full_path);

    if (dir)
        closedir(dir);
    else // Directory does not exist. 
        mkdir(full_path, 0700);

    
    create_packet(&login_packet, Client_login_type, get_id(), 0, host); //sdds construtor
    await_send_packet(&login_packet, &ack, buf, socket_id, &si_other, slen);

    if((uint8_t)buf[0] == Ack_type) {
        memcpy(&ack, buf, sizeof(ack));
        if ( ack.util == New_user ) {
            printf("This is your first time! We're creating your account...\n");

        } else {
            printf("Loggin you in...\n");
        }
    }
    else {
       kill("We failed to log you in. Try again later!\n");
    }
    /*
    sync_client();
    
    inotify_f = inotify_init();
    pthread_create(&daemon, NULL, sync_daemon, (void*) full_path);
    pthread_create(&syncserver, NULL, sync_server, (void*) full_path);
    */
    //Maybe should return the packet id ?
    return 0;
}

void start_watch (char* folder_path) {
	inotify_wd = inotify_add_watch(inotify_f, folder_path, IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE);
}

void stop_watch (char* folder_path) {
	inotify_rm_watch( inotify_f, inotify_wd );
}

void* sync_daemon (void *args) {
	
	char *folder_path = (char *) args;
	
	start_watch(folder_path);
 
	if (inotify_wd == -1)
		kill ("Creating watch failed.\n");
	
	while(true) {
		
		sleep(3);
		
		char buffer [BUF_LEN];
		
		int length = read( inotify_f, buffer, BUF_LEN ); 
 
		if ( length < 0 ) {
			kill ("Read error.\n");
		} 
		pthread_mutex_lock(&syncing_client);
	 
		int i = 0;
		while ( i < length ) {
			struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
			if ( event->len ) {
				
			
				if ( event->mask && (IN_CREATE || IN_MOVED_TO)) {
					if (event->mask & IN_ISDIR)
						printf( "The directory %s was Created.\n", event->name );      
					else
						if (((event->name)[0] != '.') || ((event->name)[1] != 'g')) {
							char *filepath_send = (char*) malloc(sizeof(char) * MAXNAME+10);
							strcpy(filepath_send, folder_path);
							strcat(filepath_send, "/");
							strcat(filepath_send, event->name);
							send_file(filepath_send);  
							free(filepath_send);   
						}
				}
			
			   
				if ( event->mask & IN_CLOSE_WRITE) {
					if (event->mask & IN_ISDIR)
						printf( "The directory %s was modified.\n", event->name );      
					else {
						char *filepath_send = (char*) malloc(sizeof(char) * MAXNAME+10);
						strcpy(filepath_send, folder_path);
						strcat(filepath_send, "/");
						strcat(filepath_send, event->name);
						send_file(filepath_send);  
						free(filepath_send);
					}
				}
				else if ( event->mask & IN_MODIFY) {
					if (event->mask & IN_ISDIR)
						printf( "The directory %s was modified.\n", event->name );      
					else {
						if (((event->name)[0] != '.') || ((event->name)[1] != 'g')) {
							char *filepath_send = (char*) malloc(sizeof(char) * MAXNAME+10);
							strcpy(filepath_send, folder_path);
							strcat(filepath_send, "/");
							strcat(filepath_send, event->name);
							send_file(filepath_send);  
							free(filepath_send);
						}
					}
				}
	//		   
	//			if ( event->mask & IN_DELETE) {
	//				if (event->mask & IN_ISDIR)
	//					printf( "The directory %s was deleted.\n", event->name );      
	//				else
	//					printf( "The file %s was deleted with WD %d\n", event->name, event->wd );      
	//			} 
			
	//		
				i += EVENT_SIZE + event->len;
			}
			else {
				
				break;
				
			}
		
		}
		
		pthread_mutex_unlock(&syncing_client);
	}
	

	
	inotify_rm_watch( inotify_f, inotify_wd );
	close( inotify_f );
	
}

void* sync_server (void *args) {
	
	char *folder_path = (char *) args;
	
	while (true) {
		char buf[PACKET_SIZE];
		Packet packet;
		Ack ack;
		ServerItem items[MAXFILES];
		ServerItem cur_item;
		int packets_received = 0;
		int packets_to_receive;
		int item_actions[MAXFILES];
		
		sleep(10);
		pthread_mutex_lock(&syncing_client);
		
		create_packet(&packet, List_type, get_id(), 0, "");
		await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

		packets_to_receive = ack.util;

		if(packets_to_receive != 0) {
			do {
				receive_packet(buf, socket_id, &si_other, &slen);
				memcpy(&packet, buf, PACKET_SIZE);
				create_ack(&ack, packet.packet_id, packet.packet_info);
				if(buf[0] == Data_type) {
					memcpy(&cur_item, packet.data, sizeof(ServerItem));
					
					items[packets_received] = cur_item;
					
					item_actions[packets_received] = 0;
					
					packets_received++;
				}
				send_ack(&ack, socket_id, &si_other, slen);
			} while(packets_received < packets_to_receive);
		}
		
		for (int i = 0; i < packets_to_receive; i++) {
			char filename [MAXNAME];
			struct stat buffer;
			strcpy(filename, folder_path);
			strcat(filename, "/");
			strcat(filename, items[i].name);
			
			if (stat(filename, &buffer) == -1)
				item_actions[i] = 1;
			else {
				if (difftime(buffer.st_mtime, items[i].mtime) < 0) // positivo quando tá mais nova que o server
					  item_actions[i] = 1;
			}
		}
		
		for (int i = 0; i < packets_to_receive; i++) {
			if (item_actions[i]) {
				stop_watch(folder_path);
				get_file(items[i].name, 1);
				start_watch(folder_path);
				
			}
		}
		
		pthread_mutex_unlock(&syncing_client);
	}
	
	
}

void send_file(char *file) {
	pthread_mutex_lock(&busy_client);
    next_id = send_file_chunks(file, socket_id, &si_other, slen, get_id(), 's');
	pthread_mutex_unlock(&busy_client);
}

void get_file(char *file, int local) { // 0 = local, 1 = sync_dir
    char buf[PACKET_SIZE];
    char full_path[MAXNAME+10];
    Packet packet;
    Ack ack;
    
    
	pthread_mutex_lock(&busy_client);

    create_packet(&packet, Download_type, get_id(), 0, file); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

    if((uint8_t)buf[0] == Ack_type) {
        memcpy(&ack, buf, sizeof(ack));
    }
    
    strcpy(full_path, "");
    if (local == 1)
		get_sync_path(full_path, USER, file);
	else {
		strcat(full_path, "./");
		strcat(full_path, file);
	}

    if(ack.util >= 0) {
        std::vector<BackupServer> null;
        receive_file(full_path, ack.util, 0, socket_id, false, null);
    }
    else
        printf("This file does not exists!\n");
        
    pthread_mutex_unlock(&busy_client);
}

void close_session() {
    char exit_message[COMMAND_LENGTH];
    
    close(socket_id);
    sprintf(exit_message, "Goodbye %s!\n", USER);
    kill(exit_message);
}

void log_out_and_close_session() {
    Packet packet;
    Ack ack;
    char buf[PACKET_SIZE];

    create_packet(&packet, Client_exit_type, get_id(), 0, "NULL"); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);
    
    close_session();
}

void delete_file(char *file) {
    char buf[PACKET_SIZE];
    char full_path[MAXNAME+10];
    Packet packet;
    Ack ack;

    create_packet(&packet, Delete_type, get_id(), 0, file); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);	
}

uint32_t get_id() {
    return ++next_id;
}

void print_time(long int stat_time) {
    time_t t = stat_time;
    struct tm lt;
    localtime_r(&t, &lt);
    char timbuf[80];
    strftime(timbuf, sizeof(timbuf), "%c", &lt);
    printf("%s", timbuf);
}
