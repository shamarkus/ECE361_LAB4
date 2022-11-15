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

#define LOGIN_CMD = "/login";
#define LOGOUT_CMD = "/logout";
#define JOINSESSION_CMD = "/joinsession";
#define LEAVESESSION_CMD = "/leavesession";
#define CREATESESSION_CMD = "/createsession";
#define LIST_CMD = "/list";
#define QUIT_CMD = "/quit";

//Struct Definitions
struct message {
	unsigned int type;
	unsigned int size;
	unsigned char source[MAX_NAME];
	unsigned char data[MAX_DATA];
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

bool inSession = false;
