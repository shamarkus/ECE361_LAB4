#include "globals.h"

#define USER_COUNT 2
#define TOTAL_LOGINS 10

int getUserIndex(struct user Users[USER_COUNT],struct message* msg);
unsigned long hash(unsigned char *str);
void* clientCallbacks(void* userInfo_p);

int getUserIndex(struct user Users[USER_COUNT],struct message* msg){
	for(int i = 0; i < USER_COUNT; i++){
		if(!strcmp(Users[i].password,msg->data)) return i;
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

int main(int argc, char** argv){
	//Username and Password List
	struct user Users[2] = {{"karlovma","12345",false,"\0",-1},{"mcint254","12345",false,"\0",-1}};

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
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
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

	if(listen(sockfd,USER_COUNT) == -1){
		perror("listen");
		exit(1);
	}
	printf("Waiting for connections\n");

	char s[INET6_ADDRSTRLEN];

	int threadCount = 0;
	pthread_t *threads = (pthread_t*) malloc(TOTAL_LOGINS * sizeof(pthread_t));

	struct sessionLL* sessions = (struct sessionLL*) malloc(sizeof(struct sessionLL));
	sessions->head = (struct sessionNode*) malloc(sizeof(struct sessionNode));
	sessions->tail = sessions->head;
	strcpy(sessions->head->sessionID,"\0");
	sessions->head->next = NULL;
	while(1){
		if(threadCount != TOTAL_LOGINS){
			int userSockfd;	
			sin_size = sizeof(their_addr);
			if(userSockfd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size) == -1){
				perror("accept");
				break;
			}

			inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s));
			printf("Server: got connection from %s\n", s);

			struct userSockStruct* userInfo = (struct userSockStruct*) malloc(sizeof(struct userSockStruct));
			userInfo->sockfd = userSockfd;
			userInfo->Users = Users;
			userInfo->p = &threads[threadCount];
			userInfo->sessions = sessions;
			printf("%d\n",userSockfd);
			pthread_create(&threads[threadCount], NULL, clientCallbacks, userInfo);
			threadCount++;
		}
	}

	return 0;
}

void* clientCallbacks(void* userInfo_p){
	struct userSockStruct* userInfo = (struct userSockStruct*) userInfo_p;
	struct message* msgRecv = (struct message*) malloc(sizeof(struct message));
	struct message* msgSend = (struct message*) malloc(sizeof(struct message));
	int clientIndx = -1;

	while(1){
		bool toSend = true;
		if(recv(userInfo->sockfd, msgRecv, sizeof(struct message), 0) == -1){
			perror("error recv\n");
			exit(1);
		}

		//CASES
		if(msgRecv->type == LOGIN){
			if(clientIndx == -1){
				if(clientIndx = getUserIndex(userInfo->Users,msgRecv) == -1){
					strcpy(msgSend->data,"User login credentials are invalid\n");
					msgSend->type = LO_NAK;
				}
				else if(userInfo->Users[clientIndx].loggedIn){
					strcpy(msgSend->data,"Multi Login, that user already logged in");
					clientIndx = -1;
					msgSend->type = LO_NAK;
				}
				else{
					//ALL GOOD
					msgSend->type = LO_ACK;
					userInfo->Users[clientIndx].loggedIn = true;
					userInfo->Users[clientIndx].sockfd = userInfo->sockfd;
					printf("Logged in user %s\n",userInfo->Users[clientIndx].username);
				}
			}
			else{
				strcpy(msgSend->data,"Please logout first before trying to log in");
				msgSend->type = LO_NAK;
			}
		}
		else if(msgRecv->type == EXIT){
			userInfo->Users[clientIndx].sessionID[0] = '\0';
			break;
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
			for(int i = 0; i < USER_COUNT; i++){
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
				
				if(temp->next == NULL) prev->next = NULL;
				else {
					strcpy(temp->sessionID,temp->next->sessionID);
					temp->next = temp->next->next;
				}
			}
			userInfo->Users[clientIndx].sessionID[0] == '\0';
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
				msgSend->type = NS_ACK;
			}
		}
		else if(msgRecv->type == QUERY){
			char listMsg[MAX_DATA];
			sprintf(listMsg,"Online Users: ");
			for(int i = 0; i < USER_COUNT; i++){
				if(userInfo->Users[i].loggedIn) strcat(strcat(listMsg,userInfo->Users[i].username),", ");
			}

			strcat(listMsg,"\nAvailable Sessions: ");
			struct sessionNode* temp = userInfo->sessions->head->next;
			while(temp != NULL){
				strcat(strcat(listMsg,temp->sessionID),", ");
				temp = temp->next;
			}
			strcpy(msgSend->data,listMsg);
			msgSend->type = QU_ACK;
		}
		else if(msgRecv->type == MESSAGE){
			toSend = false;

			strcpy(msgSend->source,msgRecv->source);
			strcpy(msgSend->data,msgRecv->data);
			msgSend->type = MESSAGE;

			for(int i = 0; i < USER_COUNT; i++){
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
			if(send(userInfo->Users[clientIndx].sockfd,msgSend,sizeof(struct message),0) == -1){
				perror("error send\n");
				exit(1);
			}
		}
	}

	return NULL;
}

