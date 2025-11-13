#pragma once
//    globals.hpp - Evil global variables for tmwa-login.
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

#include <vector>

#include "../net/timer.t.hpp"

#include "consts.hpp"


namespace tmwa
{
    namespace login
    {
        extern LoginConf login_conf;
        extern LoginLanConf login_lan_conf;
        extern AccountId account_id_count;
        extern tick_t creation_time_GM_account_file;
        extern Array<mmo_char_server, MAX_SERVERS> server;
        extern Array<Session *, MAX_SERVERS> server_session;
        extern Array<int, MAX_SERVERS> server_freezeflag;
        extern Session *login_session;
        extern int auth_fifo_pos;
        extern std::vector<AuthData> auth_data;
        extern DMap<AccountId, GmLevel> gm_account_db;
        extern pid_t pid;
    } // namespace login
} // namespace tmwa
