int getTorrentId(sqlite3* db, char* hash){
	sqlite3_stmt* queryTorrent;
	int error;
	
	error=sqlite3_prepare_v2(db,QUERY_TORRENT_ID,strlen(QUERY_TORRENT_ID),&queryTorrent,NULL);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		return DB_FAILED;
	}
	
	error=sqlite3_bind_text(queryTorrent,1,hash,strlen(hash),SQLITE_TRANSIENT);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(queryTorrent);
		return DB_FAILED;
	}
	
	error=sqlite3_step(queryTorrent);
	switch(error){
		case SQLITE_DONE:
			error=DB_NOSUCHHASH;
			break;
		case SQLITE_BUSY:
			ERROR(printf("SQLite timeout elapsed while querying torrent ID\n"));
			error=DB_FAILED;
			break;
		case SQLITE_ERROR:
			ERROR(printf("SQLite encountered an error while querying torrent ID\n"));
			error=DB_FAILED;
			break;
		case SQLITE_ROW:
			error=sqlite3_column_int(queryTorrent,0);
			break;
	}
	sqlite3_finalize(queryTorrent);
	return error;
}

int queryTorrentParam(sqlite3* db, char* query, int torrent){
	sqlite3_stmt* queryStatement;
	int error;
	
	error=sqlite3_prepare_v2(db,query,strlen(query),&queryStatement,NULL);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		return DB_FAILED;
	}
	
	error=sqlite3_bind_int(queryStatement,1,torrent);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(queryStatement);
		return DB_FAILED;
	}
	
	error=sqlite3_step(queryStatement);
	switch(error){
		case SQLITE_DONE:
			//this should never happen
			error=DB_FAILED;
			break;
		case SQLITE_BUSY:
			ERROR(printf("SQLite timeout elapsed while querying a torrent parameter\n"));
			error=DB_FAILED;
			break;
		case SQLITE_ERROR:
			ERROR(printf("SQLite encountered an error while querying a torrent Parameter\n"));
			error=DB_FAILED;
			break;
		case SQLITE_ROW:
			error=sqlite3_column_int(queryStatement,0);
			break;
	}
	sqlite3_finalize(queryStatement);
	return error;
}

int addPeer(sqlite3* db, ANNOUNCE_REQUEST* ar, int torrent){
	sqlite3_stmt* addPeer;
	int error;
	struct timeval time={0,0};
	
	if(gettimeofday(&time,NULL)){
		ERROR(printf("Could not get current time\n"));
		return E_GENERIC;
	}

	error=sqlite3_prepare_v2(db,ADD_TORRENT_PEER,strlen(ADD_TORRENT_PEER),&addPeer,NULL);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		return DB_FAILED;
	}
		
	//torrent
	error=sqlite3_bind_int(addPeer,1,torrent);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(addPeer);
		return DB_FAILED;
	}
	
	//id
	error=sqlite3_bind_text(addPeer,2,ar->peer_id,strlen(ar->peer_id),SQLITE_TRANSIENT);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(addPeer);
		return DB_FAILED;
	}
	
	//ip
	error=sqlite3_bind_text(addPeer,3,ar->ip,strlen(ar->ip),SQLITE_TRANSIENT);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(addPeer);
		return DB_FAILED;
	}
	
	//port
	error=sqlite3_bind_int(addPeer,4,ar->port);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(addPeer);
		return DB_FAILED;
	}
	
	//expires
	error=sqlite3_bind_int(addPeer,5,time.tv_sec);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(addPeer);
		return DB_FAILED;
	}
	
	//protocol
	error=sqlite3_bind_int(addPeer,6,ar->protover);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(addPeer);
		return DB_FAILED;
	}

	error=sqlite3_step(addPeer);
	switch(error){
		case SQLITE_DONE:
			NOTICE_LO(printf("Added peer %s to db\n",ar->ip));
			error=0;
			break;
		case SQLITE_BUSY:
			ERROR(printf("SQLite encountered a timeout while adding a peer\n"));
			error=DB_FAILED;
			break;
		case SQLITE_ERROR:
			ERROR(printf("SQLite encountered an error while adding a peer\n"));
			error=DB_FAILED;
			break;
	}
	
	sqlite3_finalize(addPeer);
	return error;
}

int prunePeers(sqlite3* db, int expireInterval){
	sqlite3_stmt* killPeers;
	struct timeval time={0,0};
	int error;
	
	if(gettimeofday(&time,NULL)){
		ERROR(printf("Could not get current time (prune)\n"));
		return E_GENERIC;
	}
	
	//prepare pruning
	error=sqlite3_prepare_v2(db,QUERY_PRUNE_PEERS,strlen(QUERY_PRUNE_PEERS),&killPeers,NULL);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		return DB_FAILED;
	}
	
	error=sqlite3_bind_int(killPeers,1,time.tv_sec-expireInterval);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(killPeers);
		return DB_FAILED;
	}
	
	error=sqlite3_step(killPeers);
	switch(error){
		case SQLITE_DONE:
			NOTICE_LO(printf("Pruned %d peers from database\n",sqlite3_changes(db)));
			error=0;
			break;
		case SQLITE_BUSY:
			ERROR(printf("SQLite encountered a timeout while pruning the peers table\n"));
			error=DB_FAILED;
			break;
		case SQLITE_ERROR:
			ERROR(printf("SQLite encountered an error while pruning the peers table\n"));
			error=DB_FAILED;
			break;
		default:
			NOTICE_HI(printf("SQLite return code not accounted for (prune)\n"));
			break;
	}
	
	sqlite3_finalize(killPeers);
	
	return error;
}

