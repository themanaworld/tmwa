#ifndef TMWA_MAP_STORAGE_HPP
#define TMWA_MAP_STORAGE_HPP
//    storage.hpp - Storage handling.
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

# include "fwd.hpp"

# include "../generic/fwd.hpp"

# include "../mmo/fwd.hpp"

int storage_storageopen(dumb_ptr<map_session_data> sd);
int storage_storageadd(dumb_ptr<map_session_data> sd, int index, int amount);
int storage_storageget(dumb_ptr<map_session_data> sd, int index, int amount);
int storage_storageclose(dumb_ptr<map_session_data> sd);
void do_final_storage(void);
Storage *account2storage(AccountId account_id);
Storage *account2storage2(AccountId account_id);
int storage_storage_quit(dumb_ptr<map_session_data> sd);
int storage_storage_save(AccountId account_id, int final);
int storage_storage_saved(AccountId account_id);

#endif // TMWA_MAP_STORAGE_HPP
