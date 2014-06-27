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


namespace tmwa
{
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
        LString str;
        MapFlag id;
    } flags[] =
    {
        //{"alias"_s, MapFlag::ALIAS},
        //{"nomemo"_s, MapFlag::NOMEMO},
        {"noteleport"_s, MapFlag::NOTELEPORT},
        {"noreturn"_s, MapFlag::NORETURN},
        {"monster_noteleport"_s, MapFlag::MONSTER_NOTELEPORT},
        {"nosave"_s, MapFlag::NOSAVE},
        //{"nobranch"_s, MapFlag::NOBRANCH},
        {"nopenalty"_s, MapFlag::NOPENALTY},
        {"pvp"_s, MapFlag::PVP},
        {"pvp_noparty"_s, MapFlag::PVP_NOPARTY},
        //{"pvp_noguild"_s, MapFlag::PVP_NOGUILD},
        //{"pvp_nightmaredrop"_s, MapFlag::PVP_NIGHTMAREDROP},
        {"pvp_nocalcrank"_s, MapFlag::PVP_NOCALCRANK},
        //{"gvg"_s, MapFlag::GVG},
        //{"gvg_noparty"_s, MapFlag::GVG_NOPARTY},
        //{"nozenypenalty"_s, MapFlag::NOZENYPENALTY},
        //{"notrade"_s, MapFlag::NOTRADE},
        //{"noskill"_s, MapFlag::NOSKILL},
        {"nowarp"_s, MapFlag::NOWARP},
        {"nowarpto"_s, MapFlag::NOWARPTO},
        {"nopvp"_s, MapFlag::NOPVP},
        //{"noicewall"_s, MapFlag::NOICEWALL},
        {"snow"_s, MapFlag::SNOW},
        {"fog"_s, MapFlag::FOG},
        {"sakura"_s, MapFlag::SAKURA},
        {"leaves"_s, MapFlag::LEAVES},
        {"rain"_s, MapFlag::RAIN},
        {"no_player_drops"_s, MapFlag::NO_PLAYER_DROPS},
        {"town"_s, MapFlag::TOWN},
        {"outside"_s, MapFlag::OUTSIDE},
        {"resave"_s, MapFlag::RESAVE},
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
} // namespace tmwa
