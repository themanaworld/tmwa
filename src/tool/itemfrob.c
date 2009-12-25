// Compile with
//  gcc -m32 -I src/char -I src/common charfrob.c -o charfrob  src/common/timer.o src/common/malloc.o src/common/socket.o src/common/lock.o src/common/db.o src/char/int_pet.o src/char/int_storage.o src/char/inter.o src/char/int_party.o src/char/int_guild.o

#include <stdio.h>
#include <stdlib.h>
#include "../common/mmo.h"
#include "../char/char.c"

// Well, this is not terribly elegant, but I don't have that much time.
#define MAX_ITEM_ID 65535
int  inv_translate[MAX_ITEM_ID];

void transform_char (struct mmo_charstatus *p)
{
    int  k;
    for (k = 0; k < MAX_INVENTORY; k++)
        p->inventory[k].nameid = inv_translate[p->inventory[k].nameid];
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

int init (char *source, char *dest)
{
    int  i;
    char *end_of_num;
    int  dest_nr = strtol (dest, &end_of_num, 0);
    int  range_start;
    int  mode = PARSE_MODE_NEXTNUM;

    if (*end_of_num)
    {
        fprintf (stderr, "Invalid inventory ID: `%s'\n", dest);
        return 1;
    }

    /* init */
    for (i = 0; i < MAX_ITEM_ID; i++)
        inv_translate[i] = i;

    while (*source)
    {
        int  nr = strtol (source, &end_of_num, 0);
        char sep;

        if (end_of_num == source)
        {
            fprintf (stderr, "Invalid source range description: `%s'\n",
                     source);
            return 1;
        }

        switch (mode)
        {
            case PARSE_MODE_NEXTNUM:
                inv_translate[nr] = dest_nr;
                break;
            case PARSE_MODE_RANGE:
                for (i = range_start; i <= nr; i++)
                    inv_translate[i] = dest_nr;
                break;
            default:
                fprintf (stderr, "Internal error at %d\n", __LINE__);
                return 1;
        };

        sep = *end_of_num++;

        switch (sep)
        {
            case '-':
                range_start = nr;
                mode = PARSE_MODE_RANGE;
                break;
            case ',':
                mode = PARSE_MODE_NEXTNUM;
                break;
            case 0:
                return 0;
            default:
                fprintf (stderr, "Invalid token in range spec: `%c'\n", sep);
                return 1;
        }

        source = end_of_num;
    }
    return 0;
}

int main (int argc, char *argv[])
{
    if (argc < 3)
    {
        printf
            ("Usage: %s <inventory ID input range> <inventory ID output>\n",
             argv[0]);
        printf ("e.g., %s 501-555 701\n", argv[0]);
        exit (0);
    }
    if (init (argv[1], argv[2]))
        return 1;

    mmo_char_convert ();

    return 0;
}

/* satisfy linker */
void set_termfunc (void (*termfunc) (void))
{
};
