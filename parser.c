#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bt.h"
#include "parser.h"
#include "lexer.h"

// Forward declarations for internal expression parsing helpers
static long parse_int_expression(Token **tokens, struct SymbolTable *t, int *ok);

// --------- Utility to collect and print a table of statement -> binding table snapshots ---------
typedef struct {
   char *command;
   char *binding;
   char *stack;
   char *stack_diagram; // optional multi-line diagram for this step
} TableRow;

// Global row accumulator so nested constructs (e.g., function/while bodies) can append rows
static TableRow *g_rows_ref = NULL;
static size_t g_rows_count = 0;
static size_t g_rows_cap = 0;
static bool g_suppress_next_row = false;

static char *dup_string(const char *s) {
   size_t n = strlen(s) + 1;
   char *d = (char *)malloc(n);
   if (d) memcpy(d, s, n);
   return d;
}

static char *stringify_statement(Token *start) {
   // Join tokens until and including ';' with simple spacing rules
   size_t cap = 256;
   size_t len = 0;
   char *buf = (char *)malloc(cap);
   if (!buf) return NULL;
   buf[0] = '\0';

   Token *p = start;
   bool prev_was_open_bracket = false; // '[' or '('
   while (!(p->type == TOKEN_PUNCTUATION && strcmp(p->value, ";") == 0)) {
      const char *tok = p->value;
      bool is_punc = (p->type == TOKEN_PUNCTUATION);
      bool is_close = is_punc && (strcmp(tok, ")") == 0 || strcmp(tok, "]") == 0);
      bool is_open = is_punc && (strcmp(tok, "(") == 0 || strcmp(tok, "[") == 0);

      size_t tok_len = strlen(tok);
      size_t add_space = (len > 0 && !is_close && !prev_was_open_bracket) ? 1 : 0;
      if (len + add_space + tok_len + 2 > cap) {
         cap *= 2;
         char *tmp = (char *)realloc(buf, cap);
         if (!tmp) { free(buf); return NULL; }
         buf = tmp;
      }
      if (add_space) buf[len++] = ' ';
      memcpy(buf + len, tok, tok_len);
      len += tok_len;
      buf[len] = '\0';
      prev_was_open_bracket = is_open;
      p++;
   }
   // append ';'
   if (len + 1 + 1 > cap) {
      cap *= 2;
      char *tmp = (char *)realloc(buf, cap);
      if (!tmp) { free(buf); return NULL; }
      buf = tmp;
   }
   buf[len++] = ';';
   buf[len] = '\0';
   return buf;
}

static void print_horizontal_rule(int w1, int w2) {
   printf("+");
   for (int i = 0; i < w1 + 2; i++) printf("-");
   printf("+");
   for (int i = 0; i < w2 + 2; i++) printf("-");
   printf("+\n");
}

static void print_table(TableRow *rows, size_t row_count) {
   int w1 = (int)strlen("Commands");
   int w2 = (int)strlen("Binding table");
   int w3 = (int)strlen("Stack");
   for (size_t i = 0; i < row_count; i++) {
      if ((int)strlen(rows[i].command) > w1) w1 = (int)strlen(rows[i].command);
      if ((int)strlen(rows[i].binding) > w2) w2 = (int)strlen(rows[i].binding);
      if (rows[i].stack && (int)strlen(rows[i].stack) > w3) w3 = (int)strlen(rows[i].stack);
   }
   // draw 3-column table
   printf("+"); for (int i=0;i<w1+2;i++) printf("-");
   printf("+"); for (int i=0;i<w2+2;i++) printf("-");
   printf("+"); for (int i=0;i<w3+2;i++) printf("-");
   printf("+\n");
   printf("| %-*s | %-*s | %-*s |\n", w1, "Commands", w2, "Binding table", w3, "Stack");
   printf("+"); for (int i=0;i<w1+2;i++) printf("-");
   printf("+"); for (int i=0;i<w2+2;i++) printf("-");
   printf("+"); for (int i=0;i<w3+2;i++) printf("-");
   printf("+\n");
   for (size_t i = 0; i < row_count; i++) {
      printf("| %-*s | %-*s | %-*s |\n", w1, rows[i].command, w2, rows[i].binding, w3, rows[i].stack ? rows[i].stack : "");
   }
   printf("+"); for (int i=0;i<w1+2;i++) printf("-");
   printf("+"); for (int i=0;i<w2+2;i++) printf("-");
   printf("+"); for (int i=0;i<w3+2;i++) printf("-");
   printf("+\n");
}

