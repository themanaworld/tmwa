#pragma once
//    atcommand.hpp - GM commands.
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
bool is_atcommand(Session *s, dumb_ptr<map_session_data> sd,
        ZString message, GmLevel gmlvl);

bool atcommand_config_read(ZString cfgName);

void log_atcommand(dumb_ptr<map_session_data> sd, ZString cmd);

// only used by map.cpp
extern AString gm_log;

void atcommand_config_write(ZString cfgName);
} // namespace tmwa
