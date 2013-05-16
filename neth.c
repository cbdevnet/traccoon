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
	sendString(fd,"\n");
	sendString(fd,headers);//FIXME print supplied additional headers below standard headers
	sendString(fd,"Content-type: text/plain\nConnection: close\nServer: traccoon\n\n");
	return 0;
}

int charIndex(char* hay, char needle){
	int i;
	if(!hay){
		return -1;
	}
	for(i=0;hay[i]!='\0';i++){
		if(hay[i]==needle){
			return i;
		}
	}
	return -1;
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

void destructiveURLEncodeFixup(unsigned char* data, int maxlen){
	unsigned char buffer[MAX_ENC_HASH_LEN];
	unsigned char* hexdict="0123456789abcdef";
	unsigned char* nonreserved="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";
	int i,c,first,second,len;
	
	//printf("DHF: input %s\n",data);
	
	//decode inline
	for(i=0,c=0;data[i]!='\0';i++){
		data[c]=data[i];
		if(data[i]=='%'&&data[i+1]!='\0'&&data[i+2]!='\0'){
			first=charIndex(hexdict,data[i+1]);
			second=charIndex(hexdict,data[i+2]);
			if(first!=-1&&second!=-1){
				data[c]=first*16+second;
				i+=2;
			}
		}
		c++;
	}
	data[c]=0;
	len=c;
	
	//printf("DHF: decoded %s (len %d)\n",data,len);
	
	//encode to buffer
	for(i=0,c=0;i<MAX_HASH_LEN&&i<len;i++){
		//printf("Encoding char at %d (%#X)\n",i,data[i]);
		buffer[c]=data[i];
		if(charIndex(nonreserved,data[i])==-1){
			buffer[c]='%';
			buffer[c+1]=hexdict[(data[i])>>4];
			buffer[c+2]=hexdict[(data[i])&0xF];
			c+=2;
		}
		c++;
	}
	buffer[c]=0;
	
	//printf("DHF: recoded %s\n",buffer);
	
	memcpy(data,buffer,maxlen);//FIXME 0termination
}