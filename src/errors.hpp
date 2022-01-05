#pragma once 

#include <iostream>
#include <queue>
#include <string>
#include <cstring>

using namespace std;

// Flex/bison variables
extern queue<string> errors;
extern int yylex(void);
extern int yylineno;
extern char* yytext;
extern char *filename;

// Prints error;
void yyerror(string s);

// add a error.
void addError(string error);

// Print queue of strings
void printQueue(queue<string> queueToPrint);
