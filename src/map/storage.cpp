#include "storage.hpp"
//    storage.cpp - Storage handling.
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

#include "../generic/db.hpp"

#include "../mmo/ids.hpp"
#include "../mmo/mmo.hpp"

#include "chrif.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "pc.hpp"

#include "../poison.hpp"


namespace tmwa
{
static
Map<AccountId, Storage> storage_db;

void do_final_storage(void)
{
    storage_db.clear();
}

Storage *account2storage(AccountId account_id)
{
    Storage *stor = storage_db.search(account_id);
    if (stor == nullptr)
    {
        stor = storage_db.init(account_id);
        stor->account_id = account_id;
    }
    return stor;
}

// Just to ask storage, without creation
Storage *account2storage2(AccountId account_id)
{
    return storage_db.search(account_id);
}

static
void storage_delete(AccountId account_id)
{
    storage_db.erase(account_id);
}

/*==========================================
 * カプラ倉庫を開く
 *------------------------------------------
 */
int storage_storageopen(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    if (sd->state.storage_open)
        return 1;               //Already open?

    Storage *stor = storage_db.search(sd->status_key.account_id);
    if (stor == nullptr)
    {                           //Request storage.
        intif_request_storage(sd->status_key.account_id);
        return 1;
    }

    if (stor->storage_status)
        return 1;               //Already open/player already has it open...

    stor->storage_status = 1;
    sd->state.storage_open = 1;
    clif_storageitemlist(sd, stor);
    clif_storageequiplist(sd, stor);
    clif_updatestorageamount(sd, stor);
    return 0;
}

/*==========================================
 * Internal add-item function.
 *------------------------------------------
 */
static
int storage_additem(dumb_ptr<map_session_data> sd, Storage *stor,
                            Item *item_data, int amount)
{
    struct item_data *data;

    if (!item_data->nameid || amount <= 0)
        return 1;

    data = itemdb_search(item_data->nameid);

    if (!itemdb_isequip2(data))
    {                           //Stackable
        for (SOff0 i : SOff0::iter())
        {
            if (compare_item(&stor->storage_[i], item_data))
            {
                if (amount > MAX_AMOUNT - stor->storage_[i].amount)
                    return 1;
                stor->storage_[i].amount += amount;
                clif_storageitemadded(sd, stor, i, amount);
                stor->dirty = 1;
                return 0;
            }
        }
    }
    //Add item
    SOff0 i = *std::find_if(SOff0::iter().begin(), SOff0::iter().end(),
            [&stor](SOff0 i_)
            {
                return !stor->storage_[i_].nameid;
            });
    if (!i.ok())
        return 1;

    stor->storage_[i] = *item_data;
    stor->storage_[i].amount = amount;
    stor->storage_amount++;
    clif_storageitemadded(sd, stor, i, amount);
    clif_updatestorageamount(sd, stor);
    stor->dirty = 1;
    return 0;
}

/*==========================================
 * Internal del-item function
 *------------------------------------------
 */
static
int storage_delitem(dumb_ptr<map_session_data> sd, Storage *stor,
        SOff0 n, int amount)
{

    if (!stor->storage_[n].nameid || stor->storage_[n].amount < amount)
        return 1;

    stor->storage_[n].amount -= amount;
    if (stor->storage_[n].amount == 0)
    {
        stor->storage_[n] = Item{};
        stor->storage_amount--;
        clif_updatestorageamount(sd, stor);
    }
    clif_storageitemremoved(sd, n, amount);

    stor->dirty = 1;
    return 0;
}

/*==========================================
 * Add an item to the storage from the inventory.
 *------------------------------------------
 */
int storage_storageadd(dumb_ptr<map_session_data> sd, IOff0 index, int amount)
{
    Storage *stor;

    nullpo_retz(sd);
    stor = account2storage2(sd->status_key.account_id);
    nullpo_retz(stor);

    if ((stor->storage_amount > MAX_STORAGE) || !stor->storage_status)
        return 0;               // storage full / storage closed

    if (!index.ok())
        return 0;

    if (!sd->status.inventory[index].nameid)
        return 0;               //No item on that spot

    if (amount < 1 || amount > sd->status.inventory[index].amount)
        return 0;

//  log_tostorage(sd, index, 0);
    if (storage_additem(sd, stor, &sd->status.inventory[index], amount) == 0)
    {
        // remove item from inventory
        pc_unequipinvyitem(sd, index, CalcStatus::NOW);
        pc_delitem(sd, index, amount, 0);
    }

    return 1;
}

/*==========================================
 * Retrieve an item from the storage.
 *------------------------------------------
 */
int storage_storageget(dumb_ptr<map_session_data> sd, SOff0 index, int amount)
{
    Storage *stor;
    PickupFail flag;

    nullpo_retz(sd);
    stor = account2storage2(sd->status_key.account_id);
    nullpo_retz(stor);

    if (!index.ok())
        return 0;

    if (!stor->storage_[index].nameid)
        return 0;               //Nothing there

    if (amount < 1 || amount > stor->storage_[index].amount)
        return 0;

    if ((flag = pc_additem(sd, &stor->storage_[index], amount)) == PickupFail::OKAY)
        storage_delitem(sd, stor, index, amount);
    else
        clif_additem(sd, IOff0::from(0), 0, flag);
//  log_fromstorage(sd, index, 0);
    return 1;
}

/*==========================================
 * Modified By Valaris to save upon closing [massdriller]
 *------------------------------------------
 */
int storage_storageclose(dumb_ptr<map_session_data> sd)
{
    Storage *stor;

    nullpo_retz(sd);
    stor = account2storage2(sd->status_key.account_id);
    nullpo_retz(stor);

    clif_storageclose(sd);
    if (stor->storage_status)
    {
        if (save_settings & 4)
            chrif_save(sd);    //Invokes the storage saving as well.
        else
            storage_storage_save(sd->status_key.account_id, 0);
    }
    stor->storage_status = 0;
    sd->state.storage_open = 0;

    if (sd->npc_flags.storage)
    {
        sd->npc_flags.storage = 0;
        map_scriptcont(sd, sd->npc_id);
    }

    return 0;
}

/*==========================================
 * When quitting the game.
 *------------------------------------------
 */
int storage_storage_quit(dumb_ptr<map_session_data> sd)
{
    Storage *stor;

    nullpo_retz(sd);

    stor = account2storage2(sd->status_key.account_id);
    if (stor)
    {
        chrif_save(sd);        //Invokes the storage saving as well.
        stor->storage_status = 0;
        sd->state.storage_open = 0;
    }

    return 0;
}

int storage_storage_save(AccountId account_id, int final)
{
    Storage *stor;

    stor = account2storage2(account_id);
    if (!stor)
        return 0;

    if (stor->dirty)
    {
        if (final)
        {
            stor->dirty = 2;
            stor->storage_status = 0;   //To prevent further manipulation of it.
        }
        intif_send_storage(stor);
        return 1;
    }
    if (final)
    {                           //Clear storage from memory. Nothing to save.
        storage_delete(account_id);
        return 1;
    }

    return 0;
}

//Ack from Char-server indicating the storage was saved. [Skotlex]
int storage_storage_saved(AccountId account_id)
{
    Storage *stor = account2storage2(account_id);

    if (stor)
    {
        //Only mark it clean if it's not in use. [Skotlex]
        if (stor->dirty && stor->storage_status == 0)
        {
            stor->dirty = 0;
            // sortage_sortitem(stor);
        }
        return 1;
    }
    return 0;
}
} // namespace tmwa
