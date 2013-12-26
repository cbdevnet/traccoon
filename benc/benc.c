#pragma once

BENC_ENTITY_TYPE benc_entity_type(char* buf){
	int i;
	switch(buf[0]){
		case 'd':
			return T_DICT;
		case 'l':
			return T_LIST;
		case 'i':
			return T_INT;
		default:
			for(i=0;isdigit(buf[i]);i++){//there are dependencies on this 
			}								//guaranteeing there is no \0 before ':' when returning T_STRING
			if(buf[i]==':'&&i>0){
				return T_STRING;
			}
	}
	return T_FAIL;
}

int benc_list_item_offset(char* buf, int item){
	int offset=0,cur_item=0;
	
	if(item<0||benc_entity_type(buf)!=T_LIST){
		return -1;
	}
	
	while(buf[offset+1]!='e'&&buf[offset+1]!=0){
		if(cur_item==item){
			return offset+1;
		}
		offset+=benc_entity_length(buf+offset+1,0);//FIXME errhandling
		cur_item++;
	}
	
	return -1;
}

int benc_dict_value_offset(char* buf, char* key){
	int offset=0,current_item=0;
	
	if(!key||benc_entity_type(buf)!=T_DICT){
		return -1;
	}
	
	while(buf[offset+1]!='e'&&buf[offset+1]!=0){
		if(current_item%2==0){
			//key
			if(benc_entity_type(buf+offset+1)!=T_STRING){
				return -1;
			}
			
			if(strlen(key)==benc_string_length(buf+offset+1)){
				if(!strncmp(key,buf+offset+1+benc_string_data_offset(buf+offset+1),strlen(key))){
					//find value entity
					return offset+1+benc_entity_length(buf+offset+1,0);
				}
			}
			
		}
		offset+=benc_entity_length(buf+offset+1,0);
		
		current_item++;
	}
	
	return -1; 
}

int benc_dict_key_offset(char* buf, int index){
	int offset=0,current_item=0;
	
	if(index<0||benc_entity_type(buf)!=T_DICT){
		return -1;
	}
	
	while(buf[offset+1]!='e'&&buf[offset+1]!=0){
		if(current_item%2==0){
			//key
			if(benc_entity_type(buf+offset+1)!=T_STRING){
				return -1;
			}
			
			if(current_item==index*2){
				return offset+1;
			}
		}
		offset+=benc_entity_length(buf+offset+1,0);
		
		current_item++;
	}
	return -1; 
}

int benc_int_value(char* buf, int defval){
	int i;
	if(benc_entity_type(buf)!=T_INT){
		return defval;
	}
	
	i=0;
	if(buf[i+1]=='-'){
		i++;
	}
	
	for(;isdigit(buf[i+1]);i++){
	}
	
	if(i<=1||buf[i+1]!='e'){
		return defval;
	}
	
	return strtol(buf+1,NULL,10);
}

int benc_string_length(char* buf){
	if(benc_entity_type(buf)!=T_STRING){
		return -1;
	}
	
	return strtoul(buf,NULL,10);
}

int benc_string_data_offset(char* buf){
	int i;
	if(benc_entity_type(buf)!=T_STRING){
		return -1;
	}
	for(i=0;buf[i]!=':';i++){
	}
	return i+1;
}

int benc_entity_length(char* buf, int initial_offset){
	int i,offset=0,item_length,item_index=0;
	long conv;
	switch(benc_entity_type(buf)){
		case T_DICT:
			BENC_PARSER_DEBUG(printf("Dictionary start @ %d\n",initial_offset));
			while(buf[offset+1]!='e'&&buf[offset+1]!=0){
				if(item_index%2==0&&benc_entity_type(buf+offset+1)!=T_STRING){
					BENC_PARSER_DEBUG(printf("Invalid dict key type @ %d\n",initial_offset+offset+1));
					return -1;
				}
			
				item_length=benc_entity_length(buf+offset+1,initial_offset+offset+1);
				if(item_length==-1){
					BENC_PARSER_DEBUG(printf("Inner dict element at %d failed to parse, cascading back\n",initial_offset+offset+1));
					return -1;
				}
				
				offset+=item_length;
				item_index++;
			}
			if(buf[offset+1]=='e'&&(item_index%2)==0){
				BENC_PARSER_DEBUG(printf("Dictionary[%d] inner length was %d, end @ %d\n",initial_offset,offset,initial_offset+offset+1));
				return offset+2;
			}
			else{
				BENC_PARSER_DEBUG(printf("End of stream (%d) reached while parsing dictionary @ %d\n",initial_offset+offset+1,initial_offset));
				return -1;
			}
		case T_LIST:
			BENC_PARSER_DEBUG(printf("List start @ %d\n",initial_offset));
			while(buf[offset+1]!='e'&&buf[offset+1]!=0){
				item_length=benc_entity_length(buf+offset+1,initial_offset+offset+1);
				if(item_length==-1){
					BENC_PARSER_DEBUG(printf("List item @ %d was invalid\n",initial_offset+offset+1));
					return -1;
				}
				offset+=item_length;
			}
			if(buf[offset+1]=='e'){
				BENC_PARSER_DEBUG(printf("List[%d] inner length was %d, end at %d\n",initial_offset,offset,initial_offset+offset+1));
				return offset+2;
			}
			else{
				BENC_PARSER_DEBUG(printf("End of stream (%d) reached while parsing list @ %d\n",initial_offset+offset+1,initial_offset));
				return -1;
			}
		case T_INT:
			i=1;
			if(buf[i]=='-'){
				i++;
			}
			for(;isdigit(buf[i]);i++){
			}
			if(buf[i]=='e'){
				BENC_PARSER_DEBUG(printf("Integer with length %d @ %d\n",i+1,initial_offset));
				return i+1;
			}
			BENC_PARSER_DEBUG(printf("Integer @ %d not properly terminated\n",initial_offset));
			break;
		case T_STRING:
			for(i=0;isdigit(buf[i]);i++){
			}
			if(buf[i]==':'&&i>0){
				conv=strtol(buf,NULL,10);
				BENC_PARSER_DEBUG(printf("String with length %d @ %d\n",i+conv+1,initial_offset));
				return i+conv+1;
			}
			BENC_PARSER_DEBUG(printf("Unknown entity type (this should not have been reached)\n"));
			break;
		case T_FAIL:
			//ignore
			break;
	}
	BENC_PARSER_DEBUG(printf("Entity length calculation failed @ %d\n",initial_offset));
	return -1;
}