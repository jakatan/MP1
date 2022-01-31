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
#define MAX_NAME 64

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

typedef struct{
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[MAX_NAME];
}client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


void str_overwrite_stdout(){
    printf("\r%s",">");
    fflush(stdout);
}

void str_trim_lf(char* arr, int length){
    for (int i = 0; i < length; i++){
        if(arr[i] == '\n'){
            arr[i] == '\0';
            break;
        }
    }
}

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

void *handle_client(void*arg){
    char buffer[MAX_BUFFER];
    char name[MAX_NAME];
    int leave = 0;
    cli_count++;
    
    client_t *cli = (client_t*)arg;
    
    //name
    if(recv(cli->sockfd, name, MAX_NAME, 0) <= 0 || strlen(name) < 2 || strlen(name) >= MAX_NAME -1){
        printf("Enter the name correctly\n");
        leave = 1;
    }else{
        strcpy(cli -> name, name);
        sprintf(buffer, "%s has joined\n", cli->name);
        printf("%s", buffer);
        send_message(buffer, cli->uid);
    }
    
    bzero(buffer, MAX_BUFFER);
    
    while(1){
        if(leave){
            break;
        }
        
        int receive = recv(cli->sockfd, buffer, MAX_BUFFER, 0);
        
        if(receive > 0 ){
            if(strlen(buffer)>0){
                send_message(buffer, cli->uid);
                str_trim_lf(buffer, strlen(buffer));
                printf("%s -> %s", buffer, cli->name);
            }
        } else if (receive == 0 || strcmp(buffer, "exit") == 0){
            sprintf(buffer, "%s has left the room", cli -> name);
            printf("%s", buffer);
            send_message(buffer, cli->uid);
            leave = 1;
        }else{
            printf("ERROR: -1\n");
            leave = 1;
        }
        bzero(buffer, MAX_BUFFER);
    }
    
    close(cli -> sockfd);
    queue_remove(cli -> uid);
    free(cli);
    cli_count--;
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
	    
	    // check if client count is almost max
	   if((cli_count + 1 ) == MAX_CLIENTS){
	       printf("Maximum clients are connected.");
	       print_ip_addr(cli_addr);
	       close(connfd);
	       continue;
	   }
	   
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