int updatePeer(sqlite3* db, int torrentid, ANNOUNCE_REQUEST* ar){
	sqlite3_stmt* queryPeer;
	sqlite3_stmt* modifyPeer;
	sqlite3_stmt* updateTorrent;
	int peer_row=0;
	struct timeval time={0,0};
	int error;
	
	//test if in db
	error=sqlite3_prepare_v2(db,QUERY_PEER_BY_ID,strlen(QUERY_PEER_BY_ID),&queryPeer,NULL);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		return DB_FAILED;
	}

	error=sqlite3_bind_int(queryPeer,1,torrentid);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(queryPeer);
		return DB_FAILED;
	}
	
	error=sqlite3_bind_text(queryPeer,2,ar->peer_id,strlen(ar->peer_id),SQLITE_TRANSIENT);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(queryPeer);
		return DB_FAILED;
	}
	
	error=sqlite3_step(queryPeer);
	if(error==SQLITE_ROW){
		//DEBUG(printf("Using index from row %s\n",sqlite3_column_name(queryPeer,0)));
		peer_row=sqlite3_column_int(queryPeer,0);
	}
	
	sqlite3_finalize(queryPeer);
	
	if(error==SQLITE_DONE){
		//peer not existing, add
		DEBUG(printf("Peer not in database, calling add\n"));
		error=addPeer(db,ar,torrentid);
		if(error==0){//dem returncodes...
			DEBUG(printf("Peer added successfully, recursing to update\n"));
			return updatePeer(db,torrentid,ar);//yay recursion
		}
		else{
			ERROR(printf("Could not update peer: failed to add\n"));
			return DB_FAILED;
		}
	}
	else if(error!=SQLITE_ROW){
		ERROR(printf("Database error or timeout while updating peer\n"));
		return DB_FAILED;
	}
	
	//update timer / status values
	//FIXME if event is "stopped", we should probably remove the peer from the db

	if(gettimeofday(&time,NULL)){
		ERROR(printf("Could not get current time (update)\n"));
		return E_GENERIC;
	}
	
	//prepare update statement
	error=sqlite3_prepare_v2(db,UPDATE_PEER_DATA,strlen(UPDATE_PEER_DATA),&modifyPeer,NULL);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		return DB_FAILED;
	}
	
	error=sqlite3_bind_int(modifyPeer,1,ar->left);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(modifyPeer);
		return DB_FAILED;
	}
	
	error=sqlite3_bind_int(modifyPeer,2,ar->event);//is this "completed" for seeds?
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(modifyPeer);
		return DB_FAILED;
	}
	
	error=sqlite3_bind_int(modifyPeer,3,time.tv_sec);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(modifyPeer);
		return DB_FAILED;
	}
	
	error=sqlite3_bind_int(modifyPeer,4,peer_row);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(modifyPeer);
		return DB_FAILED;
	}
	
	error=sqlite3_step(modifyPeer);
	switch(error){
		case SQLITE_DONE:
			DEBUG(printf("Updated peer with id %d\n",peer_row));
			error=0;
			break;
		case SQLITE_BUSY:
			ERROR(printf("SQLite encountered a timeout while updating a peer\n"));
			error=DB_FAILED;
			break;
		case SQLITE_ERROR:
			ERROR(printf("SQLite encountered an error while updating a peer\n"));
			error=DB_FAILED;
			break;
	}
	
	sqlite3_finalize(modifyPeer);//FIXME errhandling?
	
	if(error!=0||ar->event!=COMPLETED){
		return error;
	}
	
	error=sqlite3_prepare_v2(db,INCREASE_COMPLETION_COUNT,strlen(INCREASE_COMPLETION_COUNT),&updateTorrent,NULL);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		return DB_FAILED;
	}

	error=sqlite3_bind_int(updateTorrent,1,torrentid);
	if(error!=SQLITE_OK){
		ERROR(printf("Error while preparing statement [%d]\n",__LINE__));
		sqlite3_finalize(updateTorrent);
		return DB_FAILED;
	}
	
	error=sqlite3_step(updateTorrent);
	switch(error){
		case SQLITE_DONE:
			DEBUG(printf("Increased torrent %d completion count\n",torrentid));
			error=0;
			break;
		case SQLITE_BUSY:
			ERROR(printf("SQLite encountered a timeout while updating a torrent\n"));
			error=DB_FAILED;
			break;
		case SQLITE_ERROR:
			ERROR(printf("SQLite encountered an error while updating a torrent\n"));
			error=DB_FAILED;
			break;
	}
	
	sqlite3_finalize(updateTorrent);//FIXME errhandling?
	
	return error;
}