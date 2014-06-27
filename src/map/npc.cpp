#include "npc.hpp"
//    npc.cpp - Noncombatants.
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

#include <cassert>
#include <ctime>

#include <algorithm>
#include <list>

#include "../compat/fun.hpp"
#include "../compat/nullpo.hpp"

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/literal.hpp"

#include "../generic/db.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/read.hpp"

#include "../net/timer.hpp"

#include "../mmo/config_parse.hpp"
#include "../mmo/extract.hpp"
#include "../mmo/utils.hpp"

#include "../proto2/map-user.hpp"

#include "battle.hpp"
#include "clif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "pc.hpp"
#include "script.hpp"
#include "skill.hpp"

#include "../poison.hpp"

static
std::list<AString> npc_srcs;

static
BlockId npc_id = START_NPC_NUM;
static
int npc_warp, npc_shop, npc_script, npc_mob;

BlockId npc_get_new_npc_id(void)
{
    BlockId rv = npc_id;
    npc_id = next(npc_id);
    return rv;
}

struct event_data
{
    dumb_ptr<npc_data_script> nd;
    int pos;
};
static
Map<NpcEvent, struct event_data> ev_db;
static
DMap<NpcName, dumb_ptr<npc_data>> npcs_by_name;

// used for clock-based event triggers
// only tm_min, tm_hour, and tm_mday are used
static
struct tm ev_tm_b;

/*==========================================
 * NPCの無効化/有効化
 * npc_enable
 * npc_enable_sub 有効時にOnTouchイベントを実行
 *------------------------------------------
 */
static
void npc_enable_sub(dumb_ptr<block_list> bl, dumb_ptr<npc_data> nd)
{
    dumb_ptr<map_session_data> sd;

    nullpo_retv(bl);

    assert (bl->bl_type == BL::PC);
    {
        sd = bl->is_player();

        // not if disabled
        if (nd->flag & 1)
            return;

        NpcEvent aname;
        aname.npc = nd->name;
        aname.label = stringish<ScriptLabel>("OnTouch"_s);
        if (sd->areanpc_id == nd->bl_id)
            return;
        sd->areanpc_id = nd->bl_id;
        npc_event(sd, aname, 0);
    }
}

int npc_enable(NpcName name, bool flag)
{
    dumb_ptr<npc_data> nd = npc_name2id(name);
    if (nd == nullptr)
    {
        PRINTF("npc_enable(%s, %s) failed.\n"_fmt, name, flag ? "true"_s : "false"_s);
        return 0;
    }

    if (flag)
    {                           // 有効化
        nd->flag &= ~1;
        clif_spawnnpc(nd);
    }
    else
    {                           // 無効化
        nd->flag |= 1;
        clif_clearchar(nd, BeingRemoveWhy::GONE);
    }

    int xs = 0, ys = 0;
    if (dumb_ptr<npc_data_script> nd_ = nd->is_script())
    {
        xs = nd_->scr.xs;
        ys = nd_->scr.ys;
    }

    if (flag && (xs > 0 || ys > 0))
        map_foreachinarea(std::bind(npc_enable_sub, ph::_1, nd),
                nd->bl_m,
                nd->bl_x - xs, nd->bl_y - ys,
                nd->bl_x + xs, nd->bl_y + ys,
                BL::PC);

    return 0;
}

/*==========================================
 * NPCを名前で探す
 *------------------------------------------
 */
dumb_ptr<npc_data> npc_name2id(NpcName name)
{
    return npcs_by_name.get(name);
}

/*==========================================
 * イベントキューのイベント処理
 *------------------------------------------
 */
int npc_event_dequeue(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    sd->npc_id = BlockId();

    if (!sd->eventqueuel.empty())
    {
        if (!pc_addeventtimer(sd, std::chrono::milliseconds(100), sd->eventqueuel.front()))
        {
            PRINTF("npc_event_dequeue(): Event timer is full.\n"_fmt);
            return 0;
        }

        sd->eventqueuel.pop_front();
        return 1;
    }

    return 0;
}

int npc_delete(dumb_ptr<npc_data> nd)
{
    nullpo_retr(1, nd);

    if (nd->bl_prev == nullptr)
        return 1;

    clif_clearchar(nd, BeingRemoveWhy::DEAD);
    map_delblock(nd);
    return 0;
}

