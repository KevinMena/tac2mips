MAKE = make
PROGRAMS = tac2mips 

.PHONY: $(PROGRAMS)

all: $(PROGRAMS)

tac2mips:
	cd src && $(MAKE) 
	mv src/tac2mips .

