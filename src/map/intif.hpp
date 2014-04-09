#ifndef TMWA_MAP_INTIF_HPP
#define TMWA_MAP_INTIF_HPP
//    intif.hpp - Network interface to the internal server.
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

# include "../sanity.hpp"

# include "../strings/fwd.hpp"

# include "map.hpp"

int intif_parse(Session *);

void intif_GMmessage(XString mes);

void intif_wis_message(dumb_ptr<map_session_data> sd, CharName nick, ZString mes);
void intif_wis_message_to_gm(CharName Wisp_name, int min_gm_level, ZString mes);

void intif_saveaccountreg(dumb_ptr<map_session_data> sd);
void intif_request_accountreg(dumb_ptr<map_session_data> sd);

void intif_request_storage(int account_id);
void intif_send_storage(struct storage *stor);

void intif_create_party(dumb_ptr<map_session_data> sd, PartyName name);
void intif_request_partyinfo(int party_id);
void intif_party_addmember(int party_id, int account_id);
void intif_party_changeoption(int party_id, int account_id, int exp,
        int item);
void intif_party_leave(int party_id, int accound_id);
void intif_party_changemap(dumb_ptr<map_session_data> sd, int online);
void intif_party_message(int party_id, int account_id, XString mes);
void intif_party_checkconflict(int party_id, int account_id, CharName nick);

#endif // TMWA_MAP_INTIF_HPP
