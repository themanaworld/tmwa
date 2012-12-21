#ifndef PARTY_HPP
#define PARTY_HPP

#include <functional>

struct party;
struct map_session_data;
struct block_list;

void do_init_party(void);
struct party *party_search(int party_id);
struct party *party_searchname(const char *str);

int party_create(struct map_session_data *sd, const char *name);
int party_created(int account_id, int fail, int party_id, const char *name);
int party_request_info(int party_id);
int party_invite(struct map_session_data *sd, int account_id);
int party_member_added(int party_id, int account_id, int flag);
int party_leave(struct map_session_data *sd);
int party_removemember(struct map_session_data *sd, int account_id,
                         const char *name);
int party_member_leaved(int party_id, int account_id, const char *name);
int party_reply_invite(struct map_session_data *sd, int account_id,
                         int flag);
int party_recv_noinfo(int party_id);
int party_recv_info(struct party *sp);
int party_recv_movemap(int party_id, int account_id, const char *map, int online,
                         int lv);
int party_broken(int party_id);
int party_optionchanged(int party_id, int account_id, int exp, int item,
                          int flag);
int party_changeoption(struct map_session_data *sd, int exp, int item);

int party_send_movemap(struct map_session_data *sd);
int party_send_logout(struct map_session_data *sd);

int party_send_message(struct map_session_data *sd, const char *mes, int len);
int party_recv_message(int party_id, int account_id, const char *mes, int len);

int party_send_xy_clear(struct party *p);
void party_send_hp_check(struct block_list *bl, int party_id, int *flag);

int party_exp_share(struct party *p, int map, int base_exp, int job_exp);

void party_foreachsamemap(std::function<void(struct block_list *)> func,
                           struct map_session_data *sd, int type);

#endif // PARTY_HPP
