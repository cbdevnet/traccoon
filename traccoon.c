//#include <stdio.h>
#include <stdlib.h>			//exit
#include <stdbool.h>		//bool
#include <pthread.h>		//pthread_create
#include <ctype.h>			//tolower (gcc-strict)
#include <malloc.h>			//malloc, realloc
#include <limits.h>			//INT_MAX
#include <signal.h>			//signal
#include <unistd.h>			//close (gcc-strict)
//#include <sys/types.h>
//#include <sys/socket.h>
#include <sys/time.h>		//gettimeofday (?)
#include <arpa/inet.h>		//inet_pton, inet_ntop (gcc-strict)
#include <netdb.h>			//socket stuff
#include <string.h>			//memset

#include "sqlite3.h"

#include "neth.h"
#include "traccoon.h"
#include "statements.h"

#include "neth.c"
#include "db.c"
#include "callbacks.c"
#include "track.c"
#include "comm.c"
//#include "workerthread.c"
#include "thread.c"

int usage(char* name){
	printf("Hot diggidy, you forgot to specify some options!\n");
	printf("%s -f <dbfile>\t\tSet sqlite3 database file to use for storage (Default: tracker.db3)\n",name);
	printf("%s -c <dbfile>\t\tInitialize empty database in file\n",name);
	printf("%s -w <num>\t\tNumber of worker threads for handling connections (Default: 10)\n",name);
	printf("%s -b <host|addr>\tHost to bind server on (Default: 0.0.0.0)\n",name);
	printf("%s -i (4|6)\t\tForce IPv4/6 (Default: any)\n",name);
	printf("%s -p <num>\t\tUse Port (Default: 6969)\n",name);
	printf("%s -q <num>\t\tAccept queue size (Default: 20)\n",name);
	printf("%s -a <num>\t\tSet announce interval (seconds) (Default: 300)\n",name);
	return 1;
}

