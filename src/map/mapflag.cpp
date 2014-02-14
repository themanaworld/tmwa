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
    switch (mf)
    {
    case MapFlag::NOTELEPORT:
        return noteleport;
    case MapFlag::NORETURN:
        return noreturn;
    case MapFlag::MONSTER_NOTELEPORT:
        return monster_noteleport;
    case MapFlag::NOSAVE:
        return nosave;
    case MapFlag::NOPENALTY:
        return nopenalty;
    case MapFlag::PVP:
        return pvp;
    case MapFlag::PVP_NOPARTY:
        return pvp_noparty;
    case MapFlag::PVP_NOCALCRANK:
        return pvp_nocalcrank;
    case MapFlag::NOWARP:
        return nowarp;
    case MapFlag::NOWARPTO:
        return nowarpto;
    case MapFlag::NOPVP:
        return nopvp;
    case MapFlag::SNOW:
        return snow;
    case MapFlag::FOG:
        return fog;
    case MapFlag::SAKURA:
        return sakura;
    case MapFlag::LEAVES:
        return leaves;
    case MapFlag::RAIN:
        return rain;
    case MapFlag::NO_PLAYER_DROPS:
        return no_player_drops;
    case MapFlag::TOWN:
        return town;
    default:
        return false;
    }
}

void MapFlags::set(MapFlag mf, bool val)
{
    switch (mf)
    {
    case MapFlag::NOTELEPORT:
        noteleport = val;
        break;
    case MapFlag::NORETURN:
        noreturn = val;
        break;
    case MapFlag::MONSTER_NOTELEPORT:
        monster_noteleport = val;
        break;
    case MapFlag::NOSAVE:
        nosave = val;
        break;
    case MapFlag::NOPENALTY:
        nopenalty = val;
        break;
    case MapFlag::PVP:
        pvp = val;
        break;
    case MapFlag::PVP_NOPARTY:
        pvp_noparty = val;
        break;
    case MapFlag::PVP_NOCALCRANK:
        pvp_nocalcrank = val;
        break;
    case MapFlag::NOWARP:
        nowarp = val;
        break;
    case MapFlag::NOWARPTO:
        nowarpto = val;
        break;
    case MapFlag::NOPVP:
        nopvp = val;
        break;
    case MapFlag::SNOW:
        snow = val;
        break;
    case MapFlag::FOG:
        fog = val;
        break;
    case MapFlag::SAKURA:
        sakura = val;
        break;
    case MapFlag::LEAVES:
        leaves = val;
        break;
    case MapFlag::RAIN:
        rain = val;
        break;
    case MapFlag::NO_PLAYER_DROPS:
        no_player_drops = val;
        break;
    case MapFlag::TOWN:
        town = val;
        break;
    default:
        break;
    }
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
    };
    for (auto& pair : flags)
        if (str == pair.str)
        {
            *mf = pair.id;
            return true;
        }
    return false;
}
