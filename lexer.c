#include <stdio.h>
#include "lexer.h"
#include "parser.h"

// HELPER FUNCTIONS
/**
 * @brief Grabs the next "word" or "number" from the code
 * @return The type of the token
 * @param code A pointer to the pointer to the code string. This lets the function move past the word or number it just read.
 * @param t A pointer to the token to store the value of the word or number.
 */
TokenType read_token(const char **code, Token *t) {
   int i = 0;

   // While the current character is alphanumeric
   while (isalnum(**code)) {
      // Add the current character to the token
      t -> value[i] = **code;

      // Increment the index
      i++;

      // Increment the code pointer
      (*code)++;
   }

   // Add the null terminator to the token
   t -> value[i] = '\0';

   // Return the type of the token
   return t -> type;
}

// MAIN FUNCTIONS
Token *tokenize(const char *code) {
   // A pointer to the array of tokens we will return.
   Token *tokens = malloc(sizeof(Token) * 32); // Start with space for 32 tokens
   int token_count = 0; // Number of tokens we've found so far
   int capacity = 32;   // Current capacity of our token array

   // A pointer that we will move through the code string.
   const char *current_char = code;

   // Loop until we reach the end of the code string.
   while (*current_char != '\0') {
      // Skip any whitespace characters.
      if (isspace(*current_char)) {
         current_char++;
         continue; // Go to the next character right away.
      }

      // Check if the current character is a letter (the start of a word).
      if (isalpha(*current_char)) {
         // Create a token to store the word.
         Token word_token;
         // Call our helper function to read the word and advance the pointer.
         read_token(&current_char, &word_token);
         
         // At this point, the word has been read, and current_char has advanced past it.
         // We can now add the new token to our array.
         
      // Check if the current character is a digit (the start of a number).
      } else if (isdigit(*current_char)) {
         // This is where you will add logic to read a number token.
         
      // Check for operators and punctuation.
      } else {
         Token current_token;
         current_token.value[0] = *current_char;
         current_token.value[1] = '\0'; // Null-terminate the string

         if (*current_char == '=') {
            current_token.type = TOKEN_OPERATOR;
         } else if (*current_char == ';') {
            current_token.type = TOKEN_PUNCTUATION;
         } else {
            current_token.type = TOKEN_OPERATOR;
         }

         // Add the token to the array
         tokens[token_count++] = current_token;

         // Increment the current character pointer
         current_char++;
      }
   }
   
   // We will add a special "end of file" token at the end.
   // This helps the parser know when it's done.
   
   return tokens; // Return the array of tokens.
}