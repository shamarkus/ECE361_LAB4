#include "globals.h"

//Function prototypes
struct paramStruct* login();
struct paramStruct* logout(struct paramStruct* params,pthread_t* rcvThread);
void receive(void* params_p);
void enterSession(struct paramStruct* params,int msgType);
void leaveSession(struct paramStruct* params);
void list(struct paramStruct* params);
void sendText(struct paramStruct* params,char buf[MAX_DATA]);

void receive(void* params_p){
	struct paramStruct* params = (struct paramStruct*) params_p;
	struct message* msg = (struct message*) malloc(sizeof(struct message)); 

	while(1){
		if(recv(params->socketfd, msg, sizeof(struct message), 0) == -1){
			fprintf(stderr, "client: recv\n");
			break;
		}
		
		if (msg->type == JN_ACK ){
			printf("Successfully Joined Session ID %s\n", msg->data);
			params->inSession = true;
		}
		else if (msg->type == JN_NAK){
			printf("Failure to Join Session ID %s\n", msg->data);
			params->inSession = false;
		}
		else if (msg->type == NS_ACK){
			printf("Successfully Created & Joined Session ID\n", msg->data);
			params->inSession = true;
		}
		else if (msg->type == QU_ACK){
			printf("%s\n",msg->data);
		}
		else if (msg->type == MESSAGE){
			printf("%s:%s\n",msg->source,msg->data);
		}
		else if (msg->type == NU_ACK){
			printf("Successfully created new user with username,password:%s\n",msg->data);
		}
		else if (msg->type == PM){
			printf("[PRIVATE] %s: %s\n", msg->source, msg->data);
		}
		else if (msg->type == PM_NAK){
			printf("%s\n", msg->data);
		}
		else{
			printf("Unexpected packet\n");
		}
	}
}


struct paramStruct* login(){
	
	char* clientID = strtok(NULL," ");
	char* password = strtok(NULL," ");
	char* serverIP = strtok(NULL," ");
	char* serverPort = strtok(NULL,"\n");

	if (clientID == NULL || password == NULL || serverIP == NULL || serverPort == NULL) {
		printf("Incorrect usage: /login <client_id> <password> <server_ip> <server_port>\n");
		return NULL;
	}

	int rv, socketfd = INVALID_SOCKET;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(serverIP, serverPort, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return NULL;
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
		close(socketfd);
		return NULL;
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
		return NULL;
	}

	if ((recv(socketfd, msg, sizeof(struct message), 0)) == -1) {
		fprintf(stderr, "client: recv\n");
		close(socketfd);
		return NULL;
	}
	
	int tempType = msg->type;
	free(msg);

	if(tempType == LO_ACK){
		printf("Login Successful\n");

		struct paramStruct* params = (struct paramStruct*) malloc(sizeof(struct paramStruct)); 
		params->socketfd = socketfd;
		params->inSession = false;
		strncpy(params->clientID,clientID,MAX_NAME);

		return params;
	}

	printf("Login Unsuccessful OR Unexpected Packet Received: %s\n",msg->data);
	return NULL;
}

void enterSession(struct paramStruct* params,int msgType){
	char* sessionID = strtok(NULL,"\n");

	if(params == NULL || params->socketfd == INVALID_SOCKET){
		printf("Please login before creating a session\n");
		return;
	}
	else if(params->inSession){
		printf("Please leave current session before entering a new one\n");
		return;
	}

	struct message* msg = (struct message*) malloc(sizeof(struct message)); 
	msg->type = msgType;
	strncpy(msg->source,params->clientID,MAX_NAME);
	strncpy(msg->data,sessionID,MAX_DATA);

	if(send(params->socketfd, msg, sizeof(struct message), 0) == -1){
		printf("client: send\n");
	}
	free(msg);
}

