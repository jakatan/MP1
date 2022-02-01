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
		printf("ERROR: connection on client-side failed\n");
		return EXIT_FAILURE;
	}

	// Send name

	printf("=== WELCOME TO THE CHATROOM ===\n");
	// ------------------------------------------------------------
	// GUIDE :
	// In this function, you are suppose to connect to the server.
	// After connection is established, you are ready to send or
	// receive the message to/from the server.
	// 
	// Finally, you should return the socket fildescriptor
	// so that other functions such as "process_command" can use it
	// ------------------------------------------------------------

    // below is just dummy code for compilation, remove it.
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
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse a given command
	// and create your own message in order to communicate with
	// the server. Surely, you can use the input command without
	// any changes if your server understand it. The given command
    // will be one of the followings:
	//
	// CREATE <name>
	// DELETE <name>
	// JOIN <name>
    // LIST
	//
	// -  "<name>" is a chatroom name that you want to create, delete,
	// or join.
	// 
	// - CREATE/DELETE/JOIN and "<name>" are separated by one space.
	// ------------------------------------------------------------
	
	char* command_type = strtok(command, " ");
	char* roomName = strtok(NULL, " ");
	printf("Command given is: %s" , command_type);
	printf("Room name is: %s" , roomName);



	// ------------------------------------------------------------
	// GUIDE 2:
	// After you create the message, you need to send it to the
	// server and receive a result from the server.
	// ------------------------------------------------------------
	int sent_amount;
	int recv_amount;
	char bufferSent[256];
	char bufferRecv[256];
	
	strcpy(bufferSent, command); // copy command to the buffer to send
	
	sent_amount = send(sockfd, bufferSent, sizeof(bufferSent), 0);
	recv_amount = recv(sockfd, bufferRecv, sizeof(bufferRecv), 0);

	// ------------------------------------------------------------
	// GUIDE 3:
	// Then, you should create a variable of Reply structure
	// provided by the interface and initialize it according to
	// the result.
	//
	// For example, if a given command is "JOIN room1"
	// and the server successfully created the chatroom,
	// the server will reply a message including information about
	// success/failure, the number of members and port number.
	// By using this information, you should set the Reply variable.
	// the variable will be set as following:
	//
	// Reply reply;
	// reply.status = SUCCESS;
	// reply.num_member = number;
	// reply.port = port;
	// 
	// "number" and "port" variables are just an integer variable
	// and can be initialized using the message fomr the server.
	//
	// For another example, if a given command is "CREATE room1"
	// and the server failed to create the chatroom becuase it
	// already exists, the Reply varible will be set as following:
	//
	// Reply reply;
	// reply.status = FAILURE_ALREADY_EXISTS;
    // 
    // For the "LIST" command,
    // You are suppose to copy the list of chatroom to the list_room
    // variable. Each room name should be seperated by comma ','.
    // For example, if given command is "LIST", the Reply variable
    // will be set as following.
    //
    // Reply reply;
    // reply.status = SUCCESS;
    // strcpy(reply.list_room, list)
    // 
    // "list" is a string that contains a list of chat rooms such 
    // as "r1,r2,r3,"
	// ------------------------------------------------------------

	// REMOVE below code and write your own Reply.
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

	if(!strcmp(command_type, "CREATE") || !strcmp(command_type, "DELETE")){
		printf("inside of the CREATE/DELETE branch");
	}else if (!strcmp(command_type, "JOIN")){
		printf("inside of the JOIN branch");
		reply.num_member = strtok(bufferRecv, " ");
		reply.port = strtok(NULL, " ");
	}else if(!strcmp(command_type, "LIST")){
		int listIterator = 0;
		char* tempRoom = strtok(NULL, " ");
		while(tempRoom!= NULL){
			reply.list_room[listIterator] = tempRoom;
			listIterator++;
		}
		printf("inside of the LIST branch");
	}else{
		printf("invalid option received from server");
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
	int socket = connect_to(host,port);
	
	// ------------------------------------------------------------
	// GUIDE 1:
	// In order to join the chatroom, you are supposed to connect
	// to the server using host and port.
	// You may re-use the function "connect_to".
	// ------------------------------------------------------------
	display_title(); // show menu, get command from user
	get_command(command, sizeof(command));

	while(1){ // continue to get messages from the user and display messages from others
		get_message(message, sizeof(message));
		display_message(message);
	}

	// ------------------------------------------------------------
	// GUIDE 2:
	// Once the client have been connected to the server, we need
	// to get a message from the user and send it to server.
	// At the same time, the client should wait for a message from
	// the server.
	// ------------------------------------------------------------
	
    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    // 1. To get a message from a user, you should use a function
    // "void get_message(char*, int);" in the interface.h file
    // 
    // 2. To print the messages from other members, you should use
    // the function "void display_message(char*)" in the interface.h
    //
    // 3. Once a user entered to one of chatrooms, there is no way
    //    to command mode where the user  enter other commands
    //    such as CREATE,DELETE,LIST.
    //    Don't have to worry about this situation, and you can 
    //    terminate the client program by pressing CTRL-C (SIGINT)
	// ------------------------------------------------------------
}

