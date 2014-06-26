#include "trade.hpp"
//    trade.cpp - Inter-player item and money transactions.
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

#include "../compat/nullpo.hpp"

#include "../io/cxxstdio.hpp"

#include "battle.hpp"
#include "clif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "npc.hpp"
#include "pc.hpp"
#include "storage.hpp"

#include "../poison.hpp"

/*==========================================
 * 取引要請を相手に送る
 *------------------------------------------
 */
void trade_traderequest(dumb_ptr<map_session_data> sd, BlockId target_id)
{
    dumb_ptr<map_session_data> target_sd;

    nullpo_retv(sd);

    if ((target_sd = map_id2sd(target_id)) != NULL)
    {
        if (!battle_config.invite_request_check)
        {
            if (target_sd->party_invite)
            {
                clif_tradestart(sd, 2);    // 相手はPT要請中かGuild要請中
                return;
            }
        }
        if (target_sd->npc_id)
        {
            //Trade fails if you are using an NPC.
            clif_tradestart(sd, 2);
            return;
        }
        if ((target_sd->trade_partner) || (sd->trade_partner))
        {
            trade_tradecancel(sd); //person is in another trade
        }
        else
        {
            if (sd->bl_m != target_sd->bl_m
                || (sd->bl_x - target_sd->bl_x <= -5
                    || sd->bl_x - target_sd->bl_x >= 5)
                || (sd->bl_y - target_sd->bl_y <= -5
                    || sd->bl_y - target_sd->bl_y >= 5))
            {
                clif_tradestart(sd, 0);    //too far
            }
            else if (sd != target_sd)
            {
                target_sd->trade_partner = sd->status_key.account_id;
                sd->trade_partner = target_sd->status_key.account_id;
                clif_traderequest(target_sd, sd->status_key.name);
            }
        }
    }
    else
    {
        clif_tradestart(sd, 1);    //character does not exist
    }
}

/*==========================================
 * 取引要請
 *------------------------------------------
 */
void trade_tradeack(dumb_ptr<map_session_data> sd, int type)
{
    dumb_ptr<map_session_data> target_sd;
    nullpo_retv(sd);

    if ((target_sd = map_id2sd(account_to_block(sd->trade_partner))) != NULL)
    {
        clif_tradestart(target_sd, type);
        clif_tradestart(sd, type);
        if (type == 4)
        {                       // Cancel
            sd->deal_locked = 0;
            sd->trade_partner = AccountId();
            target_sd->deal_locked = 0;
            target_sd->trade_partner = AccountId();
        }
        if (sd->npc_id)
            npc_event_dequeue(sd);
        if (target_sd->npc_id)
            npc_event_dequeue(target_sd);

        //close STORAGE window if it's open. It protects from spooffing packets [Lupus]
        if (sd->state.storage_open)
            storage_storageclose(sd);
    }
}

/*==========================================
 * アイテム追加
 *------------------------------------------
 */
void trade_tradeadditem(dumb_ptr<map_session_data> sd, IOff2 index, int amount)
{
    dumb_ptr<map_session_data> target_sd;
    struct item_data *id;
    int trade_i;
    int trade_weight = 0;
    int free_ = 0;
    int c;

    nullpo_retv(sd);

    if (((target_sd = map_id2sd(account_to_block(sd->trade_partner))) != NULL)
        && (sd->deal_locked < 1))
    {
        if (!index.ok())
        {
            if (index.index == 0 && amount > 0 && amount <= sd->status.zeny)
            {
                sd->deal_zeny = amount;
                clif_tradeadditem(sd, target_sd, index, amount);
            }
        }
        // note: amount is overridden below!
        else if (amount <= sd->status.inventory[index.unshift()].amount
                 && amount > 0)
        {
            // determine free slots of receiver
            for (IOff0 i : IOff0::iter())
            {
                if (!target_sd->status.inventory[i].nameid
                    && target_sd->inventory_data[i] == NULL)
                    free_++;
            }
            for (trade_i = 0; trade_i < TRADE_MAX; trade_i++)
            {
                if (sd->deal_item_amount[trade_i] == 0)
                {
                    // calculate trade weight
                    trade_weight +=
                        sd->inventory_data[index.unshift()]->weight * amount;

                    // determine if item is a stackable already in receivers inventory, and up free count
                    for (IOff0 i : IOff0::iter())
                    {
                        if (target_sd->status.inventory[i].nameid ==
                            sd->status.inventory[index.unshift()].nameid
                            && target_sd->inventory_data[i] != NULL)
                        {
                            id = target_sd->inventory_data[i];
                            if (id->type != ItemType::WEAPON
                                && id->type != ItemType::ARMOR
                                && id->type != ItemType::_7
                                && id->type != ItemType::_8)
                            {
                                free_++;
                                break;
                            }
                        }
                    }

                    if (target_sd->weight + trade_weight >
                        target_sd->max_weight)
                    {
                        clif_tradeitemok(sd, index, 0, 1); //fail to add item -- the player was over weighted.
                        amount = 0; // [MouseJstr]
                    }
                    else if (free_ <= 0)
                    {
                        clif_tradeitemok(sd, index, 0, 2); //fail to add item -- no free slots at receiver
                        amount = 0; // peavey
                    }
                    else
                    {
                        for (c = 0; c == trade_i - 1; c++)
                        {       // re-deal exploit protection [Valaris]
                            if (sd->deal_item_index[c] == index)
                            {
                                trade_tradecancel(sd);
                                return;
                            }
                        }
                        pc_unequipinvyitem(sd, index.unshift(), CalcStatus::NOW);
                        sd->deal_item_index[trade_i] = index;
                        sd->deal_item_amount[trade_i] += amount;
                        clif_tradeitemok(sd, index, amount, 0);    //success to add item
                        clif_tradeadditem(sd, target_sd, index, amount);
                    }
                    break;
                }
                else
                {
                    // calculate weight for stored deal
                    trade_weight +=
                        sd->inventory_data[sd->deal_item_index[trade_i].unshift()
                                           ]->weight *
                        sd->deal_item_amount[trade_i];
                    // count free stackables in stored deal
                    for (IOff0 i : IOff0::iter())
                    {
                        if (target_sd->status.inventory[i].nameid ==
                            sd->status.
                            inventory[sd->deal_item_index[trade_i].unshift()].nameid
                            && target_sd->inventory_data[i] != NULL)
                        {
                            id = target_sd->inventory_data[i];
                            if (id->type != ItemType::WEAPON
                                && id->type != ItemType::ARMOR
                                && id->type != ItemType::_7
                                && id->type != ItemType::_8)
                            {
                                free_++;
                                break;
                            }
                        }
                    }
                }
                // used a slot, but might be cancelled out by stackable checks above
                free_--;
            }
        }
    }
}