int main(int argc,char* argv[]){
	int i=0;									//doing this c90 compliant for tcc
	int error=0;								//errstate
	int workerthreads=10;						//number of worker threads (yarly)
	char* errstr=NULL;							//used to pass to some functions returning strings for errorcodes
	sqlite3* databaseHandle=NULL;				//sqlite3 db handle
	char* trackerDatabaseFile="tracker.db3";	//filename of the db to use
	_WORKER* threads=NULL;						//array of worker thread info structures
	char* bindhost=NULL;						//host to bind to
	char* port="6969";							//port to use
	int queuesize=20;							//size of accept() backlog and queue
	_CLIENT* aqueue;							//the accept() queue
	int sockfamily=AF_UNSPEC;					//family of server socket //FIXME unused
	int servsock;								//server socket
	char ipstr[INET6_ADDRSTRLEN];				//used to convert some ip
	int stallcount;
	int announceInterval=300;
	int yes=1;
	struct timeval timeNow={0,0};
	struct timeval timeLast={0,0};
	
	struct addrinfo addrinfo;
	struct addrinfo* servinfo;
	struct addrinfo* addr_it;
	memset(&addrinfo,0,sizeof(struct addrinfo));
	
	GLOBALSTATE.killserver=false;

	printf("This is Traccoon, version %s reporting in\n",_VERSION);
	
	if(!(argc%2)){
		exit(usage(*argv));
	}
	
	for(i=1;i<argc;i+=2){
		if(*argv[i]!='-'){
			continue;
		}
		switch(*(argv[i]+1)){
			case 'f':
				trackerDatabaseFile=argv[i+1];
				printf("Using database file %s\n",trackerDatabaseFile);
				break;
			case 'c':
				//exit(createEmptyDatabase(argv[i+1])); TODO
				break;
			case 'w':
				workerthreads=strtoul(argv[i+1],NULL,10);
				printf("Using %d worker threads\n",workerthreads);
				break;
			case 'b':
				bindhost=argv[i+1];
				printf("Using bindhost %s\n",bindhost);
				break;
			case 'i':
				sockfamily=(*(argv[i+1])=='4')?AF_INET:AF_INET6;
				break;
			case 'p':
				port=argv[i+1];
				break;
			case 'q':
				queuesize=strtoul(argv[i+1],NULL,10);
				printf("Setting client queue size to %d\n",queuesize);
				break;
			case 'a':
				announceInterval=strtoul(argv[i+1],NULL,10);
				printf("Setting announce Interval to %d\n",announceInterval);
				break;
			//TODO: "no scrape" option
		}
	}
	
	threads=malloc(sizeof(_WORKER)*workerthreads);
	if(!threads){
		printf("Could not allocate memory for thread info\n");
		exit(MEM_ERROR);
	}
	
	aqueue=malloc(sizeof(_CLIENT)*queuesize);
	if(!aqueue){
		printf("Could not allocate memory for accept() buffer\n");
		free(threads);
		exit(MEM_ERROR);
	}
	memset(aqueue,0,sizeof(_CLIENT)*queuesize);
	
	if(gettimeofday(&timeLast,NULL)){
		ERROR(printf("Could not get current time (main)\n"));
		free(threads);
		free(aqueue);
		exit(GENERAL_FAILURE);
	}
	
	//client handling process
	printf("Opening database at %s\n",trackerDatabaseFile);
	error=sqlite3_open_v2(trackerDatabaseFile,&databaseHandle,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,NULL);
	if(error==SQLITE_OK){
		sqlite3_busy_timeout(databaseHandle,500);//todo error handling
		error=sqlite3_exec(databaseHandle,QUERY_TORRENT_COUNT,statusMessageCallback,(void*)CB_TORRENT_COUNT,&errstr);
		if(error==SQLITE_OK){
			error=0;
			printf("Spawning %d workers\n",workerthreads);
			for(i=0;i<workerthreads;i++){
				threads[i].tid=i+1;
				threads[i].queue=aqueue;
				threads[i].queuesize=queuesize;
				threads[i].shutdown=false;
				threads[i].announceInterval=announceInterval; //might want to add some randomness here
				threads[i].db=trackerDatabaseFile;
				
				if(pthread_create(&(threads[i].thread),NULL,workerthread,&threads[i])!=0){
					printf("Error creating thread #%d\n",i);
					//TODO destroy already created threads (and dbs)
					error=THREADCREATION_ERROR;
					break;
				}
				TRACEDEBUG(printf("%d [%d]\n",i,__LINE__));
			}
			
			LAZIEST_WORKER.threadID=1;
			LAZIEST_WORKER.activeSockets=0;
			
			if(error==0){
				addrinfo.ai_family=AF_UNSPEC;
				addrinfo.ai_socktype=SOCK_STREAM;
				addrinfo.ai_flags=AI_PASSIVE;
				
				error=getaddrinfo(bindhost,port,&addrinfo,&servinfo);
				if(error==0){
					for(addr_it=servinfo;addr_it;addr_it=servinfo->ai_next){
						servsock=socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);
						if(servsock!=-1){
							break;
						}
					}
					
					if(servsock!=-1){
						//got socket
						GLOBALSTATE.servsock=servsock;
						
						error=setsockopt(servsock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
						if(error!=-1){						
							void* addr;
							if(addr_it->ai_family==AF_INET){
								struct sockaddr_in* ipv4=(struct sockaddr_in*)addr_it->ai_addr;
								addr=&(ipv4->sin_addr);
							}
							else{
								struct sockaddr_in6* ipv6=(struct sockaddr_in6*)addr_it->ai_addr;
								addr=&(ipv6->sin6_addr);
							}

							inet_ntop(addr_it->ai_family,addr,ipstr,sizeof(ipstr));
							
							error=bind(servsock,addr_it->ai_addr,addr_it->ai_addrlen);
							if(error!=-1){
								printf("Bound to %s\n",ipstr);
								signal(SIGPIPE, SIG_IGN);
								error=listen(servsock,queuesize);
								if(error!=-1){
									signal(SIGINT,sigint_handler);
									while(!GLOBALSTATE.killserver){
										//loop through clientconns until empty found
										stallcount=0;
										for(i=0;;(i=(i+1)%queuesize)){
											if(aqueue[i].socket==0){
												//prepare slot
												aqueue[i].thread=0;
												aqueue[i].socket=accept(servsock,NULL,NULL);
												
												if(aqueue[i].socket==-1){
													printf("Error accepting a connection\n");
													aqueue[i].socket=0;
												}
												else{
													//printf("Accepted a connection\n");
													//we really dont care about race conditions here
													aqueue[i].thread=LAZIEST_WORKER.threadID;
												}
												break;
											}
											if(i==queuesize-1){
												stallcount++;
												printf("Queue exhausted, stalling (Round %d)\n",stallcount);
												usleep(5000);
												//TODO report which thread is not picking up his work
											}
										}
										
										if(gettimeofday(&timeNow,NULL)){
											ERROR(printf("Could not get current time (main update)\n"));
											error=GENERAL_FAILURE;
											GLOBALSTATE.killserver=true; //FIXME TODO check these 2 lines for working
										}
										
										//check if its time for pruning
										if(timeNow.tv_sec-timeLast.tv_sec>=(3*announceInterval)){
											NOTICE_LO(printf("Pruning peers (delta: %d)\n",timeNow.tv_sec-timeLast.tv_sec));
											timeLast=timeNow;
											prunePeers(databaseHandle,3*announceInterval);
										}
									}
								}
								else{
									printf("Failed to listen()\n");
									error=SOCKET_ERROR;
								}
							}
							else{
								//failed to bind
								printf("Failed to bind on %s\n",ipstr);
								error=SOCKET_ERROR;
							}
						}
						else{
							//could not set sock opts
							printf("Failed to set sock_reuse\n");
							error=SOCKET_ERROR;
						}
						close(servsock);
					}
					else{
						//could not get socket
						printf("Failed to get a socket\n");
						error=SOCKET_ERROR;
					}
				}
				else{
					//getaddrinfo failed
					error=SOCKET_ERROR;
					printf("Failed to get appropriate info\n");
				}
				freeaddrinfo(servinfo);
			}
		}
		else{
			printf("SQLite failed\n");
			if(errstr){
				printf("Message: %s\n",errstr);
				sqlite3_free(errstr);
			}
		}
	}
	else{
		printf("SQLite Error: %s\n",sqlite3_errmsg(databaseHandle));
		error=DB_ERROR;
	}
	
	GLOBALSTATE.killserver=true;
	if(error!=THREADCREATION_ERROR){
		printf("Waiting for worker threads to terminate...\n");
		for(i=0;i<workerthreads;i++){
			printf("Terminating thread #%2d... \n",threads[i].tid);
			threads[i].shutdown=true;
			pthread_join(threads[i].thread,NULL);
			//sqlite3_close(threads[i].db);
			printf("Thread joined.\n");
		}
	}
	
	for(i=0;i<queuesize;i++){
		if(aqueue[i].socket!=0){
			printf("Closing stray socket...\n");
			close(aqueue[i].socket);
		}
	}
	
	printf("Closing main thread db handle\n");
	if(sqlite3_close(databaseHandle)!=SQLITE_OK){
		printf("Failed to shut down main SQLite instance!\n");
	}
	
	free(threads);
	free(aqueue);
	
	printf("Exiting.\n");
	exit(error);
}