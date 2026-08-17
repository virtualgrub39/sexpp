#include <sexpp.h>

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* Trivial API */

sexpr_t *sexpr (sexpr_kind kind)
{
    sexpr_t *s = malloc (sizeof *s);
    if (!s)
    {
        return NULL;
    }

    s->rc = 1;
    s->kind = kind;

    return s;
}

sexpr_t *sexpr_integer (int i)
{
    sexpr_t *s = sexpr (SEXPR_INTEGER);
    if (!s) return NULL;

    s->integer = i;

    return s;
}

sexpr_t *sexpr_symbol (const char *sym)
{
    char *sym_clone;

    if (!sym) return NULL;

    sexpr_t *s = sexpr (SEXPR_SYMBOL);
    if (!s) return NULL;

    sym_clone = malloc ((strlen (sym) + 1) * sizeof *sym);
    if (!sym_clone) /* FIXME: preserve errno? */
    {
        free (s);
        return NULL;
    }

    strcpy (sym_clone, sym);

    s->symbol = sym_clone;

    return s;
}

sexpr_t *sexpr_symbol_n (const char *sym, size_t n)
{
    char *sym_clone;

    if (!sym || n == 0) return NULL;

    sexpr_t *s = sexpr (SEXPR_SYMBOL);
    if (!s) return NULL;

    sym_clone = malloc ((n+1) * sizeof *sym);
    if (!sym_clone) /* FIXME: preserve errno? */
    {
        free (s);
        return NULL;
    }

    memcpy (sym_clone, sym, n);
    sym_clone[n] = 0;

    s->symbol = sym_clone;

    return s;
}

sexpr_t *sexpr_pair (sexpr_t *head, sexpr_t *tail)
{
    sexpr_t *s = sexpr (SEXPR_PAIR);
    if (!s) return NULL;

    s->pair.head = sexpr_retain (head);
    s->pair.tail = sexpr_retain (tail);

    return s;
}

sexpr_t *sexpr_pair_s (sexpr_t *head, sexpr_t *tail)
{
    sexpr_t *s = sexpr (SEXPR_PAIR);
    if (!s)
    {
        sexpr_release (head);
        sexpr_release (tail);
        return NULL;
    }

    s->pair.head = head;
    s->pair.tail = tail;

    return s;
}

sexpr_t *sexpr_retain (sexpr_t *s)
{
    if (s) s->rc += 1;
    return s;
}

void sexpr_release (sexpr_t *s)
{
    sexpr_t *p = s;

    while (1)
    {
        if (!p) break;
        if (p->rc <= 0) /* assume double-free attempt */
            break;
        if ((p->rc -= 1) > 0)
            break;

        if (p->kind == SEXPR_INTEGER)
        {
            free (p);
            break;
        }
        else if (p->kind == SEXPR_SYMBOL)
        {
            free (p->symbol);
            free (p);
            break;
        }
        else if (p->kind == SEXPR_PAIR)
        {
            sexpr_t *next = p->pair.tail;

            /* assumes shallow head */
            sexpr_release (p->pair.head);
            free (p);

            p = next;
        }
    }
}

/* tokenizer */

enum
{
    TOKEN_EOF = 0,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_SYMBOL,
    TOKEN_INTEGER,
};

struct token
{
    int kind;

    const char *ptr;
    size_t length;

    struct token *next;
};

static struct token *
token_new (int kind, const char *ptr, size_t length)
{
    struct token *t = malloc (sizeof *t);
    if (!t) return NULL;

    t->kind = kind;
    t->ptr = ptr;
    t->length = length;
    t->next = NULL;

    return t;
}

static void token_free (struct token *t)
{
    while (t)
    {
        struct token *next = t->next;
        free (t);
        t = next;
    }
}

struct lexer
{
    const char *src;
    size_t idx;
    size_t len;
    sexpr_err_t *e;
};

static int reached_end (struct lexer *L)
{
    return L->idx >= L->len;
}

static char current_char (struct lexer *L)
{
    return L->src[L->idx];
}

static const char *
token_start (struct lexer *L)
{
    return L->src + L->idx;
}

static void bump (struct lexer *L)
{
    if (L->idx < L->len) L->idx += 1;
}

static int is_space (char c)
{
    return c && strchr ("\n\t\r\v ", (unsigned char) c) != NULL;
}

static int is_alpha (char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int is_digit (char c)
{
    return (c >= '0' && c <= '9');
}

static int is_symbol_char (char c)
{
    return c && (is_alpha (c) || is_digit (c) || strchr ("+-!_:/~%.<>=*", (unsigned char) c) != NULL);
}

static void skip_ws (struct lexer *L)
{
    while (!reached_end (L) && is_space (current_char (L)))
        bump (L);
}

static struct token *
lex_next (struct lexer *L)
{
    skip_ws (L);
    if (reached_end (L))
        return token_new (TOKEN_EOF, token_start (L), 0);

    struct token *t;

    switch (current_char (L))
    {
    case '(':
        t = token_new (TOKEN_LPAREN, token_start (L), 1);
        bump (L);
        return t;
    case ')':
        t = token_new (TOKEN_RPAREN, token_start (L), 1);
        bump (L);
        return t;
    }

    if (is_digit (current_char (L)))
    {
        size_t offset = L->idx + 1;

        while (offset < L->len && is_digit(L->src[offset]))
        {
            offset += 1;
        }

        t = token_new (TOKEN_INTEGER, token_start (L), offset - L->idx);

        L->idx = offset;

        return t;
    }

    if (is_symbol_char (current_char (L)))
    {
        size_t offset = L->idx + 1;

        while (offset < L->len && is_symbol_char (L->src[offset]))
        {
            offset += 1;
        }

        t = token_new (TOKEN_SYMBOL, token_start (L), offset - L->idx);

        L->idx = offset;

        return t;
    }

    if (L->e)
    {
        L->e->code = SEXPR_ERR_INVALID_CHAR;
        L->e->offset = L->idx;
    }
    return NULL;
}

static struct token *
lex (const char *src, size_t n, sexpr_err_t *out_err)
{
    struct lexer L = { src, 0, n ? n : strlen(src), out_err };
    struct token *head = NULL;
    struct token *tail = NULL;

    while (1)
    {
        struct token *next = lex_next (&L);
        if (!next)
        {
            token_free (head);
            return NULL;
        }

        if (head == NULL)
        {
            head = next;
            tail = head;
        }
        else
        {
            tail->next = next;
            tail = tail->next;
        }

        if (next->kind == TOKEN_EOF)
            break;
    }

    return head;
}

#include <stdio.h>

sexpr_t *
sexpr_deserialize (const char *s, sexpr_err_t *out_err)
{
    struct token *tokens = lex (s, 0, out_err);
    struct token *t = tokens;
    
    if (!tokens) return NULL;

    while (t)
    {
        switch (t->kind)
        {
        case TOKEN_EOF: printf ("[EOF]\n"); break;
        case TOKEN_INTEGER: printf ("[INT %.*s]\n", (int)t->length, t->ptr); break;
        case TOKEN_LPAREN: printf ("[LPAREN]\n"); break;
        case TOKEN_RPAREN: printf ("[RPAREN]\n"); break;
        case TOKEN_SYMBOL: printf ("[SYM %.*s]\n", (int)t->length, t->ptr); break;
        }

        t = t->next;
    }

    token_free (tokens);

    return (void*)696969; /* FIXME obviously... */
}
