#include <unistd.h>
#include <stdio.h>
#include "lock.hpp"
#include "socket.hpp"

/// Protected file writing
/// (Until the file is closed, it keeps the old file)

// Start writing a tmpfile
FILE *lock_fopen (const char *filename, int *info)
{
    char newfile[512];
    FILE *fp;
    int  no = getpid ();

    // Get a filename that doesn't already exist
    do
    {
        sprintf (newfile, "%s_%d.tmp", filename, no++);
    }
    while ((fp = fopen_ (newfile, "r")) && (fclose_ (fp), 1));
    *info = --no;
    return fopen_ (newfile, "w");
}

// Delete the old file and rename the new file
void lock_fclose (FILE * fp, const char *filename, int *info)
{
    char newfile[512];
    if (fp)
    {
        fclose_ (fp);
        sprintf (newfile, "%s_%d.tmp", filename, *info);
        rename (newfile, filename);
    }
}
