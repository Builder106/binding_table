#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "bt.h"

static char *read_file_to_string(const char *path) {
   FILE *f = fopen(path, "rb");
   if (!f) return NULL;
   if (fseek(f, 0, SEEK_END) != 0) {
      fclose(f);
      return NULL;
   }
   long size = ftell(f);
   if (size < 0) {
      fclose(f);
      return NULL;
   }
   rewind(f);
   char *buffer = (char *)malloc((size_t)size + 1);
   if (!buffer) {
      fclose(f);
      return NULL;
   }
   size_t readn = fread(buffer, 1, (size_t)size, f);
   fclose(f);
   buffer[readn] = '\0';
   return buffer;
}

static char *read_stdin_to_string(void) {
   const size_t chunk = 4096;
   size_t cap = chunk;
   size_t len = 0;
   char *buf = (char *)malloc(cap);
   if (!buf) return NULL;
   size_t n;
   while ((n = fread(buf + len, 1, chunk, stdin)) > 0) {
      len += n;
      if (cap - len < chunk) {
         cap *= 2;
         char *tmp = (char *)realloc(buf, cap);
         if (!tmp) {
            free(buf);
            return NULL;
         }
         buf = tmp;
      }
      if (n < chunk) break;
   }
   if (len == 0) {
      free(buf);
      return NULL;
   }
   buf[len] = '\0';
   return buf;
}

int main(int argc, char **argv) {
   char *code = NULL;
   if (argc > 1) {
      code = read_file_to_string(argv[1]);
      if (!code) {
         fprintf(stderr, "Error: could not read file: %s\n", argv[1]);
         return 1;
      }
   } else {
      code = read_stdin_to_string();
      if (!code) {
         fprintf(stderr, "Usage: %s <program-file>\n", argv[0]);
         fprintf(stderr, "Or:    echo 'int x; float y;' | %s\n", argv[0]);
         return 1;
      }
   }

   // Create and initialize the SymbolTable
   struct SymbolTable my_symbol_table;
   my_symbol_table.count = 0;
   stack_reset();

   // Tokenize the input code
   Token *tokens = tokenize(code);
   
   // Parse the tokens and fill the symbol table
   parse_program(tokens, &my_symbol_table);

   // parse_program now prints the ASCII table of command -> binding

   // Free the memory for the tokens and source
   free(tokens);
   free(code);
   
   return 0;
}