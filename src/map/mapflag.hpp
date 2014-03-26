#ifndef TMWA_MAP_MAPFLAG_HPP
#define TMWA_MAP_MAPFLAG_HPP
//    mapflag.hpp - booleans that apply to an entire map
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

# include "../sanity.hpp"

# include "../mmo/extract.hpp" // TODO remove this (requires specializing the *other* half)

# include "../strings/xstring.hpp"

// originally from script.cpp
// These are part of the script API, so they can't change ever,
// even though they are silly.
// Hm, I guess if I did them builtin instead of loading from const.txt ...
enum class MapFlag
{
    //ALIAS = 1 << 21,
    //NOMEMO = 1 << 0,
    NOTELEPORT = 1 << 1,
    NORETURN = 1 << 22,
    MONSTER_NOTELEPORT = 1 << 23,
    NOSAVE = 1 << 2,
    //NOBRANCH = 1 << 3,
    NOPENALTY = 1 << 4,
    PVP = 1 << 6,
    PVP_NOPARTY = 1 << 7,
    //PVP_NOGUILD = 1 << 8,
    //PVP_NIGHTMAREDROP = 1 << 24,
    PVP_NOCALCRANK = 1 << 25,
    //GVG = 1 << 9,
    //GVG_NOPARTY = 1 << 10,
    //NOZENYPENALTY = 1 << 5,
    //NOTRADE = 1 << 11,
    //NOSKILL = 1 << 12,
    NOWARP = 1 << 13,
    NOWARPTO = 1 << 26,
    NOPVP = 1 << 14,
    //NOICEWALL = 1 << 15,
    SNOW = 1 << 16,
    FOG = 1 << 17,
    SAKURA = 1 << 18,
    LEAVES = 1 << 19,
    RAIN = 1 << 20,
    NO_PLAYER_DROPS = 1 << 27,
    TOWN = 1 << 28,

    OUTSIDE = 1 << 29,
    RESAVE = 1 << 30,
    //UNUSED3 = 1 << 31,
};


class MapFlags
{
    uint32_t flags;
public:
    bool get(MapFlag) const;
    void set(MapFlag, bool);
};

template<>
bool extract<MapFlag, void, void>(XString str, MapFlag *mf);

MapFlag map_flag_from_int(int shift);

#endif // TMWA_MAP_MAPFLAG_HPP
