#include "globals.hpp"
//    globals.cpp - Evil global variables for tmwa-char.
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

#include "../compat/time_t.hpp"

#include "../generic/db.hpp"

#include "../proto2/net-Storage.hpp"

#include "char.hpp"
#include "char_conf.hpp"
#include "char_lan_conf.hpp"
#include "inter.hpp"
#include "inter_conf.hpp"

#include "../poison.hpp"


namespace tmwa
{
    namespace char_
    {
        CharConf char_conf;
        CharLanConf char_lan_conf;
        InterConf inter_conf;
        Array<mmo_map_server, MAX_MAP_SERVERS> server;
        Array<Session *, MAX_MAP_SERVERS> server_session;
        // Map-server anti-freeze system. Counter. 5 ok, 4...0 freezed
        Array<int, MAX_MAP_SERVERS> server_freezeflag;
        Session *login_session, *char_session;
        const CharName WISP_SERVER_NAME = stringish<CharName>("Server"_s);
        std::array<AuthFifoEntry, 256> auth_fifo;
        decltype(auth_fifo)::iterator auth_fifo_iter = auth_fifo.begin();
        CharId char_id_count = wrap<CharId>(150000);
        std::vector<CharPair> char_keys;
        std::vector<GM_Account> gm_accounts;
        // same size of char_keys, and id value of current server (or -1)
        std::vector<Session *> online_chars;
        // to update online files when we receiving information from a server (not less than 8 seconds)
        TimeT update_online;
        // For forked DB writes
        pid_t pid = 0;

        Map<AccountId, accreg> accreg_db;

        Map<PartyId, PartyMost> party_db;
        PartyId party_newid = wrap<PartyId>(100_u32);

        Map<GuildId, GuildMost> guild_db;
        GuildId guild_newid = wrap<GuildId>(100_u32);

        Map<AccountId, Storage> storage_db;
    } // namespace char_
} // namespace tmwa
