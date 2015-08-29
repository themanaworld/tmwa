#pragma once
//    guild.hpp - Large groups of long term allies.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2013 Freeyorp
//    Copyright © 2015 Rawng
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
struct GuildPair;
namespace map
{

int guild_check_empty(const GuildPair gp);
void guild_create(dumb_ptr<map_session_data> sd, GuildName guild_name);
void guild_created(AccountId account_id, int fail, GuildId guild_id, GuildName guild_name);
void guild_makemember(dumb_ptr<map_session_data> sd, int i, GuildPair gp);
void guild_recv_info(GuildPair gp);
void guild_recv_noinfo(GuildPair gp);
void guild_request_info(GuildId guild_id, AccountId account_id);
Option<GuildPair> guild_search(GuildId guild_id);
Option<GuildPair> guild_searchname(GuildName str);
XString guild_get_position(GuildPair gp, AccountId acct);
void guild_break(GuildId guild_id);
void guild_invite(dumb_ptr<map_session_data> sd, AccountId account_id);
void guild_reply_invite(dumb_ptr<map_session_data> sd, GuildId guild_id, int flag);
void guild_memberadded(GuildId guild_id, AccountId account_id, int flag);
void guild_leave(dumb_ptr<map_session_data> sd, GuildId guild_id, AccountId account_id, AString mes);
void guild_member_left(GuildId guild_id, AccountId account_id, int flag, AString mes, CharName name);
void guild_send_logout(dumb_ptr<map_session_data> sd);
void guild_send_message(dumb_ptr<map_session_data> sd, AString mbuf);
void guild_recv_message(GuildId guild_id, XString mes);
void guild_expulsion(dumb_ptr<map_session_data> sd, GuildId guild_id, AccountId account_id/*, CharId char_id*/, XString reason);
void guild_expelled(AccountId kicked_id, XString reason, AccountId kicker_id);
void guild_changememberpos(GuildId guild_id, AccountId acct_id, int position);
void guild_changememberpos_request(dumb_ptr<map_session_data> sd, AccountId acct_id, int position);

} // namespace map
} // namespace tmwa
