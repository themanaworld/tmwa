#ifndef TMWA_LOGIN_TYPES_HPP
#define TMWA_LOGIN_TYPES_HPP
//    types.hpp - externally useful types from login
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

# include "fwd.hpp"

# include <cstdint>

# include "../generic/enum.hpp"

namespace e
{
enum class VERSION_2 : uint8_t
{
    /// client supports updatehost
    UPDATEHOST = 0x01,
    /// send servers in forward order
    SERVERORDER = 0x02,
};
ENUM_BITWISE_OPERATORS(VERSION_2)
}
using e::VERSION_2;

#endif // TMWA_LOGIN_TYPES_HPP
