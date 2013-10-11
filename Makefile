CC=/usr/bin/gcc
CONSTANTS = -DDEBUG

all:
	$(CC) se.c -Wall -Werror $(CONSTANTS) -lbsd -o se

clean:
	rm -f se 
