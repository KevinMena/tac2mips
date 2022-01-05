/* recognizes the tokens and prints them out */
%option noyywrap
%option yylineno

%{
  #include <string>
  #include <queue>
  #include <iostream>

  #include "FlowGraph.hpp"
  #include "parser.tab.h"

  using namespace std;

  // queues for tokens and errors
  queue<string> errors;
  char *filename;
  string line = "";
  
  // count rows and columns to identify errors
  int yycolumn = 1;

  #define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng; \
    yylval.str = new string(yytext); 

  // add an error to queue
  void pushError(void);
%}

%%
"@staticv"                    { return MI_STATICV; }
"@string"                     { return MI_STRING; }
"@label"                      { return MI_LABEL; }
"@function"                   { return MI_FUNCTION; }
"@endfunction"                { return MI_ENDFUNCTION; }
"assignw"                     { return I_ASSIGNW; }
"assignb"                     { return I_ASSIGNB; }
"add"                         { return I_ADD; }
"sub"                         { return I_SUB; }
"mult"                        { return I_MULT; }
"div"                         { return I_DIV; }
"mod"                         { return I_MOD; }
"minus"                       { return I_MINUS; }
"ftoi"                        { return I_FTOI; }
"itof"                        { return I_ITOF; }
"eq"                          { return I_EQ; }
"neq"                         { return I_NEQ; }
"lt"                          { return I_LT; }
"leq"                         { return I_LEQ; }
"gt"                          { return I_GT; }
"geq"                         { return I_GEQ; }
"or"                          { return I_OR; }
"and"                         { return I_AND; }
"goto"                        { return I_GOTO; }
"goif"                        { return I_GOIF; }
"goifnot"                     { return I_GOIFNOT; }
"malloc"                      { return I_MALLOC; }
"memcpy"                      { return I_MEMCPY; }
"free"                        { return I_FREE; }
"param"                       { return I_PARAM; }
"call"                        { return I_CALL; }
"return"                      { return I_RETURN; }
"exit"                        { return I_EXIT; }
"printc"                      { return I_PRINTC; }
"printi"                      { return I_PRINTI; }
"printf"                      { return I_PRINTF; }
"print"                       { return I_PRINT; }
"readc"                       { return I_READC; }
"readi"                       { return I_READI; }
"readf"                       { return I_READF; }
"read"                        { return I_READ; }
"["                           { return OPEN_BRACKET; }
"]"                           { return CLOSE_BRACKET; }
"False"                       { return FALSE; }
"True"                        { return TRUE; }
[[:alpha:]_][[:alnum:]_]*     { return ID; }
-[0-9]+                        { yylval.integer = atoi(yytext); return INT; }
[0-9]+                        { yylval.integer = atoi(yytext); return INT; }
[0-9]+\.[0-9]+                { yylval.flot = atof(yytext); return FLOAT; }
[\t]                          { /* ignore tab */ }
[ ]                           { /* ignore whitespace */ }
\'.\'                         { yylval.chr = yytext[1]; return CHAR; }
\"(?:[^"\\\n]|\\.)*\"         { return STRING; }
\n                            { return NL; } 
.                             { 
                                /* add error to queue */
                                pushError();  
                              }
%%

/*
  adds a lexing error to the queue that will be printed on the parsing phase
*/
void pushError() {
  // duplicate the value of yytext
  string token = strdup(yytext);
  string file = strdup(filename);

  if (token == "\n") {
    token = "\\n";
  }

  string error = "\033[1m" + file + ":" + to_string(yylineno) + 
  	": \033[31mLexical error:\033[0m Unexpected " +
    "token \"" + token + "\".\n\n";

  // add the error to the queue
  errors.push(error);
}