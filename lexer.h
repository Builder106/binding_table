#include <ctype.h>

#ifndef LEXER_H
#define LEXER_H

typedef enum {
   TOKEN_KEYWORD,
   TOKEN_IDENTIFIER,
   TOKEN_NUMBER,
   TOKEN_OPERATOR,
   TOKEN_PUNCTUATION,
   TOKEN_END_OF_FILE,
} TokenType;

typedef struct {
   TokenType type;
   char value[64]; // Stores the actual string value of the token
} Token;

// Function prototypes
Token *tokenize (const char *code);
TokenType read_token(const char **code, Token *t);

#endif