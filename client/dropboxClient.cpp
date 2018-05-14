#include "dropboxClient.h"

struct sockaddr_in si_other;
int socket_id;
unsigned int slen;
uint32_t next_id = 0;
char *USER;
pthread_mutex_t busy_client = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t syncing_client = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {
    
    char command[COMMAND_LENGTH];
    char command_parameter[COMMAND_LENGTH];

    USER = argv[1];
    char *SERVER = argc >= 3 ? argv[2] : SERVER_DEFAULT;
    int PORT = argc >= 4 ? atoi(argv[3]) : DEFAULT_PORT;
    int action;
    char full_path[MAXNAME+10];
    
    slen = sizeof(si_other);

    socket_id = init_socket_client(PORT, SERVER, &si_other);
    login_server(USER, PORT);
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
                close_session();
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
    int packets_received = 0;
    int packets_to_receive;

    create_packet(&packet, List_type, get_id(), 0, ""); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

    packets_to_receive = ack.util;

    if(packets_to_receive == 0) {
        printf("Your remote sync_dir is empty, upload your first file !\n");
        return;
    }

    do {
        receive_packet(buf, socket_id, &si_other, &slen);
        memcpy(&packet, buf, PACKET_SIZE);
        create_ack(&ack, packet.packet_id, packet.packet_info);
        if(buf[0] == Data_type) {
            printf("%s", packet.data);
            packets_received++;
        }
        send_ack(&ack, socket_id, &si_other, slen);
    } while(packets_received < packets_to_receive);

}

int login_server(char *host, int port) {

    char buf[PACKET_SIZE];
    Packet login_packet;
    Ack ack;
    char *full_path = (char*) malloc(sizeof(char) * MAXNAME+10);
    pthread_t daemon, syncServer;

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

    socket_id = init_socket_client(ack.info, SERVER_DEFAULT, &si_other);
    
    sync_client();
    
    pthread_create(&daemon, NULL, sync_daemon, (void*) full_path);
    
    //Maybe should return the packet id ?
    return 0;
}

void* sync_daemon (void *args) {
	
	char *folder_path = (char *) args;
	int f = inotify_init();
	int wd = inotify_add_watch(f, folder_path, IN_MODIFY | IN_CLOSE_WRITE);
 
	if (wd == -1)
		kill ("Creating watch failed.\n");
	
	while(true) {
		
		sleep(3);
		
		char buffer [BUF_LEN];
		
		int length = read( f, buffer, BUF_LEN ); 
 
		if ( length < 0 ) {
			kill ("Read error.\n");
		} 
	 
		pthread_mutex_lock(&syncing_client);
	 
		int i = 0;
		while ( i < length ) {
			struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
			if ( event->len ) {
				
			
				if ( event->mask & IN_CREATE) {
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
							printf( "The file %s was modified\n", filepath_send);   // for txt      
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
						printf( "1 The file %s was modified\n", filepath_send);   // for txt
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
							printf( "2 The file %s was modified\n", filepath_send);  
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
		
		}
		
		pthread_mutex_unlock(&syncing_client);
	}
	

	
	inotify_rm_watch( f, wd );
	close( f );
	
}

void send_file(char *file) {
	pthread_mutex_lock(&busy_client);
	printf("upload pegou o mutex\n");
    next_id = send_file_chunks(file, socket_id, &si_other, slen, get_id(), 's');
    printf("upload liberou o mutex\n");
	pthread_mutex_unlock(&busy_client);
}

void get_file(char *file, int local) { // 0 = local, 1 = sync_dir
    char buf[PACKET_SIZE];
    char full_path[MAXNAME+10];
    Packet packet;
    Ack ack;
    
    
	pthread_mutex_lock(&busy_client);
	printf("download pegou o mutex\n");

    create_packet(&packet, Download_type, get_id(), 0, file); //sdds construtor
    await_send_packet(&packet, &ack, buf, socket_id, &si_other, slen);

    if((uint8_t)buf[0] == Ack_type) {
        memcpy(&ack, buf, sizeof(ack));
    }
    
    if (local == 1)
		get_sync_path(full_path, USER, file);
	else {
		strcat(full_path, "./");
		strcat(full_path, file);
	}

    if(ack.util >= 0)
        receive_file(full_path, ack.util, 0, socket_id);
    else
        printf("This file does not exists!\n");
        
    printf("download liberou o mutex\n");
    pthread_mutex_unlock(&busy_client);
}

void close_session() {
    char exit_message[COMMAND_LENGTH];
    
    close(socket_id);
    sprintf(exit_message, "Goodbye %s!\n", USER);
    kill(exit_message);
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
