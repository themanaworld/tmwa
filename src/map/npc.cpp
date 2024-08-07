#include "npc-internal.hpp"
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
#include "../io/extract.hpp"

#include "../net/timer.hpp"

#include "../proto2/map-user.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "pc.hpp"
#include "script-call.hpp"
#include "skill.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
static const std::vector<ByteCode> fake_buffer;
static const ScriptBuffer& fake_script = reinterpret_cast<const ScriptBuffer&>(fake_buffer);

static
Borrowed<const ScriptBuffer> script_or_parent(dumb_ptr<npc_data_script> nd)
{
    return borrow(nd->scr.parent ? fake_script : *nd->scr.script);
}

BlockId npc_get_new_npc_id(void)
{
    BlockId rv = npc_id;
    npc_id = next(npc_id);
    return rv;
}

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
    }
    else if (!(nd->flag & 1))
    {                           // 無効化
        clif_clearchar(nd, BeingRemoveWhy::GONE);
        nd->flag |= 1;
    }

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
 * NPC Spells Events
 *------------------------------------------
 */
static
NpcEvent spell_event2id(RString name)
{
    return spells_by_events.get(name);
}

/*==========================================
 * Spell Toknise
 * Return a pair of strings, {spellname, parameter}
 * Parameter may be empty.
 *------------------------------------------
 */
static
std::pair<XString, XString> magic_tokenise(XString src)
{
    auto seeker = std::find(src.begin(), src.end(), ' ');

    if (seeker == src.end())
    {
        return {src, XString()};
    }
    else
    {
        XString rv1 = src.xislice_h(seeker);
        ++seeker;

        while (seeker != src.end() && *seeker == ' ')
            ++seeker;

        // Note: this very well could be empty
        XString rv2 = src.xislice_t(seeker);
        return {rv1, rv2};
    }
}

/*==========================================
 * NPC Spell
 *------------------------------------------
 */
