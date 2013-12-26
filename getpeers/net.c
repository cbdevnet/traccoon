//TODO NET_ERROR macro

#include <string.h>
#include "string.c"
#include "net.h"

void inline platform_setup(){
	#ifdef _WIN32
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,0),&wsaData)!=0){
			printf("Failed to initialize WSA\n");
			return -1;
		}
	#endif
}

void inline platform_teardown(){
	#ifdef _WIN32
		WSACleanup();
	#endif
}

URL_COMPONENTS parse_url_destructive(char* url){
	URL_COMPONENTS rv={0,NULL,NULL,NULL,NULL,NULL};
	
	if(strstr(url,"://")){
		//get protocol
		rv.protocol=url;
		url=strstr(url,"://");
		*url=0;
		url+=3;
	}
	else{
		rv.status=-1;
		return rv;
	}

	if(strchr(url,'@')){
		//get user/pass (optional)
		rv.credentials=url;
		url=strchr(url,'@');
		*url=0;
		url++;
	}
		
	//get host/[port]/path from url
	rv.host=url;//http_host
	if(strchr(url,'/')){
		if(strchr(url,':')){
			//get port & set path
			url=strchr(url,':');
			*url=0;
			url++;
			rv.port=url;
		}
		url=strchr(url,'/');
		*url=0;
		url++;
		rv.path=url;
	}
	else{
		rv.status=-1;
	}
		
	return rv;
}

//returns number of bytes actually read or -1 on error. response needs to be freed by caller
int http_get(URL_COMPONENTS url, char** response, struct addrinfo* external_hints){
	int status,sock,bytes,it_bytes,offset;
	struct addrinfo* conninfo=NULL;
	struct addrinfo* conn_it=NULL;
	
	char* request_text=NULL;
	
	*response=NULL;
	int response_buffer_length=0;
	
	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	
	if(!url.host||!url.port||!url.path||!response){
		return -1;
	}
	
	if(external_hints){
		hints=*external_hints;
	}
	
	platform_setup();
	
	status=getaddrinfo(url.host,url.port,&hints,&conninfo);
	if(status==0&&conninfo){
		sock=-1;
		for(conn_it=conninfo;conn_it!=NULL;conn_it=conn_it->ai_next){
			sock=socket(conn_it->ai_family,conn_it->ai_socktype,conn_it->ai_protocol);
			if(sock==-1){
				continue;
			}
			
			status=connect(sock,conn_it->ai_addr,conn_it->ai_addrlen);
			if(status!=0){
				closesocket(sock);
				continue;
			}
				
			break;
		}
		if(sock!=-1&&status==0){
			//connection established
			//build request
			char* request_parts[]={
				"GET /",url.path," HTTP/1.1\r\n",
				"Host: ",url.host,":",url.port,"\r\n",
				"User-Agent: ","traccoon/netlib","\r\n",
				"Accept-Encoding: ","text/plain","\r\n",
				"Connection: ","close","\r\n",
				"\r\n",
				NULL
			};
			
			request_text=string_merge(request_parts);
				
			//push request
			//printf("\nRequest content length: %d\n",strlen(request_text));
			
			status=0;
			for(bytes=0;bytes<strlen(request_text)&&status<MAX_SEND_RETRY_COUNT;status++){
				it_bytes=send(sock,request_text+bytes,strlen(request_text)-bytes,0);
				if(it_bytes<0){
					printf("Error while sending\n");
					//TODO exit
					break;
				}
				bytes+=it_bytes;
			}
			//printf("%d bytes sent\n",bytes);
			free(request_text);
			
			if(it_bytes<0){
				status=-1;
			}
			else{
				//get response
				bytes=0;
				do{
					offset=response_buffer_length;
					response_buffer_length+=RESPONSE_BUFFER_CHUNK;
					*response=realloc(*response,response_buffer_length*sizeof(char));
					it_bytes=recv(sock,(*response)+offset,RESPONSE_BUFFER_CHUNK,0);
					
					if(it_bytes==-1){
						printf("Error while receiving\n"); //TODO exit
						break;
					}
					
					if(it_bytes==0){
						printf("EOT read\n");
					}
					bytes+=it_bytes;
				}
				while(it_bytes==RESPONSE_BUFFER_CHUNK);
				closesocket(sock);
				(*response)[bytes]=0;
									
				//printf("Response length %d bytes, %d bytes buffer\n",bytes,response_buffer_length);
									
				//parse response (roughly)
				if(!strncmp(*response,"HTTP/",5)){
					//all ok (not checking any further)
					status=bytes;
				}
				else{
					printf("Unknown response protocol.\n");
					status=-1;
				}
			}
		}
		else{
			printf("All connection attempts failed\n");
			status=-1;
		}
	}
	else{
		printf("getaddrinfo failed: %s\n",gai_strerror(status));
		status=-1;
	}
		
	if(conninfo){
		freeaddrinfo(conninfo);
	}
		
	platform_teardown();
		
	return status;
}