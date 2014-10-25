#include "timestamp-utils.hpp"
//    timestamp-utils.cpp - Useful stuff that hasn't been categorized.
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

#include <sys/time.h>

#include <algorithm>

#include "../strings/xstring.hpp"

#include "../compat/time_t.hpp"

#include "../io/write.hpp"

#include "../poison.hpp"


namespace tmwa
{
static_assert(sizeof(timestamp_seconds_buffer) == 20, "seconds buffer");
static_assert(sizeof(timestamp_milliseconds_buffer) == 24, "millis buffer");

void stamp_time(timestamp_seconds_buffer& out, const TimeT *t)
{
    struct tm when = t ? *t : TimeT::now();
    char buf[20];
    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &when);
    out = stringish<timestamp_seconds_buffer>(VString<19>(strings::really_construct_from_a_pointer, buf));
}
void stamp_time(timestamp_milliseconds_buffer& out)
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm when = TimeT(tv.tv_sec);
    char buf[24];
    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &when);
    sprintf(buf + 19, ".%03d", static_cast<int>(tv.tv_usec / 1000));
    out = stringish<timestamp_milliseconds_buffer>(VString<23>(strings::really_construct_from_a_pointer, buf));
}

void log_with_timestamp(io::WriteFile& out, XString line)
{
    if (!line)
    {
        out.put('\n');
        return;
    }
    timestamp_milliseconds_buffer tmpstr;
    stamp_time(tmpstr);
    out.really_put(tmpstr.data(), tmpstr.size());
    out.really_put(": ", 2);
    out.put_line(line);
}
} // namespace tmwa
