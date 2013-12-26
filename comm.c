bool parseSocket(_CLIENT* client, sqlite3* db, int announceInterval){
	char buf[1024];
	char* buffer=buf;
	int size,c;
	bool handled=false;
	ANNOUNCE_REQUEST ar;
	socklen_t socklen;
	struct sockaddr_storage conn;
	
	size=recv(client->socket,buf,sizeof(buf)-1,MSG_PEEK);
	if(size==0||size==sizeof(buf)-1){
		ERROR(printf("Erroneous link\n"));
		handled=true;
	}
	else{
		buf[size]=0;
		SPAM(printf("%d bytes of data received\n",size));
		if(!strncmp(buffer,"GET /",5)){
			buffer+=5;
			//http client
			if(!strncmp(buffer,"announce",8)){
				buffer+=8;
				//announce request, parse
				memset(&ar,0,sizeof(ANNOUNCE_REQUEST));
				socklen=sizeof(struct sockaddr_storage);
				if(getpeername(client->socket,(struct sockaddr*)&conn,&socklen)!=-1){
					//check mandatory arguments
					c=charIndex((unsigned char*)buffer,' ');
					if(c!=-1){
						buffer[c]=0;//terminate buffer after request string
						char* hash=textAfter(buffer,"info_hash=");
						char* peerid=textAfter(buffer,"peer_id=");
						char* port=textAfter(buffer,"port=");
						char* event=textAfter(buffer,"event=");
						char* numwant=textAfter(buffer,"numwant=");
						char* left=textAfter(buffer,"left=");
						
						if(hash&&peerid&&port){
							//get compact flag
							ar.compact=false;
							if(strstr(buffer,"compact=1")){
								ar.compact=true;
							}
							
							//get no_peer_id flag
							ar.no_peer_id=false;
							if(strstr(buffer,"no_peer_id=1")){
								ar.no_peer_id=true;
							}
							
							//get event data
							ar.event=NONE;
							if(event&&(!strncmp(event,"st",2)||!strncmp(event,"co",2))){
								switch(event[2]){
									case 'a':
										ar.event=STARTED;
										break;
									case 'o':
										ar.event=STOPPED;
										break;
									case 'm':
										ar.event=COMPLETED;
										break;
								}
							}
							
							//calculate parameter lengths
							int hash_len=httpParamLength(hash);
							int peerid_len=httpParamLength(peerid);
							
							if(decodedStrlen(hash,hash_len)==MAX_HASH_LEN&&decodedStrlen(peerid,peerid_len)==MAX_HASH_LEN){
								strncpy(ar.info_hash,hash,hash_len);//FIXME TODO ensure string termination
								strncpy(ar.peer_id,peerid,peerid_len);
								ar.port=strtoul(port,NULL,10);
								
								if(numwant){
									ar.numwant=strtoul(numwant,NULL,10);
								}
								if(ar.numwant>MAXPEERS_SENT||ar.numwant<=0){//there are clients that actually send 0 here?
									ar.numwant=MAXPEERS_SENT;
								}
								
								ar.left=0;
								if(left){
									ar.left=strtoul(left,NULL,10);
								}
								
								if(conn.ss_family==AF_INET6){
									inet_ntop(conn.ss_family,&(((struct sockaddr_in6*)&conn)->sin6_addr),ar.ip,INET6_ADDRSTRLEN);
									ar.protover=6;
								}
								else{
									inet_ntop(conn.ss_family,&(((struct sockaddr_in*)&conn)->sin_addr),ar.ip,INET6_ADDRSTRLEN);
									ar.protover=4;
								}
								
								//DEBUG(printf("info_hash: %s, peer_id: %s, addr %s, port %d\n",ar.info_hash,ar.peer_id,ar.ip,ar.port));
								
								track(client->socket, db, &ar, announceInterval);
								handled=true;
							}
							else{
								//vital parameters out of bounds
								if(client->recv_iter>=MAX_RECV_ITER){
									sendHttpHeaders(client->socket,"400 Bad Syntax","X-Failure-Reason: Failed Parameter Validation\r\n");
								}
							}
						}
						else{
							//request is missing vital data
							if(client->recv_iter>=MAX_RECV_ITER){
								sendHttpHeaders(client->socket,"408 Timed out","X-Failure-Reason: Missing parameters\r\n");
							}
						}
					}
					else{
						//request was not properly terminated
						if(client->recv_iter>=MAX_RECV_ITER){
							sendHttpHeaders(client->socket,"400 Bad Syntax","X-Failure-Reason: Thats just plain wrong\r\n");
						}
					}
				}
				else{
					ERROR(printf("Call to getpeername failed\n"));
					handled=true;
				}
			}
			else if(!strncmp(buffer, "scrape", 6)){
				buffer+=6;
				//scrape request
				c=charIndex((unsigned char*)buffer, ' ');
				if(c!=-1){
					buffer[c]=0;
					char* hash=textAfter(buffer,"info_hash=");
					if(!hash){
						scrape(client->socket, db, NULL);
						handled=true;
					}
					else{
						//get hash
						int hash_len=httpParamLength(hash);
						if(decodedStrlen(hash,hash_len)==MAX_HASH_LEN){
							unsigned char hashbuf[MAX_URLENC_HASH_LEN+1];
							memcpy(hashbuf,hash,hash_len);
							hashbuf[hash_len]=0;
							destructiveURLDecode(hashbuf);
							hashEncodeHex(hashbuf,(sizeof(hashbuf)/sizeof(unsigned char))-1);
							scrape(client->socket, db, hashbuf);
							handled=true;
						}
					}
				}
				else{
					//request was not properly terminated
					if(client->recv_iter>=MAX_RECV_ITER){
						sendHttpHeaders(client->socket,"400 Bad Syntax","X-Failure-Reason: Thats just plain wrong\r\n");
					}
				}
			}
			else{
				//some other http
				sendHttpHeaders(client->socket,"200 OK",STANDARD_HEADER);
				for(c=0;c<10;c++){
					sendString(client->socket,"Making Milhouse cry is not a science project\r\n");
				}
				handled=true;
			}
		}
		else{
			//non-http client or wrong operation
			sendHttpHeaders(client->socket,"405 Not supported",STANDARD_HEADER);
			handled=true;
		}
	}
	
	if(handled||client->recv_iter>=MAX_RECV_ITER){
		handled=true;
		//flush kernel buffers
		recv(client->socket,buf,sizeof(buf)-1,0);
	}
	return handled;
}