#include <stdio.h>
#include <string.h>

#define OFF(a)
#define ERROR(a) (a)

#include "../neth.c"

int main(int argc, char** argv){
	if(argc<2){
		printf("Usage: %s <hexenc-hash>\n",argv[0]);
		exit(1);
	}
	
	char hashbuf[MAX_HEXENC_HASH_LEN+1];
	strncpy(hashbuf,argv[1],(sizeof(hashbuf)/sizeof(char)));
	hashbuf[MAX_HEXENC_HASH_LEN]=0;
	
	printf("Input is \t\t%s\n",hashbuf);

	destructiveHexDecode(hashbuf);
	
	hashbuf[MAX_HASH_LEN]=0;
	printf("Decoded hash is \t%s\n",hashbuf);
	
	hashEncodeHex(hashbuf,MAX_HEXENC_HASH_LEN+1);
	
	printf("Output is \t\t%s\n",hashbuf);
}