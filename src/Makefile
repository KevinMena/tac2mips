LEXER   = flex 
PARSER  = bison
PFLAGS  = -d
CXX     = g++
CFLAGS  = -D__USE_POSIX -g -Wall -Wextra -pedantic -std=gnu++11 -Wno-unused-parameter -Wno-unused-function

.PHONY: clean

all: tac2mips clean

tac2mips: parser.tab.h lex.yy.c
	$(CXX) $(CFLAGS) *.c *.cpp -o $@

parser.tab.h: parser.y
	$(PARSER) $(PFLAGS) parser.y

lex.yy.c: lexer.l 
	$(LEXER) lexer.l

clean: 
	rm -f *.c *.h 
