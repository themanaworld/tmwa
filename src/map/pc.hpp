#ifndef PC_HPP
#define PC_HPP

#include "pc.t.hpp"

#include "clif.t.hpp"
#include "map.hpp"

inline
void pc_setsit(struct map_session_data *sd)
{
    sd->state.dead_sit = 2;
}
//pc_setstand(struct map_session_data *sd) is a normal function
inline
bool pc_isdead(struct map_session_data *sd)
{
    return sd->state.dead_sit == 1;
}
inline
bool pc_issit(struct map_session_data *sd)
{
    return sd->state.dead_sit == 2;
}
inline
void pc_setdir(struct map_session_data *sd, DIR b)
{
    sd->dir = (b);
}
inline
void pc_setchatid(struct map_session_data *sd, int n)
{
    sd->chatID = n;
}
inline
bool pc_isinvisible(struct map_session_data *sd)
{
    return bool(sd->status.option & Option::HIDE);
}
inline
bool pc_is90overweight(struct map_session_data *sd)
{
    return sd->weight*10 >= sd->max_weight*9;
}

// Checks all npcs/warps at the same location to see whether they
// should do something with the specified player.
void pc_touch_all_relevant_npcs(struct map_session_data *sd);

int pc_isGM(struct map_session_data *sd);
int pc_iskiller(struct map_session_data *src, struct map_session_data *target);   // [MouseJstr]

void pc_invisibility(struct map_session_data *sd, int enabled);    // [Fate]
int pc_counttargeted(struct map_session_data *sd, struct block_list *src,
        ATK target_lv);
int pc_setrestartvalue(struct map_session_data *sd, int type);
int pc_makesavestatus(struct map_session_data *);
int pc_setnewpc(struct map_session_data *, int, int, int, tick_t, int);
int pc_authok(int, int, time_t, short tmw_version, const struct mmo_charstatus *);
int pc_authfail(int);

EPOS pc_equippoint(struct map_session_data *sd, int n);

int pc_breakweapon(struct map_session_data *sd);  // weapon breaking [Valaris]
int pc_breakarmor(struct map_session_data *sd);   // armor breaking [Valaris]

int pc_checkskill(struct map_session_data *sd, SkillID skill_id);
int pc_checkequip(struct map_session_data *sd, EPOS pos);

int pc_walktoxy(struct map_session_data *, int, int);
int pc_stop_walking(struct map_session_data *, int);
int pc_movepos(struct map_session_data *, int, int);
int pc_setpos(struct map_session_data *, const char *, int, int, BeingRemoveWhy);
int pc_setsavepoint(struct map_session_data *, const char *, int, int);
int pc_randomwarp(struct map_session_data *sd, BeingRemoveWhy type);

ADDITEM pc_checkadditem(struct map_session_data *, int, int);
int pc_inventoryblank(struct map_session_data *);
int pc_search_inventory(struct map_session_data *sd, int item_id);
int pc_payzeny(struct map_session_data *, int);
PickupFail pc_additem(struct map_session_data *, struct item *, int);
int pc_getzeny(struct map_session_data *, int);
int pc_delitem(struct map_session_data *, int, int, int);
int pc_checkitem(struct map_session_data *);
int pc_count_all_items(struct map_session_data *player, int item_id);
int pc_remove_items(struct map_session_data *player,
        int item_id, int count);

int pc_takeitem(struct map_session_data *, struct flooritem_data *);
int pc_dropitem(struct map_session_data *, int, int);

int pc_checkweighticon(struct map_session_data *sd);

int pc_calcstatus(struct map_session_data *, int);
int pc_bonus(struct map_session_data *, SP, int);
int pc_bonus2(struct map_session_data *sd, SP, int, int);
int pc_skill(struct map_session_data *, SkillID, int, int);

int pc_steal_item(struct map_session_data *sd, struct block_list *bl);
int pc_steal_coin(struct map_session_data *sd, struct block_list *bl);

int pc_attack(struct map_session_data *, int, int);
int pc_stopattack(struct map_session_data *);

int pc_gainexp(struct map_session_data *, int, int);

int pc_gainexp_reason(struct map_session_data *, int, int,
        PC_GAINEXP_REASON reason);
int pc_extract_healer_exp(struct map_session_data *, int max);    // [Fate] Used by healers: extract healer-xp from the target, return result (up to max)

int pc_nextbaseexp(struct map_session_data *);
int pc_nextjobexp(struct map_session_data *);
int pc_need_status_point(struct map_session_data *, SP);
int pc_statusup(struct map_session_data *, SP);
int pc_statusup2(struct map_session_data *, SP, int);
int pc_skillup(struct map_session_data *, SkillID);
int pc_resetlvl(struct map_session_data *, int type);
int pc_resetstate(struct map_session_data *);
int pc_resetskill(struct map_session_data *);
int pc_equipitem(struct map_session_data *, int, EPOS);
int pc_unequipitem(struct map_session_data *, int, CalcStatus);
int pc_unequipinvyitem(struct map_session_data *, int, CalcStatus);
int pc_useitem(struct map_session_data *, int);

int pc_damage(struct block_list *, struct map_session_data *, int);
int pc_heal(struct map_session_data *, int, int);
int pc_itemheal(struct map_session_data *sd, int hp, int sp);
int pc_percentheal(struct map_session_data *sd, int, int);
int pc_setoption(struct map_session_data *, Option);
int pc_changelook(struct map_session_data *, LOOK, int);

int pc_readparam(struct map_session_data *, SP);
int pc_setparam(struct map_session_data *, SP, int);
int pc_readreg(struct map_session_data *, int);
int pc_setreg(struct map_session_data *, int, int);
char *pc_readregstr(struct map_session_data *sd, int reg);
int pc_setregstr(struct map_session_data *sd, int reg, const char *str);
int pc_readglobalreg(struct map_session_data *, const char *);
int pc_setglobalreg(struct map_session_data *, const char *, int);
int pc_readaccountreg(struct map_session_data *, const char *);
int pc_setaccountreg(struct map_session_data *, const char *, int);
int pc_readaccountreg2(struct map_session_data *, const char *);
int pc_setaccountreg2(struct map_session_data *, const char *, int);

int pc_addeventtimer(struct map_session_data *sd, interval_t tick,
        const char *name);
int pc_cleareventtimer(struct map_session_data *sd);

int pc_calc_pvprank(struct map_session_data *sd);
void pc_calc_pvprank_timer(TimerData *, tick_t, int);

int pc_marriage(struct map_session_data *sd,
        struct map_session_data *dstsd);
int pc_divorce(struct map_session_data *sd);
struct map_session_data *pc_get_partner(struct map_session_data *sd);
int pc_set_gm_level(int account_id, int level);
void pc_setstand(struct map_session_data *sd);
void pc_cleanup(struct map_session_data *sd);  // [Fate] Clean up after a logged-out PC

int pc_read_gm_account(int fd);
int pc_setinvincibletimer(struct map_session_data *sd, interval_t);
int pc_delinvincibletimer(struct map_session_data *sd);
int pc_logout(struct map_session_data *sd);   // [fate] Player logs out

int do_init_pc(void);

#endif // PC_HPP
