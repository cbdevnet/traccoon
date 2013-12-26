#include <stdio.h>

#include "../getpeers/net.c"

int main(int argc, char** argv){
	if(argc<2){
		return -1;
	}

	URL_COMPONENTS rv=parse_url_destructive(argv[1]);
	
	printf("Status\t\t%d\n",rv.status);
	printf("Protocol\t%s\n",rv.protocol);
	printf("Credentials\t%s\n",rv.credentials);
	printf("Host\t\t%s\n",rv.host);
	printf("Port\t\t%s\n",rv.port);
	printf("Path\t\t%s\n",rv.path);
	
	return 0;
}