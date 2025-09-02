#include <tmwa/shared.hpp>
//    shared/lib.cpp - Public library to ensure install is working.
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "src/conf/install.hpp"

#include "../strings/literal.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/dir.hpp"
#include "../io/read.hpp"
#include "../io/write.hpp"

#include "../poison.hpp"


namespace tmwa
{
    static
    void try_read(const io::DirFd& dirfd, LString dir_path, ZString filename)
    {
        io::ReadFile rf(dirfd, filename);
        if (!rf.is_open())
        {
            FPRINTF(stderr, "Could not open %s/%s\n"_fmt, dir_path, filename);
            abort();
        }
        AString line;
        if (!rf.getline(line))
        {
            FPRINTF(stderr, "Could not read from %s/%s\n"_fmt, dir_path, filename);
            abort();
        }
    }

    static
    void try_write(const io::DirFd& dirfd, LString dir_path, ZString filename)
    {
        io::WriteFile wf(dirfd, filename);
        if (!wf.is_open())
        {
            FPRINTF(stderr, "Could not open %s/%s\n"_fmt, dir_path, filename);
            abort();
        }
        wf.put_line("Hello, World!"_s);
        if (!wf.close())
        {
            FPRINTF(stderr, "Could not write to %s/%s\n"_fmt, dir_path, filename);
            abort();
        }
    }

    void check_paths()
    {
        ZString portable_root(strings::really_construct_from_a_pointer, getenv("TMWA_PORTABLE") ?: "", nullptr);
        bool portable = bool(portable_root);
        if (!portable)
            portable_root = "."_s;

        io::DirFd root(portable_root);

        LString etc_path = PACKAGESYSCONFDIR.xslice_t(portable);
        LString var_path = PACKAGELOCALSTATEDIR.xslice_t(portable);
        LString data_path = PACKAGEDATADIR.xslice_t(portable);

        io::DirFd etc(root, etc_path);
        io::DirFd var(root, var_path);
        io::DirFd share(root, data_path);

        try_read(etc, etc_path, "shared.conf"_s);
        try_read(share, data_path, "shared.data"_s);
        try_write(var, var_path, "shared.test"_s);

        // io::FD::open();
    }
} // namespace tmwa
