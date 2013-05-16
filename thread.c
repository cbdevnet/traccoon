void* workerthread(void* param){
	//thread local variables
	_WORKER* tp;
	sqlite3* db;				//database connection
	_CLIENT* clients;			//thread-local client buffer
	int clients_size;			//size of the above buffer
	int clients_active;			//number of active sockets in above buffer
	int clients_queued;			//number of clients queued for this thread
	int i,c;					//random numbers.
	struct timeval timeout;		//timeout used for select call
	int fdmax;					//for keeping track of the highest socket descriptor
	fd_set readset;				//set of socket descriptors passed to select
	
	//initialize thread data
	tp=param;
	clients=NULL;
	clients_size=0;
	clients_active=0;
	
	//initialize sqlite
	if(sqlite3_open_v2(tp->db,&db,SQLITE_OPEN_READWRITE,NULL)!=SQLITE_OK){
		ERROR(printf("Failed to open SQLite instance for thread #%d\n",tp->tid));
		return NULL;
	}
	if(sqlite3_busy_timeout(db,500)!=SQLITE_OK){
		//this is bad but non-lethal
		ERROR(printf("Failed to set db timeout in #%d\n",tp->tid));
	}
	
	
	while(!tp->shutdown){
		//get number of active clients
		clients_active=0;
		for(i=0;i<clients_size;i++){
			if(clients[i].socket!=0){
				clients_active++;
			}
		}
		
		//get (estimated) number of connections waiting in main queue
		clients_queued=0;
		for(i=0;i<tp->queuesize;i++){
			if(tp->queue[i].thread==tp->tid){
				clients_queued++;
			}
		}
		
		//calculate allocation delta
		i=clients_queued-(clients_size-clients_active);
		
		//expand buffer
		if(i>0){
			NOTICE_LO(printf("Thread #%d expanding client buffer by %d to %d\n",tp->tid,i,clients_size+i));
			clients=realloc(clients,sizeof(_CLIENT)*(clients_size+i));
			if(!clients){
				ERROR(printf("Thread #%d failed to expand the client buffer, aborting\n",tp->tid));
				return NULL;//FIXME abort cleanly here
			}
			
			//initialize new buffer segments
			for(c=0;c<i;c++){
				clients[clients_size+c].socket=0;	
			}
			
			clients_size+=i;
		}
		
		//copy new connections
		c=0;//used to find empty buffer slots
		for(i=0;i<tp->queuesize;i++){
			if(tp->queue[i].thread==tp->tid){
				//find a buffer slot for this client
				for(;c<clients_size;c++){
					if(clients[c].socket==0){
						break;
					}
				}
				if(c==clients_size){
					//need to realloc the buffer to fit this client
					NOTICE_HI(printf("Thread #%d needs to do on-demand realloc\n",tp->tid));
					clients=realloc(clients,++clients_size*sizeof(_CLIENT));
					if(!clients){
						ERROR(printf("Thread #%d failed to do on-demand realloc. Aborting.\n",tp->tid));
						return NULL;//FIXME this should abort cleanly (go through thread cleanup)
					}
				}
				
				//copy the connection data
				memcpy(&clients[c],&(tp->queue[i]),sizeof(_CLIENT));
				
				//initialise it for thread usage
				clients[c].recv_iter=0;
				
				//release the main queue slot
				tp->queue[i].thread=0;
				tp->queue[i].socket=0;
				
				//increase the number of active clients
				clients_active++;
			}
		}
		
		//update load balancing structures
		if(LAZIEST_WORKER.threadID==tp->tid||LAZIEST_WORKER.activeSockets>clients_active){
			OFF(SPAM(printf("#%d will become laziest\n",tp->tid)));
			LAZIEST_WORKER.threadID=tp->tid;
			LAZIEST_WORKER.activeSockets=clients_active;
		}
		
		//create select set
		fdmax=0;
		FD_ZERO(&readset);

		for(i=0;i<clients_size;i++){
			if(clients[i].socket!=0){
				FD_SET(clients[i].socket,&readset);
				if(clients[i].socket>fdmax){
					fdmax=clients[i].socket;
				}
				clients[i].recv_iter++;
			}
		}
		
		//update timeout values
		timeout.tv_sec=0;
		timeout.tv_usec=2000;
		
		if(fdmax>0){
			//select
			select(fdmax+1,&readset,NULL,NULL,&timeout);
			
			//iterate over set
			bool handled;
			for(i=0;i<clients_size;i++){
				if(clients[i].socket!=0){
					handled=false;
					if(FD_ISSET(clients[i].socket,&readset)){
						handled=parseSocket(clients+i,db,tp->announceInterval);
					}
					if(!handled&&clients[i].recv_iter>MAX_RECV_ITER){
						OFF(printf("Timed out after %d iterations\n",clients[i].recv_iter));
						sendHttpHeaders(clients[i].socket,"408 Timed Out","X-Failure-Reason: Youre doing it wrong\n");
						handled=true;
					}
					if(handled){
						close(clients[i].socket);
						clients[i].socket=0;
						clients_active--;
					}
				}
			}
		}
		else{
			//we do not have any active clients to be selected
			SPAM(printf("Idling\n"));
			usleep(20000);
		}
	}
	
	//thread cleanup
	//DEBUG(printf("Thread #%d shutting down\n",tp->tid));
	
	//get out of load balancing
	if(LAZIEST_WORKER.threadID==tp->tid){
		LAZIEST_WORKER.activeSockets=INT_MAX;
		//make sure we do not get assigned any more connections
		while(LAZIEST_WORKER.threadID==tp->tid&&!GLOBALSTATE.killserver){//FIXME this might lead to an endless loop when only one thread is active
			usleep(5000);
		}
	}
	
	//close any open connections for the thread
	if(clients){
		for(i=0;i<clients_size;i++){
			if(clients[i].socket!=0){
				close(clients[i].socket);
			}
		}
		free(clients);
	}
	
	//close/reassign clients waiting in the main queue
	for(i=0;i<tp->queuesize;i++){
		if(tp->queue[i].thread==tp->tid){
			if(GLOBALSTATE.killserver){
				//close
				close(tp->queue[i].socket);
				tp->queue[i].thread=0;
				tp->queue[i].socket=0;
			}
			else{
				//reassign
				tp->queue[i].thread=LAZIEST_WORKER.threadID;
			}
		}
	}
	
	//close sqlite
	if(sqlite3_close(db)!=SQLITE_OK){
		ERROR(printf("Thread #%d could not shut down SQLite instance\n",tp->tid));
	}
	
	return tp;
}