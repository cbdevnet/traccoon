#pragma once

#define RESPONSE_BUFFER_CHUNK 512
#define MAX_SEND_RETRY_COUNT 5

typedef struct /*_URL_COMPONENTS*/ {
	int status;
	char* protocol;
	char* credentials;
	char* host;
	char* port;
	char* path;
} URL_COMPONENTS;