#include <stdio.h>
#include "benc.h"
#include "benc.c"

int main(int argc, char** argv){
	char* inputstring="li42e3:aaai1ee";
	int item=0,off=0;
	
	if(benc_entity_type(inputstring)!=T_LIST){
		printf("Not a list.\n");
	}
	else{
		printf("Is a list!\n");
		do{
			off=benc_list_item_offset(inputstring, item);
			printf("item %d offset is %d, is ",item,off);
			switch(benc_entity_type(inputstring+off)){
				case T_LIST:
					printf("T_LIST");
					break;
				case T_DICT:
					printf("T_DICT");
					break;
				case T_INT:
					printf("T_INT");
					break;
				case T_STRING:
					printf("T_STRING");
					break;
				case T_FAIL:
					printf("T_FAIL");
					break;
			}
			printf("\n");
			item++;
		}while(off!=-1);
	}
	
	return 0;
}