#include "extract_enums.hpp"
//    extract_enums.cpp - Opt-in integer extraction support for enums.
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

#include "../poison.hpp"


namespace tmwa
{
bool impl_extract(XString str, DIR *d)
{
    unsigned di;
    if (extract(str, &di) && di < 8)
    {
        *d = static_cast<DIR>(di);
        return true;
    }
    const struct
    {
        LString str;
        DIR d;
    } dirs[] =
    {
        {"S"_s, DIR::S},
        {"SW"_s, DIR::SW},
        {"W"_s, DIR::W},
        {"NW"_s, DIR::NW},
        {"N"_s, DIR::N},
        {"NE"_s, DIR::NE},
        {"E"_s, DIR::E},
        {"SE"_s, DIR::SE},
    };
    for (auto& pair : dirs)
    {
        if (str == pair.str)
        {
            *d = pair.d;
            return true;
        }
    }
    return false;
}
} // namespace tmwa
