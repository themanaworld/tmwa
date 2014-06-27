#include "config_parse.hpp"
//    config_parse.cpp - Framework for per-server config parsers.
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

#include <algorithm>

#include "../strings/xstring.hpp"
#include "../strings/zstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/line.hpp"

#include "version.hpp"

#include "../poison.hpp"


namespace tmwa
{
bool is_comment(XString line)
{
    return not line or line.startswith("//"_s);
}

template<class ZS>
inline
bool config_split_impl(ZS line, XString *key, ZS *value)
{
    // unconditionally fail if line contains control characters
    if (std::find_if(line.begin(), line.end(),
                [](unsigned char c) { return c < ' '; }
                ) != line.end())
        return false;
    // try to find a colon, fail if not found
    typename ZS::iterator colon = std::find(line.begin(), line.end(), ':');
    if (colon == line.end())
        return false;

    *key = line.xislice_h(colon).strip();
    // move past the colon and any spaces
    ++colon;
    *value = line.xislice_t(colon).lstrip();
    return true;
}

// eventually this should go away
bool config_split(ZString line, XString *key, ZString *value)
{
    return config_split_impl(line, key, value);
}
// and use this instead
bool config_split(XString line, XString *key, XString *value)
{
    return config_split_impl(line, key, value);
}

/// Master config parser. This handles 'import' and 'version-ge' etc.
/// Then it defers to the inferior parser for a line it does not understand.
bool load_config_file(ZString filename, ConfigItemParser slave)
{
    io::LineReader in(filename);
    if (!in.is_open())
    {
        PRINTF("Unable to open file: %s\n"_fmt, filename);
        return false;
    }
    io::Line line;
    bool rv = true;
    while (in.read_line(line))
    {
        if (is_comment(line.text))
            continue;
        XString key;
        ZString value;
        if (!config_split(line.text, &key, &value))
        {
            line.error("Bad config line"_s);
            rv = false;
            continue;
        }
        if (key == "import"_s)
        {
            rv &= load_config_file(value, slave);
            continue;
        }
        else if (key == "version-lt"_s)
        {
            Version vers;
            if (!extract(value, &vers))
            {
                rv = false;
                continue;
            }
            if (CURRENT_VERSION < vers)
                continue;
            break;
        }
        else if (key == "version-le"_s)
        {
            Version vers;
            if (!extract(value, &vers))
            {
                rv = false;
                continue;
            }
            if (CURRENT_VERSION <= vers)
                continue;
            break;
        }
        else if (key == "version-gt"_s)
        {
            Version vers;
            if (!extract(value, &vers))
            {
                rv = false;
                continue;
            }
            if (CURRENT_VERSION > vers)
                continue;
            break;
        }
        else if (key == "version-ge"_s)
        {
            Version vers;
            if (!extract(value, &vers))
            {
                rv = false;
                continue;
            }
            if (CURRENT_VERSION >= vers)
                continue;
            break;
        }
        else if (!slave(key, value))
        {
            line.error("Bad config key or value"_s);
            rv = false;
            continue;
        }
        // nothing to see here, move along
    }
    return rv;
}
} // namespace tmwa
