#include "lock.hpp"
//    io/lock.hpp - Output to files with atomic replacement and backups.
//
//    Copyright © 2013 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <fcntl.h>
#include <unistd.h>

#include "../strings/zstring.hpp"

// TODO remove this violation of include order
#include "../common/cxxstdio.hpp"

#include "../poison.hpp"


/// number of backups to keep
static
const int backup_count = 10;

/// Protected file writing
/// (Until the file is closed, it keeps the old file)

// Start writing a tmpfile
static
int get_lock_open(ZString filename, int *info)
{
    int fd;
    int no = getpid();

    // Get a filename that doesn't already exist
    FString newfile;
    do
    {
        newfile = STRPRINTF("%s_%d.tmp", filename, no++);
        fd = open(newfile.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    }
    while (fd == -1 && errno == EEXIST);
    if (fd == -1)
        abort();
    *info = --no;
    return fd;
}

namespace io
{
    WriteLock::WriteLock(FString fn, bool linebuffered)
    : WriteFile(get_lock_open(fn, &tmp_suffix), linebuffered), filename(fn)
    {}
    WriteLock::~WriteLock()
    {
        if (!WriteFile::close())
        {
            // leave partial file
            FPRINTF(stderr, "Warning: failed to write replacement for %s\n", filename);
            abort();
        }

        int n = backup_count;
        FString old_filename = STRPRINTF("%s.%d", filename, n);
        while (--n)
        {
            FString newer_filename = STRPRINTF("%s.%d", filename, n);
            rename(newer_filename.c_str(), old_filename.c_str());
            old_filename = std::move(newer_filename);
        }
        rename(filename.c_str(), old_filename.c_str());

        FString tmpfile = STRPRINTF("%s_%d.tmp", filename, tmp_suffix);
        rename(tmpfile.c_str(), filename.c_str());
    }
} // namespace io
