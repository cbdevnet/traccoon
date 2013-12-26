#define _VERSION "0.5 \"obviously ordered\""

#define DB_ERROR 9001
#define MEM_ERROR 9002
#define THREADCREATION_ERROR 9003
#define SOCKET_ERROR 9004
#define GENERAL_FAILURE 9005

#define DB_NOSUCHHASH -2
#define DB_FAILED -1
#define E_GENERIC -3

#define MAXPEERS_SENT 50
#define MAX_RECV_ITER 200

#define STANDARD_HEADER "X-LOLz-Had: much\r\n"

#define OFF(a)
#define ON(a) (a)

//Debug message stuff
#define TRACEDEBUG(a)
#define SPAM(a)
#define DEBUG(a) (a)
#define NOTICE_LO(a) (a)
#define NOTICE_HI(a) (a)
#define ERROR(a) (a)

typedef struct /*_CLIENT*/ {
	volatile int thread;
	volatile int socket;
	int recv_iter;
} _CLIENT;

typedef struct /*_THREADPARAM*/ {
	int tid;				//this threads int id
	char* db;				//pointer to database filename
	pthread_t thread;		//pthread_t of this thread
	_CLIENT* queue;			//pointer to the global accept queue
	int queuesize;			//size of the global accept queue
	volatile bool shutdown;	//shutdown flag for this thread
	int announceInterval;
} _WORKER;

struct {
	volatile int threadID;
	volatile int activeSockets;
} LAZIEST_WORKER;

struct {
	volatile bool killserver;
	int servsock;
} GLOBALSTATE;

typedef enum /*BT_EVENT*/{
	STARTED=3,
	STOPPED=2,
	COMPLETED=1,
	NONE=0
} BT_EVENT;

typedef struct /*_ANNOUNCE_PARAMS*/{
	/*required data for db*/
	char info_hash[MAX_URLENC_HASH_LEN+1];	//torrent hash
	char peer_id[MAX_URLENC_HASH_LEN+1];	//peerid
	char ip[INET6_ADDRSTRLEN];
	int port;			//port
	int protover;
	
	BT_EVENT event;
	/*int uploaded;
	int downloaded;*/
	int left;
	
	/*optional data for answer*/
	bool compact;
	bool no_peer_id;
	int numwant;
} ANNOUNCE_REQUEST;