void leaveSession(struct paramStruct* params){
	if(params == NULL || params->socketfd == INVALID_SOCKET){
		printf("Please login before leaving a session\n");
		return;
	}
	else if(!params->inSession){
		printf("Please enter a session before leaving one\n");
		return;
	}
	
	printf("Successfully left session\n");
	struct message* msg = (struct message*) malloc(sizeof(struct message)); 
	msg->type = LEAVE_SESS;
	strncpy(msg->source,params->clientID,MAX_NAME);

	params->inSession = false;
	if(send(params->socketfd, msg, sizeof(struct message), 0) == -1){
		printf("client: send\n");
	}
	free(msg);
}

void list(struct paramStruct* params){
	if(params == NULL || params->socketfd == INVALID_SOCKET){
		printf("Please login before asking for a list\n");
		return;
	}

	struct message* msg = (struct message*) malloc(sizeof(struct message)); 
	msg->type = QUERY;
	strncpy(msg->source,params->clientID,MAX_NAME);

	if(send(params->socketfd, msg, sizeof(struct message), 0) == -1){
		printf("client: send\n");
	}
	free(msg);
}

struct paramStruct* createUser(struct paramStruct* params){
	
	char* clientID = strtok(NULL," ");
	char* password = strtok(NULL," ");
	char* serverIP = strtok(NULL," ");
	char* serverPort = strtok(NULL,"\n");

	if (clientID == NULL || password == NULL || serverIP == NULL || serverPort == NULL) {
		printf("Incorrect usage: /createuser <client_id> <password> <server_ip> <server_port>\n");
		return NULL;
	} 

	if (params != NULL){
		printf("Please logout before trying to create a new user\n");
		return NULL;
	}

	// create a temp socket
	int rv, socketfd = INVALID_SOCKET;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(serverIP, serverPort, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return NULL;
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
		close(socketfd);
		return NULL;
	}

	char strAddr[INET6_ADDRSTRLEN];
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), strAddr, sizeof strAddr);
	printf("client: connecting to %s\n", strAddr);
	freeaddrinfo(servinfo); // all done with this structure

	struct message* msg = (struct message*) malloc(sizeof(struct message)); 
	msg->type = NEW_USER;
	strncpy(msg->source,clientID,MAX_NAME);
	strncpy(msg->data,password,MAX_NAME);

	if ((send(socketfd, msg, sizeof(struct message), 0)) == -1) {
		fprintf(stderr, "client: send\n");
		close(socketfd);
		return NULL;
	}

	if ((recv(socketfd, msg, sizeof(struct message), 0)) == -1) {
		fprintf(stderr, "client: recv\n");
		close(socketfd);
		return NULL;
	}
	int tempType = msg->type;
	char data[MAX_DATA];
	strcpy(data, msg->data);
	free(msg);

	if(tempType == NU_ACK){

		struct paramStruct* temp_params = (struct paramStruct*) malloc(sizeof(struct paramStruct)); 
		printf("Successfully created new user with username,password:%s\n",data);
		temp_params->socketfd = socketfd;
		temp_params->inSession = false;
		strncpy(temp_params->clientID,clientID,MAX_NAME);

		return temp_params;
	}
	// duplicate
	printf("User already exists\n");
	return NULL;
}

struct paramStruct* logout(struct paramStruct* params,pthread_t* rcvThread){
	if(params == NULL || params->socketfd == INVALID_SOCKET){
		printf("Please login before trying to log out\n");
		return;
	}

	if(params->inSession) leaveSession(params);

	struct message* msg = (struct message*) malloc(sizeof(struct message)); 
	msg->type = EXIT;
	strncpy(msg->source,params->clientID,MAX_NAME);

	if(send(params->socketfd, msg, sizeof(struct message), 0) == -1){
		printf("client: send\n");
	}
	free(msg);
	pthread_cancel(*rcvThread);
	return params;
}

