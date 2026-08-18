#ifndef SEXPP_H
#define SEXPP_H

#include <stddef.h>

/* ---- TYPES ---- */

typedef enum
{
    SEXPR_NIL, /* non-instantiable: NULL for NIL */
    SEXPR_PAIR,
    SEXPR_INTEGER,
    SEXPR_SYMBOL,
} sexpr_kind;

typedef struct sexpr
{
    int rc;
    
    sexpr_kind kind;
    union
    {
        struct { struct sexpr *head, *tail; } pair;
        int integer;
        char *symbol;
    };
} sexpr_t;

/* ---- LIFETIME ---- */

sexpr_t *sexpr (sexpr_kind kind);
sexpr_t *sexpr_integer (int i);
sexpr_t *sexpr_symbol (const char *sym);
sexpr_t *sexpr_symbol_n (const char *sym, size_t n); /* sized */
sexpr_t *sexpr_pair (sexpr_t *head, sexpr_t *tail);
sexpr_t *sexpr_pair_s (sexpr_t *head, sexpr_t *tail); /* stealing */

/* ---- REF COUNTING ---- */

sexpr_t *sexpr_retain (sexpr_t *s); /* returns s */
void sexpr_release (sexpr_t *s);

/* ---- HELPERS ---- */

#define SEKIND(s) ((s) ? (s)->kind : SEXPR_NIL)
#define CAR(s) ((s)->pair.head)
#define CDR(s) ((s)->pair.tail)

/* ---- ERROR HANDLING ---- */
/* (for parsing only) */

typedef enum
{
    SEXPR_OK = 0,
    SEXPR_ERR_UNBALANCED_PAREN,
    SEXPR_ERR_INVALID_CHAR,
    SEXPR_ERR_UNEXPECTED_EOF,
    SEXPR_ERR_UNEXPECTED_TOKEN,
    SEXPR_ERR_INVALID_INTEGER,
} sexpr_err_code;

typedef struct
{
    sexpr_err_code code;
    size_t offset;
} sexpr_err_t;

/* ---- PARSING ---- */

sexpr_t *sexpr_deserialize (const char *s, sexpr_err_t *out_err);
sexpr_t *sexpr_deserialize_n (const char *s, size_t n, sexpr_err_t *out_err);

char *sexpr_serialize (sexpr_t *s, sexpr_err_t *out_err); /* heap-allocated */

#endif
