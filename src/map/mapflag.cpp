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
        ZString str;
        MapFlag id;
    } flags[] =
    {
        //{ZString("alias"), MapFlag::ALIAS},
        //{ZString("nomemo"), MapFlag::NOMEMO},
        {ZString("noteleport"), MapFlag::NOTELEPORT},
        {ZString("noreturn"), MapFlag::NORETURN},
        {ZString("monster_noteleport"), MapFlag::MONSTER_NOTELEPORT},
        {ZString("nosave"), MapFlag::NOSAVE},
        //{ZString("nobranch"), MapFlag::NOBRANCH},
        {ZString("nopenalty"), MapFlag::NOPENALTY},
        {ZString("pvp"), MapFlag::PVP},
        {ZString("pvp_noparty"), MapFlag::PVP_NOPARTY},
        //{ZString("pvp_noguild"), MapFlag::PVP_NOGUILD},
        //{ZString("pvp_nightmaredrop"), MapFlag::PVP_NIGHTMAREDROP},
        {ZString("pvp_nocalcrank"), MapFlag::PVP_NOCALCRANK},
        //{ZString("gvg"), MapFlag::GVG},
        //{ZString("gvg_noparty"), MapFlag::GVG_NOPARTY},
        //{ZString("nozenypenalty"), MapFlag::NOZENYPENALTY},
        //{ZString("notrade"), MapFlag::NOTRADE},
        //{ZString("noskill"), MapFlag::NOSKILL},
        {ZString("nowarp"), MapFlag::NOWARP},
        {ZString("nowarpto"), MapFlag::NOWARPTO},
        {ZString("nopvp"), MapFlag::NOPVP},
        //{ZString("noicewall"), MapFlag::NOICEWALL},
        {ZString("snow"), MapFlag::SNOW},
        {ZString("fog"), MapFlag::FOG},
        {ZString("sakura"), MapFlag::SAKURA},
        {ZString("leaves"), MapFlag::LEAVES},
        {ZString("rain"), MapFlag::RAIN},
        {ZString("no_player_drops"), MapFlag::NO_PLAYER_DROPS},
        {ZString("town"), MapFlag::TOWN},
        {ZString("outside"), MapFlag::OUTSIDE},
        {ZString("resave"), MapFlag::RESAVE},
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
