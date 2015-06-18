#include "npc-parse.hpp"
//    npc-parse.cpp - Noncombatants.
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

#include "../generic/enum.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/line.hpp"

#include "../mmo/config_parse.hpp"

#include "../high/extract_mmo.hpp"

#include "../ast/npc.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "npc-internal.hpp"
#include "script-parse.hpp"

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
        "MESSAGE"_s,
    }};
    if (!nd->name)
    {
        if (nd->npc_subtype == NpcSubtype::MESSAGE)
            return;
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

// extern for atcommand @addwarp
bool npc_load_warp(ast::npc::Warp& warp)
{
    MapName mapname = warp.m.data;
    int x = warp.x.data, y = warp.y.data;

    int xs = warp.xs.data, ys = warp.ys.data;
    MapName to_mapname = warp.to_m.data;
    int to_x = warp.to_x.data, to_y = warp.to_y.data;

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), abort());

    dumb_ptr<npc_data_warp> nd;
    nd.new_();
    nd->bl_id = npc_get_new_npc_id();
    nd->n = map_addnpc(m, nd);

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
    nd->warp.name = to_mapname;
    nd->warp.x = to_x;
    nd->warp.y = to_y;
    nd->warp.xs = xs;
    nd->warp.ys = ys;

    for (int i = 0; i < ys; i++)
    {
        for (int j = 0; j < xs; j++)
        {
            int x_lo = x - xs / 2;
            int y_lo = y - ys / 2;
            int xc = x_lo + j;
            int yc = y_lo + i;
            MapCell t = map_getcell(m, xc, yc);
            if (bool(t & MapCell::UNWALKABLE))
                continue;
            map_setcell(m, xc, yc, t | MapCell::NPC_NEAR);
        }
    }

    npc_warp++;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::WARP;
    map_addblock(nd);
    clif_spawnnpc(nd);
    register_npc_name(nd);

    return true;
}

static
bool npc_load_shop(ast::npc::Shop& shop)
{
    MapName mapname = shop.m.data;
    int x = shop.x.data, y = shop.y.data;
    DIR dir = shop.d.data;
    dumb_ptr<npc_data_shop> nd;
    Species npc_class = shop.npc_class.data;

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), abort());

    nd.new_();
    nd->shop_items.reserve(shop.items.data.size());
    for (auto& it : shop.items.data)
    {
        nd->shop_items.emplace_back();
        auto& back = nd->shop_items.back();
        P<item_data> id = ((extract(it.data.name.data, &back.nameid) && back.nameid)
            ? ({
                P<item_data> id_ = TRY_UNWRAP(itemdb_exists(back.nameid), { it.data.name.span.error("No item with this numerical id"_s); return false; });
                id_;
            })
            : ({
                P<item_data> id_ = TRY_UNWRAP(itemdb_searchname(XString(it.data.name.data)), { it.data.name.span.error("No item with this name"_s); return false; });
                back.nameid = id_->nameid;
                id_;
        }));

        back.value = it.data.value.data;
        if (it.data.value_multiply)
        {
            back.value = id->value_buy * back.value;
        }
    }

    nd->bl_prev = nd->bl_next = nullptr;
    nd->bl_m = m;
    nd->bl_x = x;
    nd->bl_y = y;
    nd->bl_id = npc_get_new_npc_id();
    nd->dir = dir;
    nd->flag = 0;
    nd->sit = DamageType::STAND;
    nd->name = shop.name.data;
    nd->npc_class = npc_class;
    nd->speed = 200_ms;
    nd->option = Opt0::ZERO;
    nd->opt1 = Opt1::ZERO;
    nd->opt2 = Opt2::ZERO;
    nd->opt3 = Opt3::ZERO;

    npc_shop++;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::SHOP;
    nd->n = map_addnpc(m, nd);
    map_addblock(nd);
    clif_spawnnpc(nd);
    register_npc_name(nd);

    return true;
}

