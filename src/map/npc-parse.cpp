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
#include "../io/read.hpp"

#include "../mmo/config_parse.hpp"

#include "../high/extract_mmo.hpp"

#include "battle.hpp"
#include "clif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "npc-internal.hpp"
#include "script-parse.hpp"

#include "../poison.hpp"


namespace tmwa
{
static
std::list<AString> npc_srcs;

static
int npc_warp, npc_shop, npc_script, npc_mob;

//
// 初期化関係
//

/*==========================================
 * 読み込むnpcファイルのクリア
 *------------------------------------------
 */
static
void npc_clearsrcfile(void)
{
    npc_srcs.clear();
}

/*==========================================
 * 読み込むnpcファイルの追加
 *------------------------------------------
 */
void npc_addsrcfile(AString name)
{
    if (name == "clear"_s)
    {
        npc_clearsrcfile();
        return;
    }

    npc_srcs.push_back(name);
}

/*==========================================
 * 読み込むnpcファイルの削除
 *------------------------------------------
 */
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

static
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

/*==========================================
 * warp行解析
 *------------------------------------------
 */
int npc_parse_warp(XString w1, XString, NpcName w3, XString w4)
{
    int x, y, xs, ys, to_x, to_y;
    int i, j;
    MapName mapname, to_mapname;
    dumb_ptr<npc_data_warp> nd;

    if (!extract(w1, record<','>(&mapname, &x, &y)) ||
        !extract(w4, record<','>(&xs, &ys, &to_mapname, &to_x, &to_y)))
    {
        PRINTF("bad warp line : %s\n"_fmt, w3);
        return 1;
    }

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return 1);

    nd.new_();
    nd->bl_id = npc_get_new_npc_id();
    nd->n = map_addnpc(m, nd);

    nd->bl_prev = nd->bl_next = nullptr;
    nd->bl_m = m;
    nd->bl_x = x;
    nd->bl_y = y;
    nd->dir = DIR::S;
    nd->flag = 0;
    nd->name = w3;

    if (!battle_config.warp_point_debug)
        nd->npc_class = WARP_CLASS;
    else
        nd->npc_class = WARP_DEBUG_CLASS;
    nd->speed = 200_ms;
    nd->option = Opt0::ZERO;
    nd->opt1 = Opt1::ZERO;
    nd->opt2 = Opt2::ZERO;
    nd->opt3 = Opt3::ZERO;
    nd->warp.name = to_mapname;
    xs += 2;
    ys += 2;
    nd->warp.x = to_x;
    nd->warp.y = to_y;
    nd->warp.xs = xs;
    nd->warp.ys = ys;

    for (i = 0; i < ys; i++)
    {
        for (j = 0; j < xs; j++)
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

    return 0;
}

static
bool extract(XString xs, npc_item_list *itv)
{
    XString name_or_id;
    if (!extract(xs, record<':'>(&name_or_id, &itv->value)))
        return false;

    P<struct item_data> id = ((extract(name_or_id, &itv->nameid) && itv->nameid)
    ? ({
        P<struct item_data> id_ = itemdb_search(itv->nameid);
        id_;
    })
    : ({
        P<struct item_data> id_ = TRY_UNWRAP(itemdb_searchname(name_or_id.rstrip()), return false);
        itv->nameid = id_->nameid;
        id_;
    }));

    if (itv->value < 0)
    {
        itv->value = id->value_buy * abs(itv->value);
    }
    return true;
}

/*==========================================
 * shop行解析
 *------------------------------------------
 */
static
int npc_parse_shop(XString w1, XString, NpcName w3, ZString w4a)
{
    int x, y;
    DIR dir;
    MapName mapname;
    dumb_ptr<npc_data_shop> nd;
    ZString::iterator w4comma;
    Species npc_class;

    int dir_; // TODO use enum directly in extract
    if (!extract(w1, record<','>(&mapname, &x, &y, &dir_))
        || dir_ < 0 || dir_ >= 8
        || (w4comma = std::find(w4a.begin(), w4a.end(), ',')) == w4a.end()
        || !extract(w4a.xislice_h(w4comma), &npc_class))
    {
        PRINTF("bad shop line : %s\n"_fmt, w3);
        return 1;
    }
    dir = static_cast<DIR>(dir_);
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return 1);

