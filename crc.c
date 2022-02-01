#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"


/*
 * TODO: IMPLEMENT BELOW THREE FUNCTIONS
 */
int connect_to(const char *host, const int port);
struct Reply process_command(const int sockfd, char* command);
void process_chatmode(const char* host, const int port);

int main(int argc, char** argv) 
{
	if (argc != 3) {
		fprintf(stderr,
				"usage: enter host address and port number\n");
		exit(1);
	}

    display_title();
    
	while (1) {
	
		int sockfd = connect_to(argv[1], atoi(argv[2]));
    
		char command[MAX_DATA];
        get_command(command, MAX_DATA);

		struct Reply reply = process_command(sockfd, command);
		display_reply(command, reply);
		
		touppercase(command, strlen(command) - 1);
		if (strncmp(command, "JOIN", 4) == 0) {
			printf("Now you are in the chatmode\n");
			process_chatmode(argv[1], reply.port);
		}
	
		close(sockfd);
    }

    return 0;
}

/*
 * Connect to the server using given host and port information
 *
 * @parameter host    host address given by command line argument
 * @parameter port    port given by command line argument
 * 
 * @return socket fildescriptor
 */
int connect_to(const char *host, const int port)
{
	  struct sockaddr_in server_addr;
	
	  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	  server_addr.sin_family = AF_INET; 
	  server_addr.sin_addr.s_addr = atoi(host); // assign hostname
	  server_addr.sin_port = htons(port); // assign the port

	  int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
		if (err == -1) {
		//printf("ERROR: connection on client-side failed\n");
		return EXIT_FAILURE;
	}

	// Send name

//	printf("=== WELCOME TO THE CHATROOM ===\n");

	return sockfd;
}

/* 
 * Send an input command to the server and return the result
 *
 * @parameter sockfd   socket file descriptor to commnunicate
 *                     with the server
 * @parameter command  command will be sent to the server
 *
 * @return    Reply    
 */
struct Reply process_command(const int sockfd, char* command)
{
	
	char* command_type = strtok(command, " "); // 
	char* roomName = strtok(NULL, " ");
	// printf("Command given is: %s" , command_type);
	// printf("Room name is: %s" , roomName);
	
	int sent_amount; // initialize buffers and sizes
	int recv_amount;
	char bufferSent[256];
	char bufferRecv[256];
	
	strcpy(bufferSent, command); // copy command to the buffer to send
	
	sent_amount = send(sockfd, bufferSent, sizeof(bufferSent), 0); // send information to server and receive response
	recv_amount = recv(sockfd, bufferRecv, sizeof(bufferRecv), 0);

	struct Reply reply;
	char* statusReturn;
	
	statusReturn = strtok(bufferRecv, " ");
	if(!strcmp(statusReturn, "SUCCESS")){ // check for type of status
		reply.status = 0;
	}else if (!strcmp(statusReturn, "FAILURE_ALREADY_EXISTS")) {
		reply.status = 1;
	}else if (!strcmp(statusReturn, "FAILURE_NOT_EXISTS")){
		reply.status = 2;
	}else if (!strcmp(statusReturn, "FAILURE_INVALID")){
		reply.status = 3;
	}else if(!strcmp(statusReturn, "FAILURE_UNKNOWN")){
		reply.status = 4;
	}else{
		printf("no reply status possible");	
	}

	if(!strcmp(command_type, "CREATE") || !strcmp(command_type, "DELETE")){ // change values of reply to suit command type
		//printf("inside of the CREATE/DELETE branch");
	}else if (!strcmp(command_type, "JOIN")){
		//printf("inside of the JOIN branch");
		reply.num_member = strtok(bufferRecv, " ");
		reply.port = strtok(NULL, " ");
	}else if(!strcmp(command_type, "LIST")){
		int listIterator = 0;
		char* tempRoom = strtok(NULL, " ");
		while(tempRoom!= NULL){
			reply.list_room[listIterator] = tempRoom;
			listIterator++;
		}
		//printf("inside of the LIST branch");
	}else{
		//printf("invalid option received from server");
		reply.status = FAILURE_INVALID;
	}
	return reply;
}

/* 
 * Get into the chat mode
 * 
 * @parameter host     host address
 * @parameter port     port
 */
void process_chatmode(const char* host, const int port)
{
	char*message;
	char*command;
	int socket = connect_to(host,port); // connect to socket

	display_title(); // show menu, get command from user
	get_command(command, sizeof(command));

	while(1){ // continue to get messages from the user and display messages from others
		get_message(message, sizeof(message));
		display_message(message);
	}

}

