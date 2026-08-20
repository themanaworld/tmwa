#include "npc-parse.hpp"
//    npc-parse.cpp - NPC builders shared by the Lua constructors and @addwarp.
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

#include <list>

#include "../compat/nullpo.hpp"

#include "../strings/astring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/literal.hpp"

#include "../io/cxxstdio.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "npc.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
static
void npc_clearsrcfile(void)
{
    npc_srcs.clear();
}

void npc_addsrcfile(AString name)
{
    if (name == "clear"_s)
    {
        npc_clearsrcfile();
        return;
    }

    npc_srcs.push_back(name);
}

void npc_delsrcfile(XString name)
{
    if (name == "all"_s)
    {
        npc_clearsrcfile();
        return;
    }

    for (auto it = npc_srcs.begin(); it != npc_srcs.end(); ++it)
    {
        if (*it == name)
        {
            npc_srcs.erase(it);
            return;
        }
    }
}

void register_npc_name(dumb_ptr<npc_data> nd)
{
    earray<LString, NpcSubtype, NpcSubtype::COUNT> types //=
    {{
        "WARP"_s,
        "SHOP"_s,
        "SCRIPT"_s,
    }};
    if (!nd->name)
    {
        PRINTF("WARNING: npc with no name:\n%s @ %s,%d,%d\n"_fmt,
                types[nd->npc_subtype],
                nd->bl_m->name_, nd->bl_x, nd->bl_y);
        return;
    }
    if (dumb_ptr<npc_data> nd_old = npcs_by_name.get(nd->name))
    {
        if (nd->npc_subtype != NpcSubtype::WARP
                || nd_old->npc_subtype != NpcSubtype::WARP)
        {
            PRINTF("WARNING: replacing npc with name: %s\n"_fmt, nd->name);
            PRINTF("old: %s @ %s,%d,%d\n"_fmt,
                    types[nd_old->npc_subtype],
                    nd_old->bl_m->name_, nd_old->bl_x, nd_old->bl_y);
            PRINTF("new: %s @ %s,%d,%d\n"_fmt,
                    types[nd->npc_subtype],
                    nd->bl_m->name_, nd->bl_x, nd->bl_y);
        }
    }
    // TODO also check #s ?
    npcs_by_name.put(nd->name, nd);
}

dumb_ptr<npc_data_warp> npc_create_warp(MapName mapname, int x, int y,
        int xs_file, int ys_file, MapName to_map, int to_x, int to_y)
{
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return nullptr);

    // the old warp parser added 2 to the file numbers (span n covers n+2
    // cells; -1 thus keeps meaning "span 1")
    int xs = xs_file + 2, ys = ys_file + 2;

    dumb_ptr<npc_data_warp> nd;
    nd.new_();
    nd->bl_id = npc_get_new_npc_id();
    nd->n = map_addnpc(m, nd);

    nd->sex = SEX::UNSPECIFIED;
    nd->bl_prev = nd->bl_next = nullptr;
    nd->bl_m = m;
    nd->bl_x = x;
    nd->bl_y = y;
    nd->dir = DIR::S;
    nd->flag = 0;
    nd->sit = DamageType::STAND;
    nd->name = stringish<NpcName>(STRPRINTF("w%c%i"_fmt, 6, nd->bl_id));

    nd->npc_class = WARP_CLASS;
    nd->speed = 200_ms;
    nd->option = Opt0::ZERO;
    nd->opt1 = Opt1::ZERO;
    nd->opt2 = Opt2::ZERO;
    nd->opt3 = Opt3::ZERO;
    nd->warp.name = to_map;
    nd->warp.x = to_x;
    nd->warp.y = to_y;
    nd->warp.xs = xs;
    nd->warp.ys = ys;

    nd->deletion_pending = npc_data::NOT_DELETING;

    npc_warp++;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::WARP;
    map_addblock(nd);
    clif_spawnnpc(nd);
    register_npc_name(nd);

    return nd;
}

dumb_ptr<npc_data_shop> npc_create_shop(NpcName name, MapName mapname,
        int x, int y, DIR dir, Species npc_class,
        std::vector<npc_item_list> items)
{
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return nullptr);

    dumb_ptr<npc_data_shop> nd;
    nd.new_();
    nd->shop_items = std::move(items);

    nd->sex = SEX::UNSPECIFIED;
    nd->bl_prev = nd->bl_next = nullptr;
    nd->bl_m = m;
    nd->bl_x = x;
    nd->bl_y = y;
    nd->bl_id = npc_get_new_npc_id();
    nd->dir = dir;
    nd->flag = 0;
    nd->sit = DamageType::STAND;
    nd->name = name;
    nd->npc_class = npc_class;
    nd->speed = 200_ms;
    nd->option = Opt0::ZERO;
    nd->opt1 = Opt1::ZERO;
    nd->opt2 = Opt2::ZERO;
    nd->opt3 = Opt3::ZERO;

    nd->deletion_pending = npc_data::NOT_DELETING;

    npc_shop++;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::SHOP;
    nd->n = map_addnpc(m, nd);
    map_addblock(nd);
    clif_spawnnpc(nd);
    register_npc_name(nd);

    return nd;
}