void npc_timer_event(NpcEvent eventname)
{
    struct event_data *ev = ev_db.search(eventname);
    dumb_ptr<npc_data_script> nd;
//  int xs,ys;

    if ((ev == nullptr || (nd = ev->nd) == nullptr))
    {
        PRINTF("npc_event: event not found [%s]\n"_fmt,
                eventname);
        return;
    }

    run_script(ScriptPointer(nd->scr.script.get(), ev->pos), nd->bl_id, nd->bl_id);
}

/*==========================================
 * 全てのNPCのOn*イベント実行
 *------------------------------------------
 */
static
void npc_event_doall_sub(NpcEvent key, struct event_data *ev,
        int *c, ScriptLabel name, BlockId rid, Slice<argrec_t> argv)
{
    ScriptLabel p = key.label;

    nullpo_retv(ev);

    if (name == p)
    {
        run_script_l(ScriptPointer(ev->nd->scr.script.get(), ev->pos), rid, ev->nd->bl_id,
                argv);
        (*c)++;
    }
}

int npc_event_doall_l(ScriptLabel name, BlockId rid, Slice<argrec_t> args)
{
    int c = 0;

    for (auto& pair : ev_db)
        npc_event_doall_sub(pair.first, &pair.second, &c, name, rid, args);
    return c;
}

static
void npc_event_do_sub(NpcEvent key, struct event_data *ev,
        int *c, NpcEvent name, BlockId rid, Slice<argrec_t> argv)
{
    nullpo_retv(ev);

    if (name == key)
    {
        run_script_l(ScriptPointer(ev->nd->scr.script.get(), ev->pos), rid, ev->nd->bl_id,
                argv);
        (*c)++;
    }
}

int npc_event_do_l(NpcEvent name, BlockId rid, Slice<argrec_t> args)
{
    int c = 0;

    if (!name.npc)
    {
        return npc_event_doall_l(name.label, rid, args);
    }

    for (auto& pair : ev_db)
        npc_event_do_sub(pair.first, &pair.second, &c, name, rid, args);
    return c;
}

/*==========================================
 * 時計イベント実行
 *------------------------------------------
 */
static
void npc_event_do_clock(TimerData *, tick_t)
{
    struct tm t = TimeT::now();

    ScriptLabel buf;
    if (t.tm_min != ev_tm_b.tm_min)
    {
        SNPRINTF(buf, 24, "OnMinute%02d"_fmt, t.tm_min);
        npc_event_doall(buf);
        SNPRINTF(buf, 24, "OnClock%02d%02d"_fmt, t.tm_hour, t.tm_min);
        npc_event_doall(buf);
    }
    if (t.tm_hour != ev_tm_b.tm_hour)
    {
        SNPRINTF(buf, 24, "OnHour%02d"_fmt, t.tm_hour);
        npc_event_doall(buf);
    }
    if (t.tm_mday != ev_tm_b.tm_mday)
    {
        SNPRINTF(buf, 24, "OnDay%02d%02d"_fmt, t.tm_mon + 1, t.tm_mday);
        npc_event_doall(buf);
    }
    ev_tm_b = t;
}

/*==========================================
 * OnInitイベント実行(&時計イベント開始)
 *------------------------------------------
 */
int npc_event_do_oninit(void)
{
    int c = npc_event_doall(stringish<ScriptLabel>("OnInit"_s));
    PRINTF("npc: OnInit Event done. (%d npc)\n"_fmt, c);

    Timer(gettick() + std::chrono::milliseconds(100),
            npc_event_do_clock,
            std::chrono::seconds(1)
    ).detach();

    return 0;
}

/// Callback for npc OnTimer*: labels.
/// This will be called later if you call npc_timerevent_start.
/// This function may only expire, but not deactivate, the counter.
static
void npc_timerevent(TimerData *, tick_t tick, BlockId id, interval_t data)
{
    dumb_ptr<npc_data_script> nd = map_id2bl(id)->is_npc()->is_script();
    assert (nd != nullptr);
    assert (nd->npc_subtype == NpcSubtype::SCRIPT);
    assert (nd->scr.next_event != nd->scr.timer_eventv.end());

    nd->scr.timertick = tick;
    const auto te = nd->scr.next_event;
    // nd->scr.timerid = nullptr;

    // er, isn't this the same as nd->scr.timer = te->timer?
    interval_t t = nd->scr.timer += data;
    assert (t == te->timer);
    ++nd->scr.next_event;
    if (nd->scr.next_event != nd->scr.timer_eventv.end())
    {
        interval_t next = nd->scr.next_event->timer - t;
        nd->scr.timerid = Timer(tick + next,
                std::bind(npc_timerevent, ph::_1, ph::_2,
                    id, next));
    }

    run_script(ScriptPointer(nd->scr.script.get(), te->pos), BlockId(), nd->bl_id);
}

