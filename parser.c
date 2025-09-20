#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Parser functuons here

// MAIN FUNCTIONS
/**
 * @brief Parses a declaration statement
 * @return void
 * @param token A pointer to the pointer to the token
 * @param t A pointer to the symbol table
 */
 void parse_declaration(Token **token, SymbolTable *t); {
   // Initialize the type variable
   VarType type;

   // Check if the token is a valid type
   if (strcmp((*token) -> value, "int") == 0) {
      type = TYPE_INT;
   } else if (strcmp((*token) -> value, "float") == 0) {
      type = TYPE_FLOAT;
   } else if (strcmp((*token) -> value, "double") == 0) {
      type = TYPE_DOUBLE;
   } else if (strcmp((*token) -> value, "char*") == 0) {
      type = TYPE_CHAR_PTR;
   } else if (strcmp((*token) -> value, "char") == 0) {
      type = TYPE_CHAR_ARRAY;
   } else {
      fprintf(stderr, "Error: Invalid type '%s'.\n", (*token) -> value);
      return;
   }
}

// The highest-level function that drives the parsing process.
void parse_program(Token *tokens, SymbolTable *t) {
    // This is where you'll loop through the tokens and call other
    // parsing functions for each statement.
    Token *current_token = tokens;
    
    // As long as there are tokens, parse a statement.
    // parse_statement will advance the current_token pointer.
    while (current_token->type != TOKEN_END_OF_FILE) {
        parse_statement(&current_token, t);
    }
}

void parse_statement(Token **tokens, SymbolTable *t) {
    // A statement starts with a keyword (like 'int').
    if ((*tokens)->type == TOKEN_KEYWORD) {
        // Now advance the token pointer to the next token.
        (*tokens)++;
        parse_declaration(tokens, t);
    }
}