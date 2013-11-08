MAKE=/usr/bin/make

all: documentation bin 

documentation:
	@$(MAKE) -C doc all

bin:
	@$(MAKE) -C src all

clean:
	@$(MAKE) -C src clean 
	@$(MAKE) -C doc clean 
