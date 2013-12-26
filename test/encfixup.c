#include <stdio.h>
#include "../neth.c"

int main(int argc, char* argv[]){
	if(argc<2){
		printf("Usage: %s <hash>\n",argv[0]);
		exit(1);
	}
	
	char hashbuf[MAX_URLENC_HASH_LEN+1];
	strncpy(hashbuf,argv[1],(sizeof(hashbuf)/sizeof(char)));
	
	printf("Input is %s\n",hashbuf);
	
	destructiveURLDecode(hashbuf);
	
	printf("Output is %s\n",hashbuf);
}