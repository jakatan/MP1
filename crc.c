#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"


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
    int sockfd = connect_to(argv[1], atoi(argv[2]));
	while (1) {
	
	
    
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
	
	  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	  if(sockfd < 0){
		fprintf(stderr, "Socket did not get established\n");
	  	exit(1);
	  }
	  
	  struct sockaddr_in server_addr;

	  server_addr.sin_family = AF_INET; 
	  server_addr.sin_port = htons(port); // assign the port
	  server_addr.sin_addr.s_addr = inet_addr(host); // assign hostname
	  
	  struct hostent* hostname; // setup the host
	  if(server_addr.sin_addr.s_addr == (unsigned long) INADDR_NONE){
		hostname = gethostbyname(host);
		if(hostname == (struct hostent *) NULL){
			fprintf(stderr, "Host is not resolved");
			exit(1);
		}
		memcpy(&server_addr.sin_addr, hostname -> h_addr, sizeof(server_addr.sin_addr));

	  }
		
		int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
		if (err < 0) {
			printf("ERROR: connection on client-side failed\n");
			exit(1);
		}

	// Send name

	printf("=== WELCOME TO THE CHATROOM ===\n");

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
	
    send(sockfd, command, strlen(command),0);
    struct Reply reply;
    recv(sockfd, &reply, sizeof(reply), 0);
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
	int socket = connect_to(host,port); // connect to socket
	
	//instantiate variables 
	int selectError;
	fd_set tempFd;
	struct timeval timevalue;
	timevalue.tv_sec = 0;
	timevalue.tv_usec = 10;
	
	char message[256];
	
	while(1){ // continue to get messages from the user and display messages from others
		FD_ZERO(&tempFd);
		FD_SET(0, &tempFd);
		FD_SET(socket, &tempFd);
		selectError = select(socket + 1, &tempFd, NULL, NULL, &timevalue);
		
		if(FD_ISSET(0, &tempFd)){ // user begins a chat
			get_message(message, sizeof(message));
			int bytes = send(socket, message, strlen(message), 0);
		}
		
		if(FD_ISSET(socket, &tempFd)){ // server is sending back a message
			int bytes = recv(socket, message, sizeof(message), 0);
			message[bytes] = '\0';
			printf("\r");
			display_message(message);
			
			if(!strcmp(message, "Server closing down.")){
				exit(0);
			}else if (!strcmp(message, "Chat room closing down.")){
				display_title();
				return;
			}
		}
	}

}

