#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#include "interface.h"

#define MAX_CLIENTS 256
#define MAX_BUFFER 2048

typedef struct room{
    char room_name[256];
    int port_num, slave_socket;
    int num_members;
    int clientValue;
    int clientFDS[MAX_CLIENTS];
    
}room;

typedef struct conChat{
    struct room* room;
    int clientFD;
}conChat;

int listenfd; 
int dbValue = 0; // counter for database arrays
struct room room_db[MAX_CLIENTS]; // database for all rooms

void continue_chat(struct conChat* package){
    char* buffer[MAX_DATA];
    while(1){
        int bytes = recv(package->clientFD, buffer, sizeof(buffer), 0);
        if(bytes <= 0){
            for(int i = 0; i < sizeof(package -> room -> clientFDS); i++){
                if(package-> room->clientFDS[i]==package -> clientFD){
                    close(package -> clientFD);
                }
                exit(1);
            }
            for(int i = 0; i < sizeof(package-> room -> clientFDS); i++){
                if(package-> room->clientFDS[i] != package->clientFD){ // send to everyone but original client
                    send(package->room->clientFDS[i], buffer, bytes, 0);   
            }
        }
    }
}
}
void roomListen(struct room* tempRoom){
    while(1){
        int clientFD = accept(tempRoom -> slave_socket, NULL, NULL);
        if(clientFD<0){
            exit(1);
        }
        tempRoom -> clientFDS[tempRoom->clientValue] = clientFD;
        pthread_t thread_id;
        struct conChat* conn;
        conn->room = tempRoom;
        conn->clientFD= clientFD;
        pthread_create(&thread_id, NULL, continue_chat, conn);
        pthread_join(thread_id, NULL);
        
    }
}

int createRoom(char* roomName, int* dbValue){
    for(int i = 0; i < sizeof(room_db); i++){
        if(!strcmp(room_db[i].room_name, roomName)){ // if room exists, return 0
            return 0;
        }
    }
    
    int tempFD = socket(AF_INET, SOCK_STREAM, 0);
    if(tempFD < 0){
        printf("Unable to create socket for new room");
        return -1;
    }
    
    struct sockaddr_in tempSocket;
    tempSocket.sin_family = AF_INET;
    tempSocket.sin_addr.s_addr = htonl(INADDR_ANY);
    tempSocket.sin_port = 0;
    
    if(bind(tempFD, (struct sockaddr* ) &tempSocket, sizeof(tempSocket))< 0){
        printf("Unable to bind socket for new room");
        return -1;
    }
    
    socklen_t length = sizeof(tempSocket);
    getsockname(tempFD, (struct sockaddr*)& tempSocket, &length);
    
    
    if(listen(tempFD, 10) < 0){
        printf("Failed on room listen");
        return -1;
    }
    
    struct room *new_Room = (struct room*) malloc(sizeof(struct room));
    strcpy(new_Room->room_name,roomName);
    new_Room->slave_socket = tempFD;
    new_Room -> port_num = ntohs(tempSocket.sin_port);
    room_db[*dbValue] = *new_Room;
    dbValue++;
    
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, roomListen, new_Room);
    pthread_join(thread_id, NULL);
    
    return ntohs(tempSocket.sin_port);
}

void deleteRoom(struct room* room, char* deletemsg){
    for(int i = 0; i < sizeof(room->clientFDS); i++){
        send(room->clientFDS[i], deletemsg, strlen(deletemsg), 0);
        close(room->clientFDS[i]);
    }
    close(room->slave_socket);
}


