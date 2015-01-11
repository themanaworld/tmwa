#pragma once
//    script-call.t.hpp - EAthena script frontend, engine, and library.
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

#include "fwd.hpp"

#include "../strings/zstring.hpp"


namespace tmwa
{
namespace map
{
struct argrec_t
{
    ZString name;
    union _aru
    {
        int i;
        ZString s;

        _aru(int n) : i(n) {}
        _aru(ZString z) : s(z) {}
    } v;

    argrec_t(ZString n, int i) : name(n), v(i) {}
    argrec_t(ZString n, ZString z) : name(n), v(z) {}
};
} // namespace map
} // namespace tmwa
