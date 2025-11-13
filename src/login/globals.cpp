#include "globals.hpp"
//    globals.cpp - Evil global variables for tmwa-login.
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

#include "../generic/array.hpp"
#include "../generic/db.hpp"

#include "login.hpp"
#include "login_conf.hpp"
#include "login_lan_conf.hpp"

#include "../poison.hpp"


namespace tmwa
{
    namespace login
    {
        LoginConf login_conf;
        LoginLanConf login_lan_conf;
        AccountId account_id_count = START_ACCOUNT_NUM;
        tick_t creation_time_GM_account_file;
        Array<mmo_char_server, MAX_SERVERS> server;
        Array<Session *, MAX_SERVERS> server_session;
        // Char-server anti-freeze system. Counter. 5 ok, 4...0 freezed
        Array<int, MAX_SERVERS> server_freezeflag;
        Session *login_session;
        // minimum level of player/GM (0: player, 1-99: gm) to connect on the server
        std::vector<AuthData> auth_data;
        DMap<AccountId, GmLevel> gm_account_db;
        // For forked DB writes
        pid_t pid = 0;
    } // namespace login
} // namespace tmwa
