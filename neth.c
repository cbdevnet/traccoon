#include "neth.h"

int reallySend(int fd, char* msg, size_t len){
	int sent=0;
	//while(sent<len){
		sent+=send(fd,msg+sent,len-sent,0);
	//}
	return sent;
}

int sendString(int fd, char* msg){
	return reallySend(fd, msg, strlen(msg));
}

char* textAfter(char* buf, char* param){
	char* t;
	if(buf==NULL){
		return NULL;
	}
	t=strstr(buf,param);
	if(!t){
		return NULL;
	}
	return t+strlen(param);
}

int sendHttpHeaders(int fd, char* rcode, char* headers){
	sendString(fd,"HTTP/1.1 ");
	sendString(fd,rcode);
	sendString(fd,"\r\n");
	sendString(fd,headers);//FIXME print supplied additional headers below standard headers
	sendString(fd,"Content-type: text/plain\r\nConnection: close\r\nServer: traccoon\r\n\r\n");
	return 0;
}

int decodedStrlen(char* str, int mlen){
	int i,c;
	for(i=0,c=0;str[i]!='\0'&&(mlen==-1||i<mlen);i++,c++){
		if(str[i]=='%'&&str[i+1]!='\0'&&str[i+2]!='\0'){
			i+=2;
		}
	}
	return c;
}

void strtolower(char* in){
	int i;
	for(i=0;in[i]!='\0';i++){
		in[i]=tolower(in[i]);
	}
}

int httpParamLength(char* in){
	int i;
	for(i=0;in[i]!='\0'&&in[i]!='&'&&in[i]!=' ';i++){
	}
	return i;
}

int min_zero(int a, int b){
	//TODO make this better
	if(a<0){
		if(b<0){
			return 0;
		}
		return b;
	}
	else{
		if(b<0){
			return a;
		}
		if(b>a){
			return a;
		}
		return b;
	}
}