void *handle_client(int sockfd){
    char buffer[MAX_BUFFER];
    int bytes = recv(sockfd, buffer, sizeof(buffer),0);
    
    if(bytes <=0){
        close(sockfd);
        printf("receive failed on server side");
        exit(1);
    }
    buffer[bytes] = '\0'; // make buffer null terminated
    
        char* commandType = strtok(buffer," ");
        char* roomName;
        touppercase(commandType, strlen(commandType)-1);
        printf(commandType);
            if(!strcmp(commandType, "CREATE")){ // create room, -1 and 0 indicate failure
                roomName = strtok(NULL, " ");
                printf("Command Type is: ");
                printf(commandType);
                int tempPort = createRoom(roomName, dbValue);
                struct Reply tempReply;
                if(tempPort == 0){
                    tempReply.status = FAILURE_ALREADY_EXISTS;
                } else if (tempPort == -1){
                    tempReply.status = FAILURE_UNKNOWN;
                }else{
                    tempReply.status = SUCCESS;
                }
                send(sockfd, &tempReply, sizeof(tempReply), 0);
            }else if (!strcmp(commandType, "DELETE") ){ // if room exists delete from db
                char* roomName = strtok(NULL, " ");
                struct Reply tempReply;
                for (int i = 0; i < sizeof(room_db); i++){ // search if room exists
                    if(!strcmp(roomName,room_db[i].room_name)){
                        deleteRoom(&room_db[i], "room being deleted.");
                        tempReply.status = SUCCESS;
                        send(sockfd, &tempReply, sizeof(tempReply), 0);
                        return;
                    }
                } 
                tempReply.status = FAILURE_NOT_EXISTS;
                send(sockfd, &tempReply, sizeof(tempReply), 0);
            }else if(!strcmp(commandType, "JOIN") ){ // join a room if room exists
                while((commandType = strtok(NULL, " "))!= NULL){
                    strcat(roomName,commandType);
                }
                    struct Reply tempReply;
                    for(int i = 0; i < sizeof(room_db); i++){
                        if(!strcmp(room_db[i].room_name, roomName)){
                            tempReply.status = SUCCESS;
                            tempReply.num_member = sizeof(room_db[i].clientFDS);
                            tempReply.port = room_db[i].port_num;
                            send(sockfd, &tempReply, sizeof(tempReply), 0);
                            exit(1);
                        }
                    }
                    tempReply.status = FAILURE_NOT_EXISTS;
                    send(sockfd, &tempReply, sizeof(tempReply), 0);
                    
            }else if(!strcmp(commandType, "LIST") ){ // give back names of existing rooms
                char fullList [512];
                for(int i = 0; i < sizeof(room_db); i++){
                    strcat(room_db[i].room_name, ", ");
                    strcat(fullList, room_db[i].room_name);
                }
                if(strlen(fullList)>0){
                    fullList[strlen(fullList)-2] = '\0';
                }else{
                    strcpy(fullList, "No rooms exist");
                }
                
                struct Reply tempReply;
                tempReply.status = SUCCESS;
                strcpy(tempReply.list_room, fullList);
                send(sockfd, &tempReply, sizeof(tempReply), 0);
            }else{
                struct Reply tempReply;
                tempReply.status = FAILURE_INVALID;
                send(sockfd, &tempReply, sizeof(tempReply), 0);
            }
    
            close(sockfd);    
        return NULL;
        
    }
    
void kill_server(int sign){ // in case of signal termination, kill server
    close(listenfd);
    for(int i = 0; i < sizeof(room_db); i++){
        deleteRoom(&room_db[i], "Server closing down.");
    }
    exit(1);
}

int main (int argc, char **argv){
    if (argc != 2){
        printf("Usage : %s <port>\n",argv[0]);
        exit(1);
    }
    
    int port = atoi(argv[1]);
    printf("USING PORT %d \n", port);

    listenfd = socket (AF_INET, SOCK_STREAM, 0); // initialize sockaddr
    signal(SIGINT, kill_server);
    signal(SIGTERM, kill_server);

    if(listenfd<0){
        printf("Unable to create server socket");
        exit(1);
    }
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
	
	//bind
     if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
         printf("ERROR ON BIND\n");
         return EXIT_FAILURE;
     }
	
	//listen
	if(listen(listenfd, 10) < 0){
	    printf("ERROR on listen");
	    return EXIT_FAILURE;
	}
	
	//printf("WELCOME TO CHATROOM \n");
	
	while(1){
	    pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, accept(listenfd,NULL,NULL));
        pthread_join(thread_id, NULL);
	}
	
	
}