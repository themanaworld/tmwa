#ifndef PC_HPP
#define PC_HPP

#include "pc.t.hpp"

#include "../strings/fwd.hpp"

#include "clif.t.hpp"
#include "map.hpp"

inline
void pc_setsit(dumb_ptr<map_session_data> sd)
{
    sd->state.dead_sit = 2;
}
//pc_setstand(dumb_ptr<map_session_data> sd) is a normal function
inline
bool pc_isdead(dumb_ptr<map_session_data> sd)
{
    return sd->state.dead_sit == 1;
}
inline
bool pc_issit(dumb_ptr<map_session_data> sd)
{
    return sd->state.dead_sit == 2;
}
inline
void pc_setdir(dumb_ptr<map_session_data> sd, DIR b)
{
    sd->dir = (b);
}
inline
bool pc_isinvisible(dumb_ptr<map_session_data> sd)
{
    return bool(sd->status.option & Option::HIDE);
}
inline
bool pc_is90overweight(dumb_ptr<map_session_data> sd)
{
    return sd->weight*10 >= sd->max_weight*9;
}

// Checks all npcs/warps at the same location to see whether they
// should do something with the specified player.
void pc_touch_all_relevant_npcs(dumb_ptr<map_session_data> sd);

uint8_t pc_isGM(dumb_ptr<map_session_data> sd);
int pc_iskiller(dumb_ptr<map_session_data> src, dumb_ptr<map_session_data> target);   // [MouseJstr]

void pc_invisibility(dumb_ptr<map_session_data> sd, int enabled);    // [Fate]
int pc_counttargeted(dumb_ptr<map_session_data> sd, dumb_ptr<block_list> src,
        ATK target_lv);
int pc_setrestartvalue(dumb_ptr<map_session_data> sd, int type);
void pc_makesavestatus(dumb_ptr<map_session_data>);
int pc_setnewpc(dumb_ptr<map_session_data>, int, int, int, tick_t, SEX);
int pc_authok(int, int, TimeT, short tmw_version, const struct mmo_charstatus *);
int pc_authfail(int);

EPOS pc_equippoint(dumb_ptr<map_session_data> sd, int n);

int pc_checkskill(dumb_ptr<map_session_data> sd, SkillID skill_id);
int pc_checkequip(dumb_ptr<map_session_data> sd, EPOS pos);

int pc_walktoxy(dumb_ptr<map_session_data>, int, int);
int pc_stop_walking(dumb_ptr<map_session_data>, int);
int pc_movepos(dumb_ptr<map_session_data>, int, int);
int pc_setpos(dumb_ptr<map_session_data>, MapName, int, int, BeingRemoveWhy);
void pc_setsavepoint(dumb_ptr<map_session_data>, MapName, int, int);
int pc_randomwarp(dumb_ptr<map_session_data> sd, BeingRemoveWhy type);

ADDITEM pc_checkadditem(dumb_ptr<map_session_data>, int, int);
int pc_inventoryblank(dumb_ptr<map_session_data>);
int pc_search_inventory(dumb_ptr<map_session_data> sd, int item_id);
int pc_payzeny(dumb_ptr<map_session_data>, int);
PickupFail pc_additem(dumb_ptr<map_session_data>, struct item *, int);
int pc_getzeny(dumb_ptr<map_session_data>, int);
int pc_delitem(dumb_ptr<map_session_data>, int, int, int);
int pc_checkitem(dumb_ptr<map_session_data>);
int pc_count_all_items(dumb_ptr<map_session_data> player, int item_id);
int pc_remove_items(dumb_ptr<map_session_data> player,
        int item_id, int count);

int pc_takeitem(dumb_ptr<map_session_data>, dumb_ptr<flooritem_data>);
int pc_dropitem(dumb_ptr<map_session_data>, int, int);

int pc_checkweighticon(dumb_ptr<map_session_data> sd);

int pc_calcstatus(dumb_ptr<map_session_data>, int);
int pc_bonus(dumb_ptr<map_session_data>, SP, int);
int pc_bonus2(dumb_ptr<map_session_data> sd, SP, int, int);
int pc_skill(dumb_ptr<map_session_data>, SkillID, int, int);