/// Start (or resume) counting ticks to the next npc_timerevent.
/// If the tick is already high enough, just set it to expired.
void npc_timerevent_start(dumb_ptr<npc_data_script> nd)
{
    nullpo_retv(nd);

    if (nd->scr.timer_active)
        return;
    nd->scr.timer_active = true;

    if (nd->scr.timer_eventv.empty())
        return;
    if (nd->scr.timer == nd->scr.timer_eventv.back().timer)
        return;
    assert (nd->scr.timer < nd->scr.timer_eventv.back().timer);

    nd->scr.timertick = gettick();

    auto jt = nd->scr.next_event;
    assert (jt != nd->scr.timer_eventv.end());

    interval_t next = jt->timer - nd->scr.timer;
    nd->scr.timerid = Timer(gettick() + next,
            std::bind(npc_timerevent, ph::_1, ph::_2,
                nd->bl_id, next));
}

/// Stop the tick counter.
/// If the count was expired, just deactivate it.
void npc_timerevent_stop(dumb_ptr<npc_data_script> nd)
{
    nullpo_retv(nd);

    if (!nd->scr.timer_active)
        return;
    nd->scr.timer_active = false;

    if (nd->scr.timerid)
    {
        nd->scr.timer += gettick() - nd->scr.timertick;
        nd->scr.timerid.cancel();
    }
}

/// Get the number of ticks on the counter.
/// If there is an actual timer running, this involves math.
interval_t npc_gettimerevent_tick(dumb_ptr<npc_data_script> nd)
{
    nullpo_retr(interval_t::zero(), nd);

    interval_t tick = nd->scr.timer;

    if (nd->scr.timerid)
        tick += gettick() - nd->scr.timertick;
    return tick;
}

/// Helper method to update the "next event" iterator.
/// Note that now the iterator is always valid unless it is at the end.
/// Previously, it was invalid when the counter was deactivated.
static
void npc_timerevent_calc_next(dumb_ptr<npc_data_script> nd)
{
    npc_timerevent_list phony {};
    phony.timer = nd->scr.timer;

    // find the first element such that el.timer > phony.timer;
    auto jt = std::upper_bound(nd->scr.timer_eventv.begin(), nd->scr.timer_eventv.end(), phony,
            [](const npc_timerevent_list& l, const npc_timerevent_list& r)
            {
                return l.timer < r.timer;
            }
    );
    nd->scr.next_event = jt;
}

/// Set the tick counter.
/// If the timer was active, this means stopping and restarting the timer.
/// Note: active includes expired.
void npc_settimerevent_tick(dumb_ptr<npc_data_script> nd, interval_t newtimer)
{
    nullpo_retv(nd);

    if (nd->scr.timer_eventv.empty())
        return;
    if (newtimer > nd->scr.timer_eventv.back().timer)
        newtimer = nd->scr.timer_eventv.back().timer;
    if (newtimer < interval_t::zero())
        newtimer = interval_t::zero();
    if (newtimer == nd->scr.timer)
        return;

    bool flag = nd->scr.timer_active;

    if (flag)
        npc_timerevent_stop(nd);
    nd->scr.timer = newtimer;
    npc_timerevent_calc_next(nd);
    if (flag)
        npc_timerevent_start(nd);
}

/*==========================================
 * イベント型のNPC処理
 *------------------------------------------
 */
