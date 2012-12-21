#ifndef CLIF_HPP
#define CLIF_HPP

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "map.hpp"
#include "storage.hpp"

void clif_setip(const char *);
void clif_setport(int);

struct in_addr clif_getip(void);
int clif_getport(void);
int clif_countusers(void);
void clif_setwaitclose(int);

int clif_authok(struct map_session_data *);
int clif_authfail_fd(int, int);
int clif_charselectok(int);
int clif_dropflooritem(struct flooritem_data *);
int clif_clearflooritem(struct flooritem_data *, int);
int clif_clearchar(struct block_list *, int); // area or fd
int clif_clearchar_delay(unsigned int, struct block_list *, int);
#define clif_clearchar_area(bl,type) clif_clearchar(bl,type)
int clif_clearchar_id(int, int, int);
int clif_spawnpc(struct map_session_data *);  //area
int clif_spawnnpc(struct npc_data *); // area
int clif_spawn_fake_npc_for_player(struct map_session_data *sd,
                                     int fake_npc_id);
int clif_spawnmob(struct mob_data *); // area
int clif_walkok(struct map_session_data *);   // self
int clif_movechar(struct map_session_data *); // area
int clif_movemob(struct mob_data *);  //area
int clif_changemap(struct map_session_data *, const char *, int, int);  //self
int clif_changemapserver(struct map_session_data *, const char *, int, int, struct in_addr, int);  //self
int clif_fixpos(struct block_list *); // area
int clif_fixmobpos(struct mob_data *md);
int clif_fixpcpos(struct map_session_data *sd);
int clif_npcbuysell(struct map_session_data *, int);  //self
int clif_buylist(struct map_session_data *, struct npc_data *);   //self
int clif_selllist(struct map_session_data *); //self
int clif_scriptmes(struct map_session_data *, int, const char *);   //self
int clif_scriptnext(struct map_session_data *, int);  //self
int clif_scriptclose(struct map_session_data *, int); //self
int clif_scriptmenu(struct map_session_data *, int, const char *);  //self
int clif_scriptinput(struct map_session_data *, int); //self
int clif_scriptinputstr(struct map_session_data *sd, int npcid);  // self
int clif_cutin(struct map_session_data *, const char *, int);   //self
int clif_viewpoint(struct map_session_data *, int, int, int, int, int, int);  //self
int clif_additem(struct map_session_data *, int, int, int);   //self
int clif_delitem(struct map_session_data *, int, int);    //self
int clif_updatestatus(struct map_session_data *, int);    //self
int clif_changestatus(struct block_list *, int, int); //area
int clif_damage(struct block_list *, struct block_list *, unsigned int, int, int, int, int, int, int);    // area
#define clif_takeitem(src,dst) clif_damage(src,dst,0,0,0,0,0,1,0)
int clif_changelook(struct block_list *, int, int);   // area
void clif_changelook_accessories(struct block_list *bl, struct map_session_data *dst); // area or target; list gloves, boots etc.
int clif_arrowequip(struct map_session_data *sd, int val);    //self
int clif_arrow_fail(struct map_session_data *sd, int type);   //self
int clif_statusupack(struct map_session_data *, int, int, int);   // self
int clif_equipitemack(struct map_session_data *, int, int, int);  // self
int clif_unequipitemack(struct map_session_data *, int, int, int);    // self
int clif_misceffect(struct block_list *, int);    // area
int clif_changeoption(struct block_list *);   // area
int clif_useitemack(struct map_session_data *, int, int, int);    // self

int clif_dispchat(struct chat_data *, int);   // area or fd
int clif_changechatowner(struct chat_data *, struct map_session_data *);  // chat
int clif_clearchat(struct chat_data *, int);  // area or fd
int clif_leavechat(struct chat_data *, struct map_session_data *);    // chat

void clif_emotion(struct block_list *bl, int type);
void clif_talkiebox(struct block_list *bl, const char *talkie);
void clif_wedding_effect(struct block_list *bl);
void clif_sitting(int fd, struct map_session_data *sd);
//void clif_sitting(struct map_session_data *sd);
void clif_soundeffect(struct map_session_data *sd, struct block_list *bl,
                       const char *name, int type);

// trade
int clif_traderequest(struct map_session_data *sd, const char *name);
int clif_tradestart(struct map_session_data *sd, int type);
int clif_tradeadditem(struct map_session_data *sd,
                        struct map_session_data *tsd, int index, int amount);
int clif_tradeitemok(struct map_session_data *sd, int index, int amount,
                       int fail);
int clif_tradedeal_lock(struct map_session_data *sd, int fail);
int clif_tradecancelled(struct map_session_data *sd);
int clif_tradecompleted(struct map_session_data *sd, int fail);

// storage
int clif_storageitemlist(struct map_session_data *sd, struct storage *stor);
int clif_storageequiplist(struct map_session_data *sd,
                            struct storage *stor);
int clif_updatestorageamount(struct map_session_data *sd,
                               struct storage *stor);
int clif_storageitemadded(struct map_session_data *sd, struct storage *stor,
                            int index, int amount);
int clif_storageitemremoved(struct map_session_data *sd, int index,
                              int amount);
int clif_storageclose(struct map_session_data *sd);