int pc_attack(dumb_ptr<map_session_data>, int, int);
int pc_stopattack(dumb_ptr<map_session_data>);

int pc_gainexp_reason(dumb_ptr<map_session_data>, int, int,
        PC_GAINEXP_REASON reason);
int pc_extract_healer_exp(dumb_ptr<map_session_data>, int max);    // [Fate] Used by healers: extract healer-xp from the target, return result (up to max)

int pc_nextbaseexp(dumb_ptr<map_session_data>);
int pc_nextjobexp(dumb_ptr<map_session_data>);
int pc_need_status_point(dumb_ptr<map_session_data>, SP);
int pc_statusup(dumb_ptr<map_session_data>, SP);
int pc_statusup2(dumb_ptr<map_session_data>, SP, int);
int pc_skillup(dumb_ptr<map_session_data>, SkillID);
int pc_resetlvl(dumb_ptr<map_session_data>, int type);
int pc_resetstate(dumb_ptr<map_session_data>);
int pc_resetskill(dumb_ptr<map_session_data>);
int pc_equipitem(dumb_ptr<map_session_data>, int, EPOS);
int pc_unequipitem(dumb_ptr<map_session_data>, int, CalcStatus);
int pc_unequipinvyitem(dumb_ptr<map_session_data>, int, CalcStatus);
int pc_useitem(dumb_ptr<map_session_data>, int);

int pc_damage(dumb_ptr<block_list>, dumb_ptr<map_session_data>, int);
int pc_heal(dumb_ptr<map_session_data>, int, int);
int pc_itemheal(dumb_ptr<map_session_data> sd, int hp, int sp);
int pc_percentheal(dumb_ptr<map_session_data> sd, int, int);
int pc_setoption(dumb_ptr<map_session_data>, Option);
int pc_changelook(dumb_ptr<map_session_data>, LOOK, int);

int pc_readparam(dumb_ptr<map_session_data>, SP);
int pc_setparam(dumb_ptr<map_session_data>, SP, int);
int pc_readreg(dumb_ptr<map_session_data>, SIR);
void pc_setreg(dumb_ptr<map_session_data>, SIR, int);
ZString pc_readregstr(dumb_ptr<map_session_data> sd, SIR reg);
void pc_setregstr(dumb_ptr<map_session_data> sd, SIR reg, FString str);
int pc_readglobalreg(dumb_ptr<map_session_data>, VarName );
int pc_setglobalreg(dumb_ptr<map_session_data>, VarName , int);
int pc_readaccountreg(dumb_ptr<map_session_data>, VarName );
int pc_setaccountreg(dumb_ptr<map_session_data>, VarName , int);
int pc_readaccountreg2(dumb_ptr<map_session_data>, VarName );
int pc_setaccountreg2(dumb_ptr<map_session_data>, VarName , int);

int pc_addeventtimer(dumb_ptr<map_session_data> sd, interval_t tick,
        NpcEvent name);
int pc_cleareventtimer(dumb_ptr<map_session_data> sd);

int pc_calc_pvprank(dumb_ptr<map_session_data> sd);
void pc_calc_pvprank_timer(TimerData *, tick_t, int);

int pc_marriage(dumb_ptr<map_session_data> sd,
        dumb_ptr<map_session_data> dstsd);
int pc_divorce(dumb_ptr<map_session_data> sd);
dumb_ptr<map_session_data> pc_get_partner(dumb_ptr<map_session_data> sd);
void pc_set_gm_level(int account_id, uint8_t level);
void pc_setstand(dumb_ptr<map_session_data> sd);
void pc_cleanup(dumb_ptr<map_session_data> sd);  // [Fate] Clean up after a logged-out PC

int pc_read_gm_account(int fd);
int pc_setinvincibletimer(dumb_ptr<map_session_data> sd, interval_t);
int pc_delinvincibletimer(dumb_ptr<map_session_data> sd);
int pc_logout(dumb_ptr<map_session_data> sd);   // [fate] Player logs out

int do_init_pc(void);

#endif // PC_HPP