int magic_message(dumb_ptr<map_session_data> caster, XString source_invocation)
{
    auto pair = magic_tokenise(source_invocation);
    // Spell Cast
    NpcEvent spell_event = spell_event2id(pair.first);

    RString spell_params = pair.second;

    if (spell_event.npc)
    {
        dumb_ptr<npc_data> nd = npc_name2id(spell_event.npc);

        if (nd)
        {
            argrec_t arg[1] =
            {
                {"@args$"_s, spell_params},
            };

            npc_event(caster, spell_event, 0, arg);
            return 1;
        }
    }
    return 0;
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
        if (!pc_addeventtimer(sd, 100_ms, sd->eventqueuel.front()))
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
        if (ev->nd->scr.parent != BlockId())
            return; // temporary npcs only respond to commands directly issued to them
        run_script_l(ScriptPointer(script_or_parent(ev->nd), ev->pos), rid, ev->nd->bl_id,
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

    Timer(gettick() + 100_ms,
            npc_event_do_clock,
            1_s
    ).detach();

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void npc_eventtimer(TimerData *, tick_t, BlockId, NpcEvent data)
{
    Option<P<struct event_data>> ev_ = ev_db.search(data);
    dumb_ptr<npc_data_script> nd;

    if (ev_.is_none() && data.label == stringish<ScriptLabel>("OnTouch"_s))
        return;

    P<struct event_data> ev = TRY_UNWRAP(ev_,
    {
        if (battle_config.error_log)
            PRINTF("npc_event: event not found [%s]\n"_fmt,
                    data);
        return;
    });
    if ((nd = ev->nd) == nullptr || nd->deletion_pending != npc_data::NOT_DELETING)
    {
        if (battle_config.error_log)
            PRINTF("npc_event: event not found [%s]\n"_fmt,
                    data);
        return;
    }

    if (nd->scr.parent && map_id2bl(nd->scr.parent) == nullptr)
    {
        npc_free(nd);
        return;
    }

    run_script(ScriptPointer(script_or_parent(nd), ev->pos), BlockId(), nd->bl_id);
}

/*==========================================
 *
 *------------------------------------------
 */
int npc_addeventtimer(dumb_ptr<block_list> bl, interval_t tick, NpcEvent name)
{
    int i;

    nullpo_retz(bl);
    if (bl->bl_type == BL::NPC)
    {
        dumb_ptr<npc_data> nd = bl->is_npc();
        for (i = 0; i < MAX_EVENTTIMER; i++)
            if (!nd->eventtimer[i])
                break;

        if (i < MAX_EVENTTIMER)
        {
            nd->eventtimer[i] = Timer(gettick() + tick,
                    std::bind(npc_eventtimer, ph::_1, ph::_2,
                        nd->bl_id, name));
            return 1;
        }
    }
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

    if (nd->scr.parent && map_id2bl(nd->scr.parent) == nullptr)
    {
        npc_free(nd);
        return;
    }

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

    run_script(ScriptPointer(script_or_parent(nd), te->pos), BlockId(), nd->bl_id);
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
        int mob_kill, Slice<argrec_t> args)
{
    if (!eventname.npc)
    {
        return npc_event_doall_l(eventname.label, sd->bl_id, args); // XXX maybe merge this into npc_event?
    }

    if (eventname.npc.front() == '~')
        return 0; // phony event (used mostly to uniquely identify mob spawns)

    Option<P<struct event_data>> ev_ = ev_db.search(eventname);
    dumb_ptr<npc_data_script> nd;

    if (ev_.is_none() && eventname.label == stringish<ScriptLabel>("OnTouch"_s))
        return 1;

    bool failed = false;
    struct event_data ev {};
    P<struct event_data> ev2 = TRY_UNWRAP(ev_,{ failed = true; });
    if(failed)
    {
        if (!eventname.label && eventname.npc && sd)
        {
            dumb_ptr<npc_data> fnd = npc_name2id(eventname.npc);
            if (fnd == nullptr)
            {
                PRINTF("npc_event: NPC not found when calling event [%s]\n"_fmt,
                        eventname.npc);
                return 0;
            }
            ev.nd = fnd->is_script();
            ev.pos = 0; // start from the beginning of a npc
        }
        else
        {
            if (!mob_kill && battle_config.error_log)
                PRINTF("npc_event: event not found [%s]\n"_fmt,
                        eventname);
            return 0;
        }
    }
    else
    {
        ev.nd = ev2->nd;
        ev.pos = ev2->pos;
    }

    if ((nd = ev.nd) == nullptr || nd->deletion_pending != npc_data::NOT_DELETING)
    {
        if (!mob_kill && battle_config.error_log)
            PRINTF("npc_event: event not found [%s]\n"_fmt,
                    eventname);
        return 0;
    }

    if (nd->scr.parent && map_id2bl(nd->scr.parent) == nullptr)
    {
        npc_free(nd);
        return 0;
    }

    if (sd)
    {
        if (nd->scr.event_needs_map)
        {
            int xs = nd->scr.xs;
            int ys = nd->scr.ys;
            if (nd->bl_m != sd->bl_m)
                return 1;
            if (xs > 0
                && (sd->bl_x < nd->bl_x - xs / 2 || nd->bl_x + xs / 2 < sd->bl_x))
                return 1;
            if (ys > 0
                && (sd->bl_y < nd->bl_y - ys / 2 || nd->bl_y + ys / 2 < sd->bl_y))
                return 1;
        }

        if (sd->npc_id && map_id_is_npc(sd->npc_id) == nullptr)
            npc_event_dequeue(sd); // the NPC was previously freed, so we detach it

        if (sd->npc_id && sd->npc_pos > -1 && args.size() < 1) // if called from a timer we process async, otherwise sync
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
    }
    int pos = run_script_l(ScriptPointer(script_or_parent(nd), ev.pos),
                            (sd? sd->bl_id : BlockId()), nd->bl_id, args);
    if (sd)
        sd->npc_pos = pos;
    return 0;
}

/*==========================================
 * 接触型のNPC処理
 *------------------------------------------
 */
int npc_touch_areanpc(dumb_ptr<map_session_data> sd, Borrowed<map_local> m, int x, int y)
{
    int i;
    int xs, ys;

    nullpo_retr(1, sd);

    if (sd->npc_id)
        return 1;

    for (i = 0; i < m->npc_num; i++)
    {
        if (m->npc[i] == nullptr)
            continue;

        if (m->npc[i]->flag & 1)
            continue;

        switch (m->npc[i]->npc_subtype)
        {
            case NpcSubtype::WARP:
                xs = m->npc[i]->is_warp()->warp.xs;
                ys = m->npc[i]->is_warp()->warp.ys;
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
        return 1;
    }
    switch (m->npc[i]->npc_subtype)
    {
        case NpcSubtype::WARP:
            skill_stop_dancing(sd, 0);
            pc_setpos(sd, m->npc[i]->is_warp()->warp.name,
                       m->npc[i]->is_warp()->warp.x, m->npc[i]->is_warp()->warp.y, BeingRemoveWhy::GONE);
            break;
        case NpcSubtype::SCRIPT:
        {
            NpcEvent aname;
            aname.npc = m->npc[i]->name;
            aname.label = stringish<ScriptLabel>("OnTouch"_s);

            if (sd->areanpc_id == m->npc[i]->bl_id)
                return 2;

            sd->areanpc_id = m->npc[i]->bl_id;
            if (npc_event(sd, aname, 0) > 0)
                npc_click(sd, m->npc[i]->bl_id);
            return 2;
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

    if (nd->npc_class == INVISIBLE_CLASS)
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
        if (sd->npc_id && map_id_is_npc(sd->npc_id) == nullptr)
            npc_event_dequeue(sd); // the NPC was previously freed, so we detach it
        else
        {
            if (battle_config.error_log)
                PRINTF("npc_click: npc_id != 0\n"_fmt);
            return 1;
        }
    }

    if (npc_checknear(sd, id))
    {
        clif_scriptclose(sd, id);
        return 1;
    }

    nd = map_id_is_npc(id);

    // If someone clicked on an NPC that is about to no longer exist, then
    // release them
    if (nd->deletion_pending != npc_data::NOT_DELETING)
    {
        clif_scriptclose(sd, id);
        return 1;
    }

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
            dumb_ptr<npc_data_script> nds = nd->is_script();
            if (nds->scr.parent && map_id2bl(nds->scr.parent) == nullptr)
            {
                npc_free(nds);
                return 1;
            }
            sd->npc_pos = run_script(ScriptPointer(script_or_parent(nds), 0), sd->bl_id, id);
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

    // If the NPC is about to be deleted, release the PC
    if (nd->deletion_pending != npc_data::NOT_DELETING)
    {
        clif_scriptclose(sd, id);
        npc_event_dequeue(sd);
        return 1;
    }

    if (nd->is_script()->scr.parent &&
        map_id2bl(nd->is_script()->scr.parent) == nullptr)
    {
        npc_free(nd);
        return 1;
    }

    sd->npc_pos = run_script(ScriptPointer(script_or_parent(nd->is_script()), sd->npc_pos), sd->bl_id, id);

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

        P<struct item_data> item_data = TRY_UNWRAP(itemdb_exists(item_l_id), continue);
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

        OMATCH_BEGIN_SOME (sdidn, sd->inventory_data[item_list[i].ioff2.unshift()])
        {
            GmLevel gmlvl = pc_isGM(sd);
            if (bool(sdidn->mode & ItemMode::NO_SELL_TO_NPC) && gmlvl.get_all_bits() < 60)
            {
                //clif_displaymessage(sd->sess, "This item can't be sold to an NPC."_s);
                // M+ already outputs "Unable to sell unsellable item." on return value 3.
                return 3;
            }
        }
        OMATCH_END ();

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

static
void npc_free_internal(dumb_ptr<npc_data> nd_)
{
    for (int i = 0; i < MAX_EVENTTIMER; i++)
    {
        nd_->eventtimer[i].cancel();
    }

    if (nd_->npc_subtype == NpcSubtype::SCRIPT)
    {
        dumb_ptr<npc_data_script> nd = nd_->is_script();
        nd->scr.timerid.cancel();
        nd->scr.timer_eventv.clear();

        // destroy all children (puppets), if any
        if (nd_->name && nd->scr.parent == BlockId())
        {
            for (auto& pair : npcs_by_name)
                if (pair.second->npc_subtype == NpcSubtype::SCRIPT
                    && pair.second->is_script()->scr.parent == nd_->bl_id)
                        npc_free(pair.second);
        }

        nd->scr.script.reset();
        nd->scr.label_listv.clear();
    }
    if (nd_->name)
        npcs_by_name.put(nd_->name, nullptr);

    if (nd_->bl_m != borrow(undefined_gat)) {
        nd_->bl_m->npc[nd_->n] = nullptr;
    }

    // Also clean up any events we registered to the global ev_db
    if (auto nd = nd_->is_script())
    {
        std::vector<NpcEvent> to_erase;
        for (auto& pair : ev_db)
        {
            if (pair.second.nd == nd)
            {
                to_erase.push_back(pair.first);
            }
        }
        for (auto& key : to_erase)
        {
            ev_db.erase(key);
        }
    }

    nd_.delete_();
}

static
void npc_propagate_update(dumb_ptr<npc_data> nd)
{
    if (nd->bl_m == borrow(undefined_gat))
        return;

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
    if (nd == nullptr || nd->deletion_pending == npc_data::DELETION_ACTIVE)
        return;

    nd->deletion_pending = npc_data::DELETION_ACTIVE;
    nd->flag |= 1;
    clif_clearchar(nd, BeingRemoveWhy::GONE);
    npc_propagate_update(nd);
    map_deliddb(nd);
    map_delblock(nd);
    npc_free_internal(nd);
}
} // namespace map
} // namespace tmwa
