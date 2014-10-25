#pragma once
//    human_time_diff.hpp - broken deltas
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

#include "fwd.hpp"

#include <algorithm>

#include "../strings/xstring.hpp"


namespace tmwa
{
struct HumanTimeDiff
{
    short year, month, day, hour, minute, second;

    explicit
    operator bool()
    {
        return year || month || day || hour || minute || second;
    }

    bool operator !()
    {
        return !bool(*this);
    }
};
bool extract(XString str, HumanTimeDiff *iv);
} // namespace tmwa
