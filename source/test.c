#include <stdio.h>
#include <stdlib.h>

#include <sexpp.h>

char *read_whole (FILE *in, size_t *out_sz)
{
    size_t cap = 128;
    size_t sz = 0;
    char *buf;

    if (!in) return NULL;

    buf = malloc (cap * sizeof *buf);

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
    char *str = sexpr_serialize((sexpr_t *)s, NULL);
    if (str) {
        fputs(str, stdout);
        free(str);
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
