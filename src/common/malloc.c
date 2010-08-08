#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "malloc.h"

#define SHOW_MEMORY_ERROR(file, line, func, type, msg)                      \
    fprintf (stderr, "%s:%d: in func %s(): %s() error, %s\n",               \
             file, line, func, type, msg)

void *aMalloc_ (size_t size, const char *file, int line, const char *func)
{
    void *ret = malloc (size);

    /* printf ("%s:%d: in func %s(): malloc(%d)\n", file, line, func, size); */
    if (ret == NULL)
    {
        SHOW_MEMORY_ERROR (file, line, func, "malloc", "out of memory");
        exit (EXIT_FAILURE);
    }

    return ret;
}

void *aCalloc_ (size_t num, size_t size, const char *file, int line,
                const char *func)
{
    void *ret = calloc (num, size);

    /* printf ("%s:%d: in func %s(): calloc(%d, %d)\n", file, line, func, num, 
               size); */
    if (ret == NULL)
    {
        SHOW_MEMORY_ERROR (file, line, func, "calloc", "out of memory");
        exit (EXIT_FAILURE);
    }

    return ret;
}

void *aRealloc_ (void *p, size_t size, const char *file, int line,
                 const char *func)
{
    void *ret = realloc (p, size);

    /* printf ("%s:%d: in func %s(): realloc(%p, %d)\n", file, line, func, p,
               size); */
    if (ret == NULL)
    {
        SHOW_MEMORY_ERROR (file, line, func, "realloc", "out of memory");
        exit (EXIT_FAILURE);
    }

    return ret;
}

char *aStrdup_ (const char *p, const char *file, int line, const char *func)
{
    char *ret = strdup (p);

    /* printf ("%s:%d: in func %s(): strdup(%p)\n", file, line, func, p); */
    if (ret == NULL)
    {
        SHOW_MEMORY_ERROR (file, line, func, "strdup", "out of memory");
        exit (EXIT_FAILURE);
    }

    return ret;
}
