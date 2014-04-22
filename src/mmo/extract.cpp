#include "extract.hpp"
//    extract.cpp - a simple, hierarchical, tokenizer
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

#include "../strings/astring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/vstring.hpp"

#include "mmo.hpp"

#include "../poison.hpp"

bool extract(XString str, XString *rv)
{
    *rv = str;
    return true;
}

bool extract(XString str, AString *rv)
{
    *rv = str;
    return true;
}

bool extract(XString str, struct global_reg *var)
{
    return extract(str,
            record<','>(&var->str, &var->value));
}

bool extract(XString str, struct item *it)
{
    XString ignored;
    return extract(str,
            record<',', 11>(
                &ignored,
                &it->nameid,
                &it->amount,
                &it->equip,
                &ignored,
                &ignored,
                &ignored,
                &ignored,
                &ignored,
                &ignored,
                &ignored,
                &ignored));
}

bool extract(XString str, MapName *m)
{
    XString::iterator it = std::find(str.begin(), str.end(), '.');
    str = str.xislice_h(it);
    VString<15> tmp;
    bool rv = extract(str, &tmp);
    *m = tmp;
    return rv;
}

bool extract(XString str, CharName *out)
{
    VString<23> tmp;
    if (extract(str, &tmp))
    {
        *out = CharName(tmp);
        return true;
    }
    return false;
}
