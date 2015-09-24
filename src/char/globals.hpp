#pragma once
//    globals.hpp - Evil global variables for tmwa-char.
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include <sys/types.h>

#include <array>
#include <vector>

#include "consts.hpp"


namespace tmwa
{
    namespace char_
    {
        extern CharConf char_conf;
        extern CharLanConf char_lan_conf;
        extern InterConf inter_conf;
        extern Array<mmo_map_server, MAX_MAP_SERVERS> server;
        extern Array<Session *, MAX_MAP_SERVERS> server_session;
        extern Array<int, MAX_MAP_SERVERS> server_freezeflag;
        extern Session *login_session, *char_session;
        extern const CharName WISP_SERVER_NAME;
        extern std::array<AuthFifoEntry, 256> auth_fifo;
        extern AuthFifoEntry *auth_fifo_iter;
        extern CharId char_id_count;
        extern std::vector<CharPair> char_keys;
        extern std::vector<GM_Account> gm_accounts;
        extern std::vector<Session *> online_chars;
        extern TimeT update_online;
        extern pid_t pid;

        extern Map<AccountId, accreg> accreg_db;

        extern Map<PartyId, PartyMost> party_db;
        extern PartyId party_newid;

        extern Map<GuildId, GuildMost> guild_db;
        extern GuildId guild_newid;

        extern Map<AccountId, Storage> storage_db;
    } // namespace char_
} // namespace tmwa
