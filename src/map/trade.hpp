#ifndef TRADE_HPP
#define TRADE_HPP

void trade_traderequest(struct map_session_data *sd, int target_id);
void trade_tradeack(struct map_session_data *sd, int type);
void trade_tradeadditem(struct map_session_data *sd, int index, int amount);
void trade_tradeok(struct map_session_data *sd);
void trade_tradecancel(struct map_session_data *sd);
void trade_tradecommit(struct map_session_data *sd);
void trade_verifyzeny(struct map_session_data *sd);

#endif // TRADE_HPP
