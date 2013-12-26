#pragma once

char* string_merge(char** parts){
	int len=0,i=0,off=0;
	char* rv=NULL;
	
	for(i=0;parts[i];i++){
		len+=strlen(parts[i]);
	}
	len++;
	
	rv=malloc((len)*sizeof(char));
	if(!rv){
		printf("Failed to allocate memory for string concatenation\n");
		return NULL;
	}
	
	for(i=0;parts[i];i++){
		strncpy(rv+off,parts[i],strlen(parts[i]));
		off+=strlen(parts[i]);
	}
	rv[off]=0;
	return rv;
}