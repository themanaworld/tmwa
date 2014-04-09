#ifndef TMWA_MAP_CLIF_T_HPP
#define TMWA_MAP_CLIF_T_HPP
//    clif.t.hpp - Network interface to the client.
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

# include "../sanity.hpp"

# include <cstdint>

enum class BeingRemoveWhy : uint8_t
{
    // general disappearance
    GONE = 0,
    // only case handled specially in client
    DEAD = 1,
    QUIT = 2,
    WARPED = 3,
    // handled specially in clif_clearchar - sent as 0 over network
    DISGUISE = 9,

    // handled speciall in mob_warp - not actually sent over network
    NEGATIVE1 = 0xff,
};

#endif // TMWA_MAP_CLIF_T_HPP
