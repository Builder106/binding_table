#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

// HELPER FUNCTIONS
// Reads a number token
void read_number(const char **code, Token *t) {
   int i = 0;

   // Check if the current character is a digit
   while (isdigit(**code)) {
      t->value[i] = **code;
      i++;
      (*code)++;
   }
   t->value[i] = '\0';
   t->type = TOKEN_NUMBER;
}

// Reads a word token (keyword or identifier)
void read_token(const char **code, Token *t) {
   int i = 0;

   // Check if the current character is a letter
   while (isalnum(**code)) {
      t->value[i] = **code;
      i++;
      (*code)++;
   }
   t->value[i] = '\0';

   // Check if the token is a keyword
   if (strcmp(t->value, "int") == 0 ||
       strcmp(t->value, "float") == 0 ||
       strcmp(t->value, "double") == 0 ||
       strcmp(t->value, "char*") == 0 ||
       strcmp(t->value, "char") == 0 ||
       strcmp(t->value, "if") == 0 ||
       strcmp(t->value, "else") == 0 ||
       strcmp(t->value, "while") == 0 ||
       strcmp(t->value, "do") == 0 ||
       strcmp(t->value, "for") == 0 ||
       strcmp(t->value, "return") == 0 ||
       strcmp(t->value, "break") == 0 ||
       strcmp(t->value, "continue") == 0 ||
       strcmp(t->value, "void") == 0 ||
       strcmp(t->value, "struct") == 0 ||
       strcmp(t->value, "union") == 0 ||
       strcmp(t->value, "enum") == 0) {
      t->type = TOKEN_KEYWORD;
   } else {
      t->type = TOKEN_IDENTIFIER;
   }
}

// MAIN FUNCTIONS
Token *tokenize(const char *code) {

   // Allocate memory for the tokens array
   Token *tokens = malloc(sizeof(Token) * 32);
   int capacity = 32;
   int token_count = 0;
   const char *current_char = code;

   // Check if the tokens array is null
   if (tokens == NULL) {
      fprintf(stderr, "Memory allocation failed.\n");
      exit(1);
   }

   // Loop through the code until the end of the file
   while (*current_char != '\0') {
      // Step 1: Handle whitespace and comments.
      if (isspace(*current_char)) {
         current_char++;
         continue;
      }
      
      // Step 2: Handle single-line comments.
      if (*current_char == '/' && *(current_char + 1) == '/') {
          while (*current_char != '\n' && *current_char != '\0') {
              current_char++;
          }
          continue;
      }

      // Step 3: Check if we need to resize the array.
      // If the token count is greater than the capacity, we need to resize the array
      if (token_count >= capacity) {
         capacity *= 2;
         Token *temp = realloc(tokens, sizeof(Token) * capacity);

         // Check if the reallocation failed
         if (temp == NULL) {
            fprintf(stderr, "Memory reallocation failed.\n");
            free(tokens);
            exit(1);
         }

         // Assign the new tokens array to the tokens pointer
         tokens = temp;
      }

      // Step 4: Handle different token types.
      Token current_token;

      // Check if the current character is a letter
      if (isalpha(*current_char)) {
         read_token(&current_char, &current_token);
      } else if (isdigit(*current_char)) {
         // Check if the current character is a digit
         read_number(&current_char, &current_token);
      } else {
          // Check for multi-character operators first.
          // Check if the current character is an equal sign
          if (*current_char == '=' && *(current_char + 1) == '=') {
              current_token.value[0] = '=';
              current_token.value[1] = '=';
              current_token.value[2] = '\0';
              current_token.type = TOKEN_OPERATOR;
              current_char += 2;
          // Check if the current character is an exclamation mark
          } else if (*current_char == '!' && *(current_char + 1) == '=') {
              current_token.value[0] = '!';
              current_token.value[1] = '=';
              current_token.value[2] = '\0';
              current_token.type = TOKEN_OPERATOR;
              current_char += 2;
          // Check if the current character is a less than sign
          } else if (*current_char == '<' && *(current_char + 1) == '=') {
              current_token.value[0] = '<';
              current_token.value[1] = '=';
              current_token.value[2] = '\0';
              current_token.type = TOKEN_OPERATOR;
              current_char += 2;
          // Check if the current character is a greater than sign
          } else if (*current_char == '>' && *(current_char + 1) == '=') {
              current_token.value[0] = '>';
              current_token.value[1] = '=';
              current_token.value[2] = '\0';
              current_token.type = TOKEN_OPERATOR;
              current_char += 2;
          }
          // Now, handle single-character operators.
          else if (*current_char == '=' || *current_char == '+' || *current_char == '-' ||
                   *current_char == '*' || *current_char == '/' || *current_char == '<' || *current_char == '>') {
              current_token.value[0] = *current_char;
              current_token.value[1] = '\0';
              current_token.type = TOKEN_OPERATOR;
              current_char++;
          }
          // Now, handle single-character punctuation.
          else if (*current_char == ';' || *current_char == '(' || *current_char == ')' ||
                   *current_char == '{' || *current_char == '}' || *current_char == '[' || *current_char == ']') {
              current_token.value[0] = *current_char;
              current_token.value[1] = '\0';
              current_token.type = TOKEN_PUNCTUATION;
              current_char++;
          }
          // Step 5: Handle errors gracefully.
          else {
              fprintf(stderr, "Lexer error: Invalid character '%c' found.\n", *current_char);
              free(tokens);
              exit(1);
          }
      }
      tokens[token_count++] = current_token;
   }
   
   // Step 6: Add the end-of-file token.
   if (token_count >= capacity) {
      capacity++;
      tokens = realloc(tokens, sizeof(Token) * capacity);
   }
   tokens[token_count].type = TOKEN_END_OF_FILE;
   strcpy(tokens[token_count].value, "EOF");

   return tokens;
}