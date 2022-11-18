#include "globals.h"

#define USER_COUNT 3
#define TOTAL_LOGINS 10
#define MAX_USERS 10

int getUserIndex(struct user* Users,int userCount,struct message* msg);
unsigned long hash(unsigned char *str);
void* clientCallbacks(void* userInfo_p);

int getUserIndex(struct user* Users,int userCount,struct message* msg){
	for(int i = 0; i < userCount; i++){
		if(!strcmp(Users[i].username,msg->source) && !strcmp(Users[i].password,msg->data)) return i;
	}
	return -1;
}

unsigned long hash(unsigned char *str){
    unsigned long hash = 5381;
    int c;
    while (c = *str++){
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
    return hash;
}

struct user* getUsers(char* inFile){
	char lines[MAX_DATA][MAX_DATA]; // [MAX_LINES][MAX_CHARS_IN_LINE]
	
	FILE *file;
	file = fopen("users.txt", "r");
	int line = 0;
  
	while (!feof(file) && !ferror(file))
		
		if (fgets(lines[line], MAX_DATA, file) != NULL){
			line++;
		}
		
	
	fclose(file);
	
	struct user *Users = malloc(sizeof(struct user) * MAX_USERS);
	for (int i = 0; i < line; i++){
		// printf("line: %s\n", lines[i]);
		char* clientID = strtok(lines[i],",");
		char* password = strtok(NULL,",");

		strcpy(Users[i].username, clientID);
		strcpy(Users[i].password, password);
		strcpy(Users[i].sessionID, "\0");
		Users[i].loggedIn = false;
		Users[i].sockfd = -1;
	}
	return Users;
}

getUserCount(char* inFile){
	char lines[MAX_DATA][MAX_DATA]; // [MAX_LINES][MAX_CHARS_IN_LINE]
	
	FILE *file;
	file = fopen("users.txt", "r");
	int line = 0;
  
	while (!feof(file) && !ferror(file))
		
		if (fgets(lines[line], MAX_DATA, file) != NULL){
			line++;
		}
		
	
	fclose(file);
	return line;
}

int main(int argc, char** argv){
	//Username and Password List
	struct user* Users = getUsers("users.txt");
	// struct user Users[3] = {
	// 	{"karlovma","12345",false,"\0",-1},
	// 	{"mcint254","12345",false,"\0",-1},
	// 	{"amanigillings","12345",false,"\0",-1}
	// 	};
	int userCount = getUserCount("users.txt");

	int rv, sockfd = INVALID_SOCKET;
	int yes = 1;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE; //MY IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	for(p = servinfo; p != NULL; p = p->ai_next){
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < -1){
			perror("Server: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
			close(sockfd);
			perror("Server: bind");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo); 
	if (p == NULL){
		fprintf(stderr, "Server: failed to bind\n");
		exit(1);
	}

	if(listen(sockfd,userCount) == -1){
		perror("listen");
		exit(1);
	}
	printf("Waiting for connections\n");

	char s[INET6_ADDRSTRLEN];

	struct sessionLL* sessions = (struct sessionLL*) malloc(sizeof(struct sessionLL));
	sessions->head = (struct sessionNode*) malloc(sizeof(struct sessionNode));
	sessions->tail = sessions->head;
	strcpy(sessions->head->sessionID,"\0");
	sessions->head->next = NULL;

	while(1){
		int userSockfd;	
		pthread_t p;
		sin_size = sizeof(their_addr);

		userSockfd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
		if(userSockfd == -1){
			perror("accept");
			break;
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s));
		printf("Server: got connection from %s\n", s);

		struct userSockStruct* userInfo = (struct userSockStruct*) malloc(sizeof(struct userSockStruct));

		userInfo->sockfd = userSockfd;
		userInfo->Users = Users;
		userInfo->p = p;
		userInfo->sessions = sessions;
		userInfo->userCount = userCount;
		
		pthread_create(&p, NULL, clientCallbacks, userInfo);
	}
	return 0;
}

