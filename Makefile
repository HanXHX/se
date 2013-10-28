CC=/usr/bin/gcc
VERSION = $(shell cat VERSION)
CONSTANTS = -DDEBUG -DVERSION='"$(VERSION)"'
L =

UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
	L = -lbsd
endif

all:
	$(CC) se.c -O3 -Wall -Werror $(CONSTANTS) $(L) -o se

clean:
	rm -f se 