static void append_row_with(const char *cmd_text, struct SymbolTable *t) {
   if (g_suppress_next_row) { g_suppress_next_row = false; return; }
   if (!g_rows_ref) return;
   if (g_rows_count >= g_rows_cap) {
      size_t new_cap = g_rows_cap == 0 ? 8 : g_rows_cap * 2;
      TableRow *tmp = (TableRow *)realloc(g_rows_ref, sizeof(TableRow) * new_cap);
      if (!tmp) return;
      g_rows_ref = tmp;
      g_rows_cap = new_cap;
   }
   char s_buf[1024];
   format_binding_table(t, s_buf, sizeof(s_buf));
   char st_buf[256];
   format_stack(st_buf, sizeof(st_buf));
   g_rows_ref[g_rows_count].command = dup_string(cmd_text ? cmd_text : "");
   g_rows_ref[g_rows_count].binding = dup_string(s_buf);
   g_rows_ref[g_rows_count].stack = dup_string(st_buf);
   g_rows_ref[g_rows_count].stack_diagram = format_stack_diagram(t);
   g_rows_count++;
}

static void append_row_from_tokens(Token *stmt_start, struct SymbolTable *t) {
   char *cmd = stringify_statement(stmt_start);
   append_row_with(cmd, t);
   free(cmd);
}

static long parse_int_factor(Token **tokens, struct SymbolTable *t, int *ok) {
   // Parenthesized expression: '(' expr ')'
   if ((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, "(") == 0) {
      (*tokens)++; // consume '('
      long inner = parse_int_expression(tokens, t, ok);
      if (!*ok) return 0;
      if (!((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, ")") == 0)) {
         fprintf(stderr, "Error: Expected ')' to close '(' but found '%s'.\n", (*tokens)->value);
         *ok = 0;
         return 0;
      }
      (*tokens)++; // consume ')'
      return inner;
   }

   if ((*tokens)->type == TOKEN_NUMBER) {
      long v = strtol((*tokens)->value, NULL, 10);
      (*tokens)++;
      *ok = 1;
      return v;
   }
   if ((*tokens)->type == TOKEN_IDENTIFIER) {
      struct Symbol *s = find(t, (*tokens)->value);
      if (s && s->type == TYPE_INT && s->initialized) {
         long v = s->value_int;
         (*tokens)++;
         *ok = 1;
         return v;
      } else {
         fprintf(stderr, "Error: Undefined or uninitialized identifier '%s' in expression.\n", (*tokens)->value);
         *ok = 0;
         return 0;
      }
   }
   fprintf(stderr, "Error: Expected number or identifier in expression but found '%s'.\n", (*tokens)->value);
   *ok = 0;
   return 0;
}

static long parse_int_term(Token **tokens, struct SymbolTable *t, int *ok) {
   long value = parse_int_factor(tokens, t, ok);
   if (!*ok) return 0;
   while ((*tokens)->type == TOKEN_OPERATOR &&
          (strcmp((*tokens)->value, "*") == 0 || strcmp((*tokens)->value, "/") == 0)) {
      char op = (*tokens)->value[0];
      (*tokens)++; // consume '*' or '/'
      long rhs = parse_int_factor(tokens, t, ok);
      if (!*ok) return 0;
      if (op == '*') {
         value *= rhs;
      } else {
         if (rhs == 0) {
            fprintf(stderr, "Error: Division by zero.\n");
            *ok = 0;
            return 0;
         }
         value /= rhs; // integer division
      }
   }
   return value;
}

