#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "bt.h"

void parse_declaration(Token **tokens, SymbolTable *t) {
   VarType type;

   // The lexer has already determined the token type, so we can check it
   // directly instead of using strcmp on the value.
   if ((*tokens)->type != TOKEN_KEYWORD) {
      fprintf(stderr, "Error: Expected a type keyword like 'int' but found '%s'.\n", (*tokens)->value);
      return;
   }

   // Now we use the string value to determine the specific type.
   if (strcmp((*tokens)->value, "int") == 0) {
      type = TYPE_INT;
   } else if (strcmp((*tokens)->value, "float") == 0) {
      type = TYPE_FLOAT;
   } else if (strcmp((*tokens)->value, "double") == 0) {
      type = TYPE_DOUBLE;
   } else if (strcmp((*tokens)->value, "char*") == 0) {
      type = TYPE_CHAR_PTR;
   } else if (strcmp((*tokens)->value, "char") == 0) {
      type = TYPE_CHAR_ARRAY;
   }

   // Advance to the next token, which should be the variable name.
   (*tokens)++;

   if ((*tokens)->type != TOKEN_IDENTIFIER) {
      fprintf(stderr, "Error: Expected an identifier but found '%s'.\n", (*tokens)->value);
      return;
   }
   
   // Now that we have the type and the name, we can add it to the symbol table.
   add(t, (*tokens)->value, type, NULL, 0);

   // Advance to the next token, which should be the semicolon.
   (*tokens)++;

   if ((*tokens)->type != TOKEN_PUNCTUATION || strcmp((*tokens)->value, ";") != 0) {
      fprintf(stderr, "Error: Expected a semicolon but found '%s'.\n", (*tokens)->value);
      return;
   }
}

// The highest-level function that drives the parsing process.
void parse_program(Token *tokens, SymbolTable *t) {
    Token *current_token = tokens;
    
    while (current_token->type != TOKEN_END_OF_FILE) {
        parse_statement(&current_token, t);
        current_token++; // Move to the next token
    }
}

void parse_statement(Token **tokens, SymbolTable *t) {
    if ((*tokens)->type == TOKEN_KEYWORD) {
        parse_declaration(tokens, t);
    }
}