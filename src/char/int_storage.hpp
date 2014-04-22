#ifndef TMWA_CHAR_INT_STORAGE_HPP
#define TMWA_CHAR_INT_STORAGE_HPP
//    int_storage.hpp - Internal storage handling.
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

# include "../strings/fwd.hpp"

# include "../mmo/fwd.hpp"

void inter_storage_init(void);
int inter_storage_save(void);
void inter_storage_delete(AccountId account_id);
struct storage *account2storage(AccountId account_id);

int inter_storage_parse_frommap(Session *ms);

extern AString storage_txt;

#endif // TMWA_CHAR_INT_STORAGE_HPP