static long parse_int_expression(Token **tokens, struct SymbolTable *t, int *ok) {
   long value = parse_int_term(tokens, t, ok);
   if (!*ok) return 0;
   while ((*tokens)->type == TOKEN_OPERATOR &&
          (strcmp((*tokens)->value, "+") == 0 || strcmp((*tokens)->value, "-") == 0)) {
      char op = (*tokens)->value[0];
      (*tokens)++; // consume '+' or '-'
      long rhs = parse_int_term(tokens, t, ok);
      if (!*ok) return 0;
      value = (op == '+') ? (value + rhs) : (value - rhs);
   }
   return value;
}

static void parse_assignment(Token **tokens, struct SymbolTable *t) {
   // Current token is IDENTIFIER (lhs)
   const char *lhs_name = (*tokens)->value;
   (*tokens)++; // consume identifier

   if (!((*tokens)->type == TOKEN_OPERATOR && strcmp((*tokens)->value, "=") == 0)) {
      fprintf(stderr, "Error: Expected '=' after identifier '%s'.\n", lhs_name);
      return;
   }
   (*tokens)++; // consume '='

   int ok = 0;
   long result = parse_int_expression(tokens, t, &ok);
   if (!ok) return;

   // Do not consume ';' here; leave it for the caller pattern
   if (!((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, ";") == 0)) {
      fprintf(stderr, "Error: Expected a semicolon after assignment to '%s'.\n", lhs_name);
      return;
   }

   // Ensure symbol exists as int; create if absent
   add(t, lhs_name, TYPE_INT, &result, 0);
}

static bool parse_optional_initializer(Token **tokens, VarType type, struct SymbolTable *t, const char *var_name) {
   if (!((*tokens)->type == TOKEN_OPERATOR && strcmp((*tokens)->value, "=") == 0)) {
      return false; // no initializer
   }
   (*tokens)++; // consume '='
   if (type == TYPE_INT) {
      int ok = 0;
      long result = parse_int_expression(tokens, t, &ok);
      if (!ok) return false;
      add(t, var_name, TYPE_INT, &result, 0);
      return true;
   }
   // For non-int initializers, skip for now
   return false;
}

// Relational: lhs (op rhs)?  with op in >, <, >=, <=, ==, !=
static long parse_relational(Token **tokens, struct SymbolTable *t, int *ok) {
   long lhs = parse_int_expression(tokens, t, ok);
   if (!*ok) return 0;
   if ((*tokens)->type == TOKEN_OPERATOR &&
       (strcmp((*tokens)->value, ">") == 0 || strcmp((*tokens)->value, "<") == 0 ||
        strcmp((*tokens)->value, ">=") == 0 || strcmp((*tokens)->value, "<=") == 0 ||
        strcmp((*tokens)->value, "==") == 0 || strcmp((*tokens)->value, "!=") == 0)) {
      const char *op = (*tokens)->value;
      (*tokens)++;
      long rhs = parse_int_expression(tokens, t, ok);
      if (!*ok) return 0;
      if (strcmp(op, ">") == 0) return lhs > rhs;
      if (strcmp(op, "<") == 0) return lhs < rhs;
      if (strcmp(op, ">=") == 0) return lhs >= rhs;
      if (strcmp(op, "<=") == 0) return lhs <= rhs;
      if (strcmp(op, "==") == 0) return lhs == rhs;
      if (strcmp(op, "!=") == 0) return lhs != rhs;
   }
   return lhs; // truthy if nonzero
}

static Token *find_matching_brace(Token *p) {
   int depth = 0;
   while (!(p->type == TOKEN_END_OF_FILE)) {
      if (p->type == TOKEN_PUNCTUATION && strcmp(p->value, "{") == 0) depth++;
      else if (p->type == TOKEN_PUNCTUATION && strcmp(p->value, "}") == 0) {
         depth--;
         if (depth == 0) return p;
      }
      p++;
   }
   return p;
}

