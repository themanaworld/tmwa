#include "utils.hpp"
//    utils.cpp - Useful stuff that hasn't been categorized.
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

#include <netinet/in.h>
#include <sys/time.h>

#include <algorithm>

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/write.hpp"

#include "extract.hpp"

#include "../poison.hpp"

//---------------------------------------------------
// E-mail check: return 0 (not correct) or 1 (valid).
//---------------------------------------------------
bool e_mail_check(XString email)
{
    // athena limits
    if (email.size() < 3 || email.size() > 39)
        return 0;

    // part of RFC limits (official reference of e-mail description)
    XString::iterator at = std::find(email.begin(), email.end(), '@');
    if (at == email.end())
        return 0;
    XString username = email.xislice_h(at);
    XString hostname = email.xislice_t(at + 1);
    if (!username || !hostname)
        return 0;
    if (hostname.contains('@'))
        return 0;
    if (hostname.front() == '.' || hostname.back() == '.')
        return 0;
    if (hostname.contains_seq(".."_s))
        return 0;
    if (email.contains_any(" ;"_s))
        return 0;
    return email.is_print();
}

//-------------------------------------------------
// Return numerical value of a switch configuration
// on/off, english, français, deutsch, español
//-------------------------------------------------
int config_switch(ZString str)
{
    if (str == "true"_s || str == "on"_s || str == "yes"_s
        || str == "oui"_s || str == "ja"_s
        || str == "si"_s)
        return 1;
    if (str == "false"_s || str == "off"_s || str == "no"_s
        || str == "non"_s || str == "nein"_s)
        return 0;

    int rv;
    if (extract(str, &rv))
        return rv;
    FPRINTF(stderr, "Fatal: bad option value %s"_fmt, str);
    abort();
}

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
    gettimeofday(&tv, NULL);
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
