#include "mapflag.hpp"
//    mapflag.cpp - Implementation of map flag settings
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

// because bitfields, that's why

bool MapFlags::get(MapFlag mf) const
{
    return flags & static_cast<unsigned>(mf);
}

void MapFlags::set(MapFlag mf, bool val)
{
    if (val)
        flags |= static_cast<unsigned>(mf);
    else
        flags &=~ static_cast<unsigned>(mf);
}

template<>
bool extract<MapFlag, void, void>(XString str, MapFlag *mf)
{
    const struct
    {
        char str[32];
        MapFlag id;
    } flags[] =
    {
        //{"alias", MapFlag::ALIAS},
        //{"nomemo", MapFlag::NOMEMO},
        {"noteleport", MapFlag::NOTELEPORT},
        {"noreturn", MapFlag::NORETURN},
        {"monster_noteleport", MapFlag::MONSTER_NOTELEPORT},
        {"nosave", MapFlag::NOSAVE},
        //{"nobranch", MapFlag::NOBRANCH},
        {"nopenalty", MapFlag::NOPENALTY},
        {"pvp", MapFlag::PVP},
        {"pvp_noparty", MapFlag::PVP_NOPARTY},
        //{"pvp_noguild", MapFlag::PVP_NOGUILD},
        //{"pvp_nightmaredrop", MapFlag::PVP_NIGHTMAREDROP},
        {"pvp_nocalcrank", MapFlag::PVP_NOCALCRANK},
        //{"gvg", MapFlag::GVG},
        //{"gvg_noparty", MapFlag::GVG_NOPARTY},
        //{"nozenypenalty", MapFlag::NOZENYPENALTY},
        //{"notrade", MapFlag::NOTRADE},
        //{"noskill", MapFlag::NOSKILL},
        {"nowarp", MapFlag::NOWARP},
        {"nowarpto", MapFlag::NOWARPTO},
        {"nopvp", MapFlag::NOPVP},
        //{"noicewall", MapFlag::NOICEWALL},
        {"snow", MapFlag::SNOW},
        {"fog", MapFlag::FOG},
        {"sakura", MapFlag::SAKURA},
        {"leaves", MapFlag::LEAVES},
        {"rain", MapFlag::RAIN},
        {"no_player_drops", MapFlag::NO_PLAYER_DROPS},
        {"town", MapFlag::TOWN},
        {"outside", MapFlag::OUTSIDE},
        {"resave", MapFlag::RESAVE},
    };
    for (auto& pair : flags)
        if (str == pair.str)
        {
            *mf = pair.id;
            return true;
        }
    return false;
}

MapFlag map_flag_from_int(int shift)
{
    return static_cast<MapFlag>(1 << shift);
}
