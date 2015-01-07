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

#include "../compat/borrow.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
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
static
bool do_split(io::Spanned<ZS> line, io::Spanned<XString> *key, io::Spanned<ZS> *value)
{
    typename ZS::iterator colon = std::find(line.data.begin(), line.data.end(), ':');
    if (colon == line.data.end())
        return false;
    key->data = line.data.xislice_h(colon);
    key->span = line.span;
    key->span.end.column = key->span.begin.column + key->data.size() - 1;
    ++colon;
    value->data = line.data.xislice_t(colon);
    value->span = line.span;
    value->span.begin.column = value->span.end.column - value->data.size() + 1;
    return true;
}

template<class ZS>
static
io::Spanned<ZS> do_lstrip(io::Spanned<ZS> value)
{
    io::Spanned<ZS> rv;
    rv.data = value.data.lstrip();
    rv.span = value.span;
    rv.span.begin.column += (value.data.size() - rv.data.size());
    return rv;
}

static
io::Spanned<XString> do_rstrip(io::Spanned<XString> value)
{
    io::Spanned<XString> rv;
    rv.data = value.data.rstrip();
    rv.span = value.span;
    rv.span.end.column -= (value.data.size() - rv.data.size());
    return rv;
}

static
io::Spanned<XString> do_strip(io::Spanned<XString> value)
{
    return do_lstrip(do_rstrip(value));
}

template<class ZS>
inline
bool config_split_impl(io::Spanned<ZS> line, io::Spanned<XString> *key, io::Spanned<ZS> *value)
{
    // unconditionally fail if line contains control characters
    if (std::find_if(line.data.begin(), line.data.end(),
                [](unsigned char c) { return c < ' '; }
                ) != line.data.end())
        return false;

    if (!do_split(line, key, value))
        return false;
    *key = do_strip(*key);
    *value = do_lstrip(*value);
    return true;
}

// eventually this should go away
// currently the only real offenders are io::FD::open and *PRINTF
bool config_split(io::Spanned<ZString> line, io::Spanned<XString> *key, io::Spanned<ZString> *value)
{
    return config_split_impl(line, key, value);
}

static
bool check_version(io::Spanned<XString> avers, Borrowed<bool> valid)
{
    enum
    {
        GE, LE, GT, LT
    } cmp;

    if (avers.data.startswith(">="_s))
    {
        cmp = GE;
        avers.data = avers.data.xslice_t(2);
        avers.span.begin.column += 2;
    }
    else if (avers.data.startswith('>'))
    {
        cmp = GT;
        avers.data = avers.data.xslice_t(1);
        avers.span.begin.column += 1;
    }
    else if (avers.data.startswith("<="_s))
    {
        cmp = LE;
        avers.data = avers.data.xslice_t(2);
        avers.span.begin.column += 2;
    }
    else if (avers.data.startswith('<'))
    {
        cmp = LT;
        avers.data = avers.data.xslice_t(1);
        avers.span.begin.column += 1;
    }
    else
    {
        avers.span.error("Version check must begin with one of: '>=', '>', '<=', '<'"_s);
        *valid = false;
        return false;
    }

    Version vers;
    if (!extract(avers.data, &vers))
    {
        avers.span.error("Bad value"_s);
        *valid = false;
        return false;
    }
    switch (cmp)
    {
    case GE:
        return CURRENT_VERSION >= vers;
    case GT:
        return CURRENT_VERSION > vers;
    case LE:
        return CURRENT_VERSION <= vers;
    case LT:
        return CURRENT_VERSION < vers;
    }
    abort();
}

/// Master config parser. This handles 'import' and 'version-ge' etc.
/// Then it defers to the inferior parser for a line it does not understand.
///
/// Note: old-style 'version-ge: 1.2.3' etc apply to the rest of the file, but
/// new-style 'version: >= 1.2.3' apply only up to the next 'version:'
bool load_config_file(ZString filename, ConfigItemParser slave)
{
    io::LineReader in(filename);
    if (!in.is_open())
    {
        PRINTF("Unable to open file: %s\n"_fmt, filename);
        return false;
    }
    io::Line line_;
    bool rv = true;
    bool good_version = true;
    while (in.read_line(line_))
    {
        if (is_comment(line_.text))
            continue;
        auto line = io::respan(line_.to_span(), ZString(line_.text));
        io::Spanned<XString> key;
        io::Spanned<ZString> value;
        if (!config_split(line, &key, &value))
        {
            line.span.error("Bad config line"_s);
            rv = false;
            continue;
        }
        if (key.data == "version"_s)
        {
            if (value.data == "all"_s)
            {
                good_version = true;
            }
            else
            {
                good_version = true;
                while (good_version && value.data)
                {
                    ZString::iterator it = std::find(value.data.begin(), value.data.end(), ' ');
                    io::Spanned<XString> value_head;
                    value_head.data = value.data.xislice_h(it);
                    value_head.span = value.span;
                    value.data = value.data.xislice_t(it).lstrip();

                    value_head.span.end.column = value_head.span.begin.column + value_head.data.size() - 1;
                    value.span.begin.column = value.span.end.column - value.data.size() + 1;

                    good_version &= check_version(value_head, borrow(rv));
                }
            }
            continue;
        }
        if (!good_version)
        {
            continue;
        }
        if (key.data == "import"_s)
        {
            if (!load_config_file(value.data, slave))
            {
                value.span.error("Failed to include file"_s);
                rv = false;
            }
            continue;
        }
        else if (key.data == "version-lt"_s)
        {
            Version vers;
            if (!extract(value.data, &vers))
            {
                value.span.error("Bad value"_s);
                rv = false;
                continue;
            }
            if (CURRENT_VERSION < vers)
                continue;
            break;
        }
        else if (key.data == "version-le"_s)
        {
            Version vers;
            if (!extract(value.data, &vers))
            {
                rv = false;
                value.span.error("Bad value"_s);
                continue;
            }
            if (CURRENT_VERSION <= vers)
                continue;
            break;
        }
        else if (key.data == "version-gt"_s)
        {
            Version vers;
            if (!extract(value.data, &vers))
            {
                rv = false;
                value.span.error("Bad value"_s);
                continue;
            }
            if (CURRENT_VERSION > vers)
                continue;
            break;
        }
        else if (key.data == "version-ge"_s)
        {
            Version vers;
            if (!extract(value.data, &vers))
            {
                rv = false;
                value.span.error("Bad value"_s);
                continue;
            }
            if (CURRENT_VERSION >= vers)
                continue;
            break;
        }
        else
        {
            rv &= slave(key, value);
        }
        // nothing to see here, move along
    }
    return rv;
}
} // namespace tmwa
