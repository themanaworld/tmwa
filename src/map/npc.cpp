#include "npc.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <list>

#include "../common/cxxstdio.hpp"
#include "../common/db.hpp"
#include "../common/nullpo.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"

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
std::list<std::string> npc_srcs;

static
int npc_id = START_NPC_NUM;
static
int npc_warp, npc_shop, npc_script, npc_mob;

int npc_get_new_npc_id(void)
{
    return npc_id++;
}

struct event_data
{
    dumb_ptr<npc_data_script> nd;
    int pos;
};
static
Map<std::string, struct event_data> ev_db;
static
DMap<std::string, dumb_ptr<npc_data>> npcname_db;

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
    char aname[50] {};

    nullpo_retv(bl);

    assert (bl->bl_type == BL::PC);
    {
        sd = bl->as_player();

        // not if disabled
        if (nd->flag & 1)
            return;

        strzcpy(aname, nd->name, sizeof(nd->name));
        if (sd->areanpc_id == nd->bl_id)
            return;
        sd->areanpc_id = nd->bl_id;
        strcat(aname, "::OnTouch");
        npc_event(sd, aname, 0);
    }
}

int npc_enable(const char *name, bool flag)
{
    dumb_ptr<npc_data> nd = npcname_db.get(name);
    if (nd == NULL)
        return 0;

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
dumb_ptr<npc_data> npc_name2id(const char *name)
{
    return npcname_db.get(name);
}

/*==========================================
 * イベントキューのイベント処理
 *------------------------------------------
 */
int npc_event_dequeue(dumb_ptr<map_session_data> sd)
{
    nullpo_ret(sd);

    sd->npc_id = 0;

    if (!sd->eventqueuel.empty())
    {
        if (!pc_addeventtimer(sd, std::chrono::milliseconds(100), sd->eventqueuel.front().c_str()))
        {
            PRINTF("npc_event_dequeue(): Event timer is full.\n");
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

    if (nd->bl_prev == NULL)
        return 1;

    clif_clearchar(nd, BeingRemoveWhy::DEAD);
    map_delblock(nd);
    return 0;
}

int npc_timer_event(const char *eventname) // Added by RoVeRT
{
    struct event_data *ev = ev_db.search(eventname);
    dumb_ptr<npc_data_script> nd;
//  int xs,ys;

    if ((ev == NULL || (nd = ev->nd) == NULL))
    {
        PRINTF("npc_event: event not found [%s]\n", eventname);
        return 0;
    }

    run_script(ScriptPointer(nd->scr.script.get(), ev->pos), nd->bl_id, nd->bl_id);

    return 0;
}

/*==========================================
 * 全てのNPCのOn*イベント実行
 *------------------------------------------
 */
static
void npc_event_doall_sub(const std::string& key, struct event_data *ev,
        int *c, const char *name, int rid, int argc, argrec_t *argv)
{
    const char *p = key.c_str();

    nullpo_retv(ev);

    if ((p = strchr(p, ':')) && p && strcasecmp(name, p) == 0)
    {
        run_script_l(ScriptPointer(ev->nd->scr.script.get(), ev->pos), rid, ev->nd->bl_id, argc,
                      argv);
        (*c)++;
    }
}

int npc_event_doall_l(const char *name, int rid, int argc, argrec_t *args)
{
    int c = 0;
    char buf[64] = "::";

    strzcpy(buf + 2, name, 62);
    for (auto& pair : ev_db)
        npc_event_doall_sub(pair.first, &pair.second, &c, buf, rid, argc, args);
    return c;
}

static
void npc_event_do_sub(const std::string& key, struct event_data *ev,
        int *c, const char *name, int rid, int argc, argrec_t *argv)
{
    const char *p = key.c_str();

    nullpo_retv(ev);

    if (p && strcasecmp(name, p) == 0)
    {
        run_script_l(ScriptPointer(ev->nd->scr.script.get(), ev->pos), rid, ev->nd->bl_id, argc,
                      argv);
        (*c)++;
    }
}

int npc_event_do_l(const char *name, int rid, int argc, argrec_t *args)
{
    int c = 0;

    if (*name == ':' && name[1] == ':')
    {
        return npc_event_doall_l(name + 2, rid, argc, args);
    }

    for (auto& pair : ev_db)
        npc_event_do_sub(pair.first, &pair.second, &c, name, rid, argc, args);
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

    if (t.tm_min != ev_tm_b.tm_min)
    {
        std::string buf;
        buf = STRPRINTF("OnMinute%02d", t.tm_min);
        npc_event_doall(buf.c_str());
        buf = STRPRINTF("OnClock%02d%02d", t.tm_hour, t.tm_min);
        npc_event_doall(buf.c_str());
    }
    if (t.tm_hour != ev_tm_b.tm_hour)
    {
        std::string buf;
        buf = STRPRINTF("OnHour%02d", t.tm_hour);
        npc_event_doall(buf.c_str());
    }
    if (t.tm_mday != ev_tm_b.tm_mday)
    {
        std::string buf;
        buf = STRPRINTF("OnDay%02d%02d", t.tm_mon + 1, t.tm_mday);
        npc_event_doall(buf.c_str());
    }
    ev_tm_b = t;
}

/*==========================================
 * OnInitイベント実行(&時計イベント開始)
 *------------------------------------------
 */
int npc_event_do_oninit(void)
{
    int c = npc_event_doall("OnInit");
    PRINTF("npc: OnInit Event done. (%d npc)\n", c);

    Timer(gettick() + std::chrono::milliseconds(100),
            npc_event_do_clock,
            std::chrono::seconds(1)
    ).detach();

    return 0;
}

/*==========================================
 * タイマーイベント実行
 *------------------------------------------
 */
static
void npc_timerevent(TimerData *, tick_t tick, int id, interval_t data)
{
    dumb_ptr<npc_data_script> nd = map_id2bl(id)->as_npc()->as_script();
    assert (nd != NULL);
    assert (nd->npc_subtype == NpcSubtype::SCRIPT);
    assert (nd->scr.nexttimer != nd->scr.timer_eventv.end());

    nd->scr.timertick = tick;
    auto te = nd->scr.nexttimer;
    // nd->scr.timerid = nullptr;

    // er, isn't this the same as nd->scr.timer = te->timer?
    interval_t t = nd->scr.timer += data;
    assert (t == te->timer);
    ++nd->scr.nexttimer;
    if (nd->scr.nexttimer != nd->scr.timer_eventv.end())
    {
        interval_t next = nd->scr.nexttimer->timer - t;
        nd->scr.timerid = Timer(tick + next,
                std::bind(npc_timerevent, ph::_1, ph::_2,
                    id, next));
    }

    run_script(ScriptPointer(nd->scr.script.get(), te->pos), 0, nd->bl_id);
}

/*==========================================
 * タイマーイベント開始
 *------------------------------------------
 */
void npc_timerevent_start(dumb_ptr<npc_data_script> nd)
{
    nullpo_retv(nd);

    if (nd->scr.timer_eventv.empty())
        return;
    if (nd->scr.nexttimer != nd->scr.timer_eventv.end())
        return;
    if (nd->scr.timer == nd->scr.timer_eventv.back().timer)
        return;

    npc_timerevent_list phony {};
    phony.timer = nd->scr.timer;

    // find the first element such that el.timer > phony.timer;
    auto jt = std::upper_bound(nd->scr.timer_eventv.begin(), nd->scr.timer_eventv.end(), phony,
            [](const npc_timerevent_list& l, const npc_timerevent_list& r)
            {
                return l.timer < r.timer;
            }
    );
    nd->scr.nexttimer = jt;
    nd->scr.timertick = gettick();

    if (jt == nd->scr.timer_eventv.end())
        // shouldn't happen?
        return;

    interval_t next = jt->timer - nd->scr.timer;
    nd->scr.timerid = Timer(gettick() + next,
            std::bind(npc_timerevent, ph::_1, ph::_2,
                nd->bl_id, next));
}

/*==========================================
 * タイマーイベント終了
 *------------------------------------------
 */
void npc_timerevent_stop(dumb_ptr<npc_data_script> nd)
{
    nullpo_retv(nd);

    if (nd->scr.nexttimer != nd->scr.timer_eventv.end())
    {
        nd->scr.nexttimer = nd->scr.timer_eventv.end();
        nd->scr.timer += gettick() - nd->scr.timertick;
        nd->scr.timerid.cancel();
    }
}

/*==========================================
 * タイマー値の所得
 *------------------------------------------
 */
interval_t npc_gettimerevent_tick(dumb_ptr<npc_data_script> nd)
{
    nullpo_retr(interval_t::zero(), nd);

    interval_t tick = nd->scr.timer;

    // Couldn't we just check the truthiness of the timer?
    // Or would that be affected by the (new!) detach logic?
    // Of course, you'd be slightly crazy to check the tick when you are
    // called with it.
    if (nd->scr.nexttimer != nd->scr.timer_eventv.end())
        tick += gettick() - nd->scr.timertick;
    return tick;
}

/*==========================================
 * タイマー値の設定
 *------------------------------------------
 */
void npc_settimerevent_tick(dumb_ptr<npc_data_script> nd, interval_t newtimer)
{
    nullpo_retv(nd);

    bool flag = nd->scr.nexttimer != nd->scr.timer_eventv.end();

    npc_timerevent_stop(nd);
    nd->scr.timer = newtimer;
    if (flag)
        npc_timerevent_start(nd);
}

/*==========================================
 * イベント型のNPC処理
 *------------------------------------------
 */
int npc_event(dumb_ptr<map_session_data> sd, const char *eventname,
               int mob_kill)
{
    struct event_data *ev = ev_db.search(eventname);
    dumb_ptr<npc_data_script> nd;
    int xs, ys;
    char mobevent[100];

    if (sd == NULL)
    {
        PRINTF("npc_event nullpo?\n");
    }

    if (ev == NULL && eventname
        && strcmp(((eventname) + strlen(eventname) - 9), "::OnTouch") == 0)
        return 1;

    if (ev == NULL || (nd = ev->nd) == NULL)
    {
        if (mob_kill && (ev == NULL || (nd = ev->nd) == NULL))
        {
            strcpy(mobevent, eventname);
            strcat(mobevent, "::OnMyMobDead");
            ev = ev_db.search(mobevent);
            if (ev == NULL || (nd = ev->nd) == NULL)
            {
                if (strncasecmp(eventname, "GM_MONSTER", 10) != 0)
                    PRINTF("npc_event: event not found [%s]\n", mobevent);
                return 0;
            }
        }
        else
        {
            if (battle_config.error_log)
                PRINTF("npc_event: event not found [%s]\n", eventname);
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

    if (sd->npc_id != 0)
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
void npc_command_sub(const std::string& key, struct event_data *ev, const char *npcname, const char *command)
{
    const char *p = key.c_str();
    char temp[100];

    if (strcmp(ev->nd->name, npcname) == 0 && (p = strchr(p, ':')) && p
        && strncasecmp("::OnCommand", p, 10) == 0)
    {
        sscanf(&p[11], "%s", temp);

        if (strcmp(command, temp) == 0)
            run_script(ScriptPointer(ev->nd->scr.script.get(), ev->pos), 0, ev->nd->bl_id);
    }
}

int npc_command(dumb_ptr<map_session_data>, const char *npcname, const char *command)
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
                xs = m->npc[i]->as_warp()->warp.xs;
                ys = m->npc[i]->as_warp()->warp.ys;
                break;
            case NpcSubtype::MESSAGE:
                assert (0 && "I'm pretty sure these are never put on a map");
                xs = 0;
                ys = 0;
                break;
            case NpcSubtype::SCRIPT:
                xs = m->npc[i]->as_script()->scr.xs;
                ys = m->npc[i]->as_script()->scr.ys;
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
                PRINTF("npc_touch_areanpc : some bug \n");
        }
        return 1;
    }
    switch (m->npc[i]->npc_subtype)
    {
        case NpcSubtype::WARP:
            skill_stop_dancing(sd, 0);
            pc_setpos(sd, m->npc[i]->as_warp()->warp.name,
                       m->npc[i]->as_warp()->warp.x, m->npc[i]->as_warp()->warp.y, BeingRemoveWhy::GONE);
            break;
        case NpcSubtype::MESSAGE:
            assert (0 && "I'm pretty sure these NPCs are never put on a map.");
            break;
        case NpcSubtype::SCRIPT:
        {
            char aname[50] {};
            strzcpy(aname, m->npc[i]->name, 24);

            if (sd->areanpc_id == m->npc[i]->bl_id)
                return 1;

            sd->areanpc_id = m->npc[i]->bl_id;
            strcat(aname, "::OnTouch");
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
int npc_checknear(dumb_ptr<map_session_data> sd, int id)
{
    dumb_ptr<npc_data> nd;

    nullpo_ret(sd);

    nd = map_id_as_npc(id);
    assert (nd != NULL);
    assert (nd->bl_type == BL::NPC);

    if (nd->npc_class < 0)          // イベント系は常にOK
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
int npc_click(dumb_ptr<map_session_data> sd, int id)
{
    dumb_ptr<npc_data> nd;

    nullpo_retr(1, sd);

    if (sd->npc_id != 0)
    {
        if (battle_config.error_log)
            PRINTF("npc_click: npc_id != 0\n");
        return 1;
    }

    if (npc_checknear(sd, id)) {
        clif_scriptclose(sd, id);
        return 1;
    }

    nd = map_id_as_npc(id);

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
            sd->npc_pos = run_script(ScriptPointer(nd->as_script()->scr.script.get(), 0), sd->bl_id, id);
            break;
        case NpcSubtype::MESSAGE:
            if (!nd->as_message()->message.empty())
            {
                clif_scriptmes(sd, id, nd->as_message()->message.c_str());
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
int npc_scriptcont(dumb_ptr<map_session_data> sd, int id)
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

    nd = map_id_as_npc(id);

    if (!nd /* NPC was disposed? */  || nd->npc_subtype == NpcSubtype::MESSAGE)
    {
        clif_scriptclose(sd, id);
        npc_event_dequeue(sd);
        return 0;
    }

    sd->npc_pos = run_script(ScriptPointer(nd->as_script()->scr.script.get(), sd->npc_pos), sd->bl_id, id);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int npc_buysellsel(dumb_ptr<map_session_data> sd, int id, int type)
{
    dumb_ptr<npc_data> nd;

    nullpo_retr(1, sd);

    if (npc_checknear(sd, id))
        return 1;

    nd = map_id_as_npc(id);
    if (nd->npc_subtype != NpcSubtype::SHOP)
    {
        if (battle_config.error_log)
            PRINTF("no such shop npc : %d\n", id);
        sd->npc_id = 0;
        return 1;
    }
    if (nd->flag & 1)           // 無効化されている
        return 1;

    sd->npc_shopid = id;
    if (type == 0)
    {
        clif_buylist(sd, nd->as_shop());
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
int npc_buylist(dumb_ptr<map_session_data> sd, int n,
        const uint16_t *item_list)
{
    dumb_ptr<npc_data> nd;
    double z;
    int i, j, w, itemamount = 0, new_stacks = 0;

    nullpo_retr(3, sd);
    nullpo_retr(3, item_list);

    if (npc_checknear(sd, sd->npc_shopid))
        return 3;

    nd = map_id_as_npc(sd->npc_shopid);
    if (nd->npc_subtype != NpcSubtype::SHOP)
        return 3;

    for (i = 0, w = 0, z = 0; i < n; i++)
    {
        for (j = 0; j < nd->as_shop()->shop_items.size(); j++)
        {
            if (nd->as_shop()->shop_items[j].nameid == item_list[i * 2 + 1])
                break;
        }
        if (j == nd->as_shop()->shop_items.size())
            return 3;

        z += static_cast<double>(nd->as_shop()->shop_items[j].value) * item_list[i * 2];
        itemamount += item_list[i * 2];

        switch (pc_checkadditem(sd, item_list[i * 2 + 1], item_list[i * 2]))
        {
            case ADDITEM::EXIST:
                break;
            case ADDITEM::NEW:
                if (itemdb_isequip(item_list[i * 2 + 1]))
                    new_stacks += item_list[i * 2];
                else
                    new_stacks++;
                break;
            case ADDITEM::OVERAMOUNT:
                return 2;
        }

        w += itemdb_weight(item_list[i * 2 + 1]) * item_list[i * 2];
    }

    if (z > static_cast<double>(sd->status.zeny))
        return 1;               // zeny不足
    if (w + sd->weight > sd->max_weight)
        return 2;               // 重量超過
    if (pc_inventoryblank(sd) < new_stacks)
        return 3;               // 種類数超過
    if (sd->trade_partner != 0)
        return 4;               // cant buy while trading

    pc_payzeny(sd, static_cast<int>(z));

    for (i = 0; i < n; i++)
    {
        struct item_data *item_data;
        if ((item_data = itemdb_exists(item_list[i * 2 + 1])) != NULL)
        {
            int amount = item_list[i * 2];
            struct item item_tmp {};

            item_tmp.nameid = item_data->nameid;
            item_tmp.identify = 1;  // npc販売アイテムは鑑定済み

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
int npc_selllist(dumb_ptr<map_session_data> sd, int n,
        const uint16_t *item_list)
{
    double z;
    int i, itemamount = 0;

    nullpo_retr(1, sd);
    nullpo_retr(1, item_list);

    if (npc_checknear(sd, sd->npc_shopid))
        return 1;
    for (i = 0, z = 0; i < n; i++)
    {
        int nameid;
        if (item_list[i * 2] - 2 < 0 || item_list[i * 2] - 2 >= MAX_INVENTORY)
            return 1;
        nameid = sd->status.inventory[item_list[i * 2] - 2].nameid;
        if (nameid == 0 ||
            sd->status.inventory[item_list[i * 2] - 2].amount < item_list[i * 2 + 1])
            return 1;
        if (sd->trade_partner != 0)
            return 2;           // cant sell while trading
        z += static_cast<double>(itemdb_value_sell(nameid)) * item_list[i * 2 + 1];
        itemamount += item_list[i * 2 + 1];
    }

    if (z > MAX_ZENY)
        z = MAX_ZENY;
    pc_getzeny(sd, static_cast<int>(z));
    for (i = 0; i < n; i++)
    {
        int item_id = item_list[i * 2] - 2;
        pc_delitem(sd, item_id, item_list[i * 2 + 1], 0);
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
void npc_addsrcfile(const char *name)
{
    if (strcasecmp(name, "clear") == 0)
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
void npc_delsrcfile(const char *name)
{
    if (strcasecmp(name, "all") == 0)
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

/*==========================================
 * warp行解析
 *------------------------------------------
 */
int npc_parse_warp(const char *w1, const char *, const char *w3, const char *w4)
{
    int x, y, xs, ys, to_x, to_y;
    int i, j;
    char mapname[24], to_mapname[24];
    dumb_ptr<npc_data_warp> nd;

    // 引数の個数チェック
    if (sscanf(w1, "%[^,],%d,%d", mapname, &x, &y) != 3 ||
        sscanf(w4, "%d,%d,%[^,],%d,%d", &xs, &ys, to_mapname, &to_x,
                &to_y) != 5)
    {
        PRINTF("bad warp line : %s\n", w3);
        return 1;
    }

    map_local *m = map_mapname2mapid(mapname);

    nd.new_();
    nd->bl_id = npc_get_new_npc_id();
    nd->n = map_addnpc(m, nd);

    nd->bl_prev = nd->bl_next = NULL;
    nd->bl_m = m;
    nd->bl_x = x;
    nd->bl_y = y;
    nd->dir = DIR::S;
    nd->flag = 0;
    strzcpy(nd->name, w3, 24);
    strzcpy(nd->exname, w3, 24);

    if (!battle_config.warp_point_debug)
        nd->npc_class = WARP_CLASS;
    else
        nd->npc_class = WARP_DEBUG_CLASS;
    nd->speed = std::chrono::milliseconds(200);
    nd->option = Option::ZERO;
    nd->opt1 = Opt1::ZERO;
    nd->opt2 = Opt2::ZERO;
    nd->opt3 = Opt3::ZERO;
    strzcpy(nd->warp.name, to_mapname, 16);
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

//  PRINTF("warp npc %s %d read done\n",mapname,nd->bl_id);
    npc_warp++;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::WARP;
    map_addblock(nd);
    clif_spawnnpc(nd);
    npcname_db.put(nd->name, nd);

    return 0;
}

/*==========================================
 * shop行解析
 *------------------------------------------
 */
static
int npc_parse_shop(char *w1, char *, char *w3, char *w4)
{
    char *p;
    int x, y;
    DIR dir;
    char mapname[24];
    dumb_ptr<npc_data_shop> nd;

    // 引数の個数チェック
    int dir_; // TODO use SSCANF or extract
    if (sscanf(w1, "%[^,],%d,%d,%d", mapname, &x, &y, &dir_) != 4
        || dir_ < 0 || dir_ >= 8
        || strchr(w4, ',') == NULL)
    {
        PRINTF("bad shop line : %s\n", w3);
        return 1;
    }
    dir = static_cast<DIR>(dir_);
    map_local *m = map_mapname2mapid(mapname);

    nd.new_();
    p = strchr(w4, ',');

    while (p)
    {
        int nameid, value;
        char name[24];
        struct item_data *id = NULL;
        p++;
        if (sscanf(p, "%d:%d", &nameid, &value) == 2)
        {
        }
        else if (sscanf(p, "%s :%d", name, &value) == 2)
        {
            id = itemdb_searchname(name);
            if (id == NULL)
                nameid = -1;
            else
                nameid = id->nameid;
        }
        else
            break;

        if (nameid > 0)
        {
            npc_item_list sh_it;
            sh_it.nameid = nameid;
            if (value < 0)
            {
                if (id == NULL)
                    id = itemdb_search(nameid);
                value = id->value_buy * abs(value);

            }
            sh_it.value = value;
            nd->shop_items.push_back(sh_it);
        }
        p = strchr(p, ',');
    }
    if (nd->shop_items.empty())
    {
        nd.delete_();
        return 1;
    }

    nd->bl_prev = nd->bl_next = NULL;
    nd->bl_m = m;
    nd->bl_x = x;
    nd->bl_y = y;
    nd->bl_id = npc_get_new_npc_id();
    nd->dir = dir;
    nd->flag = 0;
    strzcpy(nd->name, w3, 24);
    nd->npc_class = atoi(w4);
    nd->speed = std::chrono::milliseconds(200);
    nd->option = Option::ZERO;
    nd->opt1 = Opt1::ZERO;
    nd->opt2 = Opt2::ZERO;
    nd->opt3 = Opt3::ZERO;

    //PRINTF("shop npc %s %d read done\n",mapname,nd->bl_id);
    npc_shop++;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::SHOP;
    nd->n = map_addnpc(m, nd);
    map_addblock(nd);
    clif_spawnnpc(nd);
    npcname_db.put(nd->name, nd);

    return 0;
}

/*==========================================
 * NPCのラベルデータコンバート
 *------------------------------------------
 */
static
void npc_convertlabel_db(const std::string& lname, int pos, dumb_ptr<npc_data_script> nd)
{
    nullpo_retv(nd);

    struct npc_label_list eln {};
    strzcpy(eln.name, lname.c_str(), sizeof(eln.name));
    eln.pos = pos;
    nd->scr.label_listv.push_back(std::move(eln));
}

/*==========================================
 * script行解析
 *------------------------------------------
 */
static
int npc_parse_script(char *w1, char *w2, char *w3, char *w4,
                      const char *first_line, FILE * fp, int *lines)
{
    int x, y;
    DIR dir = DIR::S;
    map_local *m;
    int xs = 0, ys = 0, npc_class = 0;   // [Valaris] thanks to fov
    char mapname[24];
    std::unique_ptr<const ScriptBuffer> script = NULL;
    dumb_ptr<npc_data_script> nd;
    int evflag = 0;
    char *p;

    if (strcmp(w1, "-") == 0)
    {
        x = 0;
        y = 0;
        m = nullptr;
    }
    else
    {
        // 引数の個数チェック
        int dir_; // TODO use SSCANF or extract
        if (sscanf(w1, "%[^,],%d,%d,%d", mapname, &x, &y, &dir_) != 4
            || dir_ < 0 || dir_ >= 8
            || (strcmp(w2, "script") == 0 && strchr(w4, ',') == NULL))
        {
            PRINTF("bad script line : %s\n", w3);
            return 1;
        }
        dir = static_cast<DIR>(dir_);
        m = map_mapname2mapid(mapname);
    }

    if (strcmp(w2, "script") == 0)
    {
        // may be empty
        std::string srcbuf = strchrnul(first_line, '{');
        // Note: it was a bug that this was missing. I think.
        int startline = *lines;

        while (1)
        {
            size_t i = srcbuf.find_last_not_of(" \t\n\r\f\v");
            if (i != std::string::npos && srcbuf[i] == '}')
                break;
            char line[1024];
            if (!fgets(line, 1020, fp))
                // eof
                break;
            (*lines)++;
            if (feof(fp))
                break;
            if (srcbuf.empty())
            {
                // may be a no-op
                srcbuf = strchrnul(line, '{');
                // safe to execute more than once
                // But will usually only happen once
                startline = *lines;
            }
            else
                srcbuf += line;
        }
        script = parse_script(srcbuf.c_str(), startline);
        if (script == NULL)
            // script parse error?
            return 1;
    }
    else
    {
        assert(0 && "duplicate() is no longer supported!\n");
        return 0;
    }

    nd.new_();

    if (m == nullptr)
    {
        // スクリプトコピー用のダミーNPC
    }
    else if (sscanf(w4, "%d,%d,%d", &npc_class, &xs, &ys) == 3)
    {
        // 接触型NPC
        int i, j;

        if (xs >= 0)
            xs = xs * 2 + 1;
        if (ys >= 0)
            ys = ys * 2 + 1;

        if (npc_class >= 0)
        {

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
        }

        nd->scr.xs = xs;
        nd->scr.ys = ys;
    }
    else
    {                           // クリック型NPC
        npc_class = atoi(w4);
        nd->scr.xs = 0;
        nd->scr.ys = 0;
    }

    if (npc_class < 0 && m != nullptr)
    {                           // イベント型NPC
        evflag = 1;
    }

    while ((p = strchr(w3, ':')))
    {
        if (p[1] == ':')
            break;
    }
    if (p)
    {
        *p = 0;
        strzcpy(nd->name, w3, 24);
        strzcpy(nd->exname, p + 2, 24);
    }
    else
    {
        strzcpy(nd->name, w3, 24);
        strzcpy(nd->exname, w3, 24);
    }

    nd->bl_prev = nd->bl_next = NULL;
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

    //PRINTF("script npc %s %d %d read done\n",mapname,nd->bl_id,nd->class);
    npc_script++;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::SCRIPT;
    if (m != nullptr)
    {
        nd->n = map_addnpc(m, nd);
        map_addblock(nd);

        if (evflag)
        {                       // イベント型
            struct event_data ev {};
            ev.nd = nd;
            ev.pos = 0;
            ev_db.insert(nd->exname, ev);
        }
        else
            clif_spawnnpc(nd);
    }
    npcname_db.put(nd->exname, nd);

    for (auto& pair : scriptlabel_db)
        npc_convertlabel_db(pair.first, pair.second, nd);

    for (npc_label_list& el : nd->scr.label_listv)
    {
        char *lname = el.name;
        int pos = el.pos;

        if ((lname[0] == 'O' || lname[0] == 'o')
            && (lname[1] == 'N' || lname[1] == 'n'))
        {
            if (strlen(lname) > 24)
            {
                PRINTF("npc_parse_script: label name error !\n");
                exit(1);
            }
            struct event_data ev {};
            ev.nd = nd;
            ev.pos = pos;
            std::string buf = STRPRINTF("%s::%s", nd->exname, lname);
            ev_db.insert(buf, ev);
        }
    }

    //-----------------------------------------
    // ラベルデータからタイマーイベント取り込み
    for (npc_label_list& el : nd->scr.label_listv)
    {
        int t_ = 0, n = 0;
        char *lname = el.name;
        int pos = el.pos;
        if (sscanf(lname, "OnTimer%d%n", &t_, &n) == 1 && lname[n] == '\0')
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
    nd->scr.nexttimer = nd->scr.timer_eventv.end();
    // nd->scr.timerid = nullptr;

    return 0;
}

/*==========================================
 * function行解析
 *------------------------------------------
 */
static
int npc_parse_function(char *, char *, char *w3, char *,
                               char *first_line, FILE * fp, int *lines)
{
    std::string srcbuf = strchrnul(first_line, '{');
    int startline = *lines;

    while (1)
    {
        size_t i = srcbuf.find_last_not_of(" \t\n\r\f\v");
        if (i != std::string::npos && srcbuf[i] == '}')
            break;
        char line[1024];
        if (!fgets(line, 1020, fp))
            break;
        (*lines)++;
        if (feof(fp))
            break;
        if (srcbuf.empty())
        {
            srcbuf = strchrnul(line, '{');
            startline = *lines;
        }
        else
            srcbuf += line;
    }
    std::unique_ptr<const ScriptBuffer> script = parse_script(srcbuf.c_str(), startline);
    if (script == NULL)
    {
        // script parse error?
        return 1;
    }

    std::string p = w3;
    userfunc_db.put(p, std::move(script));

    return 0;
}

/*==========================================
 * mob行解析
 *------------------------------------------
 */
static
int npc_parse_mob(const char *w1, const char *, const char *w3, const char *w4)
{
    int x, y, xs, ys, mob_class, num;
    int i;
    char mapname[24];
    char eventname[24] = "";
    dumb_ptr<mob_data> md;

    xs = ys = 0;
    int delay1_ = 0, delay2_ = 0;
    // 引数の個数チェック
    if (sscanf(w1, "%[^,],%d,%d,%d,%d", mapname, &x, &y, &xs, &ys) < 3 ||
        sscanf(w4, "%d,%d,%d,%d,%s", &mob_class, &num, &delay1_, &delay2_,
                eventname) < 2)
    {
        PRINTF("bad monster line : %s\n", w3);
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

        md->bl_prev = NULL;
        md->bl_next = NULL;
        md->bl_m = m;
        md->bl_x = x;
        md->bl_y = y;
        if (strcmp(w3, "--en--") == 0)
            strzcpy(md->name, mob_db[mob_class].name, 24);
        else if (strcmp(w3, "--ja--") == 0)
            strzcpy(md->name, mob_db[mob_class].jname, 24);
        else
            strzcpy(md->name, w3, 24);

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
        md->target_id = 0;
        md->attacked_id = 0;

        md->lootitemv.clear();

        if (strlen(eventname) >= 4)
            strzcpy(md->npc_event, eventname, 24);
        else
            strzcpy(md->npc_event, "", 24);

        md->bl_type = BL::MOB;
        map_addiddb(md);
        mob_spawn(md->bl_id);

        npc_mob++;
    }
    //PRINTF("warp npc %s %d read done\n",mapname,nd->bl_id);

    return 0;
}

/*==========================================
 * マップフラグ行の解析
 *------------------------------------------
 */
static
int npc_parse_mapflag(char *w1, char *, char *w3, char *w4)
{
    char mapname[24], savemap[16];
    int savex, savey;

    // 引数の個数チェック
//  if (    sscanf(w1,"%[^,],%d,%d,%d",mapname,&x,&y,&dir) != 4 )
    if (sscanf(w1, "%[^,]", mapname) != 1)
        return 1;

    map_local *m = map_mapname2mapid(mapname);
    if (m == nullptr)
        return 1;

//マップフラグ
    if (strcasecmp(w3, "nosave") == 0)
    {
        if (strcmp(w4, "SavePoint") == 0)
        {
            strzcpy(m->save.map_, "SavePoint", 16);
            m->save.x = -1;
            m->save.y = -1;
        }
        else if (sscanf(w4, "%[^,],%d,%d", savemap, &savex, &savey) == 3)
        {
            strzcpy(m->save.map_, savemap, 16);
            m->save.x = savex;
            m->save.y = savey;
        }
        m->flag.nosave = 1;
    }
    else if (strcasecmp(w3, "nomemo") == 0)
    {
        m->flag.nomemo = 1;
    }
    else if (strcasecmp(w3, "noteleport") == 0)
    {
        m->flag.noteleport = 1;
    }
    else if (strcasecmp(w3, "nowarp") == 0)
    {
        m->flag.nowarp = 1;
    }
    else if (strcasecmp(w3, "nowarpto") == 0)
    {
        m->flag.nowarpto = 1;
    }
    else if (strcasecmp(w3, "noreturn") == 0)
    {
        m->flag.noreturn = 1;
    }
    else if (strcasecmp(w3, "monster_noteleport") == 0)
    {
        m->flag.monster_noteleport = 1;
    }
    else if (strcasecmp(w3, "nobranch") == 0)
    {
        m->flag.nobranch = 1;
    }
    else if (strcasecmp(w3, "nopenalty") == 0)
    {
        m->flag.nopenalty = 1;
    }
    else if (strcasecmp(w3, "pvp") == 0)
    {
        m->flag.pvp = 1;
    }
    else if (strcasecmp(w3, "pvp_noparty") == 0)
    {
        m->flag.pvp_noparty = 1;
    }
    else if (strcasecmp(w3, "pvp_nocalcrank") == 0)
    {
        m->flag.pvp_nocalcrank = 1;
    }
    else if (strcasecmp(w3, "nozenypenalty") == 0)
    {
        m->flag.nozenypenalty = 1;
    }
    else if (strcasecmp(w3, "notrade") == 0)
    {
        m->flag.notrade = 1;
    }
    else if (battle_config.pk_mode && strcasecmp(w3, "nopvp") == 0)
    {                           // nopvp for pk mode [Valaris]
        m->flag.nopvp = 1;
        m->flag.pvp = 0;
    }
    else if (strcasecmp(w3, "noicewall") == 0)
    {                           // noicewall [Valaris]
        m->flag.noicewall = 1;
    }
    else if (strcasecmp(w3, "snow") == 0)
    {                           // snow [Valaris]
        m->flag.snow = 1;
    }
    else if (strcasecmp(w3, "fog") == 0)
    {                           // fog [Valaris]
        m->flag.fog = 1;
    }
    else if (strcasecmp(w3, "sakura") == 0)
    {                           // sakura [Valaris]
        m->flag.sakura = 1;
    }
    else if (strcasecmp(w3, "leaves") == 0)
    {                           // leaves [Valaris]
        m->flag.leaves = 1;
    }
    else if (strcasecmp(w3, "rain") == 0)
    {                           // rain [Valaris]
        m->flag.rain = 1;
    }
    else if (strcasecmp(w3, "no_player_drops") == 0)
    {                           // no player drops [Jaxad0127]
        m->flag.no_player_drops = 1;
    }
    else if (strcasecmp(w3, "town") == 0)
    {                           // town/safe zone [remoitnane]
        m->flag.town = 1;
    }

    return 0;
}

dumb_ptr<npc_data> npc_spawn_text(map_local *m, int x, int y,
        int npc_class, const char *name, const char *message)
{
    dumb_ptr<npc_data_message> retval;
    retval.new_();
    retval->bl_id = npc_get_new_npc_id();
    retval->bl_x = x;
    retval->bl_y = y;
    retval->bl_m = m;
    retval->bl_type = BL::NPC;
    retval->npc_subtype = NpcSubtype::MESSAGE;

    strzcpy(retval->name, name, 24);
    strzcpy(retval->exname, name, 24);
    if (message)
        retval->message = message;

    retval->npc_class = npc_class;
    retval->speed = std::chrono::milliseconds(200);

    clif_spawnnpc(retval);
    map_addblock(retval);
    map_addiddb(retval);
    if (retval->name && retval->name[0])
        npcname_db.put(retval->name, retval);

    return retval;
}

static
void npc_free_internal(dumb_ptr<npc_data> nd_)
{
    if (nd_->npc_subtype == NpcSubtype::SCRIPT)
    {
        dumb_ptr<npc_data_script> nd = nd_->as_script();
        nd->scr.timer_eventv.clear();

        {
            nd->scr.script.reset();
            nd->scr.label_listv.clear();
        }
    }
    else if (nd_->npc_subtype == NpcSubtype::MESSAGE)
    {
        dumb_ptr<npc_data_message> nd = nd_->as_message();
        nd->message.clear();
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
int do_init_npc(void)
{
    // other fields unused
    ev_tm_b.tm_min = -1;
    ev_tm_b.tm_hour = -1;
    ev_tm_b.tm_mday = -1;

    for (; !npc_srcs.empty(); npc_srcs.pop_front())
    {
        std::string& nsl = npc_srcs.front();
        FILE *fp = fopen_(nsl.c_str(), "r");
        if (fp == NULL)
        {
            PRINTF("file not found : %s\n", nsl);
            exit(1);
        }
        int lines = 0;
        char line[1024];
        while (fgets(line, 1020, fp))
        {
            char w1[1024], w2[1024], w3[1024], w4[1024], mapname[1024];
            int i, j, w4pos, count;
            lines++;

            if (line[0] == '/' && line[1] == '/')
                continue;
            // 不要なスペースやタブの連続は詰める
            for (i = j = 0; line[i]; i++)
            {
                if (line[i] == ' ')
                {
                    if (!
                        ((line[i + 1]
                          && (isspace(line[i + 1]) || line[i + 1] == ','))
                         || (j && line[j - 1] == ',')))
                        line[j++] = ' ';
                }
                else if (line[i] == '\t' || line[i] == '|')
                {
                    if (!(j && (line[j - 1] == '\t' || line[j - 1] == '|')))
                        line[j++] = '\t';
                }
                else
                    line[j++] = line[i];
            }
            // 最初はタブ区切りでチェックしてみて、ダメならスペース区切りで確認
            if ((count =
                 sscanf(line, "%[^\t]\t%[^\t]\t%[^\t\r\n]\t%n%[^\t\r\n]", w1,
                         w2, w3, &w4pos, w4)) < 3
                && (count =
                    sscanf(line, "%s%s%s%n%s", w1, w2, w3, &w4pos, w4)) < 3)
            {
                continue;
            }
            // マップの存在確認
            if (strcmp(w1, "-") != 0 && strcasecmp(w1, "function") != 0)
            {
                sscanf(w1, "%[^,]", mapname);
                map_local *m = map_mapname2mapid(mapname);
                if (strlen(mapname) > 16 || m == nullptr)
                {
                    // "mapname" is not assigned to this server
                    continue;
                }
            }
            if (strcasecmp(w2, "warp") == 0 && count > 3)
            {
                npc_parse_warp(w1, w2, w3, w4);
            }
            else if (strcasecmp(w2, "shop") == 0 && count > 3)
            {
                npc_parse_shop(w1, w2, w3, w4);
            }
            else if (strcasecmp(w2, "script") == 0 && count > 3)
            {
                if (strcasecmp(w1, "function") == 0)
                {
                    npc_parse_function(w1, w2, w3, w4, line + w4pos, fp,
                                        &lines);
                }
                else
                {
                    npc_parse_script(w1, w2, w3, w4, line + w4pos, fp,
                                      &lines);
                }
            }
            else if ((i =
                      0, sscanf(w2, "duplicate%n", &i), (i > 0
                                                          && w2[i] == '('))
                     && count > 3)
            {
                npc_parse_script(w1, w2, w3, w4, line + w4pos, fp, &lines);
            }
            else if (strcasecmp(w2, "monster") == 0 && count > 3)
            {
                npc_parse_mob(w1, w2, w3, w4);
            }
            else if (strcasecmp(w2, "mapflag") == 0 && count >= 3)
            {
                npc_parse_mapflag(w1, w2, w3, w4);
            }
        }
        fclose_(fp);
        PRINTF("\rLoading NPCs [%d]: %-54s", npc_id - START_NPC_NUM,
                nsl);
        fflush(stdout);
    }
    PRINTF("\rNPCs Loaded: %d [Warps:%d Shops:%d Scripts:%d Mobs:%d] %20s\n",
            npc_id - START_NPC_NUM, npc_warp, npc_shop, npc_script, npc_mob, "");

    if (script_errors)
    {
        PRINTF("Cowardly refusing to continue after %d errors\n", script_errors);
        abort();
    }

    return 0;
}
