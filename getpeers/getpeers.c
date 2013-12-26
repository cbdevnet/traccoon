#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/sha.h>

#ifdef _WIN32
	#include <ws2tcpip.h>
	PCTSTR WSAAPI inet_ntop(int Family, PVOID pAddr, PCTSTR pStringBuf, size_t StringBufSize);
#else
	#include <unistd.h> //close
	#define closesocket close
	#include <sys/types.h> //socket
	#include <sys/socket.h> //socket
	#include <netdb.h> //getaddrinfo
	#include <arpa/inet.h> //inet_ntop
#endif

#ifdef ERROR
	#undef ERROR
#endif
#define ERROR(a) (a)
#define OFF(a)
#define DEBUG(a) (a)

#include "../benc/benc.h"
#include "../encoding.h"
#include "../encoding.c"

#include "file.c"
#include "net.c"

int usage(char* fn){
	printf("%s <infile>\t\tGet information from <infile>\n",fn);
	printf("%s -t <tracker-url>\tUse specific tracker\n",fn);
	printf("%s -h <info-hash>\t\tQuery for specific hash\n",fn);
	printf("%s -c\t\t\tRequest peers in compact notation [No]\n",fn);
	printf("%s -s\t\t\tAlso issue scrape request [No]\n",fn);//FIXME implement
	printf("%s -v <4|6>\t\tForce IPv4/6 connection\n",fn);
	printf("%s -n <peers>\t\tNumber of peers to request [50]\n",fn);
	printf("%s -p <port>\t\tOpened port [6969]\n",fn);
	printf("%s -e <event>\t\tEvent to send [none]\n",fn);
	printf("%s -i <peerid>\t\tURLEncoded peer ID to send [-TC0010-%%rand%%]\n",fn);
	return 1;
}

