#pragma once
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

#include "fwd.hpp"


namespace tmwa
{
namespace map
{
RecvResult intif_parse(Session *, uint16_t packet_id);

void intif_GMmessage(XString mes);

void intif_wis_message(dumb_ptr<map_session_data> sd, CharName nick, ZString mes);
void intif_wis_message_to_gm(CharName Wisp_name, GmLevel min_gm_level, ZString mes);

void intif_saveaccountreg(dumb_ptr<map_session_data> sd);
void intif_request_accountreg(dumb_ptr<map_session_data> sd);

void intif_request_storage(AccountId account_id);
void intif_send_storage(Borrowed<Storage> stor);

void intif_create_party(dumb_ptr<map_session_data> sd, PartyName name);
void intif_request_partyinfo(PartyId party_id);
void intif_party_addmember(PartyId party_id, AccountId account_id);
void intif_party_changeoption(PartyId party_id, AccountId account_id, int exp,
        int item);
void intif_party_leave(PartyId party_id, AccountId accound_id);
void intif_party_changemap(dumb_ptr<map_session_data> sd, int online);
void intif_party_message(PartyId party_id, AccountId account_id, XString mes);
void intif_party_checkconflict(PartyId party_id, AccountId account_id, CharName nick);
void intif_guild_create(dumb_ptr<map_session_data> sd, GuildName guild_name);
void intif_request_guildinfo(GuildId guild_id, AccountId account_id);
void intif_guild_addmember (GuildId guild_invite, AccountId id, int i, int lv, CharName name);
void intif_guild_leave(GuildId guild_id, AccountId account_id, int flag, AString mes);
void intif_guild_message(GuildId guild_id, AccountId account_id, AString mbuf);
void intif_guild_expulsion(dumb_ptr<map_session_data> sd, GuildId guild_id, AccountId account_id, /*CharId char_id,*/ XString reason);
void intif_guild_poschanged(GuildId guild_id, AccountId account_id, int position);
} // namespace map
} // namespace tmwa