    nd.new_();
    ZString w4b = w4a.xislice_t(w4comma + 1);

    if (!extract(w4b, vrec<','>(&nd->shop_items)))
    {
        PRINTF("bad shop items : %s\n"_fmt, w3);
        PRINTF("   somewhere --> %s\n"_fmt, w4b);
        nd->shop_items.clear();
    }

    if (nd->shop_items.empty())
    {
        nd.delete_();
        return 1;
    }

    nd->bl_prev = nd->bl_next = nullptr;
    nd->bl_m = m;
    nd->bl_x = x;
    nd->bl_y = y;
    nd->bl_id = npc_get_new_npc_id();
    nd->dir = dir;
    nd->flag = 0;
    nd->name = w3;
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

    return 0;
}

/*==========================================
 * NPCのラベルデータコンバート
 *------------------------------------------
 */
static
void npc_convertlabel_db(ScriptLabel lname, int pos, dumb_ptr<npc_data_script> nd)
{
    nullpo_retv(nd);

    struct npc_label_list eln {};
    eln.name = lname;
    eln.pos = pos;
    nd->scr.label_listv.push_back(std::move(eln));
}

/*==========================================
 * script行解析
 *------------------------------------------
 */
static
int npc_parse_script(XString w1, XString w2, NpcName w3, ZString w4,
        XString first_line, io::ReadFile& fp, int *lines)
{
    int x, y;
    DIR dir = DIR::S;
    int xs = 0, ys = 0;   // [Valaris] thanks to fov
    Species npc_class;
    MapName mapname;
    std::unique_ptr<const ScriptBuffer> script = nullptr;
    dumb_ptr<npc_data_script> nd;
    int evflag = 0;

    P<map_local> m = borrow(undefined_gat);
    if (w1 == "-"_s)
    {
        x = 0;
        y = 0;
        m = borrow(undefined_gat);
    }
    else
    {
        int dir_; // TODO use enum directly in extract
        if (!extract(w1, record<','>(&mapname, &x, &y, &dir_))
            || dir_ < 0 || dir_ >= 8
            || (w2 == "script"_s && !w4.contains(',')))
        {
            PRINTF("bad script line : %s\n"_fmt, w3);
            return 1;
        }
        dir = static_cast<DIR>(dir_);
        m = map_mapname2mapid(mapname).copy_or(borrow(undefined_gat));
    }

    if (w2 == "script"_s)
    {
        // may be empty
        MString srcbuf;
        srcbuf += first_line.xislice_t(std::find(first_line.begin(), first_line.end(), '{'));
        // Note: it was a bug that this was missing. I think.
        int startline = *lines;

        // while (!srcbuf.rstrip().endswith('}'))
        while (true)
        {
            auto it = std::find_if_not(srcbuf.rbegin(), srcbuf.rend(), [](char c){ return c == ' ' || c == '\n'; });
            if (it != srcbuf.rend() && *it == '}')
                break;

            AString line;
            if (!fp.getline(line))
                // eof
                break;
            (*lines)++;
            if (!srcbuf)
            {
                // may be a no-op
                srcbuf += line.xislice_t(std::find(line.begin(), line.end(), '{'));
                // safe to execute more than once
                // But will usually only happen once
                startline = *lines;
            }
            else
                srcbuf += line;
            srcbuf += '\n';
        }
        script = parse_script(AString(srcbuf), startline, false);
        if (script == nullptr)
            // script parse error?
            return 1;
    }
    else
    {
        assert(0 && "duplicate() is no longer supported!\n"_s);
        return 0;
    }

    nd.new_();

    if (m == borrow(undefined_gat))
    {
    }
    else if (extract(w4, record<','>(&npc_class, &xs, &ys)))
    {
        if (xs >= 0)
            xs = xs * 2 + 1;
        if (ys >= 0)
            ys = ys * 2 + 1;

        if (npc_class != NEGATIVE_SPECIES)
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
        }

        nd->scr.xs = xs;
        nd->scr.ys = ys;
    }
    else
    {
        XString w4x = w4;
        if (w4x.endswith(','))
            w4x = w4x.xrslice_h(1);
        if (!extract(w4x, &npc_class))
            abort();
        nd->scr.xs = 0;
        nd->scr.ys = 0;
    }

    if (npc_class == NEGATIVE_SPECIES && m != borrow(undefined_gat))
    {
        evflag = 1;
    }

    if (w3.contains(':'))
    {
        assert(false && "feature removed"_s);
        abort();
    }

    {
        nd->name = w3;
    }

    nd->bl_prev = nd->bl_next = nullptr;
    nd->bl_m = m;
    nd->bl_x = x;
    nd->bl_y = y;
    nd->bl_id = npc_get_new_npc_id();
    nd->dir = dir;
    nd->flag = 0;
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
    if (m != borrow(undefined_gat))
    {
        nd->n = map_addnpc(m, nd);
        map_addblock(nd);

        if (evflag)
        {
            struct event_data ev {};
            ev.nd = nd;
            ev.pos = 0;
            NpcEvent npcev;
            npcev.npc = nd->name;
            npcev.label = ScriptLabel();
            ev_db.insert(npcev, ev);
        }
        else
            clif_spawnnpc(nd);
    }
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

    //-----------------------------------------
    // ラベルデータからタイマーイベント取り込み
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

    return 0;
}

