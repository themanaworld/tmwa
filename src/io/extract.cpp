#include "extract.hpp"
//    extract.cpp - a simple, hierarchical, tokenizer
//
//    Copyright © 2013-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include <algorithm>

#include "../strings/astring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/vstring.hpp"

#include "../poison.hpp"


// TODO also pass an io::LineSpan around.
namespace tmwa
{
bool extract(XString str, XString *rv)
{
    *rv = str;
    return true;
}

bool extract(XString str, RString *rv)
{
    *rv = str;
    return true;
}

bool extract(XString str, AString *rv)
{
    *rv = str;
    return true;
}

bool extract(XString str, std::chrono::nanoseconds *ns)
{
    std::chrono::nanoseconds::rep rep;
    if (extract(str, &rep))
    {
        *ns = std::chrono::nanoseconds(rep);
        return true;
    }
    if (str.endswith("ns"_s))
    {
        if (extract(str.xrslice_h("ns"_s.size()), &rep))
        {
            *ns = std::chrono::nanoseconds(rep);
            return true;
        }
        return false;
    }
    std::chrono::microseconds bigger;
    if (extract(str, &bigger))
    {
        *ns = bigger;
        return *ns == bigger;
    }
    return false;
}
bool extract(XString str, std::chrono::microseconds *us)
{
    std::chrono::microseconds::rep rep;
    if (extract(str, &rep))
    {
        *us = std::chrono::microseconds(rep);
        return true;
    }
    if (str.endswith("us"_s))
    {
        if (extract(str.xrslice_h("us"_s.size()), &rep))
        {
            *us = std::chrono::microseconds(rep);
            return true;
        }
        return false;
    }
    std::chrono::milliseconds bigger;
    if (extract(str, &bigger))
    {
        *us = bigger;
        return *us == bigger;
    }
    return false;
}
bool extract(XString str, std::chrono::milliseconds *ms)
{
    std::chrono::milliseconds::rep rep;
    if (extract(str, &rep))
    {
        *ms = std::chrono::milliseconds(rep);
        return true;
    }
    if (str.endswith("ms"_s))
    {
        if (extract(str.xrslice_h("ms"_s.size()), &rep))
        {
            *ms = std::chrono::milliseconds(rep);
            return true;
        }
        return false;
    }
    std::chrono::seconds bigger;
    if (extract(str, &bigger))
    {
        *ms = bigger;
        return *ms == bigger;
    }
    return false;
}
bool extract(XString str, std::chrono::seconds *s)
{
    std::chrono::seconds::rep rep;
    if (extract(str, &rep))
    {
        *s = std::chrono::seconds(rep);
        return true;
    }
    if (str.endswith("s"_s))
    {
        if (extract(str.xrslice_h("s"_s.size()), &rep))
        {
            *s = std::chrono::seconds(rep);
            return true;
        }
        return false;
    }
    std::chrono::minutes bigger;
    if (extract(str, &bigger))
    {
        *s = bigger;
        return *s == bigger;
    }
    return false;
}
bool extract(XString str, std::chrono::minutes *min)
{
    std::chrono::minutes::rep rep;
    if (extract(str, &rep))
    {
        *min = std::chrono::minutes(rep);
        return true;
    }
    if (str.endswith("min"_s))
    {
        if (extract(str.xrslice_h("min"_s.size()), &rep))
        {
            *min = std::chrono::minutes(rep);
            return true;
        }
        return false;
    }
    std::chrono::hours bigger;
    if (extract(str, &bigger))
    {
        *min = bigger;
        return *min == bigger;
    }
    return false;
}
bool extract(XString str, std::chrono::hours *h)
{
    std::chrono::hours::rep rep;
    if (extract(str, &rep))
    {
        *h = std::chrono::hours(rep);
        return true;
    }
    if (str.endswith("h"_s))
    {
        if (extract(str.xrslice_h("h"_s.size()), &rep))
        {
            *h = std::chrono::hours(rep);
            return true;
        }
        return false;
    }
    std::chrono::duration<int, std::ratio<60*60*24>> bigger;
    if (extract(str, &bigger))
    {
        *h = bigger;
        return *h == bigger;
    }
    return false;
}
bool extract(XString str, std::chrono::duration<int, std::ratio<60*60*24>> *d)
{
    std::chrono::duration<int, std::ratio<60*60*24>>::rep rep;
    if (extract(str, &rep))
    {
        *d = std::chrono::duration<int, std::ratio<60*60*24>>(rep);
        return true;
    }
    if (str.endswith("d"_s))
    {
        if (extract(str.xrslice_h("d"_s.size()), &rep))
        {
            *d = std::chrono::duration<int, std::ratio<60*60*24>>(rep);
            return true;
        }
        return false;
    }
    return false;
}
} // namespace tmwa
