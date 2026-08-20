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

#include "../net/timer.hpp"

#include "../proto2/map-user.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "pc.hpp"
#include "skill.hpp"

#include "lua-dialog.hpp"
#include "lua-events.hpp"
#include "lua-npc.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
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
        lua_npc_event(sd, aname, LuaArgs::none());
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
 * NPC Spell: chat words registered by scripts (server.registercmd)
 *------------------------------------------
 */
int magic_message(dumb_ptr<map_session_data> caster, XString source_invocation)
{
    auto pair = magic_tokenise(source_invocation);

    AString spell_params = AString(pair.second);
    if (lua_command_dispatch(caster, pair.first, ZString(spell_params)))
        return 1;
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
    lua_dialog_abandon(sd);

    return lua_event_dequeue(sd) ? 1 : 0;
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
        {
            Option<P<map_local>> ml = map_mapname2mapid(m->npc[i]->is_warp()->warp.name);
            if (ml.map([](P<map_local> m_){ return m_->flag.get(MapFlag::NOWARPTO); }).copy_or(false)
                && !(pc_isGM(sd).satisfies(battle_config.any_warp_GM_min_level)))
            {
                clif_displaymessage(sd->sess,
                        "The destination of this warp is closed at the moment."_s);
                return 2;
            }
            if (sd->bl_m->flag.get(MapFlag::NOWARP)
                && !(pc_isGM(sd).satisfies(battle_config.any_warp_GM_min_level)))
            {
                clif_displaymessage(sd->sess,
                        "This warp is closed at the moment."_s);
                return 2;
            }
            skill_stop_dancing(sd, 0);
            pc_setpos(sd, m->npc[i]->is_warp()->warp.name,
                       m->npc[i]->is_warp()->warp.x, m->npc[i]->is_warp()->warp.y, BeingRemoveWhy::GONE);
            break;
        }
        case NpcSubtype::SCRIPT:
        {
            if (sd->areanpc_id == m->npc[i]->bl_id)
                return 2;

            sd->areanpc_id = m->npc[i]->bl_id;

            // No OnTouch handler: fall back to a click, as the old engine
            // did when the OnTouch label did not exist.
            if (!lua_npc_has_handler(m->npc[i], "OnTouch"_s))
            {
                npc_click(sd, m->npc[i]->bl_id);
                return 2;
            }

            NpcEvent aname;
            aname.npc = m->npc[i]->name;
            aname.label = stringish<ScriptLabel>("OnTouch"_s);
            lua_npc_event(sd, aname, LuaArgs::none());
            return 2;
        }
    }
    return 0;
}

/*==========================================
 * 近くかどうかの判定
 *------------------------------------------
 */
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

    switch (nd->npc_subtype)
    {
        case NpcSubtype::SHOP:
            sd->npc_id = id;
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
            // starts the dialog coroutine on the click body (sets npc_id)
            lua_npc_click(sd, nd);
            break;
    }

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

    // release the Lua side: OnTimer machine, one-shot timer slots,
    // definition table, engine registries, hook index
    lua_npc_detach(nd_);

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
    }
    if (nd_->name)
        npcs_by_name.put(nd_->name, nullptr);

    if (nd_->bl_m != borrow(undefined_gat)) {
        nd_->bl_m->npc[nd_->n] = nullptr;
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