void* clientCallbacks(void* userInfo_p){
	struct userSockStruct* userInfo = (struct userSockStruct*) userInfo_p;
	struct message msgR, msgS;
	struct message* msgRecv = &msgR;
	struct message* msgSend = &msgS;
	int clientIndx = -1;

	while(1){
		userInfo->userCount = userInfo->userCount = getUserCount("users.txt");
		bool toSend = true, toExit = false;
		memset(msgRecv, 0, sizeof(struct message));
		memset(msgSend, 0, sizeof(struct message));

		if(recv(userInfo->sockfd, msgRecv, sizeof(struct message), 0) == -1){
			perror("error recv\n");
			exit(1);
		}

		//CASES
		//Client-side takes care of corner cases
		if (msgRecv->type == NEW_USER){
			msgSend->type = NU_ACK;
			char newLine[MAX_DATA] = "\n";
			strcat(newLine, msgRecv->source);
			strcat(newLine,",");
			strcat(newLine,msgRecv->data);
			strcpy(msgSend->data, newLine);
			strcat(newLine,",");

			FILE *file;
			file = fopen("users.txt", "a");
			fputs(newLine, file);
			fclose(file);

			strcpy(userInfo->Users[userInfo->userCount].username, msgRecv->source);
			strcpy(userInfo->Users[userInfo->userCount].password, msgRecv->data);
			strcpy(userInfo->Users[userInfo->userCount].sessionID, "\0");
			userInfo->Users[userInfo->userCount].loggedIn = false;
			userInfo->Users[userInfo->userCount].sockfd = -1;
			printf("Created new user: %s\n",userInfo->Users[userInfo->userCount].username);;
			// printf("Created new user: %s\n", msgRecv->source);
		}
		//Login attempt to user that has already logged in --> drop socket, and thread exit
		else if(msgRecv->type == LOGIN){
			if((clientIndx = getUserIndex(userInfo->Users,userInfo->userCount,msgRecv)) == -1){
				strcpy(msgSend->data,"User login credentials are invalid\n");
				msgSend->type = LO_NAK;

				toExit = true;
			}
			else if(userInfo->Users[clientIndx].loggedIn){
				strcpy(msgSend->data,"Multi Login, that user already logged in");
				clientIndx = -1;
				msgSend->type = LO_NAK;

				toExit = true;
			}
			else{
				//ALL GOOD
				//adjust pthread array based on clientIndx

				msgSend->type = LO_ACK;
				userInfo->Users[clientIndx].loggedIn = true;
				userInfo->Users[clientIndx].sockfd = userInfo->sockfd;
				printf("Logged in user %s\n",userInfo->Users[clientIndx].username);
			}
		}
		else if(msgRecv->type == EXIT){
			userInfo->Users[clientIndx].sessionID[0] = '\0';
			
			if(clientIndx != -1) userInfo->Users[clientIndx].loggedIn = false;
			toSend = false;
			toExit = true;
		}
		else if(msgRecv->type == QUERY){
			char listMsg[MAX_DATA];
			sprintf(listMsg,"Online Users: ");
			for(int i = 0; i < userInfo->userCount; i++){
				if(userInfo->Users[i].loggedIn) strcat(strcat(listMsg,userInfo->Users[i].username),", ");
			}

			strcat(listMsg,"\nAvailable Sessions: ");
			struct sessionNode* temp = userInfo->sessions->head->next;
			while(temp != NULL){
				strcat(strcat(listMsg,temp->sessionID),", ");
				temp = temp->next;
			}
			printf("%s\n",listMsg);
			strcpy(msgSend->data,listMsg);
			msgSend->type = QU_ACK;
		}
		else if(clientIndx == -1){
			sprintf(msgSend->data,"%s:Please login first before trying to do something",msgRecv->data);
			msgSend->type = JN_NAK;
		}
		else if(msgRecv->type == JOIN){
			
			if(userInfo->Users[clientIndx].sessionID[0] != '\0'){
				sprintf(msgSend->data,"%s: Leave current Session before trying to join another one",msgRecv->data);
				msgSend->type = JN_NAK;	
			}
			else{
				struct sessionNode* temp = userInfo->sessions->head->next;
				while(temp != NULL && strcmp(temp->sessionID,msgRecv->data)){
					temp = temp->next;
				}

				if(temp == NULL){
					sprintf(msgSend->data,"%s: Invalid session ID",msgRecv->data);
					msgSend->type = JN_NAK;	
				}
				else{
					strcpy(msgSend->data,msgRecv->data);
					strcpy(userInfo->Users[clientIndx].sessionID,msgRecv->data);
					msgSend->type = JN_ACK;
				}
			}
		}
		else if(msgRecv->type == LEAVE_SESS){
			toSend = false;
			//CHECK IF ANONE ELSE IN LIST
			bool isAnyone = false;
			for(int i = 0; i < userInfo->userCount; i++){
				if(i != clientIndx){
					if(!strcmp(userInfo->Users[i].sessionID,userInfo->Users[clientIndx].sessionID)) isAnyone = true;
				}
			}
			//IF NOONE, DLEETE NODE FROM LIST
			if(!isAnyone){
				struct sessionNode* temp = userInfo->sessions->head->next;
				struct sessionNode* prev = userInfo->sessions->head;
				while(temp != NULL && strcmp(temp->sessionID,userInfo->Users[clientIndx].sessionID)){
					prev = temp;
					temp = temp->next;
				}
				
				//Tail affected
				if(temp->next == NULL){
					free(temp);
					prev->next = NULL;
					userInfo->sessions->tail = prev;
				}
				//Mid List affected
				else {
					strcpy(temp->sessionID,temp->next->sessionID);
					temp->next = temp->next->next;
				}
			}
			userInfo->Users[clientIndx].sessionID[0] = '\0';
		}
		else if(msgRecv->type == NEW_SESS){
			if(userInfo->Users[clientIndx].sessionID[0] != '\0'){
				sprintf(msgSend->data,"%s: Leave current Session before trying to join another one",msgRecv->data);
				msgSend->type = JN_NAK;	
			}
			else{

				struct sessionNode* newNode = (struct sessionNode*) malloc(sizeof(struct sessionNode));
				strcpy(newNode->sessionID,msgRecv->data);
				newNode->next = NULL;

				userInfo->sessions->tail->next = newNode;
				userInfo->sessions->tail = newNode;

				struct sessionNode* temp = userInfo->sessions->head->next;
				strcpy(userInfo->Users[clientIndx].sessionID,msgRecv->data);
				strcpy(msgSend->data,msgRecv->data);
				msgSend->type = NS_ACK;
			}
		}
		else if(msgRecv->type == MESSAGE){
			toSend = false;

			strcpy(msgSend->source,msgRecv->source);
			strcpy(msgSend->data,msgRecv->data);
			msgSend->type = MESSAGE;
			printf("%s:%s\n",msgRecv->source,msgRecv->data);

			for(int i = 0; i < userInfo->userCount; i++){
				if(i != clientIndx){
					if(!strcmp(userInfo->Users[i].sessionID,userInfo->Users[clientIndx].sessionID)){
						if(send(userInfo->Users[i].sockfd,msgSend,sizeof(struct message),0) == -1){
							perror("error send\n");
							exit(1);
						}
					}
				}
			}
		}

		if(toSend){
			if(send(userInfo->sockfd,msgSend,sizeof(struct message),0) == -1){
				perror("error send\n");
				exit(1);
			}
		}

		if(toExit){
			close(userInfo->sockfd);
			pthread_exit(userInfo);
		}
	}
}

