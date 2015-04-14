#pragma once
//    pc.hpp - Player state changes.
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

#include "pc.t.hpp"

#include "fwd.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../mmo/clif.t.hpp"
#include "map.hpp"


namespace tmwa
{
namespace map
{
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
    return bool(sd->status.option & Opt0::HIDE);
}
inline
bool pc_is90overweight(dumb_ptr<map_session_data> sd)
{
    return sd->weight*10 >= sd->max_weight*9;
}

// Checks all npcs/warps at the same location to see whether they
// should do something with the specified player.
void pc_touch_all_relevant_npcs(dumb_ptr<map_session_data> sd);

GmLevel pc_isGM(dumb_ptr<map_session_data> sd);
int pc_iskiller(dumb_ptr<map_session_data> src, dumb_ptr<map_session_data> target);   // [MouseJstr]

void pc_invisibility(dumb_ptr<map_session_data> sd, int enabled);    // [Fate]
int pc_counttargeted(dumb_ptr<map_session_data> sd, dumb_ptr<block_list> src,
        ATK target_lv);
int pc_setrestartvalue(dumb_ptr<map_session_data> sd, int type);
void pc_makesavestatus(dumb_ptr<map_session_data>);
int pc_setnewpc(dumb_ptr<map_session_data>, AccountId, CharId, int, uint32_t /*tick_t*/, SEX);
int pc_authok(AccountId, int, short tmw_version, const CharKey *, const CharData *);
int pc_authfail(AccountId accid);

EPOS pc_equippoint(dumb_ptr<map_session_data> sd, IOff0 n);

int pc_checkskill(dumb_ptr<map_session_data> sd, SkillID skill_id);
IOff0 pc_checkequip(dumb_ptr<map_session_data> sd, EPOS pos);

int pc_walktoxy(dumb_ptr<map_session_data>, int, int);
int pc_stop_walking(dumb_ptr<map_session_data>, int);
int pc_setpos(dumb_ptr<map_session_data>, MapName, int, int, BeingRemoveWhy);
void pc_setsavepoint(dumb_ptr<map_session_data>, MapName, int, int);
int pc_randomwarp(dumb_ptr<map_session_data> sd, BeingRemoveWhy type);

ADDITEM pc_checkadditem(dumb_ptr<map_session_data>, ItemNameId, int);
int pc_inventoryblank(dumb_ptr<map_session_data>);
IOff0 pc_search_inventory(dumb_ptr<map_session_data> sd, ItemNameId item_id);
int pc_payzeny(dumb_ptr<map_session_data>, int);
PickupFail pc_additem(dumb_ptr<map_session_data>, Item *, int);
int pc_getzeny(dumb_ptr<map_session_data>, int);
int pc_delitem(dumb_ptr<map_session_data>, IOff0, int, int);
int pc_checkitem(dumb_ptr<map_session_data>);
int pc_count_all_items(dumb_ptr<map_session_data> player, ItemNameId item_id);
int pc_remove_items(dumb_ptr<map_session_data> player,
        ItemNameId item_id, int count);

int pc_takeitem(dumb_ptr<map_session_data>, dumb_ptr<flooritem_data>);
int pc_dropitem(dumb_ptr<map_session_data>, IOff0, int);

int pc_checkweighticon(dumb_ptr<map_session_data> sd);

int pc_calcstatus(dumb_ptr<map_session_data>, int);
int pc_bonus(dumb_ptr<map_session_data>, SP, int);
int pc_bonus2(dumb_ptr<map_session_data> sd, SP, int, int);
int pc_skill(dumb_ptr<map_session_data>, SkillID, int, int);

int pc_attack(dumb_ptr<map_session_data>, BlockId, int);
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
int pc_resetstate(dumb_ptr<map_session_data>);
int pc_resetskill(dumb_ptr<map_session_data>);
int pc_equipitem(dumb_ptr<map_session_data>, IOff0, EPOS);
int pc_unequipitem(dumb_ptr<map_session_data>, IOff0, CalcStatus);
int pc_unequipinvyitem(dumb_ptr<map_session_data>, IOff0, CalcStatus);
int pc_useitem(dumb_ptr<map_session_data>, IOff0);

int pc_damage(dumb_ptr<block_list>, dumb_ptr<map_session_data>, int);
int pc_heal(dumb_ptr<map_session_data>, int, int);
int pc_itemheal(dumb_ptr<map_session_data> sd, int hp, int sp);
int pc_percentheal(dumb_ptr<map_session_data> sd, int, int);
int pc_changelook(dumb_ptr<map_session_data>, LOOK, int);

int pc_readparam(dumb_ptr<map_session_data>, SP);
int pc_setparam(dumb_ptr<map_session_data>, SP, int);
int pc_readreg(dumb_ptr<map_session_data>, SIR);
void pc_setreg(dumb_ptr<map_session_data>, SIR, int);
ZString pc_readregstr(dumb_ptr<map_session_data> sd, SIR reg);
void pc_setregstr(dumb_ptr<map_session_data> sd, SIR reg, RString str);
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
void pc_calc_pvprank_timer(TimerData *, tick_t, BlockId);

int pc_marriage(dumb_ptr<map_session_data> sd,
        dumb_ptr<map_session_data> dstsd);
int pc_divorce(dumb_ptr<map_session_data> sd);
dumb_ptr<map_session_data> pc_get_partner(dumb_ptr<map_session_data> sd);
void pc_set_gm_level(AccountId account_id, GmLevel level);
void pc_setstand(dumb_ptr<map_session_data> sd);
void pc_cleanup(dumb_ptr<map_session_data> sd);  // [Fate] Clean up after a logged-out PC

int pc_read_gm_account(Session *, const std::vector<Packet_Repeat<0x2b15>>&);
int pc_setpvptimer(dumb_ptr<map_session_data> sd, interval_t);
int pc_delpvptimer(dumb_ptr<map_session_data> sd);
int pc_setinvincibletimer(dumb_ptr<map_session_data> sd, interval_t);
int pc_delinvincibletimer(dumb_ptr<map_session_data> sd);
int pc_logout(dumb_ptr<map_session_data> sd);   // [fate] Player logs out

void pc_show_motd(dumb_ptr<map_session_data> sd);

void do_init_pc(void);
} // namespace map
} // namespace tmwa
