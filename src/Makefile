CC=/usr/bin/gcc
MAKE=/usr/bin/make
VERSION = $(shell cat ../VERSION)
CONSTANTS = -DDEBUG -DVERSION='"$(VERSION)"'
L =

UNAME=$(shell uname)
ifeq ($(UNAME), Linux)
	L = -lbsd
endif

all:  
	@echo 'Compiling...'
	@$(CC) se.c -O3 -Wall -Werror $(CONSTANTS) $(L) -o ../se
	@echo 'Done...'

clean:
	@rm -f ../se 
