// Compile with
//  gcc -m32 -I src/char -I src/common charfrob.c -o charfrob  src/common/timer.o src/common/malloc.o src/common/socket.o src/common/lock.o src/common/db.o src/char/int_pet.o src/char/int_storage.o src/char/inter.o src/char/int_party.o src/char/int_guild.o

#include <stdio.h>
#include <stdlib.h>
#include "../common/mmo.h"
#include "../char/char.c"

// Well, this is not terribly elegant, but I don't have that much time.
#define MAX_MAP 1024
#define MAP_NAME_SIZE 32
int  maps_nr = 0;
struct
{
    char old[MAP_NAME_SIZE], new[MAP_NAME_SIZE];
} maps[MAX_MAP];

void transform_point (struct point *p)
{
    int  k;

    if (!p->map[0])
        return;

    for (k = 0; k < maps_nr; k++)
        if (!strcmp (p->map, maps[k].old))
        {
            strcpy (p->map, maps[k].new);
            return;
        }

    fprintf (stderr, "Warning: untranslated map `%s'\n", p->map);
}

void transform_char (struct mmo_charstatus *p)
{
    int  i;

    transform_point (&p->last_point);
    transform_point (&p->save_point);

    for (i = 0; i < 10; i++)
        transform_point (&p->memo_point[i]);
}

int mmo_char_convert ()
{
    char line[965536];
    int  ret;
    struct mmo_charstatus char_dat;
    FILE *ifp, *ofp;

    ifp = stdin;
    ofp = stdout;
    while (fgets (line, 65535, ifp))
    {
        memset (&char_dat, 0, sizeof (struct mmo_charstatus));
        ret = mmo_char_fromstr (line, &char_dat);
        if (ret)
        {
            transform_char (&char_dat);
            mmo_char_tostr (line, &char_dat);
            fprintf (ofp, "%s" RETCODE, line);
        }
    }
    fcloseall ();
    return 0;
}

#define PARSE_MODE_NEXTNUM 0
#define PARSE_MODE_RANGE 1

int init (int count, char **translates)
{
    int  i;
    char *suffix = ".gat";

    for (i = 0; i < count; i++)
    {
        char *src = translates[i];
        char *dest = strchr (src, ':');

        if (!dest)
        {
            fprintf (stderr, "Missing colon in: `%s'\n", src);
            return 1;
        }

        *dest++ = 0;

        if (strlen (src) + strlen (suffix) >= MAP_NAME_SIZE)
        {
            fprintf (stderr, "Map name prefix too long: `%s'\n", src);
            return 1;
        }

        if (strlen (dest) + strlen (suffix) >= MAP_NAME_SIZE)
        {
            fprintf (stderr, "Map name prefix too long: `%s'\n", dest);
            return 1;
        }

        strncpy (maps[maps_nr].old, src, MAP_NAME_SIZE);
        strcat (maps[maps_nr].old, suffix);
        strncpy (maps[maps_nr].new, dest, MAP_NAME_SIZE);
        strcat (maps[maps_nr].new, suffix);

        ++maps_nr;
    }

    return 0;
}

int main (int argc, char *argv[])
{
    if (argc < 2)
    {
        printf ("Usage: %s oldmap0:newmap0 oldmap1:newmap1 ...\n", argv[0]);
        printf ("e.g., %s new_1-1:001-2 new_2-1:001-1\n", argv[0]);
        puts ("The extension `.gat' is appended implicitly.");
        exit (0);
    }
    if (init (argc - 1, argv + 1))
        return 1;

    mmo_char_convert ();

    return 0;
}

/* satisfy linker */
void set_termfunc (void (*termfunc) (void))
{
};
