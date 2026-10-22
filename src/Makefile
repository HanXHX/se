CC ?= gcc
VERSION = $(shell cat ../VERSION)
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS += -Werror -DVERSION='"$(VERSION)"'

all:
	@echo 'Compiling...'
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) se.c -o ../se
	@echo 'Done...'

clean:
	@rm -f ../se
