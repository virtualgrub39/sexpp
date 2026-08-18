#include <sexpp.h>

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

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
    size_t offset;
    size_t length;

    struct token *next;
};

static struct token *
token_new (int kind, const char *ptr, size_t offset, size_t length)
{
    struct token *t = malloc (sizeof *t);
    if (!t) return NULL;

    t->kind = kind;
    t->ptr = ptr;
    t->offset = offset;
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
        return token_new (TOKEN_EOF, token_start (L), L->idx, 0);

    struct token *t;

    switch (current_char (L))
    {
    case '(':
        t = token_new (TOKEN_LPAREN, token_start (L), L->idx, 1);
        bump (L);
        return t;
    case ')':
        t = token_new (TOKEN_RPAREN, token_start (L), L->idx, 1);
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

        t = token_new (TOKEN_INTEGER, token_start (L), L->idx, offset - L->idx);

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

        t = token_new (TOKEN_SYMBOL, token_start (L), L->idx, offset - L->idx);

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

static struct token *
next (struct token **head, sexpr_err_t *out_err)
{
    struct token *next_tok;
    if (!head || !*head)
    {
        if (out_err)
        {
            out_err->code = SEXPR_ERR_UNEXPECTED_EOF;
            out_err->offset = 0;
        }
        
        return NULL;
    }

    next_tok = *head;
    *head = (*head)->next;

    return next_tok;
}

static struct token *
peek (struct token **head, sexpr_err_t *out_err)
{
    if (!head || !*head)
    {
        if (out_err)
        {
            out_err->code = SEXPR_ERR_UNEXPECTED_EOF;
            out_err->offset = 0;
        }
        
        return NULL;
    }

    return *head;
}

static struct token *
expect (struct token **head, int kind, sexpr_err_t *out_err)
{
    struct token *next_tok = next (head, out_err);
    if (!next_tok) return NULL; /* Safety check for NULL on EOF */

    if (next_tok->kind != kind)
    {
        if (out_err)
        {
            out_err->code = SEXPR_ERR_UNEXPECTED_TOKEN;
            out_err->offset = next_tok->offset;
        }

        return NULL;
    }

    return next_tok;
}

static sexpr_t *parse_list_tail (struct token **head, sexpr_err_t *out_err);
static sexpr_t *parse_sexpr (struct token **head, sexpr_err_t *out_err);

static sexpr_t *
parse_list (struct token **head, sexpr_err_t *out_err)
{
    if (!expect (head, TOKEN_LPAREN, out_err))
        return NULL;

    return parse_list_tail (head, out_err);
}

static sexpr_t *
parse_list_tail (struct token **head, sexpr_err_t *out_err)
{
    struct token *tok = peek (head, out_err);
    if (!tok) return NULL;

    if (tok->kind == TOKEN_RPAREN)
    {
        next (head, out_err); 
        return NULL;
    }

    sexpr_t *head_expr = parse_sexpr (head, out_err);
    if (!head_expr) return NULL;

    sexpr_t *tail_expr = parse_list_tail (head, out_err);
    if (!tail_expr && out_err && out_err->code != SEXPR_OK)
    {
        sexpr_release (head_expr);
        return NULL;
    }

    return sexpr_pair_s (head_expr, tail_expr);
}

#include <stdio.h>

static sexpr_t *
parse_integer (struct token **head, sexpr_err_t *out_err)
{
    struct token *int_tok = expect (head, TOKEN_INTEGER, out_err);
    char *int_buf;
    char *end_ptr;
    int i;

    if (!int_tok) return NULL;
    
    int_buf = calloc (int_tok->length + 1, sizeof *int_buf);
    memcpy (int_buf, int_tok->ptr, int_tok->length);
    i = (int)strtol (int_buf, &end_ptr, 10);
    
    if (*end_ptr != 0)
    {
        free (int_buf);
        /* FIXME: unreachable, if tokenizer doesn't suck ass */
        if (out_err)
        {
            out_err->code = SEXPR_ERR_INVALID_INTEGER;
            out_err->offset = int_tok->offset;
        }
        return NULL;
    }

    free (int_buf);
    return sexpr_integer (i);
}

static sexpr_t *
parse_symbol (struct token **head, sexpr_err_t *out_err)
{
    struct token *sym_tok = expect (head, TOKEN_SYMBOL, out_err);
    
    if (!sym_tok) return NULL;

    return sexpr_symbol_n(sym_tok->ptr, sym_tok->length);
}

static sexpr_t *
parse_sexpr (struct token **head, sexpr_err_t *out_err)
{
    struct token *next_tok = peek (head, out_err);
    if (!next_tok) return NULL;

    switch (next_tok->kind)
    {
    case TOKEN_INTEGER: return parse_integer (head, out_err);
    case TOKEN_SYMBOL:  return parse_symbol (head, out_err);
    case TOKEN_LPAREN:  return parse_list (head, out_err);
    case TOKEN_EOF:
    {
        if (out_err)
        {
            out_err->code = SEXPR_ERR_UNEXPECTED_EOF;
            out_err->offset = next_tok->offset;
        }
        return NULL;
    }
    case TOKEN_RPAREN:
    {
        if (out_err)
        {
            out_err->code = SEXPR_ERR_UNBALANCED_PAREN;
            out_err->offset = next_tok->offset;
        }
        return NULL;
    }
    default:
        return NULL;
    }
}

/* public API again */

sexpr_t *
sexpr_deserialize_n (const char *s, size_t n, sexpr_err_t *out_err)
{
    struct token *tokens = lex (s, n, out_err);
    struct token **head = &tokens;
    if (!tokens) return NULL;

    sexpr_t *root = parse_sexpr (head, out_err);

    token_free (tokens);

    return root;
}

sexpr_t *
sexpr_deserialize (const char *s, sexpr_err_t *out_err)
{
    return sexpr_deserialize_n(s, 0, out_err);
}