// map_forallinmovearea callbacks
void clif_pcinsight(struct block_list *, struct map_session_data *);
void clif_pcoutsight(struct block_list *, struct map_session_data *);
void clif_mobinsight(struct block_list *, struct mob_data *);
void clif_moboutsight(struct block_list *, struct mob_data *);

int clif_npc_class_change(struct block_list *bl, int npc_class, int type);
int clif_mob_class_change(struct mob_data *md, int mob_class);
int clif_mob_equip(struct mob_data *md, int nameid);  // [Valaris]

int clif_skillinfo(struct map_session_data *sd, SkillID skillid, int type,
                     int range);
int clif_skillinfoblock(struct map_session_data *sd);
int clif_skillup(struct map_session_data *sd, SkillID skill_num);

int clif_skillcasting(struct block_list *bl,
                        int src_id, int dst_id, int dst_x, int dst_y,
                        SkillID skill_num, int casttime);
int clif_skillcastcancel(struct block_list *bl);
int clif_skill_fail(struct map_session_data *sd, SkillID skill_id, int type,
                      int btype);
int clif_skill_damage(struct block_list *src, struct block_list *dst,
                        unsigned int tick, int sdelay, int ddelay, int damage,
                        int div, SkillID skill_id, int skill_lv, int type);
int clif_skill_nodamage(struct block_list *src, struct block_list *dst,
                          SkillID skill_id, int heal, int fail);
int clif_skill_poseffect(struct block_list *src, SkillID skill_id, int val,
                           int x, int y, int tick);
int clif_skill_estimation(struct map_session_data *sd,
                            struct block_list *dst);
int clif_skill_warppoint(struct map_session_data *sd, SkillID skill_num,
                           const char *map1, const char *map2,
                           const char *map3, const char *map4);
int clif_skill_memo(struct map_session_data *sd, int flag);
int clif_skill_teleportmessage(struct map_session_data *sd, int flag);

int clif_produceeffect(struct map_session_data *sd, int flag, int nameid);

int clif_skill_setunit(struct skill_unit *unit);
int clif_skill_delunit(struct skill_unit *unit);

int clif_01ac(struct block_list *bl);

int clif_autospell(struct map_session_data *sd, int skilllv);
int clif_devotion(struct map_session_data *sd, int target);
int clif_spiritball(struct map_session_data *sd);
int clif_combo_delay(struct block_list *src, int wait);
int clif_bladestop(struct block_list *src, struct block_list *dst,
                     int boolean);
int clif_changemapcell(int m, int x, int y, int cell_type, int type);

int clif_status_change(struct block_list *bl,
        StatusChange type, int flag);

int clif_wis_message(int fd, const char *nick, const char *mes, int mes_len);
int clif_wis_end(int fd, int flag);

int clif_solved_charname(struct map_session_data *sd, int char_id);

int clif_itemlist(struct map_session_data *sd);
int clif_equiplist(struct map_session_data *sd);

int clif_cart_additem(struct map_session_data *, int, int, int);
int clif_cart_delitem(struct map_session_data *, int, int);
int clif_cart_itemlist(struct map_session_data *sd);
int clif_cart_equiplist(struct map_session_data *sd);

int clif_item_identify_list(struct map_session_data *sd);
int clif_item_repair_list(struct map_session_data *sd);

int clif_item_skill(struct map_session_data *sd, SkillID skillid, int skilllv,
                      const char *name);

int clif_mvp_effect(struct map_session_data *sd);
int clif_mvp_item(struct map_session_data *sd, int nameid);
int clif_mvp_exp(struct map_session_data *sd, int exp);

int clif_movetoattack(struct map_session_data *sd, struct block_list *bl);

// party
int clif_party_created(struct map_session_data *sd, int flag);
int clif_party_info(struct party *p, int fd);
int clif_party_invite(struct map_session_data *sd,
                        struct map_session_data *tsd);
int clif_party_inviteack(struct map_session_data *sd, const char *nick, int flag);
int clif_party_option(struct party *p, struct map_session_data *sd,
                        int flag);
int clif_party_leaved(struct party *p, struct map_session_data *sd,
                        int account_id, const char *name, int flag);
int clif_party_message(struct party *p, int account_id, const char *mes, int len);
int clif_party_xy(struct party *p, struct map_session_data *sd);
int clif_party_hp(struct party *p, struct map_session_data *sd);

// atcommand
int clif_displaymessage(int fd, const char *mes);
int clif_disp_onlyself(struct map_session_data *sd, const char *mes, int len);
int clif_GMmessage(struct block_list *bl, const char *mes, int len, int flag);
int clif_heal(int fd, int type, int val);
int clif_resurrection(struct block_list *bl, int type);
int clif_set0199(int fd, int type);
int clif_pvpset(struct map_session_data *sd, int pvprank, int pvpnum,
                  int type);
int clif_send0199(int map, int type);
int clif_refine(int fd, struct map_session_data *sd, int fail, int index,
                  int val);

int clif_specialeffect(struct block_list *bl, int type, int flag);    // special effects [Valaris]
int clif_message(struct block_list *bl, const char *msg);   // messages (from mobs/npcs) [Valaris]

int clif_GM_kick(struct map_session_data *sd, struct map_session_data *tsd,
                   int type);

int clif_foreachclient(std::function<void(struct map_session_data *)>);

int do_init_clif (void);

#endif // CLIF_HPP
