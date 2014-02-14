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

# include "../common/extract.hpp" // TODO remove this (requires specializing the *other* half)

# include "../strings/xstring.hpp"

// originally from script.cpp
// These are part of the script API, so they can't change ever,
// even though they are silly.
// Hm, I guess if I did them builtin instead of loading from const.txt ...
enum class MapFlag
{
    //ALIAS = 21,
    //NOMEMO = 0,
    NOTELEPORT = 1,
    NORETURN = 22,
    MONSTER_NOTELEPORT = 23,
    NOSAVE = 2,
    //NOBRANCH = 3,
    NOPENALTY = 4,
    PVP = 6,
    PVP_NOPARTY = 7,
    //PVP_NOGUILD = 8,
    //PVP_NIGHTMAREDROP = 24,
    PVP_NOCALCRANK = 25,
    //GVG = 9,
    //GVG_NOPARTY = 10,
    //NOZENYPENALTY = 5,
    //NOTRADE = 11,
    //NOSKILL = 12,
    NOWARP = 13,
    NOWARPTO = 26,
    NOPVP = 14,
    //NOICEWALL = 15,
    SNOW = 16,
    FOG = 17,
    SAKURA = 18,
    LEAVES = 19,
    RAIN = 20,
    NO_PLAYER_DROPS = 27,
    TOWN = 28,

    COUNT = 29,
};


class MapFlags
{
public:
    unsigned noteleport:1;
    unsigned noreturn:1;
    unsigned monster_noteleport:1;
    unsigned nosave:1;
    unsigned nopenalty:1;
    unsigned pvp:1;
    unsigned pvp_noparty:1;
    unsigned pvp_nocalcrank:1;
    unsigned nowarp:1;
    unsigned nowarpto:1;
    unsigned nopvp:1;       // [Valaris]
    unsigned snow:1;        // [Valaris]
    unsigned fog:1;         // [Valaris]
    unsigned sakura:1;      // [Valaris]
    unsigned leaves:1;      // [Valaris]
    unsigned rain:1;        // [Valaris]
    unsigned no_player_drops:1; // [Jaxad0127]
    unsigned town:1;        // [remoitnane]
public:
    bool get(MapFlag) const;
    void set(MapFlag, bool);
};

template<>
bool extract<MapFlag, void, void>(XString str, MapFlag *mf);

#endif // TMWA_MAP_MAPFLAG_HPP