void sendText(struct paramStruct* params,char buf[MAX_DATA]){
	if(params == NULL || params->socketfd == INVALID_SOCKET){
		printf("Please login before sending a message\n");
		return;
	}
	else if(!params->inSession){
		printf("Please enter a session before trying to send a message\n");
		return;
	}

	struct message* msg = (struct message*) malloc(sizeof(struct message)); 

	msg->type = MESSAGE;
	msg->size = strlen(buf);
	strncpy(msg->source,params->clientID,MAX_NAME);

	strncpy(msg->data,buf,MAX_DATA);

	if(send(params->socketfd, msg, sizeof(struct message), 0) == -1){
		printf("client: send\n");
	}
	free(msg);
}

void private_msg(struct paramStruct* params){
	if(params == NULL || params->socketfd == INVALID_SOCKET){
		printf("Please login before sending a private message\n");
		return;
	}

	char* reciever = strtok(NULL," ");
	char* message = strtok(NULL, "\n");
	char data[MAX_DATA];

	// printf("%s: %s\n", reciever, message);

	if (reciever == NULL || message == NULL){
		printf("Incorrect usage: /pm <clientID> <msg>\n");
		return;
	}

	sprintf(data, "%s %s\n",reciever, message);

	struct message* msg = (struct message*) malloc(sizeof(struct message)); 

	msg->type = PM;
	msg->size = strlen(msg);

	strncpy(msg->source,params->clientID,MAX_NAME);
	strncpy(msg->data,data,MAX_DATA);

	if(send(params->socketfd, msg, sizeof(struct message), 0) == -1){
		printf("client: send\n");
	}
	free(msg);
}

int main(){
	
	char buf[MAX_DATA];
	pthread_t rcvThread;
	struct paramStruct* params = NULL; 

	while(fgets(buf,MAX_DATA, stdin)){
		char buf_copy[MAX_DATA];
		strcpy(buf_copy, buf);
		char* cmd = strtok(buf," ");
		int tokenLen = strlen(cmd);

		if(strcmp(cmd, LOGIN_CMD) == 0) {
			if(params == NULL){
				params = login();
				if(params != NULL && params->socketfd != INVALID_SOCKET) pthread_create(&rcvThread, NULL, &receive, params);
			}
			else printf("Please logout of current account before attempting to login\n");
		}
		else if (strcmp(cmd, CREATESESSION_CMD) == 0) {
			enterSession(params,NEW_SESS);
		}
		else if (strcmp(cmd, JOINSESSION_CMD) == 0) {
			enterSession(params,JOIN);
		}
		else if (strcmp(cmd, NEWUSER_CMD) == 0){
			struct paramStruct* temp = createUser(params);
			if(temp != NULL && temp->socketfd != INVALID_SOCKET){
			 	pthread_create(&rcvThread, NULL, &receive, temp);
				free(logout(temp,&rcvThread));
				temp = NULL;
				//login
				// cmd = strtok(buf_copy," ");
				// params = login();
				// if(params != NULL && params->socketfd != INVALID_SOCKET) pthread_create(&rcvThread, NULL, &receive, params);
			}
		}
		else if (strcmp(cmd, PRIVATEMSG_CMD) == 0){
			private_msg(params);
		}
		else{
			if(cmd[0] == '/') cmd[tokenLen - 1] = '\0';

			if (strcmp(cmd, LIST_CMD) == 0) {
				list(params);
			}
			else if (strcmp(cmd, LEAVESESSION_CMD) == 0) {
				leaveSession(params);
			}
			else if (strcmp(cmd, LOGOUT_CMD) == 0) {
				free(logout(params,&rcvThread));
				params = NULL;
			}
			else if (strcmp(cmd, QUIT_CMD) == 0) {
				if(params != NULL) free(logout(params,&rcvThread));
				break;
			} 
			else {
				buf[tokenLen] = ' ';
				buf[strcspn(buf,"\n")] = 0;
				sendText(params,buf);
			}
		}
		memset(buf,0,MAX_DATA);
	}

	printf("Quit successfully\n");
	return 0;
}
