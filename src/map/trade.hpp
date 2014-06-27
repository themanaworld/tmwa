#ifndef TMWA_MAP_TRADE_HPP
#define TMWA_MAP_TRADE_HPP
//    trade.hpp - Inter-player item and money transactions.
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

# include "../generic/fwd.hpp"

# include "clif.t.hpp"


namespace tmwa
{
void trade_traderequest(dumb_ptr<map_session_data> sd, BlockId target_id);
void trade_tradeack(dumb_ptr<map_session_data> sd, int type);
void trade_tradeadditem(dumb_ptr<map_session_data> sd, IOff2 index, int amount);
void trade_tradeok(dumb_ptr<map_session_data> sd);
void trade_tradecancel(dumb_ptr<map_session_data> sd);
void trade_tradecommit(dumb_ptr<map_session_data> sd);
void trade_verifyzeny(dumb_ptr<map_session_data> sd);
} // namespace tmwa

#endif // TMWA_MAP_TRADE_HPP