static char *stringify_while(Token *start) {
   // start at 'while', capture until matching '}'
   Token *p = start;
   // Find opening '{'
   while (!(p->type == TOKEN_PUNCTUATION && strcmp(p->value, "{") == 0) && p->type != TOKEN_END_OF_FILE) p++;
   if (p->type == TOKEN_END_OF_FILE) return dup_string("while ...");
   Token *end = find_matching_brace(p);
   // Build string from start to end inclusive
   size_t cap = 512, len = 0;
   char *buf = (char *)malloc(cap);
   if (!buf) return NULL;
   buf[0] = '\0';
   Token *q = start;
   bool prev_open = false;
   while (q <= end) {
      const char *tok = q->value;
      bool is_p = (q->type == TOKEN_PUNCTUATION);
      bool is_close = is_p && (strcmp(tok, ")") == 0 || strcmp(tok, "]") == 0 || strcmp(tok, "}") == 0);
      bool is_open = is_p && (strcmp(tok, "(") == 0 || strcmp(tok, "[") == 0 || strcmp(tok, "{") == 0);
      size_t tok_len = strlen(tok);
      size_t add_space = (len > 0 && !is_close && !prev_open) ? 1 : 0;
      if (len + add_space + tok_len + 2 > cap) {
         cap *= 2;
         char *tmp = (char *)realloc(buf, cap);
         if (!tmp) { free(buf); return NULL; }
         buf = tmp;
      }
      if (add_space) buf[len++] = ' ';
      memcpy(buf + len, tok, tok_len);
      len += tok_len;
      buf[len] = '\0';
      prev_open = is_open;
      q++;
   }
   return buf;
}

static void parse_while(Token **tokens, struct SymbolTable *t) {
   // consume 'while'
   (*tokens)++;
   if (!((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, "(") == 0)) {
      fprintf(stderr, "Error: Expected '(' after while.\n");
      return;
   }
   (*tokens)++; // after '('
   Token *cond_start = *tokens;
   // Scan ahead to find ')' from cond_start without consuming main pointer
   int ok = 0;
   Token *tmp = cond_start;
   long cond = parse_relational(&tmp, t, &ok);
   if (!ok) return;
   if (!(tmp->type == TOKEN_PUNCTUATION && strcmp(tmp->value, ")") == 0)) {
      fprintf(stderr, "Error: Expected ')' after while condition.\n");
      return;
   }
   tmp++; // token after ')'
   if (!(tmp->type == TOKEN_PUNCTUATION && strcmp(tmp->value, "{") == 0)) {
      fprintf(stderr, "Error: Expected '{' to start while body.\n");
      return;
   }
   Token *body_start = tmp + 1;
   Token *body_end = find_matching_brace(tmp);

   // Execute loop
   int iteration = 1;
   while (cond) {
      Token *bp = body_start;
      while (!(bp->type == TOKEN_PUNCTUATION && strcmp(bp->value, "}") == 0)) {
         Token *stmt_start = bp;
         parse_statement(&bp, t);
         // Build labeled command: "iter k: <stmt>"
         char *stmt_str = stringify_statement(stmt_start);
         char label[32];
         snprintf(label, sizeof(label), "iter %d: ", iteration);
         size_t total = strlen(label) + (stmt_str ? strlen(stmt_str) : 0) + 1;
         char *combined = (char*)malloc(total);
         if (combined) {
            combined[0] = '\0';
            strcat(combined, label);
            if (stmt_str) strcat(combined, stmt_str);
            append_row_with(combined, t);
            free(combined);
         }
         free(stmt_str);
         bp++; // move past ';' or inner '}'
      }
      // Re-evaluate condition
      Token *cp = cond_start;
      ok = 0;
      cond = parse_relational(&cp, t, &ok);
      if (!ok) break;
      if (!(cp->type == TOKEN_PUNCTUATION && strcmp(cp->value, ")") == 0)) break;
      iteration++;
   }

   // Position main token pointer at body_end (the '}' token). Caller will increment.
   *tokens = body_end;
}

