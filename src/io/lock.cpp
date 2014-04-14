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

#include <cerrno>
#include <cstdlib>

#include "../strings/zstring.hpp"

#include "cxxstdio.hpp"
#include "fd.hpp"

#include "../poison.hpp"


/// number of backups to keep
static
const int backup_count = 10;

/// Protected file writing
/// (Until the file is closed, it keeps the old file)

namespace io
{
    // Start writing a tmpfile
    static
    FD get_lock_open(ZString filename, int *info)
    {
        FD fd;
        int no = getpid();

        // Get a filename that doesn't already exist
        AString newfile;
        do
        {
            newfile = STRPRINTF("%s_%d.tmp"_fmt, filename, no++);
            fd = FD::open(newfile, O_WRONLY | O_CREAT | O_EXCL, 0666);
        }
        while (fd == FD() && errno == EEXIST);
        if (fd == FD())
            abort();
        *info = --no;
        return fd;
    }

    WriteLock::WriteLock(RString fn, bool linebuffered)
    : WriteFile(get_lock_open(fn, &tmp_suffix), linebuffered), filename(fn)
    {}
    WriteLock::~WriteLock()
    {
        if (!WriteFile::close())
        {
            // leave partial file
            FPRINTF(stderr, "Warning: failed to write replacement for %s\n"_fmt, filename);
            abort();
        }

        int n = backup_count;
        AString old_filename = STRPRINTF("%s.%d"_fmt, filename, n);
        while (--n)
        {
            AString newer_filename = STRPRINTF("%s.%d"_fmt, filename, n);
            rename(newer_filename.c_str(), old_filename.c_str());
            old_filename = std::move(newer_filename);
        }
        rename(filename.c_str(), old_filename.c_str());

        AString tmpfile = STRPRINTF("%s_%d.tmp"_fmt, filename, tmp_suffix);
        rename(tmpfile.c_str(), filename.c_str());
    }
} // namespace io
