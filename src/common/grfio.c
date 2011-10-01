// Reads .gat files by name-mapping .wlk files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "utils.h"
#include "grfio.h"
#include "mmo.h"
#include "socket.h"

//----------------------------
//  file entry table struct
//----------------------------
typedef struct
{
    size_t declen;
    int16_t next; // next index into the filelist[] array, or -1
    char fn[128 - 4 - 2];       // file name
} FILELIST;

#define FILELIST_LIMIT  32768   // limit to number of filelists - if you increase this, change all shorts to int
#define FILELIST_ADDS   1024    // amount to increment when reallocing

static FILELIST *filelist = NULL;
/// Number of entries used
static uint16_t filelist_entrys = 0;
/// Number of FILELIST entries actually allocated
static uint16_t filelist_maxentry = 0;

/// First index of the given hash, into the filelist[] array
static int16_t filelist_hash[256] = {[0 ... 255] = -1};

/// Hash a filename
static uint8_t filehash (const char *fname)
{
    // Larger than the return type - upper bits are used in the process
    uint32_t hash = 0;
    while (*fname)
    {
        hash = (hash << 1) + (hash >> 7) * 9 + (unsigned char)*fname;
        fname++;
    }
    return hash;
}

/// Find the filelist entry for the given filename, or NULL if it is not
FILELIST *filelist_find (const char *fname)
{
    int16_t index = filelist_hash[filehash (fname)];
    while (index >= 0)
    {
        if (strcmp (filelist[index].fn, fname) == 0)
            return &filelist[index];
        index = filelist[index].next;
    }
    return NULL;
}

/// Copy a temporary entry into the hash map
static FILELIST *filelist_add (FILELIST * entry)
{
    if (filelist_entrys >= FILELIST_LIMIT)
    {
        fprintf (stderr, "filelist limit : filelist_add\n");
        exit (1);
    }

    if (filelist_entrys >= filelist_maxentry)
    {
        RECREATE(filelist, FILELIST, filelist_maxentry + FILELIST_ADDS);
        memset (filelist + filelist_maxentry, '\0',
                FILELIST_ADDS * sizeof (FILELIST));
        filelist_maxentry += FILELIST_ADDS;
    }

    uint16_t new_index = filelist_entrys++;
    uint8_t hash = filehash (entry->fn);
    entry->next = filelist_hash[hash];
    filelist_hash[hash] = new_index;

    filelist[new_index] = *entry;

    return &filelist[new_index];
}

static FILELIST *filelist_modify (FILELIST * entry)
{
    FILELIST *fentry = filelist_find (entry->fn);
    if (fentry)
    {
        entry->next = fentry->next;
        *fentry = *entry;
        return fentry;
    }
    return filelist_add (entry);
}

/// Change fname data/*.gat to lfname data/*.wlk
// TODO even if the file exists, don't keep reopening it every time one loads
void grfio_resnametable (const char *fname, char *lfname)
{
    char restable[] = "data/resnametable.txt";

    FILE *fp = fopen_ (restable, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "No resnametable, can't look for %s\n", fname);
        strcpy(lfname, fname);
        char* ext = lfname + strlen(lfname) - 4;
        if (!strcmp(ext, ".gat"))
            strcpy(ext, ".wlk");
        return;
    }

    char line[512];
    while (fgets (line, sizeof (line), fp))
    {
        char w1[256], w2[256];
        if (
            // line is of the form foo.gat#foo.wlk#
            (sscanf (line, "%[^#]#%[^#]#", w1, w2) == 2)
            // strip data/ from foo.gat before comparing
            && (!strcmp (w1, fname + 5)))
        {
            strcpy (lfname, "data/");
            strcpy (lfname + 5, w2);
            fclose_ (fp);
            return;
        }
    }
    fprintf(stderr, "Unable to find resource: %s\n", fname);
    fclose_ (fp);

    strcpy(lfname, fname);
    char* ext = lfname + strlen(lfname) - 4;
    if (!strcmp(ext, ".gat"))
        strcpy(ext, ".wlk");
    return;
}

/// Size of resource
size_t grfio_size (const char *fname)
{
    FILELIST *entry = filelist_find (fname);
    if (entry)
        return entry->declen;

    char lfname[256];
    FILELIST lentry;
    struct stat st;

    grfio_resnametable (fname, lfname);

    for (char *p = lfname; *p; p++)
        if (*p == '\\')
            *p = '/';

    if (stat (lfname, &st) == 0)
    {
        strncpy (lentry.fn, fname, sizeof (lentry.fn) - 1);
        lentry.declen = st.st_size;
        entry = filelist_modify (&lentry);
    }
    else
    {
        printf ("%s not found\n", fname);
        return 0;
    }
    return entry->declen;
}

void *grfio_reads (const char *fname, size_t *size)
{
    char lfname[256];
    grfio_resnametable (fname, lfname);

    for (char *p = &lfname[0]; *p != 0; p++)
        if (*p == '\\')
            *p = '/';       // * At the time of Unix

    FILE *in = fopen_ (lfname, "rb");
    if (!in)
    {
        fprintf (stderr, "%s not found\n", fname);
        return NULL;
    }
    FILELIST lentry;
    FILELIST *entry = filelist_find (fname);
    if (entry)
    {
        lentry.declen = entry->declen;
    }
    else
    {
        fseek (in, 0, SEEK_END);
        lentry.declen = ftell (in);
        fseek (in, 0, SEEK_SET);
        strncpy (lentry.fn, fname, sizeof (lentry.fn) - 1);
        entry = filelist_modify (&lentry);
    }
    uint8_t *buf2;
    CREATE (buf2, uint8_t, lentry.declen + 1024);
    if (fread (buf2, 1, lentry.declen, in) != lentry.declen)
        exit(1);
    fclose_ (in);
    in = NULL;

    if (size)
        *size = entry->declen;
    return buf2;
}