/*==========================================
 * アイテム追加完了(ok押し)
 *------------------------------------------
 */
void trade_tradeok(dumb_ptr<map_session_data> sd)
{
    dumb_ptr<map_session_data> target_sd;
    int trade_i;

    nullpo_retv(sd);

    for (trade_i = 0; trade_i < TRADE_MAX; trade_i++)
    {
        IOff2 index = sd->deal_item_index[trade_i];
        if (!index.ok())
            continue;
        if (sd->deal_item_amount[trade_i] >
            sd->status.inventory[index.unshift()].amount
            || sd->deal_item_amount[trade_i] < 0)
        {
            trade_tradecancel(sd);
            return;
        }

    }

    if ((target_sd = map_id2sd(account_to_block(sd->trade_partner))) != NULL)
    {
        sd->deal_locked = 1;
        clif_tradeitemok(sd, IOff2::from(0), 0, 0);
        clif_tradedeal_lock(sd, 0);
        clif_tradedeal_lock(target_sd, 1);
    }
}

/*==========================================
 * 取引キャンセル
 *------------------------------------------
 */
void trade_tradecancel(dumb_ptr<map_session_data> sd)
{
    dumb_ptr<map_session_data> target_sd;
    int trade_i;

    nullpo_retv(sd);

    if ((target_sd = map_id2sd(account_to_block(sd->trade_partner))) != NULL)
    {
        for (trade_i = 0; trade_i < TRADE_MAX; trade_i++)
        {                       //give items back (only virtual)
            if (sd->deal_item_amount[trade_i] != 0)
            {
                assert (sd->deal_item_index[trade_i].ok());
                clif_additem(sd,
                        sd->deal_item_index[trade_i].unshift(),
                        sd->deal_item_amount[trade_i],
                        PickupFail::OKAY);
                sd->deal_item_index[trade_i] = IOff2::from(0);
                sd->deal_item_amount[trade_i] = 0;
            }
            if (target_sd->deal_item_amount[trade_i] != 0)
            {
                assert (target_sd->deal_item_index[trade_i].ok());
                clif_additem(target_sd,
                        target_sd->deal_item_index[trade_i].unshift(),
                        target_sd->deal_item_amount[trade_i],
                        PickupFail::OKAY);
                target_sd->deal_item_index[trade_i] = IOff2::from(0);
                target_sd->deal_item_amount[trade_i] = 0;
            }
        }
        if (sd->deal_zeny)
        {
            sd->deal_zeny = 0;
            clif_updatestatus(sd, SP::ZENY);
        }
        if (target_sd->deal_zeny)
        {
            clif_updatestatus(target_sd, SP::ZENY);
            target_sd->deal_zeny = 0;
        }
        sd->deal_locked = 0;
        sd->trade_partner = AccountId();
        target_sd->deal_locked = 0;
        target_sd->trade_partner = AccountId();
        clif_tradecancelled(sd);
        clif_tradecancelled(target_sd);
    }
}

