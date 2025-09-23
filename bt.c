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
   if (value == NULL) {
      s -> initialized = false;
      switch (type) {
         case TYPE_INT:
            s -> value_int = 0;
            break;
         case TYPE_FLOAT:
         case TYPE_DOUBLE:
            s -> value_float = 0.0;
            break;
         case TYPE_CHAR_ARRAY:
         case TYPE_CHAR_PTR:
            s -> address = 0;
            s -> array_len = array_len;
            break;
      }
      return;
   }

   s -> initialized = true;
   switch (type) {
      case TYPE_INT:
         s -> value_int = *(long *)value;
         break;
      case TYPE_FLOAT:
      case TYPE_DOUBLE:
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

static void print_symbol_binding(const struct Symbol *s) {
   printf("%s |-> ", s -> name);
   switch (s -> type) {
      case TYPE_INT:
         if (s -> initialized) {
            printf("%ld", s -> value_int);
         } else {
            printf("?");
         }
         break;
      case TYPE_FLOAT:
      case TYPE_DOUBLE:
         if (s -> initialized) {
            printf("%g", s -> value_float);
         } else {
            printf("?");
         }
         break;
      case TYPE_CHAR_ARRAY:
         if (s -> initialized) {
            printf("addr");
         } else {
            printf("addr");
         }
         break;
      case TYPE_CHAR_PTR:
         if (s -> initialized) {
            printf("addr");
         } else {
            printf("?");
         }
         break;
   }
}

void print_binding_table(struct SymbolTable *t) {
   printf("S = {");
   for (size_t i = 0; i < t -> count; i++) {
      print_symbol_binding(&t -> items[i]);
      if (i + 1 < t -> count) {
         printf("; ");
      }
   }
   printf("}\n");
}

void format_binding_table(const struct SymbolTable *t, char *buffer, size_t buffer_size) {
   size_t used = 0;
   int n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "S = {");
   if (n > 0) used += (size_t)n;
   for (size_t i = 0; i < t -> count; i++) {
      const struct Symbol *s = &t -> items[i];
      // name
      n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "%s |-> ", s -> name);
      if (n > 0) used += (size_t)n;
      // value
      switch (s -> type) {
         case TYPE_INT:
            if (s -> initialized) n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "%ld", s -> value_int);
            else n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "?");
            break;
         case TYPE_FLOAT:
         case TYPE_DOUBLE:
            if (s -> initialized) n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "%g", s -> value_float);
            else n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "?");
            break;
         case TYPE_CHAR_ARRAY:
            n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "addr");
            break;
         case TYPE_CHAR_PTR:
            if (s -> initialized) n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "addr");
            else n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "?");
            break;
      }
      if (n > 0) used += (size_t)n;
      if (i + 1 < t -> count) {
         n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "; ");
         if (n > 0) used += (size_t)n;
      }
   }
   snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "}");
}

// --- Stack model for scopes (visualization only) ---
typedef struct {
   char names[64][64];
   int top;
   int scope_marks[64];
   int scope_top;
} StackViz;

static StackViz g_stack = { .top = -1, .scope_top = -1 };

void stack_reset(){ g_stack.top = -1; g_stack.scope_top = -1; }

void stack_enter_scope(){
   if (g_stack.scope_top + 1 < 64){
      g_stack.scope_top++;
      g_stack.scope_marks[g_stack.scope_top] = g_stack.top;
   }
}

bool remove_symbol(struct SymbolTable *t, const char *var_name){
   for (int i = 0; i < (int)t->count; i++){
      if (strcmp(t->items[i].name, var_name) == 0){
         // shift left
         for (int j = i + 1; j < (int)t->count; j++) t->items[j-1] = t->items[j];
         t->count--;
         return true;
      }
   }
   return false;
}

void stack_exit_scope(struct SymbolTable *t){
   if (g_stack.scope_top < 0) return;
   int prev_top = g_stack.scope_marks[g_stack.scope_top];
   while (g_stack.top > prev_top){
      const char *name = g_stack.names[g_stack.top];
      remove_symbol(t, name);
      g_stack.top--;
   }
   g_stack.scope_top--;
}

void stack_on_declare(struct SymbolTable *t, const char *var_name){
   (void)t;
   if (g_stack.top + 1 < 64){
      g_stack.top++;
      strncpy(g_stack.names[g_stack.top], var_name, sizeof(g_stack.names[g_stack.top]) - 1);
      g_stack.names[g_stack.top][sizeof(g_stack.names[g_stack.top]) - 1] = '\0';
   }
}

void format_stack(char *buffer, size_t buffer_size){
   size_t used = 0;
   int n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "Top ");
   if (n > 0) used += (size_t)n;
   for (int i = g_stack.top; i >= 0; i--){
      n = snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "[%s]%s", g_stack.names[i], i>0?"->":"");
      if (n > 0) used += (size_t)n;
   }
   if (used == 4) snprintf(buffer + used, buffer_size > used ? buffer_size - used : 0, "(empty)");
}

char *format_stack_diagram(const struct SymbolTable *t){
   // Build a simple ASCII box stack from top to bottom
   // Example:
   //  +-----+
   //  |  x  |
   //  +-----+
   //  |  i  |
   //  +-----+
   size_t cap = 1024;
   char *out = (char*)malloc(cap);
   if (!out) return NULL;
   size_t len = 0;
   // Header
   const char *header = "Stack (top at first box):\n";
   size_t hlen = strlen(header);
   memcpy(out + len, header, hlen); len += hlen; out[len] = '\0';
   if (g_stack.top < 0){
      const char *empty = "(empty)\n";
      size_t elen = strlen(empty);
      if (len + elen + 1 > cap){ cap *= 2; out = (char*)realloc(out, cap); }
      memcpy(out + len, empty, elen); len += elen; out[len] = '\0';
      return out;
   }
   for (int i = g_stack.top; i >= 0; i--){
      // Lookup value
      const struct Symbol *s = NULL;
      for (int k = 0; k < (int)t->count; k++) {
         if (strcmp(t->items[k].name, g_stack.names[i]) == 0) { s = &t->items[k]; break; }
      }
      char display[64];
      if (s) {
         switch (s->type) {
            case TYPE_INT:
               if (s->initialized) snprintf(display, sizeof(display), "%s = %ld", s->name, s->value_int);
               else snprintf(display, sizeof(display), "%s = ?", s->name);
               break;
            case TYPE_FLOAT:
            case TYPE_DOUBLE:
               if (s->initialized) snprintf(display, sizeof(display), "%s = %g", s->name, s->value_float);
               else snprintf(display, sizeof(display), "%s = ?", s->name);
               break;
            case TYPE_CHAR_ARRAY:
               snprintf(display, sizeof(display), "%s = addr", s->name);
               break;
            case TYPE_CHAR_PTR:
               if (s->initialized) snprintf(display, sizeof(display), "%s = addr", s->name);
               else snprintf(display, sizeof(display), "%s = ?", s->name);
               break;
         }
      } else {
         snprintf(display, sizeof(display), "%s", g_stack.names[i]);
      }
      char line[128];
      snprintf(line, sizeof(line), "+----------------+\n| %-14s |\n+----------------+\n", display);
      size_t l = strlen(line);
      if (len + l + 1 > cap){ cap *= 2; out = (char*)realloc(out, cap); }
      memcpy(out + len, line, l); len += l; out[len] = '\0';
   }
   return out;
}