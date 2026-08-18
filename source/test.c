#include <stdio.h>
#include <stdlib.h>

#include <sexpp.h>

char *read_whole (FILE *in, size_t *out_sz)
{
    if (!in) return NULL;

    size_t cap = 128;
    size_t sz = 0;
    char *buf = malloc (cap * sizeof *buf);

    while (1)
    {
        size_t n = fread (buf + sz, sizeof *buf, cap - sz, in);
        if (n == 0)
        {
            if (feof (in)) break;
            else if (ferror (in))
            {
                perror ("fread");
                free (buf);
                return NULL;
            }
        }

        sz += n;

        if (sz >= (cap-1))
        {
            cap *= 2;
            buf = realloc (buf, cap * sizeof *buf);
        }
    }

    buf = realloc (buf, (sz + 1) * sizeof *buf);
    buf[sz] = 0;

    if (out_sz) *out_sz = sz;

    return buf;
}

void sexpr_print(const sexpr_t *s)
{
    if (!s || s->kind == SEXPR_NIL) {
        printf("nil");
        return;
    }

    switch (s->kind) {
    case SEXPR_INTEGER:
        printf("%d", s->integer);
        break;

    case SEXPR_SYMBOL:
        printf("%s", s->symbol ? s->symbol : "nil");
        break;

    case SEXPR_PAIR: {
        printf("(");
        const sexpr_t *curr = s;
        while (curr && curr->kind == SEXPR_PAIR) {
            sexpr_print(curr->pair.head);

            const sexpr_t *next = curr->pair.tail;
            if (!next || next->kind == SEXPR_NIL) {
                break;
            } else if (next->kind == SEXPR_PAIR) {
                printf(" ");
                curr = next;
            } else {
                printf(" . ");
                sexpr_print(next);
                break;
            }
        }
        printf(")");
        break;
    }

    default:
        printf("?");
        break;
    }
}

int
main (int argc, char *argv[])
{
    FILE *in = stdin;
    char *src;
    size_t src_sz;

    sexpr_err_t e;
    sexpr_t *expr;

    if (argc > 1)
        in = fopen (argv[1], "rb");

    src = read_whole (in, &src_sz);
    if (!src)
    {
        fprintf (stderr, "fuck you\n");
        return 1;
    }

    expr = sexpr_deserialize (src, &e);

    free (src);

    if (!expr)
    {
        fprintf (stderr, "dupa: %d; %lu\n", e.code, e.offset);
    }
    else
    {
        sexpr_print (expr);
        printf ("\n");
        fflush (stdout);
        sexpr_release (expr);
    }

    if (argc > 1)
        fclose (in);
    return 0;
}
