#pragma once
//    timestamp-utils.hpp - Useful stuff that hasn't been categorized.
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

#include "../strings/vstring.hpp"


namespace tmwa
{
struct timestamp_seconds_buffer : VString<19> {};
struct timestamp_milliseconds_buffer : VString<23> {};
void stamp_time(timestamp_seconds_buffer&, const TimeT *t=nullptr);
void stamp_time(timestamp_milliseconds_buffer&);

void log_with_timestamp(io::WriteFile& out, XString line);

// TODO VString?
#define TIMESTAMP_DUMMY "YYYY-MM-DD HH:MM:SS"
static_assert(sizeof(TIMESTAMP_DUMMY) == sizeof(timestamp_seconds_buffer),
        "timestamp size");
#define WITH_TIMESTAMP(str) str TIMESTAMP_DUMMY
//  str:            prefix: YYYY-MM-DD HH:MM:SS
//  sizeof:        01234567890123456789012345678
//  str + sizeof:                               ^
//  -1:                     ^
// there's probably a better way to do this now
#define REPLACE_TIMESTAMP(str, t)                           \
    stamp_time(                                             \
            reinterpret_cast<timestamp_seconds_buffer *>(   \
                str + sizeof(str)                           \
            )[-1],                                          \
            &t                                              \
    )
} // namespace tmwa
