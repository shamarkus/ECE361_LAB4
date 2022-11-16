#include <globals.h>

#define USER_COUNT 2
#define TOTAL_LOGINS 10

int getUserIndex(struct user Users[USER_COUNT],struct message* msg);

int getUserIndex(struct user Users[USER_COUNT],struct message* msg){
	for(int i = 0; i < USER_COUNT; i++){
		if(!strcmp(Users[i].username,msg->username) && !strcmp(Users[i].password,msg->password) return i;
	}
	return -1;
}

int main(int argc, char** argv){
	//Username and Password List
	struct users Users[2] = {{"karlovma","12345",false,0,0,0},{"mcint254","12345",false,0,0,0}};

	int port = atoi(argv[1]);
	int rv, sockfd = INVALID_SOCKET;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	struct sigaction sa;
	socklen_t sin_size;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE; //MY IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
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

			pthread_create(&threads[threadCount], NULL, clientCallbacks, userInfo)
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
		bool toSend = false;
		if(recv(userInfo->sockfd, msgRecv, sizeof(struct message), 0) == -1){
			perror("error recv\n");
			exit(1);
		}

		//CASES
		
		if(msgRecv->type == LOGIN){
			toSend = true;
			if(clientIndx == -1){
				if(clientIndx = getUserIndx(userInfo->Users,msgRecv) == -1){
					strcpy(msg->data,"User login credentials are invalid\n");
					msgSend->type = LO_NAK;
				}
				else if(userInfo->Users[clientIndx].loggedIn){
					strcpy(msg->data,"Multi Login, that user already logged in");
					msgSend->type = LO_NAK;
				}
				else{
					//ALL GOOD
				}
			}
			else{
				strcpy(msg->data,"Please logout first before trying to log in");
				msgSend->type = LO_NAK;
			}
		}
		else{

		}
		else if(msgRecv->type == EXIT){

		}
		else if(msgRecv->type == JOIN){

		}
		else if(msgRecv->type == LEAVE_SESS){

		}
		else if(msgRecv->type == NEW_SESS){

		}
		else if(msgRecv->type == MESSAGE){
		}
		else if(msgRecv->type == QUERY){
		}

	}
	return NULL;
}