/*==========================================
 * 取引許諾(trade押し)
 *------------------------------------------
 */
void trade_tradecommit(dumb_ptr<map_session_data> sd)
{
    dumb_ptr<map_session_data> target_sd;
    int trade_i;

    nullpo_retv(sd);

    if ((target_sd = map_id2sd(account_to_block(sd->trade_partner))) != NULL)
    {
        MAP_LOG_PC(sd, " TRADECOMMIT WITH %d GIVE %d GET %d"_fmt,
                target_sd->status_key.char_id, sd->deal_zeny,
                target_sd->deal_zeny);
        if ((sd->deal_locked >= 1) && (target_sd->deal_locked >= 1))
        {                       // both have pressed 'ok'
            if (sd->deal_locked < 2)
            {
                sd->deal_locked = 2;
            }                   // set locked to 2
            if (target_sd->deal_locked == 2)
            {                   // the other one pressed 'trade' too
                if (sd->deal_zeny > sd->status.zeny)
                {
                    sd->deal_zeny = 0;
                    trade_tradecancel(sd);
                    MAP_LOG_PC(sd, " TRADECANCEL"_fmt);
                    return;
                }
                if (target_sd->deal_zeny > target_sd->status.zeny)
                {
                    target_sd->deal_zeny = 0;
                    trade_tradecancel(sd);
                    MAP_LOG_PC(sd, " TRADECANCEL"_fmt);
                    return;
                }
                sd->trade_partner = AccountId();
                target_sd->trade_partner = AccountId();
                for (trade_i = 0; trade_i < TRADE_MAX; trade_i++)
                {
                    if (sd->deal_item_amount[trade_i] != 0)
                    {
                        assert (sd->deal_item_index[trade_i].ok());
                        IOff0 n = sd->deal_item_index[trade_i].unshift();
                        PickupFail flag = pc_additem(target_sd,
                                &sd->status.inventory[n],
                                sd->deal_item_amount[trade_i]);
                        if (flag == PickupFail::OKAY)
                            pc_delitem(sd, n, sd->deal_item_amount[trade_i],
                                        1);
                        else
                            clif_additem(sd, n,
                                    sd->deal_item_amount[trade_i],
                                    PickupFail::OKAY);
                        sd->deal_item_index[trade_i] = IOff2::from(0);
                        sd->deal_item_amount[trade_i] = 0;
                    }
                    if (target_sd->deal_item_amount[trade_i] != 0)
                    {
                        assert (target_sd->deal_item_index[trade_i].ok());
                        IOff0 n = target_sd->deal_item_index[trade_i].unshift();
                        PickupFail flag = pc_additem(sd,
                                &target_sd->status.inventory[n],
                                target_sd->deal_item_amount[trade_i]);
                        if (flag == PickupFail::OKAY)
                            pc_delitem(target_sd, n,
                                        target_sd->deal_item_amount[trade_i],
                                        1);
                        else
                            clif_additem(target_sd, n,
                                    target_sd->deal_item_amount[trade_i],
                                    PickupFail::OKAY);
                        target_sd->deal_item_index[trade_i] = IOff2::from(0);
                        target_sd->deal_item_amount[trade_i] = 0;
                    }
                }
                if (sd->deal_zeny)
                {
                    int deal = sd->deal_zeny;
                    sd->deal_zeny = 0;
                    sd->status.zeny -= deal;
                    clif_updatestatus(sd, SP::ZENY);
                    target_sd->status.zeny += deal;
                    clif_updatestatus(target_sd, SP::ZENY);
                }
                if (target_sd->deal_zeny)
                {
                    int deal = target_sd->deal_zeny;
                    target_sd->deal_zeny = 0;
                    target_sd->status.zeny -= deal;
                    clif_updatestatus(target_sd, SP::ZENY);
                    sd->status.zeny += deal;
                    clif_updatestatus(sd, SP::ZENY);
                }
                sd->deal_locked = 0;
                target_sd->deal_locked = 0;
                clif_tradecompleted(sd, 0);
                clif_tradecompleted(target_sd, 0);
                MAP_LOG_PC(sd, " TRADEOK"_fmt);
            }
        }
    }
}

// This is called when a char's zeny is changed
// This helps prevent money duplication and other problems
// [Jaxad0127]
void trade_verifyzeny(dumb_ptr<map_session_data> sd)
{
    dumb_ptr<map_session_data> target_sd;

    nullpo_retv(sd);

    if ((target_sd = map_id2sd(account_to_block(sd->trade_partner))) != NULL)
    {
        if (sd->deal_zeny > sd->status.zeny)
        {
            if (sd->deal_locked < 1)
                trade_tradeadditem(sd, IOff2::from(0), sd->status.zeny);    // Fix money ammount
            else
                trade_tradecancel(sd); // Or cancel the trade if we can't fix it
        }
    }
}
