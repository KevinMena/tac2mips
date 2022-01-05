MAKE = make
PROGRAMS = tac2mips 

.PHONY: $(PROGRAMS)

all: $(PROGRAMS)

tac2mips:
	cd src && $(MAKE) || \
	(echo -e "\n\033[1;31mError. \033[0mTac2mips compilation termined." && exit 1)
	mv src/tac2mips bin/ 
	echo -e "\n\033[1;36mTac2mips compilation successfully.\033[0m"

clean:
	rm ./bin/*
