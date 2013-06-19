#ifndef INTIF_HPP
#define INTIF_HPP

#include "../common/const_array.hpp"

#include "map.hpp"

int intif_parse(int fd);

void intif_GMmessage(const_string mes);

void intif_wis_message(dumb_ptr<map_session_data> sd, const char *nick, const char *mes);
void intif_wis_message_to_gm(const char *Wisp_name, int min_gm_level, const char *mes);

void intif_saveaccountreg(dumb_ptr<map_session_data> sd);
void intif_request_accountreg(dumb_ptr<map_session_data> sd);

void intif_request_storage(int account_id);
void intif_send_storage(struct storage *stor);

void intif_create_party(dumb_ptr<map_session_data> sd, const char *name);
void intif_request_partyinfo(int party_id);
void intif_party_addmember(int party_id, int account_id);
void intif_party_changeoption(int party_id, int account_id, int exp,
        int item);
void intif_party_leave(int party_id, int accound_id);
void intif_party_changemap(dumb_ptr<map_session_data> sd, int online);
void intif_party_message(int party_id, int account_id, const char *mes);
void intif_party_checkconflict(int party_id, int account_id, const char *nick);

#endif // INTIF_HPP