static void parse_function(Token **tokens, struct SymbolTable *t) {
   // Expect: keyword 'int' or 'void' etc., identifier, '(', ')', '{' ... '}'
   // This is a minimal skipper that dives into body and parses statements
   // Consume return type keyword if present
   if ((*tokens)->type == TOKEN_KEYWORD) (*tokens)++;
   // Function name
   if ((*tokens)->type == TOKEN_IDENTIFIER) (*tokens)++;
   // Params '('
   if (!((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, "(") == 0)) return;
   // Skip to ')'
   while (!((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, ")") == 0) && (*tokens)->type != TOKEN_END_OF_FILE) {
      (*tokens)++;
   }
   if ((*tokens)->type == TOKEN_PUNCTUATION) (*tokens)++; // consume ')'
   if (!((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, "{") == 0)) return;
   // Parse body with new scope
   stack_enter_scope();
   (*tokens)++; // into body
   while (!((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, "}") == 0) && (*tokens)->type != TOKEN_END_OF_FILE) {
      Token *stmt_start = *tokens;
      parse_statement(tokens, t);
      append_row_from_tokens(stmt_start, t);
      (*tokens)++;
   }
   stack_exit_scope(t);
}

void parse_declaration(Token **tokens, struct SymbolTable *t) {
   VarType type;
   size_t array_len = 0;

   // The lexer has already determined the token type, so we can check it directly instead of using strcmp on the value.
   if ((*tokens)->type != TOKEN_KEYWORD) {
      fprintf(stderr, "Error: Expected a type keyword like 'int' but found '%s'.\n", (*tokens)->value);
      return;
   }

   // Now we use the string value to determine the specific type.
   if (strcmp((*tokens)->value, "int") == 0) {
      type = TYPE_INT;
      // Advance to the next token, which should be the variable name.
      (*tokens)++; 
   } else if (strcmp((*tokens)->value, "float") == 0) {
      type = TYPE_FLOAT;
      (*tokens)++;
   } else if (strcmp((*tokens)->value, "double") == 0) {
      type = TYPE_DOUBLE;
      (*tokens)++;
   } else if (strcmp((*tokens)->value, "char") == 0) {
      // char, char *name; or char[NUM] name;
      (*tokens)++;
      if ((*tokens)->type == TOKEN_OPERATOR && strcmp((*tokens)->value, "*") == 0) {
         type = TYPE_CHAR_PTR;
         (*tokens)++; // consume '*'
      } else if ((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, "[") == 0) {
         // char[NUM]
         (*tokens)++; // consume '['
         if ((*tokens)->type != TOKEN_NUMBER) {
            fprintf(stderr, "Error: Expected array length after '[' but found '%s'.\n", (*tokens)->value);
            return;
         }
         array_len = (size_t)strtoul((*tokens)->value, NULL, 10);
         (*tokens)++; // consume number
         if (!((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, "]") == 0)) {
            fprintf(stderr, "Error: Expected ']' after array length but found '%s'.\n", (*tokens)->value);
            return;
         }
         (*tokens)++; // consume ']'
         type = TYPE_CHAR_ARRAY;
      } else {
         type = TYPE_CHAR_ARRAY; // unspecified length; kept as addr
      }
   } else {
      fprintf(stderr, "Error: Unknown type '%s'.\n", (*tokens)->value);
      return;
   }

   if ((*tokens)->type != TOKEN_IDENTIFIER) {
      fprintf(stderr, "Error: Expected an identifier but found '%s'.\n", (*tokens)->value);
      return;
   }
   
   // Capture name then add symbol (uninitialized first)
   const char *name = (*tokens)->value;
   add(t, name, type, NULL, array_len);
   stack_on_declare(t, name);

   // Advance to possible initializer or semicolon
   (*tokens)++;

   // Optional initializer for int declarations: int x = <expr>;
   parse_optional_initializer(tokens, type, t, name);

   // Expect semicolon
   if (!((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, ";") == 0)) {
      fprintf(stderr, "Error: Expected a semicolon but found '%s'.\n", (*tokens)->value);
      return;
   }
}

