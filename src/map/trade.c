#include <stdio.h>
#include <string.h>

#include "clif.h"
#include "itemdb.h"
#include "map.h"
#include "trade.h"
#include "pc.h"
#include "npc.h"
#include "battle.h"
#include "storage.h"
#include "nullpo.h"

/*==========================================
 * 取引要請を相手に送る
 *------------------------------------------
 */
void trade_traderequest (struct map_session_data *sd, int target_id)
{
    struct map_session_data *target_sd;

    nullpo_retv (sd);

    if ((target_sd = map_id2sd (target_id)) != NULL)
    {
        if (!battle_config.invite_request_check)
        {
            if (target_sd->guild_invite > 0 || target_sd->party_invite > 0)
            {
                clif_tradestart (sd, 2);    // 相手はPT要請中かGuild要請中
                return;
            }
        }
        if (target_sd->npc_id)
        {
            //Trade fails if you are using an NPC.
            clif_tradestart (sd, 2);
            return;
        }
        if ((target_sd->trade_partner != 0) || (sd->trade_partner != 0))
        {
            trade_tradecancel (sd); //person is in another trade
        }
        else
        {
            if (sd->bl.m != target_sd->bl.m
                || (sd->bl.x - target_sd->bl.x <= -5
                    || sd->bl.x - target_sd->bl.x >= 5)
                || (sd->bl.y - target_sd->bl.y <= -5
                    || sd->bl.y - target_sd->bl.y >= 5))
            {
                clif_tradestart (sd, 0);    //too far
            }
            else if (sd != target_sd)
            {
                target_sd->trade_partner = sd->status.account_id;
                sd->trade_partner = target_sd->status.account_id;
                clif_traderequest (target_sd, sd->status.name);
            }
        }
    }
    else
    {
        clif_tradestart (sd, 1);    //character does not exist
    }
}

/*==========================================
 * 取引要請
 *------------------------------------------
 */
void trade_tradeack (struct map_session_data *sd, int type)
{
    struct map_session_data *target_sd;
    nullpo_retv (sd);

    if ((target_sd = map_id2sd (sd->trade_partner)) != NULL)
    {
        clif_tradestart (target_sd, type);
        clif_tradestart (sd, type);
        if (type == 4)
        {                       // Cancel
            sd->deal_locked = 0;
            sd->trade_partner = 0;
            target_sd->deal_locked = 0;
            target_sd->trade_partner = 0;
        }
        if (sd->npc_id != 0)
            npc_event_dequeue (sd);
        if (target_sd->npc_id != 0)
            npc_event_dequeue (target_sd);

        //close STORAGE window if it's open. It protects from spooffing packets [Lupus]
        if (sd->state.storage_flag == 1)
            storage_storageclose (sd);
        else if (sd->state.storage_flag == 2)
            storage_guild_storageclose (sd);
    }
}

/*==========================================
 * アイテム追加
 *------------------------------------------
 */
