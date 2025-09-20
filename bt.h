#ifndef BT_H
#define BT_H

#include <stdbool.h>
#include <stddef.h>

// Create an enum for the variable types
typedef enum  {
   TYPE_INT,
   TYPE_FLOAT,
   TYPE_CHAR_PTR,
   TYPE_CHAR_ARRAY
} VarType;

// Create a struct for the symbol
struct Symbol {
   VarType type;
   char name[64];
   bool initialized;
   size_t array_len;
   union {
      long value_int;
      double value_float;
      long address;
   };
};

// Create a struct for the symbol table
struct SymbolTable {
   struct Symbol items[32];
   size_t count;
};

// HELPER FUNCTIONS
/**
 * @brief Strips a trailing semicolon from a string, if one exists.
 * @return void
 * @param s A pointer to the null-terminated string to modify.
 */
void strip_semicolon(char *s);

bool is_table_full(struct SymbolTable *t, const char *var_name);

void set(struct Symbol *s, VarType type, void *value, size_t array_len);

// MAIN FUNCTIONS
/**
 * @brief Looks up a Symbol by name in a SymbolTable
 * @return A pointer to the found Symbol, or NULL if not found
 * @param t A pointer to the SymbolTable to search
 * @param varname A pointer to the name of the variable to find
 */
struct Symbol *find(struct SymbolTable *t, const char *var_name);

/**
 * @brief Adds a new symbol to the symbol table or updates an existing symbol
 * @return true if the symbol was added successfully or updated, false otherwise
 * @param t A pointer to the symbol table
 * @param var_name A pointer to the name of the variable
 * @param type The type of the variable
 * @param value A pointer to the value of the variable
 * @param array_len The length of the array
 */
bool add(struct SymbolTable *t, const char *var_name, VarType type, void *value, size_t array_len);

/**
 * @brief Free's memory after the program execution
 * @return void
 * @param t A pointer to the desired SymbolTable
 */
void free_symbols(struct SymbolTable *t);

#endif