#ifndef TMWA_MAP_TMW_HPP
#define TMWA_MAP_TMW_HPP
//    tmw.hpp - Some random functions added by TMW.
//
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

# include "fwd.hpp"

# include "../strings/fwd.hpp"

# include "../mmo/dumb_ptr.hpp"

# include "map.hpp"

int tmw_CheckChatSpam(dumb_ptr<map_session_data> sd, XString message);
void tmw_GmHackMsg(ZString line);

#endif // TMWA_MAP_TMW_HPP
