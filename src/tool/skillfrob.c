// Compile with
//  gcc -m32 -Wall -Wno-pointer-sign -fno-strict-aliasing -I src/char -I src/common src/tool/skillfrob.c -o skillfrob  src/common/timer.o src/common/malloc.o src/common/socket.o src/common/lock.o src/common/db.o src/char/int_storage.o src/char/inter.o src/char/int_party.o src/char/int_guild.o

#include <stdio.h>
#include <stdlib.h>
#include "../common/mmo.h"
#include "../char/char.c"

unsigned char skills[MAX_SKILL];

void transform_char (struct mmo_charstatus *p)
{
    int  i;

    for (i = 0; i < MAX_SKILL; i++)
    {
        if (skills[(*p).skill[i].id])
        {
            (*p).skill[i].lv = 0;
            (*p).skill[i].flags = 0;
        }
    }
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

int init (int count, char **translates)
{
    int  i, skill;

    memset (skills, 0, sizeof (skills));

    for (i = 0; i < count; i++)
    {
        skill = atoi (translates[i]);
        if (skill > 0)
        {
            skills[skill] = 1;
        }
    }

    return 0;
}

int main (int argc, char *argv[])
{
    if (argc < 2)
    {
        printf ("Usage: %s skillid1 skillid2 ...\n", argv[0]);
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
