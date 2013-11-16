#include "trade.hpp"

#include "../io/cxxstdio.hpp"

#include "../common/nullpo.hpp"

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
void trade_traderequest(dumb_ptr<map_session_data> sd, int target_id)
{
    dumb_ptr<map_session_data> target_sd;

    nullpo_retv(sd);

    if ((target_sd = map_id2sd(target_id)) != NULL)
    {
        if (!battle_config.invite_request_check)
        {
            if (target_sd->party_invite > 0)
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
        if ((target_sd->trade_partner != 0) || (sd->trade_partner != 0))
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
                target_sd->trade_partner = sd->status.account_id;
                sd->trade_partner = target_sd->status.account_id;
                clif_traderequest(target_sd, sd->status.name);
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

    if ((target_sd = map_id2sd(sd->trade_partner)) != NULL)
    {
        clif_tradestart(target_sd, type);
        clif_tradestart(sd, type);
        if (type == 4)
        {                       // Cancel
            sd->deal_locked = 0;
            sd->trade_partner = 0;
            target_sd->deal_locked = 0;
            target_sd->trade_partner = 0;
        }
        if (sd->npc_id != 0)
            npc_event_dequeue(sd);
        if (target_sd->npc_id != 0)
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
void trade_tradeadditem(dumb_ptr<map_session_data> sd, int index, int amount)
{
    dumb_ptr<map_session_data> target_sd;
    struct item_data *id;
    int trade_i;
    int trade_weight = 0;
    int free_ = 0;
    int c;
    int i;

    nullpo_retv(sd);

    if (((target_sd = map_id2sd(sd->trade_partner)) != NULL)
        && (sd->deal_locked < 1))
    {
        if (index < 2 || index >= MAX_INVENTORY + 2)
        {
            if (index == 0 && amount > 0 && amount <= sd->status.zeny)
            {
                sd->deal_zeny = amount;
                clif_tradeadditem(sd, target_sd, 0, amount);
            }
        }
        // note: amount is overridden below!
        else if (amount <= sd->status.inventory[index - 2].amount
                 && amount > 0)
        {
            // determine free slots of receiver
            for (i = 0; i < MAX_INVENTORY; i++)
            {
                if (target_sd->status.inventory[i].nameid == 0
                    && target_sd->inventory_data[i] == NULL)
                    free_++;
            }
            for (trade_i = 0; trade_i < 10; trade_i++)
            {
                if (sd->deal_item_amount[trade_i] == 0)
                {
                    // calculate trade weight
                    trade_weight +=
                        sd->inventory_data[index - 2]->weight * amount;

                    // determine if item is a stackable already in receivers inventory, and up free count
                    for (i = 0; i < MAX_INVENTORY; i++)
                    {
                        if (target_sd->status.inventory[i].nameid ==
                            sd->status.inventory[index - 2].nameid
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
                        pc_unequipinvyitem(sd, index - 2, CalcStatus::NOW);
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
                        sd->inventory_data[sd->deal_item_index[trade_i] -
                                           2]->weight *
                        sd->deal_item_amount[trade_i];
                    // count free stackables in stored deal
                    for (i = 0; i < MAX_INVENTORY; i++)
                    {
                        if (target_sd->status.inventory[i].nameid ==
                            sd->status.
                            inventory[sd->deal_item_index[trade_i] - 2].nameid
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

    for (trade_i = 0; trade_i < 10; trade_i++)
    {
        if (sd->deal_item_amount[trade_i] >
            sd->status.inventory[sd->deal_item_index[trade_i] - 2].amount
            || sd->deal_item_amount[trade_i] < 0)
        {
            trade_tradecancel(sd);
            return;
        }

    }

    if ((target_sd = map_id2sd(sd->trade_partner)) != NULL)
    {
        sd->deal_locked = 1;
        clif_tradeitemok(sd, 0, 0, 0);
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

    if ((target_sd = map_id2sd(sd->trade_partner)) != NULL)
    {
        for (trade_i = 0; trade_i < 10; trade_i++)
        {                       //give items back (only virtual)
            if (sd->deal_item_amount[trade_i] != 0)
            {
                clif_additem(sd,
                        sd->deal_item_index[trade_i] - 2,
                        sd->deal_item_amount[trade_i],
                        PickupFail::OKAY);
                sd->deal_item_index[trade_i] = 0;
                sd->deal_item_amount[trade_i] = 0;
            }
            if (target_sd->deal_item_amount[trade_i] != 0)
            {
                clif_additem(target_sd,
                        target_sd->deal_item_index[trade_i] - 2,
                        target_sd->deal_item_amount[trade_i],
                        PickupFail::OKAY);
                target_sd->deal_item_index[trade_i] = 0;
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
        sd->trade_partner = 0;
        target_sd->deal_locked = 0;
        target_sd->trade_partner = 0;
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

    if ((target_sd = map_id2sd(sd->trade_partner)) != NULL)
    {
        MAP_LOG_PC(sd, " TRADECOMMIT WITH %d GIVE %d GET %d",
                    target_sd->status.char_id, sd->deal_zeny,
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
                    MAP_LOG_PC(sd, " TRADECANCEL");
                    return;
                }
                if (target_sd->deal_zeny > target_sd->status.zeny)
                {
                    target_sd->deal_zeny = 0;
                    trade_tradecancel(sd);
                    MAP_LOG_PC(sd, " TRADECANCEL");
                    return;
                }
                sd->trade_partner = 0;
                target_sd->trade_partner = 0;
                for (trade_i = 0; trade_i < 10; trade_i++)
                {
                    if (sd->deal_item_amount[trade_i] != 0)
                    {
                        int n = sd->deal_item_index[trade_i] - 2;
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
                        sd->deal_item_index[trade_i] = 0;
                        sd->deal_item_amount[trade_i] = 0;
                    }
                    if (target_sd->deal_item_amount[trade_i] != 0)
                    {
                        int n = target_sd->deal_item_index[trade_i] - 2;
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
                        target_sd->deal_item_index[trade_i] = 0;
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
                MAP_LOG_PC(sd, " TRADEOK");
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

    if ((target_sd = map_id2sd(sd->trade_partner)) != NULL)
    {
        if (sd->deal_zeny > sd->status.zeny)
        {
            if (sd->deal_locked < 1)
                trade_tradeadditem(sd, 0, sd->status.zeny);    // Fix money ammount
            else
                trade_tradecancel(sd); // Or cancel the trade if we can't fix it
        }
    }
}
