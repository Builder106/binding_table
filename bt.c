#include "bt.h" // Now includes our new header file
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// All the function definitions go here.

// HELPER FUNCTIONS
void strip_semicolon(char *s) {
   size_t n = strlen(s);
   if ((n > 0) && (s[n-1] == ';')) {
      s[n-1] = '\0';
   }
}

bool is_table_full(struct SymbolTable *t, const char *var_name) {

   // Check if the count of the SymbolTable is 32
   if (t -> count == 32) {
      fprintf(stderr, "Error: SymbolTable is full. Cannot add '%s'.\n", var_name);
      return true;
   }
   return false;
}

void set(struct Symbol *s, VarType type, void *value, size_t array_len) {
   s -> type = type;
   s -> initialized = true;
   switch (type) {
      case TYPE_INT:
         s -> value_int = *(long *)value;
         break;
      case TYPE_FLOAT:
         s -> value_float = *(double *)value;
         break;
      case TYPE_CHAR_ARRAY:
      case TYPE_CHAR_PTR:
         s -> address = (long)value;
         s -> array_len = array_len;
         break;
   }
}

// MAIN FUNCTIONS
struct Symbol *find(struct SymbolTable *t, const char *var_name) {
   for (int i = 0; i < t -> count; i++) {
      // Check if the variable name matches the symbol name
      if (strcmp(t -> items[i].name, var_name) == 0) {
         return &t -> items[i];
      }
   }
   return NULL;
}

bool add(struct SymbolTable *t, const char *var_name, VarType type, void *value, size_t array_len) {
   if (is_table_full(t, var_name)) {
      return false;
   }

   // Check if the symbol already exists
   struct Symbol *found_symbol = find(t, var_name);
   if (found_symbol) {
      set(found_symbol, type, value, array_len);
      return true;
   } else {
      // If the symbol does not exist, create a new symbol
      struct Symbol *new_symbol = &t -> items[t -> count];

      // Copy the variable name to the new symbol
      strcpy(new_symbol -> name, var_name);
      set(new_symbol, type, value, array_len);
      t -> count++;
   }
   return true;
}

void free_symbols(struct SymbolTable *t){

   // Free the memory for the character arrays and pointers
   for (int i = 0; i < t -> count; i++) {
      if ( t -> items[i].type == TYPE_CHAR_ARRAY || t -> items[i].type == TYPE_CHAR_PTR ) {
         free( (void*)t -> items[i].address );
      }
   }
}