int npc_event(dumb_ptr<map_session_data> sd, NpcEvent eventname,
        int mob_kill)
{
    struct event_data *ev = ev_db.search(eventname);
    dumb_ptr<npc_data_script> nd;
    int xs, ys;

    if (sd == nullptr)
    {
        PRINTF("npc_event nullpo?\n"_fmt);
    }

    if (ev == nullptr && eventname.label == stringish<ScriptLabel>("OnTouch"_s))
        return 1;

    if (ev == nullptr || (nd = ev->nd) == nullptr)
    {
        if (mob_kill)
        {
            {
                return 0;
            }
        }
        else
        {
            if (battle_config.error_log)
                PRINTF("npc_event: event not found [%s]\n"_fmt,
                        eventname);
            return 0;
        }
    }

    xs = nd->scr.xs;
    ys = nd->scr.ys;
    if (xs >= 0 && ys >= 0)
    {
        if (nd->bl_m != sd->bl_m)
            return 1;
        if (xs > 0
            && (sd->bl_x < nd->bl_x - xs / 2 || nd->bl_x + xs / 2 < sd->bl_x))
            return 1;
        if (ys > 0
            && (sd->bl_y < nd->bl_y - ys / 2 || nd->bl_y + ys / 2 < sd->bl_y))
            return 1;
    }

    if (sd->npc_id)
    {
        sd->eventqueuel.push_back(eventname);
        return 1;
    }
    if (nd->flag & 1)
    {                           // 無効化されている
        npc_event_dequeue(sd);
        return 0;
    }

    sd->npc_id = nd->bl_id;
    sd->npc_pos =
        run_script(ScriptPointer(nd->scr.script.get(), ev->pos), sd->bl_id, nd->bl_id);
    return 0;
}

static
void npc_command_sub(NpcEvent key, struct event_data *ev, NpcName npcname, XString command)
{
    if (ev->nd->name == npcname
        && key.label.startswith("OnCommand"_s))
    {
        XString temp = key.label.xslice_t(9);

        if (command == temp)
            run_script(ScriptPointer(ev->nd->scr.script.get(), ev->pos), BlockId(), ev->nd->bl_id);
    }
}

int npc_command(dumb_ptr<map_session_data>, NpcName npcname, XString command)
{
    for (auto& pair : ev_db)
        npc_command_sub(pair.first, &pair.second, npcname, command);

    return 0;
}

/*==========================================
 * 接触型のNPC処理
 *------------------------------------------
 */
int npc_touch_areanpc(dumb_ptr<map_session_data> sd, map_local *m, int x, int y)
{
    int i, f = 1;
    int xs, ys;

    nullpo_retr(1, sd);

    if (sd->npc_id)
        return 1;

    for (i = 0; i < m->npc_num; i++)
    {
        if (m->npc[i]->flag & 1)
        {                       // 無効化されている
            f = 0;
            continue;
        }

        switch (m->npc[i]->npc_subtype)
        {
            case NpcSubtype::WARP:
                xs = m->npc[i]->is_warp()->warp.xs;
                ys = m->npc[i]->is_warp()->warp.ys;
                break;
            case NpcSubtype::MESSAGE:
                assert (0 && "I'm pretty sure these are never put on a map"_s);
                xs = 0;
                ys = 0;
                break;
            case NpcSubtype::SCRIPT:
                xs = m->npc[i]->is_script()->scr.xs;
                ys = m->npc[i]->is_script()->scr.ys;
                break;
            default:
                continue;
        }
        if (x >= m->npc[i]->bl_x - xs / 2
            && x < m->npc[i]->bl_x - xs / 2 + xs
            && y >= m->npc[i]->bl_y - ys / 2
            && y < m->npc[i]->bl_y - ys / 2 + ys)
            break;
    }
    if (i == m->npc_num)
    {
        if (f)
        {
            if (battle_config.error_log)
                PRINTF("npc_touch_areanpc : some bug \n"_fmt);
        }
        return 1;
    }
    switch (m->npc[i]->npc_subtype)
    {
        case NpcSubtype::WARP:
            skill_stop_dancing(sd, 0);
            pc_setpos(sd, m->npc[i]->is_warp()->warp.name,
                       m->npc[i]->is_warp()->warp.x, m->npc[i]->is_warp()->warp.y, BeingRemoveWhy::GONE);
            break;
        case NpcSubtype::MESSAGE:
            assert (0 && "I'm pretty sure these NPCs are never put on a map."_s);
            break;
        case NpcSubtype::SCRIPT:
        {
            NpcEvent aname;
            aname.npc = m->npc[i]->name;
            aname.label = stringish<ScriptLabel>("OnTouch"_s);

            if (sd->areanpc_id == m->npc[i]->bl_id)
                return 1;

            sd->areanpc_id = m->npc[i]->bl_id;
            if (npc_event(sd, aname, 0) > 0)
                npc_click(sd, m->npc[i]->bl_id);
            break;
        }
    }
    return 0;
}

