#include "grfio.hpp"
//    grfio.cpp - Don't read GRF files, just name-map .wlk files.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include <cassert>

#include <map>

#include "../strings/mstring.hpp"
#include "../strings/rstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/read.hpp"

#include "../high/extract_mmo.hpp"
#include "../high/mmo.hpp"

#include "../poison.hpp"


namespace tmwa
{
static
std::map<MapName, RString> resnametable;

bool load_resnametable(ZString filename)
{
    io::ReadFile in(filename);
    if (!in.is_open())
    {
        FPRINTF(stderr, "Missing %s\n"_fmt, filename);
        return false;
    }

    bool rv = true;
    AString line;
    while (in.getline(line))
    {
        MapName key;
        AString value;
        if (!extract(line,
                    record<'#'>(&key, &value)))
        {
            PRINTF("Bad resnametable line: %s\n"_fmt, line);
            rv = false;
            continue;
        }
        // TODO add "data/" here ...
        resnametable[key] = value;
    }
    return rv;
}

/// Change *.gat to *.wlk
static
RString grfio_resnametable(MapName rname)
{
    // TODO return an error instead of throwing an exception
    return resnametable.at(rname);
}

std::vector<uint8_t> grfio_reads(MapName rname)
{
    MString lfname_;
    // TODO ... instead of here
    lfname_ += "data/"_s;
    lfname_ += grfio_resnametable(rname);
    AString lfname = AString(lfname_);

    // TODO wrap this immediately
    int fd = open(lfname.c_str(), O_RDONLY);
    if (fd == -1)
    {
        FPRINTF(stderr, "Resource %s (file %s) not found\n"_fmt,
                rname, lfname);
        return {};
    }
    off_t len = lseek(fd, 0, SEEK_END);
    assert (len != -1);
    std::vector<uint8_t> buffer(len);
    ssize_t err = pread(fd, buffer.data(), len, 0);
    assert (err == len);
    close(fd);
    return buffer;
}
} // namespace tmwa
