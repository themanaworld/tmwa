#pragma once
//    version.hpp - Prevent mass rebuild when conf/version.hpp changes.
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

#include "fwd.hpp"

#include <cstdint>

#include "../strings/fwd.hpp"


namespace tmwa
{
// TODO make these bitwise enums
#define TMWA_FLAG_REGISTRATION 0x01

#define TMWA_SERVER_LOGIN      0x01
#define TMWA_SERVER_CHAR       0x02
#define TMWA_SERVER_INTER      0x04
#define TMWA_SERVER_MAP        0x08

// TODO now that I generate the protocol, split 'flags' out of the struct
struct Version
{
    uint8_t major;
    uint8_t minor; // flavor1
    uint8_t patch; // flavor2
    uint8_t devel; // flavor3

    uint8_t flags;
    uint8_t which;
    uint16_t vend;
    // can't add vendor name yet

    constexpr friend
    bool operator < (Version l, Version r)
    {
        return (l.major < r.major
                || (l.major == r.major
                    && (l.minor < r.minor
                        || (l.minor == r.minor
                            && (l.patch < r.patch
                                || (l.patch == r.patch
                                    && (l.devel < r.devel
                                        || (l.devel == r.devel
                                            && l.vend < r.vend))))))));
    }
    constexpr friend
    bool operator > (Version l, Version r)
    {
        return r < l;
    }
    constexpr friend
    bool operator <= (Version l, Version r)
    {
        return !(r < l);
    }
    constexpr friend
    bool operator >= (Version l, Version r)
    {
        return !(l < r);
    }
};
static_assert(sizeof(Version) == 8, "this is sent over the network, can't change");

extern Version CURRENT_VERSION;

extern Version CURRENT_LOGIN_SERVER_VERSION;
extern Version CURRENT_CHAR_SERVER_VERSION;
extern Version CURRENT_MAP_SERVER_VERSION;

extern LString CURRENT_VERSION_STRING;

bool extract(XString str, Version *vers);
} // namespace tmwa
