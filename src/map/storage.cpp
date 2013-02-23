// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see COPYING in the main folder

#include "storage.hpp"

#include <cstdlib>
#include <cstring>

#include "../common/db.hpp"
#include "../common/nullpo.hpp"

#include "chrif.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "pc.hpp"

#include "../poison.hpp"

static
Map<int, struct storage> storage_db;

/*==========================================
 * 倉庫内アイテムソート
 *------------------------------------------
 */
static
int storage_comp_item(const void *_i1, const void *_i2)
{
    const struct item *i1 = (const struct item *) _i1;
    const struct item *i2 = (const struct item *) _i2;

    if (i1->nameid == i2->nameid)
        return 0;
    else if (!(i1->nameid) || !(i1->amount))
        return 1;
    else if (!(i2->nameid) || !(i2->amount))
        return -1;
    return i1->nameid - i2->nameid;
}

static
void sortage_sortitem(struct storage *stor)
{
    nullpo_retv(stor);
    qsort(stor->storage_, MAX_STORAGE, sizeof(struct item),
           storage_comp_item);
}

void do_init_storage(void)
{
}

void do_final_storage(void)
{
    storage_db.clear();
}

struct storage *account2storage(int account_id)
{
    struct storage *stor = storage_db.search(account_id);
    if (stor == NULL)
    {
        stor = storage_db.init(account_id);
        stor->account_id = account_id;
    }
    return stor;
}

// Just to ask storage, without creation
struct storage *account2storage2(int account_id)
{
    return storage_db.search(account_id);
}

static
void storage_delete(int account_id)
{
    storage_db.erase(account_id);
}

/*==========================================
 * カプラ倉庫を開く
 *------------------------------------------
 */
int storage_storageopen(struct map_session_data *sd)
{
    nullpo_ret(sd);

    if (sd->state.storage_open)
        return 1;               //Already open?

    struct storage *stor = storage_db.search(sd->status.account_id);
    if (stor == NULL)
    {                           //Request storage.
        intif_request_storage(sd->status.account_id);
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
int storage_additem(struct map_session_data *sd, struct storage *stor,
                            struct item *item_data, int amount)
{
    struct item_data *data;
    int i;

    if (item_data->nameid <= 0 || amount <= 0)
        return 1;

    data = itemdb_search(item_data->nameid);

    if (!itemdb_isequip2(data))
    {                           //Stackable
        for (i = 0; i < MAX_STORAGE; i++)
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
    for (i = 0; i < MAX_STORAGE && stor->storage_[i].nameid; i++);

    if (i >= MAX_STORAGE)
        return 1;

    memcpy(&stor->storage_[i], item_data, sizeof(stor->storage_[0]));
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
int storage_delitem(struct map_session_data *sd, struct storage *stor,
                            int n, int amount)
{

    if (stor->storage_[n].nameid == 0 || stor->storage_[n].amount < amount)
        return 1;

    stor->storage_[n].amount -= amount;
    if (stor->storage_[n].amount == 0)
    {
        memset(&stor->storage_[n], 0, sizeof(stor->storage_[0]));
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
int storage_storageadd(struct map_session_data *sd, int index, int amount)
{
    struct storage *stor;

    nullpo_ret(sd);
    stor = account2storage2(sd->status.account_id);
    nullpo_ret(stor);

    if ((stor->storage_amount > MAX_STORAGE) || !stor->storage_status)
        return 0;               // storage full / storage closed

    if (index < 0 || index >= MAX_INVENTORY)
        return 0;

    if (sd->status.inventory[index].nameid <= 0)
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
int storage_storageget(struct map_session_data *sd, int index, int amount)
{
    struct storage *stor;
    PickupFail flag;

    nullpo_ret(sd);
    stor = account2storage2(sd->status.account_id);
    nullpo_ret(stor);

    if (index < 0 || index >= MAX_STORAGE)
        return 0;

    if (stor->storage_[index].nameid <= 0)
        return 0;               //Nothing there

    if (amount < 1 || amount > stor->storage_[index].amount)
        return 0;

    if ((flag = pc_additem(sd, &stor->storage_[index], amount)) == PickupFail::OKAY)
        storage_delitem(sd, stor, index, amount);
    else
        clif_additem(sd, 0, 0, flag);
//  log_fromstorage(sd, index, 0);
    return 1;
}

/*==========================================
 * Modified By Valaris to save upon closing [massdriller]
 *------------------------------------------
 */
int storage_storageclose(struct map_session_data *sd)
{
    struct storage *stor;

    nullpo_ret(sd);
    stor = account2storage2(sd->status.account_id);
    nullpo_ret(stor);

    clif_storageclose(sd);
    if (stor->storage_status)
    {
        if (save_settings & 4)
            chrif_save(sd);    //Invokes the storage saving as well.
        else
            storage_storage_save(sd->status.account_id, 0);
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
int storage_storage_quit(struct map_session_data *sd)
{
    struct storage *stor;

    nullpo_ret(sd);

    stor = account2storage2(sd->status.account_id);
    if (stor)
    {
        chrif_save(sd);        //Invokes the storage saving as well.
        stor->storage_status = 0;
        sd->state.storage_open = 0;
    }

    return 0;
}

int storage_storage_save(int account_id, int final)
{
    struct storage *stor;

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
int storage_storage_saved(int account_id)
{
    struct storage *stor;

    if ((stor = account2storage2(account_id)) != NULL)
    {                           //Only mark it clean if it's not in use. [Skotlex]
        if (stor->dirty && stor->storage_status == 0)
        {
            stor->dirty = 0;
            sortage_sortitem(stor);
        }
        return 1;
    }
    return 0;
}
