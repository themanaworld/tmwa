#pragma once
//    party.hpp - Small groups of temporary allies.
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

#include <functional>


namespace tmwa
{
namespace map
{
void do_init_party(void);
Option<PartyPair> party_search(PartyId party_id);
Option<PartyPair> party_searchname(PartyName str);

int party_create(dumb_ptr<map_session_data> sd, PartyName name);
void party_created(AccountId account_id, int fail, PartyId party_id, PartyName name);
void party_request_info(PartyId party_id);
int party_invite(dumb_ptr<map_session_data> sd, AccountId account_id);
int party_member_added(PartyId party_id, AccountId account_id, int flag);
int party_leave(dumb_ptr<map_session_data> sd);
int party_removemember(dumb_ptr<map_session_data> sd, AccountId account_id);
int party_member_leaved(PartyId party_id, AccountId account_id, CharName name);
int party_reply_invite(dumb_ptr<map_session_data> sd, AccountId account_id,
        int flag);
int party_recv_noinfo(PartyId party_id);
int party_recv_info(const PartyPair sp);
void party_recv_movemap(PartyId party_id, AccountId account_id, MapName map,
        int online, int lv);
int party_broken(PartyId party_id);
int party_optionchanged(PartyId party_id, AccountId account_id, int exp, int item,
        int flag);
int party_changeoption(dumb_ptr<map_session_data> sd, int exp, int item);

int party_send_movemap(dumb_ptr<map_session_data> sd);
int party_send_logout(dumb_ptr<map_session_data> sd);

void party_send_message(dumb_ptr<map_session_data> sd, XString mes);
void party_recv_message(PartyId party_id, AccountId account_id, XString mes);

void party_send_xy_clear(PartyPair p);
void party_send_hp_check(dumb_ptr<block_list> bl, PartyId party_id, int *flag);

int party_exp_share(PartyPair p, Borrowed<map_local> map, int base_exp, int job_exp);

void party_foreachsamemap(std::function<void(dumb_ptr<block_list>)> func,
        dumb_ptr<map_session_data> sd, int type);
} // namespace map
} // namespace tmwa