/*==========================================
 * 近くかどうかの判定
 *------------------------------------------
 */
static
int npc_checknear(dumb_ptr<map_session_data> sd, BlockId id)
{
    dumb_ptr<npc_data> nd;

    nullpo_retz(sd);

    nd = map_id_is_npc(id);
    // this actually happens
    if (nd == nullptr)
        return 1;
    if (nd->bl_type != BL::NPC)
        return 1;

    if (nd->npc_class == NEGATIVE_SPECIES)
        return 0;

    // エリア判定
    if (nd->bl_m != sd->bl_m ||
        nd->bl_x < sd->bl_x - AREA_SIZE - 1
        || nd->bl_x > sd->bl_x + AREA_SIZE + 1
        || nd->bl_y < sd->bl_y - AREA_SIZE - 1
        || nd->bl_y > sd->bl_y + AREA_SIZE + 1)
        return 1;

    return 0;
}

/*==========================================
 * クリック時のNPC処理
 *------------------------------------------
 */
int npc_click(dumb_ptr<map_session_data> sd, BlockId id)
{
    dumb_ptr<npc_data> nd;

    nullpo_retr(1, sd);

    if (sd->npc_id)
    {
        if (battle_config.error_log)
            PRINTF("npc_click: npc_id != 0\n"_fmt);
        return 1;
    }

    if (npc_checknear(sd, id)) {
        clif_scriptclose(sd, id);
        return 1;
    }

    nd = map_id_is_npc(id);

    if (nd->flag & 1)           // 無効化されている
        return 1;

    sd->npc_id = id;
    switch (nd->npc_subtype)
    {
        case NpcSubtype::SHOP:
            clif_npcbuysell(sd, id);
            npc_event_dequeue(sd);
            break;
        case NpcSubtype::SCRIPT:
            sd->npc_pos = run_script(ScriptPointer(nd->is_script()->scr.script.get(), 0), sd->bl_id, id);
            break;
        case NpcSubtype::MESSAGE:
            if (nd->is_message()->message)
            {
                clif_scriptmes(sd, id, nd->is_message()->message);
                clif_scriptclose(sd, id);
            }
            break;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int npc_scriptcont(dumb_ptr<map_session_data> sd, BlockId id)
{
    dumb_ptr<npc_data> nd;

    nullpo_retr(1, sd);

    if (id != sd->npc_id)
        return 1;
    if (npc_checknear(sd, id))
    {
        clif_scriptclose(sd, id);
        return 1;
    }

    nd = map_id_is_npc(id);

    if (!nd /* NPC was disposed? */  || nd->npc_subtype == NpcSubtype::MESSAGE)
    {
        clif_scriptclose(sd, id);
        npc_event_dequeue(sd);
        return 0;
    }

    sd->npc_pos = run_script(ScriptPointer(nd->is_script()->scr.script.get(), sd->npc_pos), sd->bl_id, id);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int npc_buysellsel(dumb_ptr<map_session_data> sd, BlockId id, int type)
{
    dumb_ptr<npc_data> nd;

    nullpo_retr(1, sd);

    if (npc_checknear(sd, id))
        return 1;

    nd = map_id_is_npc(id);
    if (nd->npc_subtype != NpcSubtype::SHOP)
    {
        if (battle_config.error_log)
            PRINTF("no such shop npc : %d\n"_fmt, id);
        sd->npc_id = BlockId();
        return 1;
    }
    if (nd->flag & 1)           // 無効化されている
        return 1;

    sd->npc_shopid = id;
    if (type == 0)
    {
        clif_buylist(sd, nd->is_shop());
    }
    else
    {
        clif_selllist(sd);
    }
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// TODO enumify return type
int npc_buylist(dumb_ptr<map_session_data> sd,
        const std::vector<Packet_Repeat<0x00c8>>& item_list)
{
    dumb_ptr<npc_data> nd;
    double z;
    int i, j, w, itemamount = 0, new_stacks = 0;

    nullpo_retr(3, sd);

    if (npc_checknear(sd, sd->npc_shopid))
        return 3;

    nd = map_id_is_npc(sd->npc_shopid);
    if (nd->npc_subtype != NpcSubtype::SHOP)
        return 3;

    for (i = 0, w = 0, z = 0; i < item_list.size(); i++)
    {
        const uint16_t& item_l_count = item_list[i].count;
        const ItemNameId& item_l_id = item_list[i].name_id;

        for (j = 0; j < nd->is_shop()->shop_items.size(); j++)
        {
            if (nd->is_shop()->shop_items[j].nameid == item_l_id)
                break;
        }
        if (j == nd->is_shop()->shop_items.size())
            return 3;

        z += static_cast<double>(nd->is_shop()->shop_items[j].value) * item_l_count;
        itemamount += item_l_count;

        switch (pc_checkadditem(sd, item_l_id, item_l_count))
        {
            case ADDITEM::EXIST:
                break;
            case ADDITEM::NEW:
                if (itemdb_isequip(item_l_id))
                    new_stacks += item_l_count;
                else
                    new_stacks++;
                break;
            case ADDITEM::OVERAMOUNT:
                return 2;
        }

        w += itemdb_weight(item_l_id) * item_l_count;
    }

    if (z > static_cast<double>(sd->status.zeny))
        return 1;               // zeny不足
    if (w + sd->weight > sd->max_weight)
        return 2;               // 重量超過
    if (pc_inventoryblank(sd) < new_stacks)
        return 3;               // 種類数超過
    if (sd->trade_partner)
        return 4;               // cant buy while trading

    pc_payzeny(sd, static_cast<int>(z));

    for (i = 0; i < item_list.size(); i++)
    {
        const uint16_t& item_l_count = item_list[i].count;
        const ItemNameId& item_l_id = item_list[i].name_id;

        struct item_data *item_data;
        if ((item_data = itemdb_exists(item_l_id)) != nullptr)
        {
            int amount = item_l_count;
            Item item_tmp {};

            item_tmp.nameid = item_data->nameid;

            if (amount > 1
                && (item_data->type == ItemType::WEAPON
                    || item_data->type == ItemType::ARMOR
                    || item_data->type == ItemType::_7
                    || item_data->type == ItemType::_8))
            {
                for (j = 0; j < amount; j++)
                {
                    pc_additem(sd, &item_tmp, 1);
                }
            }
            else
            {
                pc_additem(sd, &item_tmp, amount);
            }
        }
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int npc_selllist(dumb_ptr<map_session_data> sd,
        const std::vector<Packet_Repeat<0x00c9>>& item_list)
{
    double z;
    int i, itemamount = 0;

    nullpo_retr(1, sd);

    if (npc_checknear(sd, sd->npc_shopid))
        return 1;
    for (i = 0, z = 0; i < item_list.size(); i++)
    {
        if (!item_list[i].ioff2.ok())
            return 1;
        ItemNameId nameid = sd->status.inventory[item_list[i].ioff2.unshift()].nameid;
        if (!nameid ||
            sd->status.inventory[item_list[i].ioff2.unshift()].amount < item_list[i].count)
            return 1;
        if (sd->trade_partner)
            return 2;           // cant sell while trading
        z += static_cast<double>(itemdb_value_sell(nameid)) * item_list[i].count;
        itemamount += item_list[i].count;
    }

    if (z > MAX_ZENY)
        z = MAX_ZENY;
    pc_getzeny(sd, static_cast<int>(z));
    for (i = 0; i < item_list.size(); i++)
    {
        IOff0 item_id = item_list[i].ioff2.unshift();
        pc_delitem(sd, item_id, item_list[i].count, 0);
    }

    return 0;

}

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

    map_local *m = map_mapname2mapid(mapname);

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
    nd->speed = std::chrono::milliseconds(200);
    nd->option = Option::ZERO;
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
    struct item_data *id = nullptr;
    if (extract(name_or_id, &itv->nameid) && itv->nameid)
        goto return_true;

    id = itemdb_searchname(name_or_id.rstrip());
    if (id == nullptr)
        return false;
    itv->nameid = id->nameid;
    goto return_true;

return_true:
    if (itv->value < 0)
    {
        if (id == nullptr)
            id = itemdb_search(itv->nameid);
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
    map_local *m = map_mapname2mapid(mapname);

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
    nd->speed = std::chrono::milliseconds(200);
    nd->option = Option::ZERO;
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
    map_local *m;
    int xs = 0, ys = 0;   // [Valaris] thanks to fov
    Species npc_class;
    MapName mapname;
    std::unique_ptr<const ScriptBuffer> script = nullptr;
    dumb_ptr<npc_data_script> nd;
    int evflag = 0;

    if (w1 == "-"_s)
    {
        x = 0;
        y = 0;
        m = nullptr;
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
        m = map_mapname2mapid(mapname);
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

    if (m == nullptr)
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

    if (npc_class == NEGATIVE_SPECIES && m != nullptr)
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
    nd->speed = std::chrono::milliseconds(200);
    nd->scr.script = std::move(script);
    nd->option = Option::ZERO;
    nd->opt1 = Opt1::ZERO;
    nd->opt2 = Opt2::ZERO;
    nd->opt3 = Opt3::ZERO;

    npc_script++;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::SCRIPT;
    if (m != nullptr)
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

    map_local *m = map_mapname2mapid(mapname);

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

    map_local *m = map_mapname2mapid(mapname);
    if (m == nullptr)
        return 1;

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

dumb_ptr<npc_data> npc_spawn_text(map_local *m, int x, int y,
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
    retval->speed = std::chrono::milliseconds(200);

    clif_spawnnpc(retval);
    map_addblock(retval);
    map_addiddb(retval);
    register_npc_name(retval);

    return retval;
}

static
void npc_free_internal(dumb_ptr<npc_data> nd_)
{
    if (nd_->npc_subtype == NpcSubtype::SCRIPT)
    {
        dumb_ptr<npc_data_script> nd = nd_->is_script();
        nd->scr.timer_eventv.clear();

        {
            nd->scr.script.reset();
            nd->scr.label_listv.clear();
        }
    }
    else if (nd_->npc_subtype == NpcSubtype::MESSAGE)
    {
        dumb_ptr<npc_data_message> nd = nd_->is_message();
        nd->message = AString();
    }
    nd_.delete_();
}

static
void npc_propagate_update(dumb_ptr<npc_data> nd)
{
    int xs = 0, ys = 0;
    if (dumb_ptr<npc_data_script> nd_ = nd->is_script())
    {
        xs = nd_->scr.xs;
        ys = nd_->scr.ys;
    }
    map_foreachinarea(std::bind(npc_enable_sub, ph::_1, nd),
            nd->bl_m,
            nd->bl_x - xs, nd->bl_y - ys,
            nd->bl_x + xs, nd->bl_y + ys,
            BL::PC);
}

void npc_free(dumb_ptr<npc_data> nd)
{
    clif_clearchar(nd, BeingRemoveWhy::GONE);
    npc_propagate_update(nd);
    map_deliddb(nd);
    map_delblock(nd);
    npc_free_internal(nd);
}

/*==========================================
 * npc初期化
 *------------------------------------------
 */
bool do_init_npc(void)
{
    bool rv = true;
    // other fields unused
    ev_tm_b.tm_min = -1;
    ev_tm_b.tm_hour = -1;
    ev_tm_b.tm_mday = -1;

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
                map_local *m = map_mapname2mapid(mapname);
                if (m == nullptr)
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
                npc_parse_warp(w1, w2, npcname, w4z);
            }
            else if (w2 == "shop"_s)
            {
                NpcName npcname = stringish<NpcName>(w3);
                npc_parse_shop(w1, w2, npcname, w4z);
            }
            else if (w2 == "script"_s)
            {
                if (w1 == "function"_s)
                {
                    npc_parse_function(w1, w2, w3, w4z,
                            w4x, fp, &lines);
                }
                else
                {
                    NpcName npcname = stringish<NpcName>(w3);
                    npc_parse_script(w1, w2, npcname, w4z,
                            w4x, fp, &lines);
                }
            }
            else if (w2 == "monster"_s)
            {
                MobName mobname = stringish<MobName>(w3);
                npc_parse_mob(w1, w2, mobname, w4z);
            }
            else if (w2 == "mapflag"_s)
            {
                npc_parse_mapflag(w1, w2, w3, w4z);
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
