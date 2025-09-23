## Binding Table Interpreter (COMP211)

A tiny interpreter front-end that tokenizes and parses a very small C-like language and builds a symbol/binding table. The binding table is printed in the form:

```
S = {x |-> 5; name |-> addr; A |-> ?}
```

where:
- `x |-> 5` means variable `x` is bound to the value 5
- `name |-> addr` means `name` is an array-like storage with an address
- `A |-> ?` means `A` is declared but not (yet) initialized

### Quick start

```bash
make       # builds the executable `bt`
make run   # builds and runs
make clean # removes `bt`
```

Out of the box, `main.c` feeds this program:

```c
int my_variable; float another_var;
```

and prints:

```
S = {my_variable |-> ?; another_var |-> ?}
```

## Project layout

- `lexer.c/.h`  — converts an input string into a stream of tokens
- `parser.c/.h` — consumes tokens and populates a symbol table (binding table)
- `bt.c/.h`     — symbol table data structures and operations, including pretty-printing
- `main.c`      — example driver wiring lexer + parser + binding table together
- `Makefile`    — simple build/run targets

## Supported language (subset)

This is intentionally small. Currently supported statements are declaration statements of the form:

- Type keywords: `int`, `float`, `double`, `char`, `char*`
- Identifiers: start with a letter or underscore, followed by letters, digits, or underscores
- Terminated by a semicolon (`;`)

Examples that parse today:

```c
int x;
float y;
double z;
char name;     // treated as a char array slot in the binding table
char* p;       // pointer type
```

Not yet implemented (roadmap):

- Assignments (e.g., `x = 5;`, `x = x + 3;`)
- Array declarations with explicit sizes (e.g., `char[10] name;`)
- Expressions and evaluation

## Binding table model

The binding table tracks variables and what they are bound to. See `bt.h` for types:

- `TYPE_INT`, `TYPE_FLOAT`, `TYPE_DOUBLE`
- `TYPE_CHAR_ARRAY` (covers `char`/array storage)
- `TYPE_CHAR_PTR` (covers pointer-to-char)

Each entry records:
- `name`: identifier
- `type`: one of the types above
- `initialized`: whether a value/address has been set
- `array_len` and union of `value_int`, `value_float`, `address`

### Print format

Printing is handled by `print_binding_table(struct SymbolTable*)` in `bt.c`. The format is:

```
S = {id1 |-> v1; id2 |-> v2; ...}
```

Rules used when printing values (`v`):
- For `int/float/double`: print the numeric value if initialized, otherwise `?`
- For `char` (modeled as array storage): prints `addr` (placeholder for address)
- For `char*`: prints `addr` if initialized, otherwise `?`

This mirrors the example binding-table style used in class: values show actual numbers when known, `addr` for array-like storage, and `?` when a value is not yet known.

## How it works

### 1) Lexing (`lexer.c`)

The lexer scans the input and produces tokens. Supported kinds:

- `TOKEN_KEYWORD`     — reserved words such as `int`, `float`, `double`, `char`, `char*`, etc.
- `TOKEN_IDENTIFIER`  — names like `my_var`, `_tmp2` (letters/underscore start; then letters/digits/underscore)
- `TOKEN_NUMBER`      — digit sequences (for roadmap when expressions are added)
- `TOKEN_OPERATOR`    — operators such as `=`, `+`, `-`, `*`, `/`, `<`, `>`, `==`, `!=`, `<=`, `>=` (for roadmap)
- `TOKEN_PUNCTUATION` — `;`, `()`, `{}`, `[]`
- `TOKEN_END_OF_FILE`

Other notes:
- Single-line comments `// ...` are skipped
- On unknown characters, the lexer prints an error and exits
- The lexer currently recognizes identifiers with underscores (fix already applied)

### 2) Parsing (`parser.c`)

The parser currently supports declarations of the form:

```
<type-keyword> <identifier> ;
```

Parsing flow:
- `parse_program` walks the token stream until `EOF`
- `parse_statement` dispatches to `parse_declaration` when it sees a type keyword
- `parse_declaration` checks type keyword → identifier → semicolon and then calls `add(...)` on the symbol table

Roadmap additions would extend `parse_statement` to handle assignments and expressions.

### 3) Binding table (`bt.c/.h`)

Key operations:
- `find` — look up a symbol by name
- `add` — add a symbol if new, or update existing via `set`
- `set` — set a symbol's value/address and initialization state; importantly, it safely handles `NULL` (uninitialized) without dereferencing it
- `free_symbols` — frees any heap storage owned by char arrays/pointers (when those are introduced)
- `print_binding_table` — prints the `S = { ... }` representation

## Example runs

### Current default (no initialization)

Input (see `main.c`):

```c
int my_variable; float another_var;
``;

Output:

```
S = {my_variable |-> ?; another_var |-> ?}
```

### Target examples (for future milestones)

These are not yet implemented but illustrate the intended semantics:

1) `int x = 5;` → `S = {x |-> 5}`
2) `x = x + 3;` → `S = {x |-> 8}`
3) `char[10] name;` → `S = {x |-> 8; name |-> addr}`
4) `char * A;` → `S = {x |-> 8; name |-> addr; A |-> ?}`

Adding these requires extending the parser with assignments, array declarations with sizes, and expression evaluation.

## Building and running

Prerequisites: `gcc` (or any C compiler that supports this C subset)

```bash
make       # builds `bt`
make run   # builds and runs `bt`
make clean # removes `bt`
```

## Testing and TDD

While the repo does not yet include a test framework, recommended TDD approach:

1. Write small C-based unit tests (e.g., in a `tests/` folder) that feed input strings into `tokenize` and `parse_program`, then assert on the resulting `SymbolTable` state. For example, after `int x;` expect one entry named `x` with type `TYPE_INT` and `initialized == false`.
2. For formatting, call `print_binding_table` and compare against expected strings such as `S = {x |-> ?}`.
3. Add tests for negative cases: missing semicolons, unknown keywords, illegal identifiers, etc.

If you plan to grow the language, add tests first for each new construct (assignment, arrays, expressions), then implement parser/lexer changes until the tests pass.

## Extending the interpreter

Suggested milestones:

- Declarations with initialization: parse `int x = 5;` and set `initialized = true` with `value_int = 5`
- Simple assignments: parse `x = x + 3;` and evaluate expressions over the binding table
- Arrays: `char[10] name;` (store length in `array_len` and allocate storage)
- Pointers: `char* A;` with assignment from array address `A = &name[0];`
- Pretty-print values in a more detailed way (e.g., show array lengths or pointer addresses consistently)

## Notes

- The current design keeps the front-end modular: you can swap out `main.c` with a file reader, or embed the components in a larger project.
- Error handling aims to be informative but simple; feel free to convert `fprintf + exit` to error codes if integrating elsewhere.


