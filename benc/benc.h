#pragma once

#include <ctype.h>
#include <string.h>

#define BENC_PARSER_DEBUG(a)

typedef enum /*_BENC_ENTITY*/ {
	T_FAIL,
	T_DICT,
	T_LIST,
	T_INT,
	T_STRING
} BENC_ENTITY_TYPE;

//get type of entity beginning at supplied buffer
BENC_ENTITY_TYPE benc_entity_type(char* buf);

//get offset of nth item of list beginning at supplied buffer (zero-based index)
int benc_list_item_offset(char* buf, int item);

//get offset for nth key in dict (zero-based index)
int benc_dict_key_offset(char* buf, int index);

//get offset for value stored for given key
int benc_dict_value_offset(char* buf, char* key);

//get value of given bencoded int
int benc_int_value(char* buf, int defval);

//get length of given bencoded string
int benc_string_length(char* buf);

//get offset for data part of bencoded string
int benc_string_data_offset(char* buf);

//get string length of entity beginning at the first character of the supplied buffer
int benc_entity_length(char* buf, int initial_offset);

#include "benc.c"