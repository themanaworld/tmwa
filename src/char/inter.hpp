#ifndef TMWA_CHAR_INTER_HPP
#define TMWA_CHAR_INTER_HPP
//    inter.hpp - Internal server.
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

# include "fwd.hpp"

# include "../strings/fwd.hpp"

# include "../net/fwd.hpp"


namespace tmwa
{
bool inter_config(XString key, ZString value);
void inter_init2();
void inter_save(void);
RecvResult inter_parse_frommap(Session *ms, uint16_t packet_id);

extern int party_share_level;
} // namespace tmwa

#endif // TMWA_CHAR_INTER_HPP