/*==========================================
 * function行解析
 *------------------------------------------
 */
static
int npc_parse_function(XString, XString, XString w3, ZString,
        XString first_line, io::ReadFile& fp, int *lines)
{
    MString srcbuf;
    srcbuf += first_line.xislice_t(std::find(first_line.begin(), first_line.end(), '{'));
    int startline = *lines;

    while (true)
    {
        auto it = std::find_if_not(srcbuf.rbegin(), srcbuf.rend(), [](char c){ return c == ' ' || c == '\n'; });
        if (it != srcbuf.rend() && *it == '}')
            break;

        AString line;
        if (!fp.getline(line))
            break;
        (*lines)++;
        if (!srcbuf)
        {
            srcbuf += line.xislice_t(std::find(line.begin(), line.end(), '{'));
            startline = *lines;
        }
        else
            srcbuf += line;
        srcbuf += '\n';
    }
    std::unique_ptr<const ScriptBuffer> script = parse_script(AString(srcbuf), startline, false);
    if (script == nullptr)
    {
        // script parse error?
        return 1;
    }

    userfunc_db.put(w3, std::move(script));

    return 0;
}

/*==========================================
 * mob行解析
 *------------------------------------------
 */
static
int npc_parse_mob(XString w1, XString, MobName w3, ZString w4)
{
    int x, y, xs, ys, num;
    Species mob_class;
    int i;
    MapName mapname;
    NpcEvent eventname;
    dumb_ptr<mob_data> md;

    xs = ys = 0;
    int delay1_ = 0, delay2_ = 0;
    if (!extract(w1, record<',', 3>(&mapname, &x, &y, &xs, &ys)) ||
        !extract(w4, record<',', 2>(&mob_class, &num, &delay1_, &delay2_, &eventname)))
    {
        PRINTF("bad monster line : %s\n"_fmt, w3);
        return 1;
    }
    interval_t delay1 = std::chrono::milliseconds(delay1_);
    interval_t delay2 = std::chrono::milliseconds(delay2_);

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return 1);

    if (num > 1 && battle_config.mob_count_rate != 100)
    {
        if ((num = num * battle_config.mob_count_rate / 100) < 1)
            num = 1;
    }

    for (i = 0; i < num; i++)
    {
        md.new_();

        md->bl_prev = nullptr;
        md->bl_next = nullptr;
        md->bl_m = m;
        md->bl_x = x;
        md->bl_y = y;
        if (w3 == ENGLISH_NAME)
            md->name = get_mob_db(mob_class).name;
        else if (w3 == JAPANESE_NAME)
            md->name = get_mob_db(mob_class).jname;
        else
            md->name = w3;

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

    return 0;
}

/*==========================================
 * マップフラグ行の解析
 *------------------------------------------
 */
