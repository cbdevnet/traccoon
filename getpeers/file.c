int read_torrentfile(char* filename, char** tracker, char** hash){
	FILE* input;
	long filesize=0;
	char* buffer=NULL;
	int length=0, offset=0, i=0;
	unsigned char* hexdict=(unsigned char*)"0123456789abcdef";
	
	if(!filename||!tracker||!hash){
		printf("Invalid arguments for read_torrentfile\n");
		return -1;
	}
	
	if(!(*hash)){
		printf("Hash pointer was NULL\n");
		return -1;
	}
	
	input=fopen(filename,"rb");
	if(!input){
		printf("Failed to open input file\n");
		return -1;
	}
		
	fseek(input,0,SEEK_END);
	filesize=ftell(input);
	rewind(input);
		
	buffer=malloc((filesize+1)*sizeof(char));
	if(!buffer){
		printf("Failed to allocate memory for buffering file, aborting\n");
		fclose(input);
		return -1;
	}
	
	length=fread(buffer,1,filesize,input);
	fclose(input);
	buffer[length]=0;
		
	if(benc_entity_type(buffer)!=T_DICT||benc_entity_length(buffer,0)<0){
		printf("Input file is not a valid torrent, aborting\n");
		free(buffer);
		return -1;
	}

	if(!(*tracker)){
		offset=benc_dict_value_offset(buffer,"announce");
		length=benc_string_length(buffer+offset);
		if(offset<0||length<0){
			printf("Input file does not contain tracker information, aborting\n");
			free(buffer);
			return -1;
		}
		(*tracker)=malloc((length+1)*sizeof(char));
		//TODO memory allocation sanity check
		memcpy((*tracker),buffer+benc_string_data_offset(buffer+offset)+offset,length);
		(*tracker)[length]=0;
	}
		
	if((**hash)==0){
		uint8_t hash_temp[SHA_DIGEST_LENGTH];
		offset=benc_dict_value_offset(buffer,"info");
		length=benc_entity_length(buffer+offset,0);
		if(offset<0||length<0||benc_entity_type(buffer+offset)!=T_DICT){
			printf("Failed to parse info key, aborting\n");
			if(tracker){
				free(tracker);
			}
			free(buffer);
			return -1;
		}
		SHA1((unsigned char*)buffer+offset,length,hash_temp);
		for(i=0;i<SHA_DIGEST_LENGTH;i++){
			(*hash)[i*2]=hexdict[(hash_temp[i]>>4)];
			(*hash)[(i*2)+1]=hexdict[(hash_temp[i]&0xF)];
		}
		(*hash)[i*2]=0;
	}
	free(buffer);
	return 0;
}