int main(int argc, char** argv){
	srand(time(NULL));
	int i,c,len;
	char* infile=NULL;
	
	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	
	char* response_buffer=NULL;
	char* response_body=NULL;
	
	char* numwant=NULL;
	char* local_port=NULL;
	char* bt_event=NULL;
	char* tracker=NULL;
	bool request_compact=false;
	bool request_scrape=false;
	char hash[MAX_URLENC_HASH_LEN+1];
	char peerid[MAX_URLENC_HASH_LEN+1];
	memset(hash,0,sizeof(hash));
	memset(peerid,0,sizeof(hash));
	
	if(argc<2){
		exit(usage(argv[0]));
	}
	
	for(i=1;i<argc;i++){
		if(argv[i][0]=='-'){
			switch(argv[i][1]){
				case 'c':
					request_compact=true;
					continue;
				case 's':
					request_scrape=true;
					continue;
			}
			if(i+1<argc){
				switch(argv[i][1]){
					case 't':
						tracker=malloc((strlen(argv[i+1])+1)*sizeof(char));
						if(!tracker){
							printf("Failed to allocate sufficient memory, aborting\n");
							return -1;
						}
						memcpy(tracker,argv[i+1],strlen(argv[i+1]));
						tracker[strlen(argv[i+1])]=0;
						break;
					case 'h':
						len=strlen(argv[i+1]);
						if(len>MAX_URLENC_HASH_LEN){
							printf("Hash argument too long, aborting\n");
							exit(usage(argv[0]));
						}
						for(c=0;c<len;c++){
							if(!isxdigit(argv[i+1][c])){
								printf("Argument is not a valid hash, aborting\n");
								exit(usage(argv[0]));
							}
						}
						strncpy(hash,argv[i+1],len+1);
						break;
					case 'v':
						switch(argv[i+1][0]){
							case '4':
								hints.ai_family=AF_INET;
								break;
							case '6':
								hints.ai_family=AF_INET6;
								break;
							default:
								printf("Unrecognized IP Version, aborting\n");
								exit(usage(argv[0]));
						}
						break;
					case 'n':
						len=strlen(argv[i+1]);
						for(c=0;c<len;c++){
							if(!isdigit(argv[i+1][c])){
								printf("Peer count argument invalid, aborting\n");
								exit(usage(argv[0]));
							}
						}
						numwant=argv[i+1];
						break;
					case 'p':
						len=strlen(argv[i+1]);
						for(c=0;c<len;c++){
							if(!isdigit(argv[i+1][c])){
								printf("Port argument invalid, aborting\n");
								exit(usage(argv[0]));
							}
						}
						local_port=argv[i+1];
						break;
					case 'e':
						bt_event=argv[i+1];
						break;
					case 'i':
						if(strlen(argv[i+1])<MAX_URLENC_HASH_LEN){
							memcpy(peerid,argv[i+1],strlen(argv[i+1])+1);
							if(destructiveURLDecode((unsigned char*)peerid)!=20){
								printf("Supplied peer ID has invalid length, aborting\n");
								exit(1);
							}
							hashEncodeURL((unsigned char*)peerid,sizeof(peerid)/sizeof(char));
						}
						else{
							printf("Illegal peer ID supplied, aborting\n");
							exit(1);
						}
						break;
				}
				i++;
			}
		}
		else{
			infile=argv[i];
		}
	}
	
	if(!infile&&(!tracker||!hash[0])){
		printf("Insufficient data\n");
		exit(usage(argv[0]));
	}
	
	//generate random peer id
	if(!peerid[0]){
		strncpy(peerid,"-TC0010-",8);
		for(i=8;i<20;i++){
			peerid[i]=(rand()%128)+33;
		}
		hashEncodeURL((unsigned char*)peerid, sizeof(peerid)/sizeof(char));
	}
	
	if(infile&&(!tracker||!hash[0])){
		char* hptr=hash;
		if(read_torrentfile(infile, &tracker, &hptr)<0){
			printf("Reading the input file failed, aborting\n");
			exit(1);
		}
	}
	
	//sanity check
	if(!tracker){
		printf("Something went terribly wrong. Aborting.\n");
		exit(9001);
	}
	
	printf("Input:\n\tInput file\t%s\n\tTracker\t\t%s\n\tHash\t\t%s\n",infile?infile:"-none-",tracker?tracker:"-none-",hash[0]?hash:"-none-");
	printf("\nRequest Settings:\n\tCompact\t\t%s\n\tScrape\t\t%s\n\tReq. Peers\t%s\n\tPeer ID\t\t%s\n",request_compact?"true":"false",request_scrape?"true":"false",numwant?numwant:"50",peerid);
	
	//urlencode hash
	destructiveHexDecode((unsigned char*)hash);
	hashEncodeURL((unsigned char*)hash,MAX_URLENC_HASH_LEN);
	
	//parse tracker url
	URL_COMPONENTS url=parse_url_destructive(tracker);
	if(url.status!=0){
		printf("Failed to parse tracker URL, aborting.");
		free(tracker);
		return 1;
	}
	
	if(strcmp(url.protocol,"http")){
		printf("Unknown tracker protocol (%s), aborting\n",url.protocol);
		free(tracker);
		return 1;
	}
	
	if(!url.port){
		url.port="80";
	}
	
	if(url.host&&url.path){
		char* path_parts[]={
			url.path,
			"?info_hash=",hash,
			"&port=",local_port?local_port:"6969",
			"&numwant=",numwant?numwant:"50",
			"&peer_id=",peerid, //FIXME peerid here
			bt_event?"&event=":"",bt_event?bt_event:"",
			"&compact=",request_compact?"1":"0",
			NULL
		};
		url.path=string_merge(path_parts);
		
		//printf("\nRequesting %s\n",url.path);
		
		//do request
		i=http_get(url,&response_buffer,&hints);
		
		free(url.path);
		
		if(i<0||!response_buffer){
			printf("Error fetching peers, aborting\n");
			free(tracker);
			if(response_buffer){
				free(response_buffer);
			}
			return 1;
		}
	}
	else{
		printf("Tracker URL invalid");
		free(tracker);
		return 1;
	}
	
	//parse response
	response_body=strstr(response_buffer,"\r\n\r\n");
	if(!response_body){
		printf("Invalid reply or non-protocol conforming server, aborting\n");
		
		free(response_buffer);
		free(tracker);
		return -1;
	}
	
	response_body+=4;
	if(benc_entity_type(response_body)!=T_DICT){
		printf("Response not a proper bencoded dictionary\n");
		
		free(response_buffer);
		free(tracker);
		return -1;
	}
	
	int offset;
	//printf("\nBody: \"%s\"\n",response_body);
	printf("\nTracker response:\n");
	
	offset=benc_dict_value_offset(response_body,"failure reason");
	if(offset>0){
		printf("Tracker error: ");
		if(benc_entity_type(response_body+offset)!=T_STRING){
			printf("Reason in invalid format");
		}
		else{
			fwrite(response_body+offset+benc_string_data_offset(response_body+offset),1,benc_string_length(response_body+offset),stdout);
		}
	}
	else{
		//normal parsing
		offset=benc_dict_value_offset(response_body,"interval");
		if(offset>0){
			printf("\tAnnounce interval: %d seconds\n",benc_int_value(response_body+offset,-1));
		}
		else{
			printf("Tracker response did not include announce interval\n");
		}
		
		offset=benc_dict_value_offset(response_body,"complete");
		if(offset>0){
			printf("\tActive Seeds: %d\n",benc_int_value(response_body+offset,-1));
		}
		offset=benc_dict_value_offset(response_body,"incomplete");
		if(offset>0){
			printf("\tActive Peers: %d\n",benc_int_value(response_body+offset,-1));
		}
		
		offset=benc_dict_value_offset(response_body,"peers");
		switch(benc_entity_type(response_body+offset)){
			case T_LIST:
				//non-compact syntax
				printf("\tPeers (received in non-compact format):\n");
				int item=0;
				int item_offset=0;
				for(item_offset=benc_list_item_offset(response_body+offset,item);item_offset>0;item++){
					
					if(benc_entity_type(response_body+offset+item_offset)!=T_DICT){
						printf("Invalid data received\n");
					}
					else{
						int ip_offset=benc_dict_value_offset(response_body+offset+item_offset,"ip");
						int port_offset=benc_dict_value_offset(response_body+offset+item_offset,"port");
						int id_offset=benc_dict_value_offset(response_body+offset+item_offset,"peer id");
						
						if(ip_offset<0||port_offset<0){
							printf("Incomplete peer dictionary\n");
						}
						else{
							printf("\t\t");
							fwrite(response_body+offset+item_offset+ip_offset+benc_string_data_offset(response_body+offset+item_offset+ip_offset),1,benc_string_length(response_body+offset+item_offset+ip_offset),stdout);
							printf(":%d",benc_int_value(response_body+offset+item_offset+port_offset,-1));
							if(id_offset>0){
								printf(" ");
								fwrite(response_body+offset+item_offset+id_offset+benc_string_data_offset(response_body+offset+item_offset+id_offset),1,benc_string_length(response_body+offset+item_offset+id_offset),stdout);
							}
							printf("\n");
						}
					}
					
					item_offset=benc_list_item_offset(response_body+offset,item+1);
				}
				break;
			case T_STRING:
				platform_setup();
			
				//compact syntax
				printf("\tPeers (received in compact format):\n");
				len=benc_string_length(response_body+offset);
				offset+=benc_string_data_offset(response_body+offset);
				if(len%6!=0){
					printf("Compact peer data is invalid\n");
				}
				else{
					char addr_plain[INET6_ADDRSTRLEN];
					uint16_t port=0;
					
					for(i=0;i<len;i+=6){
						if(!inet_ntop(AF_INET,response_body+offset+i,addr_plain,sizeof(addr_plain)/sizeof(char))){
							printf("Failed to convert IP address\n");
						}
						else{
							memcpy(&port,response_body+offset+i+4,sizeof(int16_t));
							port=ntohs(port);
							
							printf("\t\t%s:%d\n",addr_plain,port);
						}
					}
				}
				
				platform_teardown();
				break;
			default:
				printf("Failed to read peers section of reply\n");
		}
		
		//TODO peers6
	}
	
	free(response_buffer);
	free(tracker);
	return 0;
}