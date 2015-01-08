#include "globals.hpp"
//    globals.cpp - Evil global variables for tmwa-admin.
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

#include "../strings/tstring.hpp"

#include "../mmo/ids.hpp"

#include "admin_conf.hpp"

#include "../poison.hpp"


namespace tmwa
{
    namespace admin
    {
        bool eathena_interactive_session;
        AdminConf admin_conf;
        Session *login_session;
        // flag to know if we waiting bytes from login-server
        bool bytes_to_read = false;
        // needs to be global since it's passed to the parse function
        // really should be added to session data
        TString parameters;
        // parameters to display a list of accounts
        AccountId list_first, list_last;
        int list_type, list_count;
        // sometimes, the exit function is called twice... so, don't log twice the message
        bool already_exit_function = false;
    } // namespace admin
} // namespace tmwa
