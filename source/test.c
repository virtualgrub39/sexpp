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

int
main (int argc, char *argv[])
{
    FILE *in = stdin;
    if (argc > 1)
        in = fopen (argv[1], "rb");

    size_t src_sz;
    char *src = read_whole (in, &src_sz);
    if (!src)
    {
        fprintf (stderr, "fuck you\n");
        return 1;
    }

    sexpr_err_t e;
    if (!sexpr_deserialize (src, &e))
    {
        fprintf (stderr, "dupa: %d; %lu\n", e.code, e.offset);
    }

    free (src);

    if (argc > 1)
        fclose (in);
    return 0;
}
