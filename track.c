void track(int socket, sqlite3* db, ANNOUNCE_REQUEST* ar, int interval){
	sqlite3_stmt* queryPeers;
	int error, torrentid, i, c, pos4, port, pos6, proto;
	int16_t bePort;
	char buf[1024];
	unsigned char buf4[1024];
	unsigned char buf6[1024];
	const unsigned char* ip;
	const unsigned char* peerid;
	
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	
	//prepare data
	strtolower(ar->ip);
	strtolower(ar->info_hash);
	destructiveURLEncodeFixup(ar->info_hash,MAX_ENC_HASH_LEN);
	
	sendHttpHeaders(socket,"200 OK",STANDARD_HEADER);
	
	torrentid=getTorrentId(db,ar->info_hash);
	switch(torrentid){
		case DB_NOSUCHHASH:
			NOTICE_LO(printf("Got query for unknown hash %s\n",ar->info_hash));
			sendString(socket,"d14:failure reason19:Hash not recognizede");
			return;
		case DB_FAILED:
			sendString(socket,"d14:failure reason20:Database error (TID)e");
			return;
	}
	
	NOTICE_LO(printf("%s:%d requested %d %s peers for %d\n",ar->ip,ar->port,ar->numwant,(ar->compact)?"compact":"non-compact",torrentid));
	
	//get peers
	error=sqlite3_prepare_v2(db,QUERY_TORRENT_PEERS,strlen(QUERY_TORRENT_PEERS),&queryPeers,NULL);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sendString(socket,"d14:failure reason12:Server errore");
		return;
	}
	
	error=sqlite3_bind_int(queryPeers,1,torrentid);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(queryPeers);
		sendString(socket,"d14:failure reason12:Server errore");
		return;
	}
	
	//only using non-compact syntax
	//send dict begin
	snprintf(buf,sizeof(buf)-1,"d8:intervali%de5:peers%s",interval,(ar->compact)?"":"l");
	//char* respOpen="d8:intervali300e5:peersl";//TODO test non-compact replies
	char* respClose="ee";
	if(ar->compact){
		//respOpen="d8:intervali300e5:peers";
		respClose="e";
		pos4=0;
		pos6=0;
	}
	sendString(socket,buf);
	
	DEBUG(printf("Opening with \"%s\"\n",buf));

	//TODO plausible deniability
	
	error=SQLITE_ROW;
	for(i=0;error==SQLITE_ROW&&i<ar->numwant;i++){
		error=sqlite3_step(queryPeers);
		switch(error){
			case SQLITE_DONE:
				NOTICE_LO(printf("End of peer list reached\n"));
				//end
				break;
			case SQLITE_BUSY:
				ERROR(printf("SQLite encountered a timeout while querying peers\n"));
				break;
			case SQLITE_ERROR:
				ERROR(printf("SQLite encountered an error while querying peers\n"));
				break;
			case SQLITE_ROW:
				//push row to socket
				//get data
				ip=sqlite3_column_text(queryPeers,1);
				peerid=sqlite3_column_text(queryPeers,0);
				port=sqlite3_column_int(queryPeers,2);
				proto=sqlite3_column_int(queryPeers,3);
				
				//check if we're our own peer
				if(!strcmp(ar->ip,(char*)ip)&&!strcmp(ar->peer_id,(char*)peerid)){
					NOTICE_LO(printf("Is peer itself, skip\n"));
					continue;//skip
				}
				
				NOTICE_LO(printf("Pushing peer %s Port %d ",ip,port));
				if(!ar->compact){//non-compact response
					NOTICE_LO(printf("via non-compact\n"));
					if(ar->no_peer_id){
						snprintf(buf,sizeof(buf)-1,"d2:ip%d:%s4:porti%dee",strlen((char*)ip),ip,port);//TODO check return value
					}
					else{
						snprintf(buf,sizeof(buf)-1,"d2:ip%d:%s4:porti%de7:peer id%d:%se",strlen((char*)ip),ip,port,strlen((char*)peerid),peerid);//TODO check return value
					}
					sendString(socket,buf);
				}
				else{
					//build compact response
					NOTICE_LO(printf("via compact\n"));
					if(proto==4){
						//get address in network order
						if(inet_pton(AF_INET,(const char*)ip,&(sin.sin_addr))!=1){
							ERROR(printf("Could not convert ip to network order [%d]\n",__LINE__));
							continue;
						}
						//copy into buffer
						memcpy(buf4+pos4,&(sin.sin_addr),sizeof(sin.sin_addr));
						pos4+=sizeof(sin.sin_addr);
						//add port
						bePort=htons(port);//FIXME TODO this might be wrong
						memcpy(buf4+pos4,&bePort,sizeof(int16_t));
						pos4+=sizeof(int16_t);
					}
					else{
						//TODO test this whole branch
						//add to peer6
						if(inet_pton(AF_INET6,(const char*)ip,&(sin6.sin6_addr))!=1){
							ERROR(printf("Could not convert ip to network order [%d]\n",__LINE__));
							continue;
						}
						//copy into buffer
						memcpy(buf6+pos6,&(sin6.sin6_addr),sizeof(sin6.sin6_addr));
						DEBUG(printf("Copied %d bytes of ipv6 address\n",sizeof(sin6.sin6_addr)));
						pos6+=sizeof(sin6.sin6_addr);
						//add port
						bePort=htons(port);
						memcpy(buf6+pos6,&bePort,sizeof(int16_t));
						DEBUG(printf("Copied %d bytes of ipv6 port\n",sizeof(int16_t)));
						pos6+=sizeof(int16_t);
					}
				}
				break;
		}
	}
	sqlite3_finalize(queryPeers);
	
	if(ar->compact){
		c=snprintf(buf,sizeof(buf)-1,"%d:",pos4);
		memcpy(buf+c,buf4,pos4);
		reallySend(socket,buf,c+pos4);
		DEBUG(printf("Sent %d bytes of compact ipv4 peer list\n",pos4));	
		if(pos6>0){
			c=snprintf(buf,sizeof(buf)-1,"10:peers_ipv6%d:",pos6);//FIXME this might be the wrong dict key
			memcpy(buf+c,buf6,pos6);
			reallySend(socket,buf,c+pos6);
			DEBUG(printf("Sent %d bytes of compact ipv6 peer list\n",pos6));
		}
	}
	
	//send dict end
	sendString(socket,respClose);
	
	updatePeer(db,torrentid,ar);
}

void scrape(int socket, sqlite3* db, char* hash){
	char buf[512];
	int torrent;
	if(hash){
		sendHttpHeaders(socket,"200 OK",STANDARD_HEADER);
		
		torrent=getTorrentId(db,hash);
		switch(torrent){
			case DB_NOSUCHHASH:
				sendString(socket,"d14:failure reason19:Hash not recognizede");
				return;
			case DB_FAILED:
				sendString(socket,"d14:failure reason20:Database error (TID)e");
				return;
		}

		NOTICE_LO(printf("Scraping for torrent %d\n",torrent));

		snprintf(buf,sizeof(buf)-1,"d5:files%d:%sd8:completei%de10:downloadedi%de10:incompletei%deee",strlen(hash),hash,queryTorrentParam(db,GET_SEED_COUNT,torrent),queryTorrentParam(db,GET_COMPLETION_COUNT,torrent),queryTorrentParam(db,GET_PEERS_COUNT,torrent));
		sendString(socket,buf);
	}
	else{
		NOTICE_LO(printf("Client tried a global scrape\n"));
		sendHttpHeaders(socket,"501 Not Implemented",STANDARD_HEADER);
	}
}