static
bool npc_load_monster(ast::npc::Monster& monster)
{
    MapName mapname = monster.m.data;
    int x = monster.x.data, y = monster.y.data;
    int xs = monster.xs.data, ys = monster.ys.data;

    Species mob_class = monster.mob_class.data;
    int num = monster.num.data;
    interval_t delay1 = monster.delay1.data;
    interval_t delay2 = monster.delay2.data;
    NpcEvent eventname = monster.event.data;

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), abort());

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
        MobName expected = get_mob_db(mob_class).jname;
        if (monster.name.data != expected)
        {
            monster.name.span.warning(STRPRINTF("Visible label/jname should match: %s"_fmt, expected));
        }
        if (monster.name.data == ENGLISH_NAME)
            md->name = get_mob_db(mob_class).name;
        else if (monster.name.data == JAPANESE_NAME)
            md->name = get_mob_db(mob_class).jname;
        else
            md->name = monster.name.data;

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

        md->npc_event = eventname;

        md->bl_type = BL::MOB;
        map_addiddb(md);
        mob_spawn(md->bl_id);

        npc_mob++;
    }

    return true;
}

static
bool npc_load_mapflag(ast::npc::MapFlag& mapflag)
{
    MapName mapname = mapflag.m.data;
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), abort());

    MapFlag mf;
    if (!extract(mapflag.name.data, &mf))
    {
        mapflag.name.span.error("No such mapflag"_s);
        return false;
    }

    if (mf == MapFlag::NOPVP)
    {
        if (mapflag.vec_extra.data.size())
        {
            mapflag.vec_extra.span.error("No extra argument expected for mapflag 'nopvp'"_s);
            return false;
        }
        m->flag.set(MapFlag::NOPVP, 1);
        m->flag.set(MapFlag::PVP, 0);
        return true;
    }

    MapName savemap;
    int savex, savey, mask;

    if (mf == MapFlag::NOSAVE)
    {
        if (mapflag.vec_extra.data.size() == 3
                && extract(mapflag.vec_extra.data[0].data, &savemap)
                && extract(mapflag.vec_extra.data[1].data, &savex)
                && extract(mapflag.vec_extra.data[2].data, &savey)
                && map_mapname2mapid(savemap).is_some())
        {
            m->save.map_ = savemap;
            m->save.x = savex;
            m->save.y = savey;
        }
        else
        {
            mapflag.vec_extra.span.error("Unable to extract nosave savepoint"_s);
            return false;
        }
    }
    else if (mf == MapFlag::RESAVE)
    {
        if (mapflag.vec_extra.data.size() == 3
                && extract(mapflag.vec_extra.data[0].data, &savemap)
                && extract(mapflag.vec_extra.data[1].data, &savex)
                && extract(mapflag.vec_extra.data[2].data, &savey)
                && map_mapname2mapid(savemap).is_some())
        {
            m->resave.map_ = savemap;
            m->resave.x = savex;
            m->resave.y = savey;
        }
        else
        {
            mapflag.vec_extra.span.error("Unable to extract resave savepoint"_s);
            return false;
        }
    }
    else if (mf == MapFlag::MASK)
    {
        if (mapflag.vec_extra.data.size() == 1
                && extract(mapflag.vec_extra.data[0].data, &mask))
        {
            m->mask = mask;
        }
        else
        {
            mapflag.vec_extra.span.error("Unable to extract map mask"_s);
            return false;
        }
    }
    else
    {
        if (mapflag.vec_extra.data.size())
        {
            mapflag.vec_extra.span.error("No extra argument expected for mapflag"_s);
            return false;
        }
    }
    m->flag.set(mf, true);

    return true;
}

static
void npc_convertlabel_db(ScriptLabel lname, int pos, dumb_ptr<npc_data_script> nd)
{
    nullpo_retv(nd);

    struct npc_label_list eln {};
    eln.name = lname;
    eln.pos = pos;
    nd->scr.label_listv.push_back(std::move(eln));
}