// The highest-level function that drives the parsing process.
void parse_program(Token *tokens, struct SymbolTable *t) {
    Token *current_token = tokens;
    // Collect rows
    size_t cap = 8, count = 0;
    TableRow *rows = (TableRow *)malloc(sizeof(TableRow) * cap);
    g_rows_ref = rows; g_rows_count = 0; g_rows_cap = cap;
    
    while (current_token->type != TOKEN_END_OF_FILE) {
        Token *stmt_start = current_token;
        char *cmd = NULL;
        if (current_token->type == TOKEN_KEYWORD && strcmp(current_token->value, "while") == 0) {
            // While rows are added per-iteration for body statements; do not add a header row
            parse_while(&current_token, t);
            cmd = NULL;
        } else if (current_token->type == TOKEN_KEYWORD &&
                   (strcmp(current_token->value, "int") == 0 || strcmp(current_token->value, "void") == 0)) {
            // Heuristically treat as a function if it looks like: <kw> IDENT '('
            Token *look = stmt_start + 1;
            if (look->type == TOKEN_IDENTIFIER && (look+1)->type == TOKEN_PUNCTUATION && strcmp((look+1)->value, "(") == 0) {
                parse_function(&current_token, t);
                cmd = NULL; // no separate function row
            } else {
                parse_statement(&current_token, t);
                cmd = stringify_statement(stmt_start);
            }
        } else {
            parse_statement(&current_token, t);
            // Build command string from tokens between stmt_start and ';'
            cmd = stringify_statement(stmt_start);
        }
        // Build binding snapshot
        if (cmd) { append_row_with(cmd, t); free(cmd); }
        current_token++; // Move to the next token
    }

    // Sync locals with globals in case we reallocated
    rows = g_rows_ref; count = g_rows_count; cap = g_rows_cap;
    if (rows) {
        print_table(rows, count);
        // After the table, print the step-by-step stack diagrams
        printf("\nStack evolution by step:\n\n");
        for (size_t i = 0; i < count; i++) {
            printf("Step %zu: %s\n", i + 1, rows[i].command);
            if (rows[i].stack_diagram) {
                printf("%s\n", rows[i].stack_diagram);
            }
        }
        for (size_t i = 0; i < count; i++) {
            free(rows[i].command);
            free(rows[i].binding);
            free(rows[i].stack);
            free(rows[i].stack_diagram);
        }
        free(rows);
    }
}

void parse_statement(Token **tokens, struct SymbolTable *t) {
    if ((*tokens)->type == TOKEN_KEYWORD) {
        if (strcmp((*tokens)->value, "while") == 0) {
            parse_while(tokens, t);
            // Suppress the outer 'while ...' row; body rows were already added
            g_suppress_next_row = true;
        } else if (strcmp((*tokens)->value, "return") == 0) {
            // return [expr] ;  — skip optional expression then ';'
            (*tokens)++;
            if (!((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, ";") == 0)) {
                int ok = 0; (void)parse_int_expression(tokens, t, &ok);
            }
            if (!((*tokens)->type == TOKEN_PUNCTUATION && strcmp((*tokens)->value, ";") == 0)) {
                fprintf(stderr, "Error: Expected ';' after return.\n");
                return;
            }
            // Suppress adding a row for return statements
            g_suppress_next_row = true;
        } else {
            parse_declaration(tokens, t);
        }
    } else if ((*tokens)->type == TOKEN_IDENTIFIER) {
        parse_assignment(tokens, t);
    } else {
        fprintf(stderr, "Error: Expected a keyword or identifier but found '%s'.\n", (*tokens)->value);
        return;
    }
}