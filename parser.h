#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include "bt.h"
#include "lexer.h"



void parse_expression(Token **token, SymbolTable *t);
void parse_statement(Token **token, SymbolTable *t);
void parse_program(Token *token, SymbolTable *t);

#endif