static
bool npc_load_script_function(ast::script::ScriptBody& body, ast::npc::ScriptFunction& script_function)
{
    std::unique_ptr<const ScriptBuffer> script = compile_script(STRPRINTF("script function \"%s\""_fmt, script_function.name.data), body, false);
    if (script == nullptr)
        return false;

    userfunc_db.put(script_function.name.data, std::move(script));

    return true;
}

static
bool npc_load_script_none(ast::script::ScriptBody& body, ast::npc::ScriptNone& script_none)
{
    std::unique_ptr<const ScriptBuffer> script = compile_script(STRPRINTF("script npc \"%s\""_fmt, script_none.name.data), body, false);
    if (script == nullptr)
        return false;

    dumb_ptr<npc_data_script> nd;
    nd.new_();
    nd->scr.event_needs_map = false;

    nd->name = script_none.name.data;

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
    nd->scr.script = std::move(script);
    nd->option = Opt0::ZERO;
    nd->opt1 = Opt1::ZERO;
    nd->opt2 = Opt2::ZERO;
    nd->opt3 = Opt3::ZERO;

    npc_script++;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::SCRIPT;

    id_db.put(nd->bl_id, nd); // fix to get the oid in OnInit
    register_npc_name(nd);

    for (auto& pair : scriptlabel_db)
        npc_convertlabel_db(pair.first, pair.second, nd);

    for (npc_label_list& el : nd->scr.label_listv)
    {
        ScriptLabel lname = el.name;
        int pos = el.pos;

        if (lname.startswith("On"_s))
        {
            struct event_data ev {};
            ev.nd = nd;
            ev.pos = pos;
            NpcEvent buf;
            buf.npc = nd->name;
            buf.label = lname;
            ev_db.insert(buf, ev);
        }
    }

    for (npc_label_list& el : nd->scr.label_listv)
    {
        int t_ = 0;
        ScriptLabel lname = el.name;
        int pos = el.pos;
        if (lname.startswith("OnTimer"_s) && extract(lname.xslice_t(7), &t_) && t_ > 0)
        {
            interval_t t = static_cast<interval_t>(t_);

            npc_timerevent_list tel {};
            tel.timer = t;
            tel.pos = pos;

            auto it = std::lower_bound(nd->scr.timer_eventv.begin(), nd->scr.timer_eventv.end(), tel,
                    [](const npc_timerevent_list& l, const npc_timerevent_list& r)
                    {
                        return l.timer < r.timer;
                    }
            );
            assert (it == nd->scr.timer_eventv.end() || it->timer != tel.timer);

            nd->scr.timer_eventv.insert(it, std::move(tel));
        }
    }
    // The counter starts stopped with 0 ticks, which is the first event,
    // unless there is none, in which case begin == end.
    nd->scr.timer = interval_t::zero();
    nd->scr.next_event = nd->scr.timer_eventv.begin();
    // nd->scr.timerid = nullptr;

    return true;
}

