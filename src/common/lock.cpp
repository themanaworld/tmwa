#include "lock.hpp"

#include <unistd.h>

#include <cstdio>

#include "cxxstdio.hpp"
#include "socket.hpp"

#include "../poison.hpp"

/// number of backups to keep
static
const int backup_count = 10;

/// Protected file writing
/// (Until the file is closed, it keeps the old file)

// Start writing a tmpfile
FILE *lock_fopen(ZString filename, int *info)
{
    FILE *fp;
    int no = getpid();

    // Get a filename that doesn't already exist
    FString newfile;
    do
    {
        newfile = STRPRINTF("%s_%d.tmp", filename, no++);
        fp = fopen(newfile.c_str(), "wx");
    }
    while (!fp);
    *info = --no;
    return fp;
}

// Delete the old file and rename the new file
void lock_fclose(FILE *fp, ZString filename, int *info)
{
    if (fp)
    {
        fclose(fp);
        int n = backup_count;
        FString old_filename = STRPRINTF("%s.%d", filename, n);
        while (--n)
        {
            FString newer_filename = STRPRINTF("%s.%d", filename, n);
            rename(newer_filename.c_str(), old_filename.c_str());
            old_filename = std::move(newer_filename);
        }
        rename(filename.c_str(), old_filename.c_str());

        FString tmpfile = STRPRINTF("%s_%d.tmp", filename, *info);
        rename(tmpfile.c_str(), filename.c_str());
    }
}
