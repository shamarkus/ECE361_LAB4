#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "3001"
#define MAX_STRING_SIZE 1024
#define MAX_NAME 32
#define MAX_DATA 512

// supported commands
const char* LOGIN_CMD = "/login";
const char* LOGOUT_CMD = "/logout";
const char* JOINSESSION_CMD = "/joinsession";
const char* LEAVESESSION_CMD = "/leavesession";
const char* CREATESESSION_CMD = "/createsession";
const char* LIST_CMD = "/list";
const char* QUIT_CMD = "/quit";

//Struct Definitions

struct message {
	unsigned int type;
	unsigned int size;
	unsigned char source[MAX_NAME];
	unsigned char data[MAX_DATA];
};

//Function prototypes
void *get_in_addr(struct sockaddr *sa);
int login();

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
	}
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int login(){
	
	char* clientID = strtok(NULL," ");
	char* password = strtok(NULL," ");
	char* serverIP = strtok(NULL," ");
	char* serverPort = strtok(NULL," ");
	if (clientID == NULL || password == NULL || serverIP == NULL || serverPort == NULL) {
		printf("Incorrect usage: /login <client_id> <password> <server_ip> <server_port>\n");
		return INVALID_SOCKET;
	}

	int rv, socketfd = INVALID_SOCKET;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(serverIP, serverPort, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return INVALID_SOCKET;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			fprintf(stderr ,"client: socket\n");
			continue;
		}
		if (connect(socketfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(socketfd);
			fprintf(stderr, "client: connect\n");
			continue;
		}
		break; 
	}
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect from addrinfo\n");
		close(*socketfd_p);
		return INVALID_SOCKET;
	}

	char strAddr[INET6_ADDRSTRLEN];
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), strAddr, sizeof strAddr);
	printf("client: connecting to %s\n", strAddr);
	freeaddrinfo(servinfo); // all done with this structure

	struct message* msg = (struct message*) malloc(sizeof(struct message)); 
	msg->type = LOGIN;
	msg->size = strlen(msg->data);
	strncpy(msg->source, clientID, MAX_NAME);
	strncpy(msg->data, password, MAX_DATA);

	if ((send(socketfd, msg, sizeof(struct message), 0)) == -1) {
		fprintf(stderr, "client: send\n");
		close(socketfd);
		return INVALID_SOCKET;
	}

	if ((recv(socketfd, msg, sizeof(struct message), 0)) == -1) {
		fprintf(stderr, "client: recv\n");
		close(socketfd);
		return INVALID_SOCKET;
	}
	
	if(msg->type == LO_ACK){
		printf("Login Successful\n");
		return socketfd;
	}

	printf("Login Unsuccessful OR Unexpected Packet Received\n");
	return INVALID_SOCKET;
}

int main(){
	
	char buf[MAX_STRING_SIZE];
	pthread_t rcvThread;
	int socketfd = INVALID_SOCKET;

	while(fgets(buf,MAX_STRING_SIZE, stdin)){
		char* cmd = strtok(buf," ");
		int tokenLen = strlen(cmd);

		if(strcmp(cmd, LOGIN_CMD) == 0) {
			socketfd = login();
			//MAKE RECEIVE FUNCTION
			if(socketfd != INVALID_SOCKET) pthread_create(&rcvThread, NULL, receive, socketfd);
		}
		else if (strcmp(cmd, LOGOUT_CMD) == 0) {
			logout(&socketfd);
		}
		else if (strcmp(cmd, JOINSESSION_CMD) == 0) {
			joinsession(cmd, &socketfd);
		}
		else if (strcmp(cmd, LEAVESESSION_CMD) == 0) {
			leavesession(socketfd);
		}
		else if (strcmp(cmd, CREATESESSION_CMD) == 0) {
			createsession(socketfd);
		}
		else if (strcmp(cmd, LIST_CMD) == 0) {
			list(socketfd);
		}
		else if (strcmp(cmd, QUIT_CMD) == 0) {
			logout(&socketfd);
			break;
		} 
		else {
			buf[tokenLen] = ' ';
			send_text(socketfd);
		}
	}
	printf("Quit successfully\n");
	return 0;
}
