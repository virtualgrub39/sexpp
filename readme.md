# S-EXPression Parser - SEXPP

Very serious, simple and inneficient S-Expression parser.

Intended usecase is for parsing data, in form of S-expression (config files, etc.).

My personal usecase is for intermediate representation in my various compiler-related projects.

## Including in Your project

In Your makefile, you can do

```makefile
include path/to/sexpp/sexpp.mk
```

Needed source files will be appended to `SOURCES` variable, and needed flags will be appended to `CFLAGS` variable.

Alternatively, You can rawdog everything by adding:

```makefile
SEXPP_DIR = path/to/sexpp
SEXPP_LIB = $(SEXPP_DIR)/lib/libsexpp.a

CFLAGS += -I$(SEXPP_DIR)/include

$(SEXPP_LIB):
    $(MAKE) -C $(SEXPP_DIR)
```

Or equivalent for Your build system.

This library is meant to be embedded in other projects as a very light dependency, so there is no `install` target. You're encouraged to modify both source code and build system for `sexpp` to suit Your project.

## Usage

Library consists of two main parts:

### Lifetime API

That is: creating and destroying S-expression atoms.

The library is "optimized" for proper-lists, of unmutable atoms. Meaning, if you're building a interpreter or something - this is a wrong tool.

Memory management is done through reference-counting. Fancy features like loop detection, etc. are just not here, so be careful. 

```c
/* For creating an Atom */

sexpr_t *sexpr (sexpr_kind kind); /* uninitialized atom */
sexpr_t *sexpr_integer (int i);
sexpr_t *sexpr_symbol (const char *sym);
sexpr_t *sexpr_symbol_n (const char *sym, size_t n); /* sized */
sexpr_t *sexpr_string (const char *str);
sexpr_t *sexpr_string_n (const char *str, size_t n); /* sized */
sexpr_t *sexpr_pair (sexpr_t *head, sexpr_t *tail);
sexpr_t *sexpr_pair_s (sexpr_t *head, sexpr_t *tail); /* stealing (doesn't increment RC) */

/* Reference counting stuff */

sexpr_t *sexpr_retain (sexpr_t *s); /* increments rc, returns s */
void sexpr_release (sexpr_t *s); /* cleans up, if rc = 0 */

```

`nil` atoms are represented by `NULL` pointer. Meaning: all functions are "NULL-friendly": `NULL` argument is not regarded as error.

### Parsing API

For parsing and serializing S-expressions. 

```c
sexpr_t *sexpr_deserialize (const char *s, sexpr_err_t *out_err);
sexpr_t *sexpr_deserialize_n (const char *s, size_t n, sexpr_err_t *out_err); /* sized */

char *sexpr_serialize (sexpr_t *s, size_t *out_sz); /* returns heap-allocated c-string. Must be `free`d */

```

Usage of those is obvious I hope.

## License

see [LICENSE](LICENSE)