static
bool npc_load_script_map(ast::script::ScriptBody& body, ast::npc::ScriptMap& script_map)
{
    MapName mapname = script_map.m.data;
    int x = script_map.x.data, y = script_map.y.data;
    DIR dir = script_map.d.data;

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
            {
                script_map.m.span.error("No such map"_s);
                return false;
            });

    std::unique_ptr<const ScriptBuffer> script = compile_script(STRPRINTF("script npc \"%s\""_fmt, script_map.name.data), body, false);
    if (script == nullptr)
        return false;


    dumb_ptr<npc_data_script> nd;
    nd.new_();

    Species npc_class = script_map.npc_class.data;
    int xs = script_map.xs.data, ys = script_map.ys.data;

    {
        for (int i = 0; i < ys; i++)
        {
            for (int j = 0; j < xs; j++)
            {
                int x_lo = x - xs / 2;
                int y_lo = y - ys / 2;
                int xc = x_lo + j;
                int yc = y_lo + i;
                MapCell t = map_getcell(m, xc, yc);
                if (bool(t & MapCell::UNWALKABLE))
                    continue;
                map_setcell(m, xc, yc, t | MapCell::NPC_NEAR);
            }
        }

        nd->scr.xs = xs;
        nd->scr.ys = ys;
        nd->scr.event_needs_map = true;
    }

    nd->name = script_map.name.data;

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
    nd->scr.script = std::move(script);
    nd->option = Opt0::ZERO;
    nd->opt1 = Opt1::ZERO;
    nd->opt2 = Opt2::ZERO;
    nd->opt3 = Opt3::ZERO;

    npc_script++;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::SCRIPT;

    nd->n = map_addnpc(m, nd);
    map_addblock(nd);

    clif_spawnnpc(nd);

    register_npc_name(nd);

    for (auto& pair : scriptlabel_db)
        npc_convertlabel_db(pair.first, pair.second, nd);

    for (npc_label_list& el : nd->scr.label_listv)
    {
        ScriptLabel lname = el.name;
        int pos = el.pos;

        if (lname.startswith("On"_s))
        {
            struct event_data ev {};
            ev.nd = nd;
            ev.pos = pos;
            NpcEvent buf;
            buf.npc = nd->name;
            buf.label = lname;
            ev_db.insert(buf, ev);
        }
    }

    for (npc_label_list& el : nd->scr.label_listv)
    {
        int t_ = 0;
        ScriptLabel lname = el.name;
        int pos = el.pos;
        if (lname.startswith("OnTimer"_s) && extract(lname.xslice_t(7), &t_) && t_ > 0)
        {
            interval_t t = static_cast<interval_t>(t_);

            npc_timerevent_list tel {};
            tel.timer = t;
            tel.pos = pos;

            auto it = std::lower_bound(nd->scr.timer_eventv.begin(), nd->scr.timer_eventv.end(), tel,
                    [](const npc_timerevent_list& l, const npc_timerevent_list& r)
                    {
                        return l.timer < r.timer;
                    }
            );
            assert (it == nd->scr.timer_eventv.end() || it->timer != tel.timer);

            nd->scr.timer_eventv.insert(it, std::move(tel));
        }
    }
    // The counter starts stopped with 0 ticks, which is the first event,
    // unless there is none, in which case begin == end.
    nd->scr.timer = interval_t::zero();
    nd->scr.next_event = nd->scr.timer_eventv.begin();
    // nd->scr.timerid = nullptr;

    return true;
}

static
bool npc_load_script_any(ast::npc::Script *script)
{
    MATCH_BEGIN (*script)
    {
        MATCH_CASE (ast::npc::ScriptFunction&, script_function)
        {
            return npc_load_script_function(script->body, script_function);
        }
        MATCH_CASE (ast::npc::ScriptNone&, script_none)
        {
            return npc_load_script_none(script->body, script_none);
        }
        MATCH_CASE (ast::npc::ScriptMap&, script_map)
        {
            auto& mapname = script_map.m;
            Option<P<map_local>> m = map_mapname2mapid(mapname.data);
            if (m.is_none())
            {
                mapname.span.error(STRPRINTF("Map not found: %s"_fmt, mapname.data));
                return false;
            }
            return npc_load_script_map(script->body, script_map);
        }
    }
    MATCH_END ();
    abort();
}

