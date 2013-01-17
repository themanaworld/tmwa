#include "lock.hpp"

#include <unistd.h>

#include <cstdio>

#include "cxxstdio.hpp"
#include "socket.hpp"

#include "../poison.hpp"

/// Protected file writing
/// (Until the file is closed, it keeps the old file)

// Start writing a tmpfile
FILE *lock_fopen(const char *filename, int *info)
{
    FILE *fp;
    int no = getpid();

    // Get a filename that doesn't already exist
    std::string newfile;
    do
    {
        newfile = STRPRINTF("%s_%d.tmp", filename, no++);
        fp = fopen_(newfile.c_str(), "wx");
    }
    while (!fp);
    *info = --no;
    return fp;
}

// Delete the old file and rename the new file
void lock_fclose(FILE *fp, const char *filename, int *info)
{
    if (fp)
    {
        fclose_(fp);
        std::string newfile = STRPRINTF("%s_%d.tmp", filename, *info);
        rename(newfile.c_str(), filename);
    }
}
