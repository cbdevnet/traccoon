all:
	tcc traccoon.c -lsqlite3 -o traccoon
	
gcc:
	gcc traccoon.c -g -lsqlite3 -o traccoon

gcc-rel:
	gcc traccoon.c -lsqlite3 -o traccoon

gcc-strict:
	gcc traccoon.c -g -lsqlite3 -o traccoon -Wall
	
debug:
	valgrind --log-file=../valgrind-traccoon.log ./traccoon -q 50 -w 3

debug-full:
	valgrind --log-file=../valgrind-traccoon.log --leak-check=full --show-reachable=yes ./traccoon -q 50 -w 3