dumb_ptr<npc_data> npc_spawn_text(Borrowed<map_local> m, int x, int y,
        Species npc_class, NpcName name, AString message)
{
    dumb_ptr<npc_data_message> retval;
    retval.new_();
    retval->bl_id = npc_get_new_npc_id();
    retval->bl_x = x;
    retval->bl_y = y;
    retval->bl_m = m;
    retval->bl_type = BL::NPC;
    retval->npc_subtype = NpcSubtype::MESSAGE;

    retval->name = name;
    if (message)
        retval->message = message;

    retval->npc_class = npc_class;
    retval->speed = 200_ms;

    clif_spawnnpc(retval);
    map_addblock(retval);
    map_addiddb(retval);
    register_npc_name(retval);

    return retval;
}

static
bool load_one_npc(io::LineCharReader& fp, bool& done)
{
    auto res = TRY_UNWRAP(ast::npc::parse_top(fp), { done = true; return true; });
    if (res.get_failure())
        PRINTF("%s\n"_fmt, res.get_failure());
    ast::npc::TopLevel tl = TRY_UNWRAP(std::move(res.get_success()), return false);

    MATCH_BEGIN (tl)
    {
        MATCH_CASE (ast::npc::Comment&, c)
        {
            (void)c;
            return true;
        }
        MATCH_CASE (ast::npc::Warp&, warp)
        {
            auto& mapname = warp.m;
            Option<P<map_local>> m = map_mapname2mapid(mapname.data);
            if (m.is_none())
            {
                mapname.span.error(STRPRINTF("Map not found: %s"_fmt, mapname.data));
                return false;
            }
            return npc_load_warp(warp);
        }
        MATCH_CASE (ast::npc::Shop&, shop)
        {
            auto& mapname = shop.m;
            Option<P<map_local>> m = map_mapname2mapid(mapname.data);
            if (m.is_none())
            {
                mapname.span.error(STRPRINTF("Map not found: %s"_fmt, mapname.data));
                return false;
            }
            return npc_load_shop(shop);
        }
        MATCH_CASE (ast::npc::Monster&, monster)
        {
            auto& mapname = monster.m;
            Option<P<map_local>> m = map_mapname2mapid(mapname.data);
            if (m.is_none())
            {
                mapname.span.error(STRPRINTF("Map not found: %s"_fmt, mapname.data));
                return false;
            }
            return npc_load_monster(monster);
        }
        MATCH_CASE (ast::npc::MapFlag&, mapflag)
        {
            auto& mapname = mapflag.m;
            Option<P<map_local>> m = map_mapname2mapid(mapname.data);
            if (m.is_none())
            {
                mapname.span.error(STRPRINTF("Map not found: %s"_fmt, mapname.data));
                return false;
            }
            return npc_load_mapflag(mapflag);
        }
        MATCH_CASE (ast::npc::Script&, script)
        {
            return npc_load_script_any(&script);
        }
    }
    MATCH_END ();
    abort();
}

static
bool load_npc_file(ZString nsl)
{
    io::LineCharReader fp(nsl);
    if (!fp.is_open())
    {
        PRINTF("file not found : %s\n"_fmt, nsl);
        return false;
    }
    PRINTF("Loading NPCs [%d]: %-54s\r"_fmt, unwrap<BlockId>(npc_id) - unwrap<BlockId>(START_NPC_NUM),
            nsl);

    bool done = false;
    while (!done)
    {
        if (!load_one_npc(fp, done))
            return false;
    }
    return true;
}

bool do_init_npc(void)
{
    bool rv = true;

    for (; !npc_srcs.empty(); npc_srcs.pop_front())
    {
        AString nsl = npc_srcs.front();
        rv &= load_npc_file(nsl);
    }
    PRINTF("NPCs Loaded: %d [Warps:%d Shops:%d Scripts:%d Mobs:%d] %20s\n"_fmt,
            unwrap<BlockId>(npc_id) - unwrap<BlockId>(START_NPC_NUM), npc_warp, npc_shop, npc_script, npc_mob, ""_s);

    if (script_errors)
    {
        PRINTF("Cowardly refusing to continue after %d errors\n"_fmt, script_errors);
        rv = false;
    }
    return rv;
}
} // namespace map
} // namespace tmwa