int npc_create_monster(MapName mapname, int x, int y, int xs, int ys,
        MobName name, Species mob_class, int amount,
        interval_t delay1, interval_t delay2, NpcEvent event)
{
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return -1);

    int num = amount;
    if (num > 1 && battle_config.mob_count_rate != 100)
    {
        num = num * battle_config.mob_count_rate / 100;
        if (num < 1)
            num = 1;
    }

    for (int i = 0; i < num; i++)
    {
        dumb_ptr<mob_data> md;
        md.new_();

        md->bl_prev = nullptr;
        md->bl_next = nullptr;
        md->bl_m = m;
        md->bl_x = x;
        md->bl_y = y;
        const mob_db_& mob_info = get_mob_db(mob_class);
        if (name == ENGLISH_NAME)
            md->name = mob_info.name;
        else if (name == JAPANESE_NAME)
            md->name = mob_info.jname;
        else
            md->name = name;

        md->n = i;
        md->mob_class = mob_class;
        md->bl_id = npc_get_new_npc_id();
        md->spawn.m = m;
        md->spawn.x0 = x;
        md->spawn.y0 = y;
        md->spawn.xs = xs;
        md->spawn.ys = ys;
        md->spawn.delay1 = delay1;
        md->spawn.delay2 = delay2;

        really_memzero_this(&md->state);
        // md->timer = nullptr;
        md->target_id = BlockId();
        md->attacked_id = BlockId();

        md->lootitemv.clear();

        md->npc_event = event;

        md->bl_type = BL::MOB;
        map_addiddb(md);
        mob_spawn(md->bl_id);

        npc_mob++;
    }

    return num;
}

bool npc_set_mapflag(MapName mapname, MapFlag mf, MapName extra_map,
        int extra_x, int extra_y, int mask)
{
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return false);

    if (mf == MapFlag::NOPVP)
    {
        m->flag.set(MapFlag::NOPVP, 1);
        m->flag.set(MapFlag::PVP, 0);
        return true;
    }

    if (mf == MapFlag::NOSAVE)
    {
        if (!extra_map || map_mapname2mapid(extra_map).is_none())
            return false;
        m->save.map_ = extra_map;
        m->save.x = extra_x;
        m->save.y = extra_y;
    }
    else if (mf == MapFlag::RESAVE)
    {
        if (!extra_map || map_mapname2mapid(extra_map).is_none())
            return false;
        m->resave.map_ = extra_map;
        m->resave.x = extra_x;
        m->resave.y = extra_y;
    }
    else if (mf == MapFlag::MASK)
    {
        m->mask = mask;
    }
    m->flag.set(mf, true);

    return true;
}

dumb_ptr<npc_data_script> npc_create_script_npc(NpcName name,
        MapName mapname, bool placed, int x, int y, DIR dir,
        Species npc_class, int xs, int ys)
{
    dumb_ptr<npc_data_script> nd;

    if (!placed)
    {
        // floating NPC (the old "script npc -" form)
        nd.new_();
        nd->scr.event_needs_map = false;

        nd->name = name;

        nd->sex = SEX::UNSPECIFIED;
        nd->bl_prev = nd->bl_next = nullptr;
        nd->bl_m = borrow(undefined_gat);
        nd->bl_x = 0;
        nd->bl_y = 0;
        nd->bl_id = npc_get_new_npc_id();
        nd->dir = DIR::S;
        nd->flag = 0;
        nd->sit = DamageType::STAND;
        nd->npc_class = INVISIBLE_CLASS;
        nd->speed = 200_ms;
        nd->option = Opt0::ZERO;
        nd->opt1 = Opt1::ZERO;
        nd->opt2 = Opt2::ZERO;
        nd->opt3 = Opt3::ZERO;

        nd->deletion_pending = npc_data::NOT_DELETING;

        npc_script++;
        nd->bl_type = BL::NPC;
        nd->npc_subtype = NpcSubtype::SCRIPT;

        id_db.put(nd->bl_id, nd); // fix to get the oid in OnInit
        register_npc_name(nd);
    }
    else
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
                return nullptr);

        nd.new_();

        nd->scr.xs = xs;
        nd->scr.ys = ys;
        nd->scr.event_needs_map = true;

        nd->name = name;
        nd->sex = SEX::UNSPECIFIED;
        nd->bl_prev = nd->bl_next = nullptr;
        nd->bl_m = m;
        nd->bl_x = x;
        nd->bl_y = y;
        nd->bl_id = npc_get_new_npc_id();
        nd->dir = dir;
        nd->flag = 0;
        nd->sit = DamageType::STAND;
        nd->npc_class = npc_class;
        nd->speed = 200_ms;
        nd->option = Opt0::ZERO;
        nd->opt1 = Opt1::ZERO;
        nd->opt2 = Opt2::ZERO;
        nd->opt3 = Opt3::ZERO;

        nd->deletion_pending = npc_data::NOT_DELETING;

        npc_script++;
        nd->bl_type = BL::NPC;
        nd->npc_subtype = NpcSubtype::SCRIPT;

        nd->n = map_addnpc(m, nd);
        map_addblock(nd);

        clif_spawnnpc(nd);

        register_npc_name(nd);
    }

    // The OnTimer machine starts stopped with 0 ticks; the interval list is
    // seeded afterwards by lua_npc_timer_setup (npc.script).
    nd->scr.timer = interval_t::zero();
    nd->scr.next_event = nd->scr.timer_eventv.begin();

    return nd;
}
} // namespace map
} // namespace tmwa
