#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#include "interface.h"

#define MAX_CLIENTS 256
#define MAX_BUFFER 2048

static int uid = 10;


room room_db[MAX_CLIENTS];

typedef struct{
    struct sockaddr_in address;
    int sockfd;
    int uid;
}client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void queue_add(client_t *cl){
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++){
        if(!clients[i]){
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void queue_remove(int uid){
    pthread_mutex_lock(&clients_mutex);
    
  for(int i = 0; i < MAX_CLIENTS; i++){
      if(clients[i]){
          if(clients[i]->uid == uid){
              clients[i] = NULL;
              break;
          } 
      }
        
    }
    pthread_mutex_unlock(&clients_mutex);
}
void print_ip_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d" , addr.sin_addr.s_addr & 0xff, (addr.sin_addr.s_addr & 0xff00) >> 8, (addr.sin_addr.s_addr & 0xf0000) >> 16, (addr.sin_addr.s_addr &0xff000000) >> 24);
}

void send_message(char* s, int uid){
    pthread_mutex_lock(&clients_mutex);
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i] -> uid != uid){
                if(write(clients[i]->sockfd, s, strlen(s))<0){
                    printf("ERROR: writing to the descriptor failed");
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(int *dbValue, int sockfd){
    char buffer[MAX_BUFFER];
    int leave = 0;
    
    bzero(buffer, MAX_BUFFER);
    
    while(1){
        if(leave){
            break;
        }
        
        int receive = recv(sockfd, buffer, MAX_BUFFER, 0);
        char* commandType = strtok(buffer," ");
        char buffer_Sent[256];
        int sent_Amount;
        char* status;
        if(receive > 0){
            if(!strcmp(commandType, "CREATE")){ // create room
                char* roomName = strtok(NULL, " ");
                for (int i = 0; i < strlen(room_db); i++){ // search if room exists
                    if(!strcmp(roomName,room_db[i].room_name)){
                        status = "FAILURE_ALREADY_EXISTS";
                    }
                }
                int slaveSock = socket (AF_INET, SOCK_STREAM, 0); // create new room + socket
                room newRoom;
                strcpy(newRoom.room_name, roomName);
                newRoom.port_num = "127.0.0.1"; // might need to change
                newRoom.num_members = 1;
                strcpy(newRoom.slave_socket,slaveSock);
                
                room_db[*dbValue] = newRoom; // add room to databse, iterate counter
                dbValue++;
                status = "SUCCESS";
                strcpy(buffer_Sent, status);
            }else if (!strcmp(commandType, "DELETE") ){
                int found = 0;
                char* roomName = strtok(NULL, " ");
                for (int i = 0; i < strlen(room_db); i++){ // search if room exists
                    if(!strcmp(roomName,room_db[i].room_name)){
                        strcpy(room_db[i].room_name, "");
                        strcpy(room_db[i].port_num, "");
                        strcpy(room_db[i].num_members, 0);
                        strcpy(room_db[i].slave_socket, 0);
                    }
                } 
                if(!found){
                    status = "FAILURE_NOT_EXISTS";
                }else{
                    status = "SUCCESS";
                }
                strcpy(buffer_Sent, status); // if room already exists update status
            }else if(!strcmp(commandType, "JOIN") ){
                int found = 0;
                char* roomName = strtok(NULL, " ");
                for (int i = 0; i < strlen(room_db); i++){ // search if room exists
                    if(!strcmp(roomName,room_db[i].room_name)){
                        status = "SUCCESS ";
                        strcat(status, room_db[i].num_members+1);
                        strcat(status, room_db[i].port_num);
                        room_db[i].num_members = room_db[i].num_members+1;  // update room number
                        found = 1;
                    }
                } 
                if(!found){
                    status = "FAILURE_NOT_EXISTS";
                }
                strcpy(buffer_Sent, status);
            }else if(!strcmp(commandType, "LIST") ){
                int found = 0;
                if(strlen(room_db)){
                    status = "SUCCESS ";
                    for(int i = 0; i < strlen(room_db); i++){
                        strcat(status, room_db[i].room_name);
                        strcat(status, " ");
                    }
                }else{
                    status = "FAILURE_INVALID ";
                }
                strcpy(buffer_Sent,status);
            }else{
                printf("command type does not exist from server side");
            }
        }else{
            printf("ERROR: -1\n");
            leave = 1;
        }
    }
    
    pthread_detach(pthread_self());
    
    return NULL;
        
    }
int main (int argc, char **argv){
    if (argc!=2){
        printf("Usage : %s <port>\n",argv[0]);
        return EXIT_FAILURE;
    }
    
    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);
    printf("USING PORT %d \n", port);
    int option = 1;
    int listenfd = 0;
    int connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;
    
    listenfd = socket (AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    
    signal (SIGPIPE, SIG_IGN);
    
    //socket creation
    
	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
	    printf("ERROR ON SET SOCKET");
	    return EXIT_FAILURE;
	}
	
	//bind
     if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
         printf("ERROR ON BIND");
         return EXIT_FAILURE;
     }
	
	//listen
	if(listen(listenfd, 10) < 0){
	    printf("ERROR on listen");
	    return EXIT_FAILURE;
	}
	
	printf("WELCOME TO CHATROOM \n");
	
	while(1){
	    socklen_t clientlen = sizeof(cli_addr);
	    connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clientlen );
	   
	   //client settings
	   
	   client_t *cli = (client_t*)malloc(sizeof(client_t));
	   cli -> address = cli_addr;
	   cli -> sockfd = connfd;
	   cli -> uid = uid++;
	   // add a client to the queue
	   queue_add(cli);
	   pthread_create(&tid, NULL, &handle_client, (void*)cli);
	   
	   sleep(1);
	}
}