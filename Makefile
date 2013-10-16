CC=/usr/bin/gcc
CONSTANTS = -DDEBUG
L =

UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
	L = -lbsd
endif

all:
	$(CC) se.c -Wall -Werror $(CONSTANTS) $(L) -o se

clean:
	rm -f se 