void trade_tradeadditem (struct map_session_data *sd, int index, int amount)
{
    struct map_session_data *target_sd;
    struct item_data *id;
    int  trade_i;
    int  trade_weight = 0;
    int  free = 0;
    int  c;
    int  i;

    nullpo_retv (sd);

    if (((target_sd = map_id2sd (sd->trade_partner)) != NULL)
        && (sd->deal_locked < 1))
    {
        if (index < 2 || index >= MAX_INVENTORY + 2)
        {
            if (index == 0 && amount > 0 && amount <= sd->status.zeny)
            {
                sd->deal_zeny = amount;
                clif_tradeadditem (sd, target_sd, 0, amount);
            }
        }
        else if (amount <= sd->status.inventory[index - 2].amount
                 && amount > 0)
        {
            // determine free slots of receiver
            for (i = 0; i < MAX_INVENTORY; i++)
            {
                if (target_sd->status.inventory[i].nameid == 0
                    && target_sd->inventory_data[i] == NULL)
                    free++;
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
                            if (id->type != 4 && id->type != 5
                                && id->type != 7 && id->type != 8)
                            {
                                free++;
                                break;
                            }
                        }
                    }

                    if (target_sd->weight + trade_weight >
                        target_sd->max_weight)
                    {
                        clif_tradeitemok (sd, index, 0, 1); //fail to add item -- the player was over weighted.
                        amount = 0; // [MouseJstr]
                    }
                    else if (free <= 0)
                    {
                        clif_tradeitemok (sd, index, 0, 2); //fail to add item -- no free slots at receiver
                        amount = 0; // peavey
                    }
                    else
                    {
                        for (c = 0; c == trade_i - 1; c++)
                        {       // re-deal exploit protection [Valaris]
                            if (sd->deal_item_index[c] == index)
                            {
                                trade_tradecancel (sd);
                                return;
                            }
                        }
                        pc_unequipinvyitem (sd, index - 2, 0);
                        sd->deal_item_index[trade_i] = index;
                        sd->deal_item_amount[trade_i] += amount;
                        clif_tradeitemok (sd, index, amount, 0);    //success to add item
                        clif_tradeadditem (sd, target_sd, index, amount);
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
                            if (id->type != 4 && id->type != 5
                                && id->type != 7 && id->type != 8)
                            {
                                free++;
                                break;
                            }
                        }
                    }
                }
                // used a slot, but might be cancelled out by stackable checks above
                free--;
            }
        }
    }
}

/*==========================================
 * アイテム追加完了(ok押し)
 *------------------------------------------
 */
void trade_tradeok (struct map_session_data *sd)
{
    struct map_session_data *target_sd;
    int  trade_i;

    nullpo_retv (sd);

    for (trade_i = 0; trade_i < 10; trade_i++)
    {
        if (sd->deal_item_amount[trade_i] >
            sd->status.inventory[sd->deal_item_index[trade_i] - 2].amount
            || sd->deal_item_amount[trade_i] < 0)
        {
            trade_tradecancel (sd);
            return;
        }

    }

    if ((target_sd = map_id2sd (sd->trade_partner)) != NULL)
    {
        sd->deal_locked = 1;
        clif_tradeitemok (sd, 0, 0, 0);
        clif_tradedeal_lock (sd, 0);
        clif_tradedeal_lock (target_sd, 1);
    }
}

/*==========================================
 * 取引キャンセル
 *------------------------------------------
 */
void trade_tradecancel (struct map_session_data *sd)
{
    struct map_session_data *target_sd;
    int  trade_i;

    nullpo_retv (sd);

    if ((target_sd = map_id2sd (sd->trade_partner)) != NULL)
    {
        for (trade_i = 0; trade_i < 10; trade_i++)
        {                       //give items back (only virtual)
            if (sd->deal_item_amount[trade_i] != 0)
            {
                clif_additem (sd, sd->deal_item_index[trade_i] - 2,
                              sd->deal_item_amount[trade_i], 0);
                sd->deal_item_index[trade_i] = 0;
                sd->deal_item_amount[trade_i] = 0;
            }
            if (target_sd->deal_item_amount[trade_i] != 0)
            {
                clif_additem (target_sd,
                              target_sd->deal_item_index[trade_i] - 2,
                              target_sd->deal_item_amount[trade_i], 0);
                target_sd->deal_item_index[trade_i] = 0;
                target_sd->deal_item_amount[trade_i] = 0;
            }
        }
        if (sd->deal_zeny)
        {
            sd->deal_zeny = 0;
            clif_updatestatus (sd, SP_ZENY);
        }
        if (target_sd->deal_zeny)
        {
            clif_updatestatus (target_sd, SP_ZENY);
            target_sd->deal_zeny = 0;
        }
        sd->deal_locked = 0;
        sd->trade_partner = 0;
        target_sd->deal_locked = 0;
        target_sd->trade_partner = 0;
        clif_tradecancelled (sd);
        clif_tradecancelled (target_sd);
    }
}

