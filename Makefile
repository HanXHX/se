CC=/usr/bin/gcc
MAKE=/usr/bin/make
VERSION = $(shell cat VERSION)
CONSTANTS = -DDEBUG -DVERSION='"$(VERSION)"'
L =

UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
	L = -lbsd
endif

all: documentation bin 

documentation:
	@$(MAKE) -C doc all

bin:
	@echo 'Compiling...'
	@$(CC) se.c -O3 -Wall -Werror $(CONSTANTS) $(L) -o se
	@echo 'Done...'

clean:
	@rm -f se 
	@$(MAKE) -C doc clean 
