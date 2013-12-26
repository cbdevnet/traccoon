#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdint.h>
#include <openssl/sha.h>

#include "../benc/benc.h"

#define MAX_PRINTABLE_STRING_LENGTH 15

int usage(char* fn){
	printf("traccooon torrentinfo utility\nUsage:\n\t\t");
	printf("%s [-<opts>] <torrentfile>\t- Display info about <torrentfile>\n",fn);
	printf("Options:\n");
	printf("\t\t-h\t\tShow this help\n");
	printf("\t\t-r\t\tRecurse into compound entities\n");
	printf("\t\t-H\t\tDisplay hashes\n");
	printf("\t\t-t\t\tDisplay type information\n");
	printf("\t\t-v\t\tDisplay value/content\n");
	printf("\t\t-k <node-path>\tUse <node-path> (eg 'info.length') as root\n");
	return 1;
}

void tab(int num){
	int i=0;
	for(;i<num;i++){
		putc('\t',stdout);
	}
}

int handle_bencoded_node(unsigned recursion_depth, char* current_node, bool recurse, bool display_hash, bool display_type, bool display_values){
	int i,c,off,key_offset,len;
	uint8_t hash[SHA_DIGEST_LENGTH];
	tab(recursion_depth);
	
	if(display_hash){
		len=benc_entity_length(current_node,0);
		if(len<0){
			printf("Length calculation failed\n");
			return -1;
		}
	
		//calculate	hash
		SHA1((unsigned char*)current_node,len,hash);
		
		//print hash
		for(i=0;i<SHA_DIGEST_LENGTH;i++){
			printf("%02x",hash[i]);
		}
		putc(' ',stdout);
	}
	
	switch(benc_entity_type(current_node)){
		case T_DICT:
			if(display_type){
				printf("Dict ");
			}
			for(i=0;benc_dict_key_offset(current_node,i)>0;i++){
			}
			if(display_values&&display_type){
				printf("(%d keys)",i);
			}
			printf("\n");
			if(display_values&&!recurse){
				for(c=0;c<i;c++){
					off=benc_dict_key_offset(current_node,c);
					handle_bencoded_node(recursion_depth+1,current_node+off,recurse,display_hash,display_type,display_values);
				}
			}
			if(recurse){
				for(c=0;c<i;c++){
					key_offset=benc_dict_key_offset(current_node,c);
					off=key_offset+benc_entity_length(current_node+key_offset,0);
					
					/*
					len=benc_string_length(current_node+key_offset);
					key_offset=benc_string_data_offset(current_node+key_offset)+key_offset;
					
					tab(recursion_depth+1);
					printf("\"");
					for(pos=0;pos<len;pos++){
						putc(current_node[key_offset+pos],stdout);
					}
					printf("\" =>\n");
					*/
					
					handle_bencoded_node(recursion_depth+1,current_node+key_offset,recurse,display_hash,display_type,display_values);
					handle_bencoded_node(recursion_depth+2,current_node+off,recurse,display_hash,display_type,display_values);
				}
			}
			return 0;
		case T_LIST:
			if(display_type){
				printf("List ");
			}
			for(i=0;benc_list_item_offset(current_node,i)>0;i++){
			}
			if(display_values&&display_type){
				printf("(%d entries)",i);
			}
			printf("\n");
			if(recurse){
				for(c=0;c<i;c++){
					off=benc_list_item_offset(current_node,c);
					//printf("Node %d (Offset %d):",c,off);
					if(handle_bencoded_node(recursion_depth+1,current_node+off,recurse,display_hash,display_type,display_values)<0){
						printf("Failed to handle node %d with offset %d\n",c,off);
						return -1;
					}
				}
			}
			return 0;
		case T_INT:
			c=benc_int_value(current_node,-1);
			if(display_type){
				printf("Integer ");
			}
			if(display_values){
				printf("%d",c);
			}
			printf("\n");
			return 0;
		case T_STRING:
			c=benc_string_length(current_node);
			if(display_type){
				printf("String[%d] ",c);
			}
			if(display_values){
				off=benc_string_data_offset(current_node);
				for(i=0;i<c;i++){
					if(i>=MAX_PRINTABLE_STRING_LENGTH&&recursion_depth>0){
						printf("[...]");
						break;
					}
					if(isprint(current_node[off+i])){
						putc(current_node[off+i],stdout);
					}
					else{
						printf("[Non-printable]");
						break;
					}
				}
			}
			printf("\n");
			return 0;
		case T_FAIL:
			printf("Type detection failed @ %s\n",current_node);
			return -1;
	}
}

