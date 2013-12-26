.PHONY: all gcc gcc-rel gcc-strict debug debug-full

all:
	cc -Wall traccoon.c -lsqlite3 -o traccoon
	
gcc:
	gcc -Wall traccoon.c -g -lsqlite3 -o traccoon

gcc-rel:
	gcc traccoon.c -lsqlite3 -o traccoon
	
debug:
	valgrind --log-file=../valgrind-traccoon.log ./traccoon -q 50 -w 3

debug-full:
	valgrind --log-file=../valgrind-traccoon.log --leak-check=full --show-reachable=yes ./traccoon -q 50 -w 3
	
dist:
	-mkdir bin
	@cd getpeers && make
	@cd torrentinfo && make
	@make
	mv getpeers/getpeers bin/
	mv torrentinfo/torrentinfo bin/
	mv traccoon bin/