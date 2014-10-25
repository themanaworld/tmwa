#pragma once
//    chrif.hpp - Network interface to the character server.
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

#include "fwd.hpp"


namespace tmwa
{
void chrif_setuserid(AccountName);
void chrif_setpasswd(AccountPass);
AccountPass chrif_getpasswd(void);

void chrif_setip(IP4Address);
void chrif_setport(int);

int chrif_isconnect(void);

int chrif_authreq(dumb_ptr<map_session_data>);
int chrif_save(dumb_ptr<map_session_data>);
int chrif_charselectreq(dumb_ptr<map_session_data>);

int chrif_changemapserver(dumb_ptr<map_session_data> sd,
        MapName name, int x, int y,
        IP4Address ip, short port);

void chrif_changegm(AccountId id, ZString pass);
void chrif_changeemail(AccountId id, AccountEmail actual_email, AccountEmail new_email);
void chrif_char_ask_name(AccountId id, CharName character_name, short operation_type,
        HumanTimeDiff modif);
int chrif_saveaccountreg2(dumb_ptr<map_session_data> sd);
int chrif_reloadGMdb(void);
int chrif_send_divorce(CharId char_id);

void do_init_chrif(void);

// only used by intif.cpp
// and clif.cpp for the new on_delete stuff ...
extern Session *char_session;
} // namespace tmwa
