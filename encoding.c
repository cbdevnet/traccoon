#include "encoding.h"

int charIndex(unsigned char* hay, unsigned char needle){
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

int destructiveURLDecode(unsigned char* encoded){
	unsigned char* hexdict=(unsigned char*)"0123456789abcdef";
	unsigned i,c;
	int first,second;
	
	i=0;
	c=0;
	for(;encoded[i]!='\0';i++){
		encoded[c]=encoded[i];
		if(encoded[i]=='%'&&encoded[i+1]!='\0'&&encoded[i+2]!='\0'){
			first=charIndex(hexdict,encoded[i+1]);
			second=charIndex(hexdict,encoded[i+2]);
			if(first!=-1&&second!=-1){
				encoded[c]=first*16+second;
				i+=2;
			}
			else{
				return -1;
			}
		}
		c++;
	}
	encoded[c]=0;
	
	OFF(DEBUG(printf("[destructiveURLDecode] Decoded length: %d\n",c)));
	return c;
}

int destructiveHexDecode(unsigned char* encoded){
	unsigned char* hexdict=(unsigned char*)"0123456789abcdef";
	unsigned i,c;
	int first,second;
	
	c=0;
	for(i=0;encoded[i]!='\0';i+=2){
		if(encoded[i+1]=='\0'){
			return -1;
		}
		first=charIndex(hexdict,encoded[i]);
		second=charIndex(hexdict,encoded[i+1]);
		if(first!=-1&&second!=-1){
			encoded[c]=first*16+second;
		}
		else{
			ERROR(printf("Input contains invalid characters.\n"));
			return -1;
		}
		c++;
	}
	encoded[c]=0;
	
	return c;
}

int hashEncodeHex(unsigned char* hash, int buffer_length){
	unsigned i;
	unsigned char buffer[MAX_HEXENC_HASH_LEN+1];
	unsigned char* hexdict=(unsigned char*)"0123456789abcdef";
	
	if(buffer_length<(sizeof(buffer)/sizeof(unsigned char))){
		ERROR(printf("Could not hexencode hash: Insufficient buffer size (Supplied: %d, Internal: %d)\n",buffer_length,(sizeof(buffer)/sizeof(unsigned char))));
		return -1;
	}
	
	for(i=0;i<MAX_HASH_LEN;i++){
		buffer[i*2]=hexdict[(hash[i])>>4];
		buffer[(i*2)+1]=hexdict[(hash[i])&0xF];
	}
	buffer[i*2]=0;
	
	memcpy(hash,buffer,MAX_HEXENC_HASH_LEN+1);
	return 0;
}

int hashEncodeURL(unsigned char* data, int maxlen){
	unsigned char buffer[MAX_URLENC_HASH_LEN+1];
	unsigned char* hexdict=(unsigned char*)"0123456789abcdef";
	unsigned char* nonreserved=(unsigned char*)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";
	int i,c;
	
	//TODO buffer size sanity check
	
	//encode to buffer
	i=0;
	c=0;
	for(;i<MAX_HASH_LEN;i++){
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
	
	strncpy((char*)data,(char*)buffer,maxlen);
	return 0;
}