static
int npc_parse_mapflag(XString w1, XString, XString w3, ZString w4)
{
    MapName mapname, savemap;
    int savex, savey;

    mapname = stringish<MapName>(w1);
    if (!mapname)
        return 1;

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return 1);

    MapFlag mf;
    if (!extract(w3, &mf))
        return 1;

    if (battle_config.pk_mode && mf == MapFlag::NOPVP)
    {
        m->flag.set(MapFlag::NOPVP, 1);
        m->flag.set(MapFlag::PVP, 0);
        return 0;
    }

    if (mf == MapFlag::NOSAVE)
    {
        if (w4 == "SavePoint"_s)
        {
            m->save.map_ = stringish<MapName>("SavePoint"_s);
            m->save.x = -1;
            m->save.y = -1;
        }
        else if (extract(w4, record<','>(&savemap, &savex, &savey)))
        {
            m->save.map_ = savemap;
            m->save.x = savex;
            m->save.y = savey;
        }
    }
    if (mf == MapFlag::RESAVE)
    {
        if (extract(w4, record<','>(&savemap, &savex, &savey)))
        {
            m->resave.map_ = savemap;
            m->resave.x = savex;
            m->resave.y = savey;
        }
    }
    m->flag.set(mf, true);

    return 0;
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

/*==========================================
 * npc初期化
 *------------------------------------------
 */
bool do_init_npc(void)
{
    bool rv = true;

    for (; !npc_srcs.empty(); npc_srcs.pop_front())
    {
        AString nsl = npc_srcs.front();
        io::ReadFile fp(nsl);
        if (!fp.is_open())
        {
            PRINTF("file not found : %s\n"_fmt, nsl);
            rv = false;
            continue;
        }
        PRINTF("\rLoading NPCs [%d]: %-54s"_fmt, unwrap<BlockId>(npc_id) - unwrap<BlockId>(START_NPC_NUM),
                nsl);
        int lines = 0;
        AString zline;
        while (fp.getline(zline))
        {
            XString w1, w2, w3, w4x;
            ZString w4z;
            lines++;

            if (is_comment(zline))
                continue;

            if (!extract(zline, record<'|', 3>(&w1, &w2, &w3, &w4x)) || !w1 || !w2 || !w3)
            {
                FPRINTF(stderr, "%s:%d: Broken script line: %s\n"_fmt, nsl, lines, zline);
                rv = false;
                continue;
            }
            if (&*w4x.end() == &*zline.end())
            {
                w4z = zline.xrslice_t(w4x.size());
            }
            assert(bool(w4x) == bool(w4z));

            if (w1 != "-"_s && w1 != "function"_s)
            {
                auto comma = std::find(w1.begin(), w1.end(), ',');
                MapName mapname = stringish<MapName>(w1.xislice_h(comma));
                Option<P<map_local>> m = map_mapname2mapid(mapname);
                if (m.is_none())
                {
                    // "mapname" is not assigned to this server
                    FPRINTF(stderr, "%s:%d: Map not found: %s\n"_fmt, nsl, lines, mapname);
                    rv = false;
                    continue;
                }
            }
            if (w2 == "warp"_s)
            {
                NpcName npcname = stringish<NpcName>(w3);
                rv &= !npc_parse_warp(w1, w2, npcname, w4z);
            }
            else if (w2 == "shop"_s)
            {
                NpcName npcname = stringish<NpcName>(w3);
                rv &= !npc_parse_shop(w1, w2, npcname, w4z);
            }
            else if (w2 == "script"_s)
            {
                if (w1 == "function"_s)
                {
                    rv &= !npc_parse_function(w1, w2, w3, w4z,
                            w4x, fp, &lines);
                }
                else
                {
                    NpcName npcname = stringish<NpcName>(w3);
                    rv &= !npc_parse_script(w1, w2, npcname, w4z,
                            w4x, fp, &lines);
                }
            }
            else if (w2 == "monster"_s)
            {
                MobName mobname = stringish<MobName>(w3);
                rv &= !npc_parse_mob(w1, w2, mobname, w4z);
            }
            else if (w2 == "mapflag"_s)
            {
                rv &= !npc_parse_mapflag(w1, w2, w3, w4z);
            }
            else
            {
                PRINTF("odd script line: %s\n"_fmt, zline);
                script_errors++;
            }
        }
        fflush(stdout);
    }
    PRINTF("\rNPCs Loaded: %d [Warps:%d Shops:%d Scripts:%d Mobs:%d] %20s\n"_fmt,
            unwrap<BlockId>(npc_id) - unwrap<BlockId>(START_NPC_NUM), npc_warp, npc_shop, npc_script, npc_mob, ""_s);

    if (script_errors)
    {
        PRINTF("Cowardly refusing to continue after %d errors\n"_fmt, script_errors);
        rv = false;
    }
    return rv;
}
} // namespace tmwa
