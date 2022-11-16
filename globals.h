#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT "3001"
#define MAX_NAME 32
#define MAX_DATA 512
#define INVALID_SOCKET -1

#define LOGIN_CMD "/login"
#define LOGOUT_CMD "/logout"
#define JOINSESSION_CMD "/joinsession"
#define LEAVESESSION_CMD "/leavesession"
#define CREATESESSION_CMD "/createsession"
#define LIST_CMD "/list"
#define QUIT_CMD "/quit"

//Struct Definitions
struct user;
struct message {
	unsigned int type;
	unsigned int size;
	unsigned char source[MAX_NAME];
	unsigned char data[MAX_DATA];
};

struct paramStruct {
	int socketfd;
	bool inSession;
	int clientID[MAX_NAME];
};

struct user {
	char username[MAX_NAME];
	char password[MAX_NAME];
	bool loggedIn;
	char sessionID[MAX_NAME];
	int sockfd;
};

struct userSockStruct {
	struct user* Users;
	struct sessionLL* sessions;
	pthread_t* p;
	int sockfd;
};

struct sessionLL{
	struct sessionNode* head;
	struct sessionNode* tail;
};

struct sessionNode {
	char sessionID[MAX_NAME];
	struct sessionNode* next;
};

enum msgType {
    LOGIN,
    LO_ACK,
    LO_NAK,
    EXIT,
    JOIN,
    JN_ACK,
    JN_NAK,
    LEAVE_SESS,
    NEW_SESS,
    NS_ACK,
    MESSAGE,
    QUERY,
    QU_ACK
};

void* get_in_addr(struct sockaddr *sa);

// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
	}
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

