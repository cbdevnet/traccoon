#pragma ONCE

//_really_ send stuff
int reallySend(int fd, char* msg, size_t len);

//send a string
int sendString(int fd, char* msg);

//return a pointer _after_ the first occurrence of /param/ in buf or NULL if none
char* textAfter(char* buf, char* param);

//push http headers with return code /rcode/ and additional headers /headers/ (should end in \n)
int sendHttpHeaders(int fd, char* rcode, char* headers);

//get length of /str/ when urldecoded (up to a maximum of /mlen/ chars read)
int decodedStrlen(char* str, int mlen);

//forcibly convert /in/ to all lowercase ascii
void strtolower(char* in);

//get offset of the next parameter-delimiting entity after /in/ (\0, &, ' ')
int httpParamLength(char* in);

//return the minimum of a and b if both are >0, the bigger one if one is <0 and 0 if both are <0
int min_zero(int a, int b);