#define CB_TORRENT_COUNT 9001

int statusMessageCallback(void* param, int argc, char **argv, char **column){
	int id=(intptr_t)param;
	switch(id){
		case CB_TORRENT_COUNT:
			printf("Tracking %s torrents\n",argv[0]);
			break;
	}
	return 0;
}

int dumpDataCallback(void* param, int argc, char **argv, char **column){
	int i;
	
	for(i=0;i<argc;i++){
		printf("%s - %s\n",column[i],argv[i]);
	}
	return 0;
}

void sigint_handler(int param){
	printf("Received SIGINT, setting shutdown flag\n");
	GLOBALSTATE.killserver=true;
	//shutdown(GLOBALSTATE.servsock,0); //FIXME work this in 
	close(GLOBALSTATE.servsock);
}