#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Create a enum for the variable types
typedef enum  {
   TYPE_INT,
   TYPE_FLOAT,
   TYPE_CHAR_PTR,
   TYPE_CHAR_ARRAY
} VarType;

// Create a struct for the symbol
// A Symbol represents a variable in the program
struct Symbol {
   VarType type;
   char name[64];
   bool initialized; // 0 if not initialized, 1 if initialized
   size_t array_len; // Using size_t for non-negative count of items in array
   union {
      long value_int; // for ints
      double value_float; // for floats
      long address; // for arrays/pointers
   };
}

// Create a struct for the symbol table
// A SymbolTable is a collection of Symbols
struct SymbolTable {
   Symbol items[32];
   size_t count;
}

// HELPER FUNCTIONS
/**
 * @brief Strips a trailing semicolon from a string, if one exists.
 * @return void
 * @param s A pointer to the null-terminated string to modify.
 * * Think of it as a street address for where the string is stored in the computer's memory.
 * * The string must have a special end character so the function knows where it stops.
 * 
 * This function modifies the string in-place.
 * If the last character of the string is a semicolon, it is replaced with a null terminator ('\0')
 */
void strip_semicolon(char *s) {
   // Get the length of the string
   size_t n = strlen(s);

   // Check if the string is not empty and the last character is a semicolon
   if ((n > 0) && (s[n-1] == ';')) {
      // Replace the semicolon with a null terminator
      s[n-1] = '\0';
   }
}

bool is_table_full(SymbolTable *t, const char *var_name) {
   if (t -> count == 32) {
      fprintf(stderr, "Error: SymbolTable is full. Cannot add '%s'.\n", var_name);
      return true;
   }
   return false;
}

void set(Symbol *s, VarType type, void *value, size_t array_len) {
   s -> type = type; // Set the type of the symbol
   s -> initialized = true; // Set the initialized flag to true 'cause it might be declared without a value

   // Use a switch statement to set the value of the symbol based on the type
   switch (type) {
      case TYPE_INT:
         s -> value_int = *(long *)value;
         break;
      case TYPE_FLOAT:
         s -> value_float = *(double *)value;
         break;
      case TYPE_CHAR_ARRAY:
      case TYPE_CHAR_PTR:
         // Store the address of the string in the 'address' field
         s -> address = (long)value;
         s -> array_len = array_len;
         break;
   }
}

// MAIN FUNCTIONS
/**
 * @brief Looks up a Symbol by name in a SymbolTable
 * @return A pointer to the found Symbol, or NULL if not found
 * @param t A pointer to the SymbolTable to search
 * @param varname A pointer to the name of the variable to find
 * Note that we're passing a pointer to the string (char array) and not the string itself due to C's compiler
 */
 Symbol *find(SymbolTable *t, const char *var_name) {
   // Loop through all symbols currently in the table
   for (int i = 0; i < t -> count; i++) {
      // Compare the name of the current symbol with the var_name we're looking for
      if (strcmp(t -> items[i].name, var_name) == 0) {
         // If a match is found, return a pointer to that Symbol
         // We return a pointer to the original Symbol to allow for direct modification and to avoid creating a copy.
         return &t -> items[i];
      }
   }

   // If the loop finishes without finding a mtch, the symbol doesn't exist
   // In this case, return NULL
   return NULL;
}

// MAIN FUNCTIONS
/**
 * @brief Adds a new symbol to the symbol table or updates an existing symbol
 * @return true if the symbol was added successfully or updated, false otherwise
 * @param t A pointer to the symbol table
 * @param var_name A pointer to the name of the variable
 * @param type The type of the variable
 * @param value A pointer to the value of the variable
 * @param array_len The length of the array
 */
bool add(SymbolTable *t, const char *var_name, VarType type, void *value, size_t array_len) {
   // Check if the table is full
   if (is_table_full(t, var_name)) {
      return false;
   }

   // Find the symbol in the table
   Symbol *found_symbol = find(t, var_name);

   if (found_symbol) {
      // If the symbol exists, update it
      set(found_symbol, type, value, array_len);
      return true;
   } else {
      // If the symbol doesn't exist, create a new one
      Symbol *new_symbol = &t -> items[t -> count];

      // Copy the name of the variable into the symbol
      strcpy(new_symbol -> name, var_name);

      // Set the properties of the new symbol
      set(new_symbol, type, value, array_len);

      // Increment the count of symbols in the table
      t -> count++;
   }

   return true;
}

/**
 * @brief Free's memory after the program execution
 * @return void
 * @param t A pointer to the desired SymbolTable
 */
void free_symbols(SymbolTable *t){
   // Loop through the SymbolTable
   for (int i = 0; i < t -> count; i++) {
      // Find a char array or char pointer and free it's memory
      if ( t -> items[i].type == TYPE_CHAR_ARRAY || t -> items[i].type == TYPE_CHAR_PTR ) {
         free( (void*)t -> items[i].address );
      }
   }
}