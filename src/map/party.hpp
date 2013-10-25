#ifndef PARTY_HPP
#define PARTY_HPP

#include <functional>

#include "../strings/fwd.hpp"

#include "map.hpp"

struct party;
struct map_session_data;
struct block_list;

void do_init_party(void);
struct party *party_search(int party_id);
struct party *party_searchname(PartyName str);

int party_create(dumb_ptr<map_session_data> sd, PartyName name);
void party_created(int account_id, int fail, int party_id, PartyName name);
void party_request_info(int party_id);
int party_invite(dumb_ptr<map_session_data> sd, int account_id);
int party_member_added(int party_id, int account_id, int flag);
int party_leave(dumb_ptr<map_session_data> sd);
int party_removemember(dumb_ptr<map_session_data> sd, int account_id);
int party_member_leaved(int party_id, int account_id, CharName name);
int party_reply_invite(dumb_ptr<map_session_data> sd, int account_id,
        int flag);
int party_recv_noinfo(int party_id);
int party_recv_info(const struct party *sp);
void party_recv_movemap(int party_id, int account_id, MapName map,
        int online, int lv);
int party_broken(int party_id);
int party_optionchanged(int party_id, int account_id, int exp, int item,
        int flag);
int party_changeoption(dumb_ptr<map_session_data> sd, int exp, int item);

int party_send_movemap(dumb_ptr<map_session_data> sd);
int party_send_logout(dumb_ptr<map_session_data> sd);

void party_send_message(dumb_ptr<map_session_data> sd, XString mes);
void party_recv_message(int party_id, int account_id, XString mes);

void party_send_xy_clear(struct party *p);
void party_send_hp_check(dumb_ptr<block_list> bl, int party_id, int *flag);

int party_exp_share(struct party *p, map_local *map, int base_exp, int job_exp);

void party_foreachsamemap(std::function<void(dumb_ptr<block_list>)> func,
        dumb_ptr<map_session_data> sd, int type);

#endif // PARTY_HPP
