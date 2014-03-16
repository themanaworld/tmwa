#ifndef TMWA_MAP_TRADE_HPP
#define TMWA_MAP_TRADE_HPP

# include "map.hpp"

void trade_traderequest(dumb_ptr<map_session_data> sd, int target_id);
void trade_tradeack(dumb_ptr<map_session_data> sd, int type);
void trade_tradeadditem(dumb_ptr<map_session_data> sd, int index, int amount);
void trade_tradeok(dumb_ptr<map_session_data> sd);
void trade_tradecancel(dumb_ptr<map_session_data> sd);
void trade_tradecommit(dumb_ptr<map_session_data> sd);
void trade_verifyzeny(dumb_ptr<map_session_data> sd);

#endif // TMWA_MAP_TRADE_HPP
