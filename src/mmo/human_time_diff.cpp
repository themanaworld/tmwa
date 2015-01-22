#include "human_time_diff.hpp"
//    human_time_diff.cpp - broken deltas
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

#include "../io/extract.hpp"

#include "../poison.hpp"


namespace tmwa
{
bool impl_extract(XString str, HumanTimeDiff *iv)
{
    // str is a sequence of [-+]?[0-9]+([ay]|m|[jd]|h|mn|s)
    // there are NO spaces here
    // parse by counting the number starts
    auto is_num = [](char c)
    { return c == '-' || c == '+' || ('0' <= c && c <= '9'); };
    if (!str || !is_num(str.front()))
        return false;
    *iv = HumanTimeDiff{};
    while (str)
    {
        auto it = std::find_if_not(str.begin(), str.end(), is_num);
        auto it2 = std::find_if(it, str.end(), is_num);
        XString number = str.xislice_h(it);
        XString suffix = str.xislice(it, it2);
        str = str.xislice_t(it2);

        short *ptr = nullptr;
        if (suffix == "y"_s || suffix == "a"_s)
            ptr = &iv->year;
        else if (suffix == "m"_s)
            ptr = &iv->month;
        else if (suffix == "j"_s || suffix == "d"_s)
            ptr = &iv->day;
        else if (suffix == "h"_s)
            ptr = &iv->hour;
        else if (suffix == "mn"_s)
            ptr = &iv->minute;
        else if (suffix == "s"_s)
            ptr = &iv->second;
        else
            return false;
        if (number.startswith('+') && !number.startswith("+-"_s))
            number = number.xslice_t(1);
        if (*ptr || !extract(number, ptr))
            return false;
    }
    return true;
}
} // namespace tmwa
