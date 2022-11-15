#include <globals.h>

#define USER_COUNT 2

int getUserIndex(const char usernames[USER_COUNT][MAX_NAME],char username[MAX_NAME]);

int getUserIndex(struct user Users[USER_COUNT],char username[MAX_NAME]){
	for(int i = 0; i < USER_COUNT; i++){
		if(!strcmp(Users[i].username,username)) return i;
	}
	return -1;
}

void init_client(struct users Users[USER_COUNT],int sockfd){
	
	struct message* msg = (struct message*) malloc(sizeof(struct message));
	if(recv(sockfd, msg, sizeof(struct message), 0) == -1){
		perror("error recv\n");
		exit(1);
	}

	if(
	int userIndx = getUserIndex
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
	while(1){
		int userSockfd;	
		sin_size = sizeof(their_addr);
		if(userSockfd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size) == -1){
			perror("accept");
			break;
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s));
		printf("Server: got connection from %s\n", s);

		init_client(Users,userSockfd);
	}

	return 0;
}
