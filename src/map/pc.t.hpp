#ifndef TMWA_MAP_PC_T_HPP
#define TMWA_MAP_PC_T_HPP
//    pc.t.hpp - Player state changes.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2013 Freeyorp
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

# include "fwd.hpp"

# include <cstdint>

enum class PC_GAINEXP_REASON
{
    KILLING = 0,
    HEALING = 1,
    SCRIPT  = 2,
    SHARING = 3,

    UNKNOWN,
    COUNT,
};

enum class ADDITEM
{
    EXIST,
    NEW,
    OVERAMOUNT,

    // when used as error in nullpo_retr
    ZERO = 0,
};

enum class CalcStatus
{
    NOW,
    LATER,
};

#endif // TMWA_MAP_PC_T_HPP
