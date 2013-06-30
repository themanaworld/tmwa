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

#include "../poison.hpp"

bool extract(XString str, XString *rv)
{
    *rv = str;
    return true;
}

bool extract(XString str, FString *rv)
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
    it->broken = 0;
    return extract(str,
            record<',', 11>(
                &it->id,
                &it->nameid,
                &it->amount,
                &it->equip,
                &it->identify,
                &it->refine,
                &it->attribute,
                &it->card[0],
                &it->card[1],
                &it->card[2],
                &it->card[3],
                &it->broken));
}
