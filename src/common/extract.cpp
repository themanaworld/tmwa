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

bool extract(const_string str, const_string *rv)
{
    *rv = str;
    return true;
}

bool extract(const_string str, std::string *rv)
{
    *rv = std::string(str.begin(), str.end());
    return true;
}

bool extract(const_string str, struct global_reg *var)
{
    return extract(str,
            record<','>(&var->str, &var->value));
}

bool extract(const_string str, struct item *it)
{
    return extract(str,
            record<','>(
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
                &it->broken))
        || extract(str,
            record<','>(
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
                &it->card[3]));
}