#define MAP_LOG_PC(sd, fmt, args...) MAP_LOG("PC%d %d:%d,%d " fmt, sd->status.char_id, sd->bl.m, sd->bl.x, sd->bl.y, ## args)

/*==========================================
 * 取引許諾(trade押し)
 *------------------------------------------
 */
void trade_tradecommit (struct map_session_data *sd)
{
    struct map_session_data *target_sd;
    int  trade_i;

    nullpo_retv (sd);

    if ((target_sd = map_id2sd (sd->trade_partner)) != NULL)
    {
        MAP_LOG_PC (sd, " TRADECOMMIT WITH %d GIVE %d GET %d",
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
                    trade_tradecancel (sd);
                    MAP_LOG_PC (sd, " TRADECANCEL");
                    return;
                }
                if (target_sd->deal_zeny > target_sd->status.zeny)
                {
                    target_sd->deal_zeny = 0;
                    trade_tradecancel (sd);
                    MAP_LOG_PC (sd, " TRADECANCEL");
                    return;
                }
                sd->trade_partner = 0;
                target_sd->trade_partner = 0;
                for (trade_i = 0; trade_i < 10; trade_i++)
                {
                    if (sd->deal_item_amount[trade_i] != 0)
                    {
                        int  n = sd->deal_item_index[trade_i] - 2;
                        int  flag;
                        flag =
                            pc_additem (target_sd, &sd->status.inventory[n],
                                        sd->deal_item_amount[trade_i]);
                        if (flag == 0)
                            pc_delitem (sd, n, sd->deal_item_amount[trade_i],
                                        1);
                        else
                            clif_additem (sd, n,
                                          sd->deal_item_amount[trade_i], 0);
                        sd->deal_item_index[trade_i] = 0;
                        sd->deal_item_amount[trade_i] = 0;
                    }
                    if (target_sd->deal_item_amount[trade_i] != 0)
                    {
                        int  n = target_sd->deal_item_index[trade_i] - 2;
                        int  flag;
                        flag =
                            pc_additem (sd, &target_sd->status.inventory[n],
                                        target_sd->deal_item_amount[trade_i]);
                        if (flag == 0)
                            pc_delitem (target_sd, n,
                                        target_sd->deal_item_amount[trade_i],
                                        1);
                        else
                            clif_additem (target_sd, n,
                                          target_sd->deal_item_amount
                                          [trade_i], 0);
                        target_sd->deal_item_index[trade_i] = 0;
                        target_sd->deal_item_amount[trade_i] = 0;
                    }
                }
                if (sd->deal_zeny)
                {
                    int  deal = sd->deal_zeny;
                    sd->deal_zeny = 0;
                    sd->status.zeny -= deal;
                    clif_updatestatus (sd, SP_ZENY);
                    target_sd->status.zeny += deal;
                    clif_updatestatus (target_sd, SP_ZENY);
                }
                if (target_sd->deal_zeny)
                {
                    int  deal = target_sd->deal_zeny;
                    target_sd->deal_zeny = 0;
                    target_sd->status.zeny -= deal;
                    clif_updatestatus (target_sd, SP_ZENY);
                    sd->status.zeny += deal;
                    clif_updatestatus (sd, SP_ZENY);
                }
                sd->deal_locked = 0;
                target_sd->deal_locked = 0;
                clif_tradecompleted (sd, 0);
                clif_tradecompleted (target_sd, 0);
                MAP_LOG_PC (sd, " TRADEOK");
            }
        }
    }
}

// This is called when a char's zeny is changed
// This helps prevent money duplication and other problems
// [Jaxad0127]
void trade_verifyzeny (struct map_session_data *sd)
{
    struct map_session_data *target_sd;

    nullpo_retv (sd);

    if ((target_sd = map_id2sd (sd->trade_partner)) != NULL)
    {
        if (sd->deal_zeny > sd->status.zeny)
        {
            if (sd->deal_locked < 1)
                trade_tradeadditem (sd, 0, sd->status.zeny);    // Fix money ammount
            else
                trade_tradecancel (sd); // Or cancel the trade if we can't fix it
        }
    }
}
