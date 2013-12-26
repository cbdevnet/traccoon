#pragma once

#define MAX_HASH_LEN 20
#define MAX_URLENC_HASH_LEN (MAX_HASH_LEN*3)
#define MAX_HEXENC_HASH_LEN (MAX_HASH_LEN*2)

//get offset of first occurrence of /needle/ in /haystack/ or -1 if none
int charIndex(unsigned char* hay, unsigned char needle);

//decode urlencoded input to binary representation, returns decoded length
int destructiveURLDecode(unsigned char* encoded);

//decode ascii hex string to binary representation, returns decoded length
int destructiveHexDecode(unsigned char* encoded);

//encode given hash to hex representation, returns -1 on failure
int hashEncodeHex(unsigned char* hash, int buffer_length);

//encode given hash to urlencode representation
int hashEncodeURL(unsigned char* data, int maxlen);