int main(int argc, char** argv){
	int i=0,c;
	FILE* in;
	size_t filesize, read_bytes;
	char* buffer=NULL;
	char* infile=NULL;
	char* rnode=NULL;
	char* node_token=NULL;
	unsigned rnode_offset=0;
	
	//flag options
	bool recurse=false;
	bool display_modifiers=false;
	bool display_hash=false;
	bool display_type=false;
	bool display_values=false;
	
	for(int i=1;i<argc;i++){
		if(argv[i][0]=='-'){
			switch(argv[i][1]){
				case 'r':
					//recurse flag
					recurse=true;
					break;
				case 'H':
					//hash flag
					display_modifiers=true;
					display_hash=true;
					break;
				case 't':
					//type flag
					display_modifiers=true;
					display_type=true;
					break;
				case 'v':
					//value flag
					display_modifiers=true;
					display_values=true;
					break;
				case 'k':
					//set root node
					if(argc<=i+1){
						printf("Root node flag needs additional parameters\n");
						exit(usage(argv[0]));
					}
					rnode=argv[++i];
					break;
				case 'h':
					//help
					usage(argv[0]);
					return 0;
				default:
					printf("Unknown flag: %s\n",argv[i]);
					exit(usage(argv[0]));
			}
		}
		else{
			infile=argv[i];
		}
	}
	
	if(!display_modifiers){
		display_hash=true;
		display_type=true;
		display_values=true;
	}
	
	in=fopen(infile,"rb");
	if(!in){
		printf("Could not open input file \"%s\", exiting.\n",infile);
		usage(argv[0]);
		return 1;
	}
	
	fseek(in,0,SEEK_END);
	filesize=ftell(in);
	rewind(in);
	
	//printf("Input file is %ld bytes long\n",filesize);
	
	buffer=malloc((filesize+1)*sizeof(char));
	if(!buffer){
		printf("Failed to allocate file buffer, aborting\n");
		fclose(in);
		return 1;
	}
	read_bytes=fread(buffer,1,filesize,in);
	fclose(in);
	buffer[read_bytes]=0;
	
	//printf("Read in %d bytes from input file\n",read_bytes);
	
	if(rnode){
		char* node_token=strtok(rnode,".");
		while(node_token!=NULL){
			switch(benc_entity_type(buffer+rnode_offset)){
				case T_LIST:
					for(i=0;isdigit(node_token[i]);i++){
					}
					if(node_token[i]!=0){
						printf("Non-integral key supplied for list, skipping\n");
					}
					else{
						i=strtoul(node_token,NULL,10);
						i=benc_list_item_offset(buffer+rnode_offset,i);
						if(i<0){
							printf("List has no item %s, skipping\n",node_token);
						}
						else{
							rnode_offset+=i;
						}
					}
					break;
				case T_DICT:
					i=benc_dict_value_offset(buffer+rnode_offset,node_token);
					if(i>0){
						rnode_offset+=i;
					}
					else{
						printf("Dict has no key \"%s\", skipping\n",node_token);
					}
					break;
				default:
					printf("Could not resolve full root path, token \"%s\" skipped\n",node_token);
					break;
			}
			node_token=strtok(NULL,".");
		}
	}
	
	int result=handle_bencoded_node(0,buffer+rnode_offset,recurse,display_hash,display_type,display_values);
	
	free(buffer);
	
	if(result<0){
		printf("Error while handling data\n");
		return -1;
	}
	return 0;
}