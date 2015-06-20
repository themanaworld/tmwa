#pragma once
//    clif.hpp - Network interface to the client.
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

#include "../mmo/clif.t.hpp"

#include "fwd.hpp"

#include <functional>

#include "../high/mmo.hpp"

#include "../net/timer.t.hpp"

#include "battle.t.hpp"
#include "map.t.hpp"
#include "pc.t.hpp"
#include "../mmo/skill.t.hpp"


namespace tmwa
{
namespace map
{
int clif_countusers(void);
void clif_setwaitclose(Session *);

int clif_authok(dumb_ptr<map_session_data>);
int clif_authfail_fd(Session *, int);
int clif_charselectok(BlockId);
int clif_dropflooritem(dumb_ptr<flooritem_data>);
int clif_clearflooritem(dumb_ptr<flooritem_data>, Session *);
int clif_clearchar(dumb_ptr<block_list>, BeingRemoveWhy); // area or fd
void clif_clearchar_id(BlockId, BeingRemoveWhy, Session *);
int clif_spawnpc(dumb_ptr<map_session_data>);  //area
int clif_spawnnpc(dumb_ptr<npc_data>); // area
int clif_spawn_fake_npc_for_player(dumb_ptr<map_session_data> sd,
        BlockId fake_npc_id);
int clif_spawnmob(dumb_ptr<mob_data>); // area
int clif_walkok(dumb_ptr<map_session_data>);   // self
int clif_movechar(dumb_ptr<map_session_data>); // area
int clif_movemob(dumb_ptr<mob_data>);  //area
void clif_changemap(dumb_ptr<map_session_data>, MapName, int, int);  //self
void clif_changemapserver(dumb_ptr<map_session_data>, MapName, int, int, IP4Address, int);  //self
void clif_fixpos(dumb_ptr<block_list>); // area
int clif_fixmobpos(dumb_ptr<mob_data> md);
int clif_fixpcpos(dumb_ptr<map_session_data> sd);
int clif_npcbuysell(dumb_ptr<map_session_data>, BlockId);  //self
int clif_buylist(dumb_ptr<map_session_data>, dumb_ptr<npc_data_shop>);   //self
int clif_selllist(dumb_ptr<map_session_data>); //self
void clif_scriptmes(dumb_ptr<map_session_data>, BlockId, XString);   //self
void clif_scriptnext(dumb_ptr<map_session_data>, BlockId);  //self
void clif_scriptclose(dumb_ptr<map_session_data>, BlockId); //self
void clif_scriptmenu(dumb_ptr<map_session_data>, BlockId, XString);  //self
void clif_scriptinput(dumb_ptr<map_session_data>, BlockId); //self
void clif_scriptinputstr(dumb_ptr<map_session_data> sd, BlockId npcid);  // self
void clif_map_pvp(dumb_ptr<map_session_data>); // self

int clif_additem(dumb_ptr<map_session_data>, IOff0, int, PickupFail);   //self
void clif_delitem(dumb_ptr<map_session_data>, IOff0, int);    //self
int clif_updatestatus(dumb_ptr<map_session_data>, SP);    //self
int clif_damage(dumb_ptr<block_list>, dumb_ptr<block_list>,
        tick_t, interval_t, interval_t,
        int, int, DamageType);    // area
inline
int clif_takeitem(dumb_ptr<block_list> src, dumb_ptr<block_list> dst)
{
    return clif_damage(src, dst, tick_t(), interval_t::zero(), interval_t::zero(), 0, 0, DamageType::TAKEITEM);
}
int clif_changelook(dumb_ptr<block_list>, LOOK, int);   // area
void clif_changelook_accessories(dumb_ptr<block_list> bl, dumb_ptr<map_session_data> dst); // area or target; list gloves, boots etc.
int clif_arrowequip(dumb_ptr<map_session_data> sd, IOff0 val);    //self
int clif_arrow_fail(dumb_ptr<map_session_data> sd, int type);   //self
int clif_statusupack(dumb_ptr<map_session_data>, SP, int, int);   // self
int clif_equipitemack(dumb_ptr<map_session_data>, IOff0, EPOS, int);  // self
int clif_unequipitemack(dumb_ptr<map_session_data>, IOff0, EPOS, int);    // self
int clif_misceffect(dumb_ptr<block_list>, int);    // area
void clif_pvpstatus(dumb_ptr<map_session_data>); // area
int clif_changeoption(dumb_ptr<block_list>);   // area
int clif_useitemack(dumb_ptr<map_session_data>, IOff0, int, int);    // self

void clif_emotion(dumb_ptr<block_list> bl, int type);
void clif_emotion_towards(dumb_ptr<block_list> bl, dumb_ptr<block_list> target, int type);
void clif_sitting(Session *, dumb_ptr<map_session_data> sd);
void clif_sitnpc(dumb_ptr<npc_data> nd, DamageType dmg);
void clif_sitnpc_towards(dumb_ptr<map_session_data> sd, dumb_ptr<npc_data> nd, DamageType dmg);
void clif_setnpcdirection(dumb_ptr<npc_data> nd, DIR direction);
void clif_setnpcdirection_towards(dumb_ptr<map_session_data> sd, dumb_ptr<npc_data> nd, DIR direction);
void clif_npc_send_title(Session *s, BlockId npcid, XString msg);

// trade
void clif_traderequest(dumb_ptr<map_session_data> sd, CharName name);
void clif_tradestart(dumb_ptr<map_session_data> sd, int type);
void clif_tradeadditem(dumb_ptr<map_session_data> sd,
        dumb_ptr<map_session_data> tsd, IOff2 index2, int amount);
int clif_tradeitemok(dumb_ptr<map_session_data> sd, IOff2 index, int amount,
        int fail);
int clif_tradedeal_lock(dumb_ptr<map_session_data> sd, int fail);
int clif_tradecancelled(dumb_ptr<map_session_data> sd);
int clif_tradecompleted(dumb_ptr<map_session_data> sd, int fail);

// storage
int clif_storageitemlist(dumb_ptr<map_session_data> sd, Borrowed<Storage> stor);
int clif_storageequiplist(dumb_ptr<map_session_data> sd,
        Borrowed<Storage> stor);
int clif_updatestorageamount(dumb_ptr<map_session_data> sd,
        Borrowed<Storage> stor);
int clif_storageitemadded(dumb_ptr<map_session_data> sd, Borrowed<Storage> stor,
        SOff0 index, int amount);
int clif_storageitemremoved(dumb_ptr<map_session_data> sd, SOff0 index,
        int amount);
int clif_storageclose(dumb_ptr<map_session_data> sd);

// map_forallinmovearea callbacks
void clif_pcinsight(dumb_ptr<block_list>, dumb_ptr<map_session_data>);
void clif_pcoutsight(dumb_ptr<block_list>, dumb_ptr<map_session_data>);
void clif_mobinsight(dumb_ptr<block_list>, dumb_ptr<mob_data>);
void clif_moboutsight(dumb_ptr<block_list>, dumb_ptr<mob_data>);

void clif_skillinfoblock(dumb_ptr<map_session_data> sd);
int clif_skillup(dumb_ptr<map_session_data> sd, SkillID skill_num);

int clif_skillcastcancel(dumb_ptr<block_list> bl);
int clif_skill_fail(dumb_ptr<map_session_data> sd, SkillID skill_id, int type,
        int btype);
int clif_skill_damage(dumb_ptr<block_list> src, dumb_ptr<block_list> dst,
        tick_t tick, interval_t sdelay, interval_t ddelay, int damage,
        int div, SkillID skill_id, int skill_lv, int type);

int clif_status_change(dumb_ptr<block_list> bl,
        StatusChange type, int flag);

void clif_wis_message(Session *s, CharName nick, XString mes);
void clif_wis_end(Session *s, int flag);

void clif_itemlist(dumb_ptr<map_session_data> sd);
void clif_equiplist(dumb_ptr<map_session_data> sd);

int clif_movetoattack(dumb_ptr<map_session_data> sd, dumb_ptr<block_list> bl);

// party
int clif_party_created(dumb_ptr<map_session_data> sd, int flag);
int clif_party_info(PartyPair p, Session *s);
void clif_party_invite(dumb_ptr<map_session_data> sd,
        dumb_ptr<map_session_data> tsd);
void clif_party_inviteack(dumb_ptr<map_session_data> sd, CharName nick, int flag);
void clif_party_option(PartyPair p, dumb_ptr<map_session_data> sd,
        int flag);
void clif_party_leaved(PartyPair p, dumb_ptr<map_session_data> sd,
        AccountId account_id, CharName name, int flag);
void clif_party_message(PartyPair p, AccountId account_id, XString mes);
int clif_party_xy(PartyPair p, dumb_ptr<map_session_data> sd);
int clif_party_hp(PartyPair p, dumb_ptr<map_session_data> sd);

// atcommand
void clif_displaymessage(Session *s, XString mes);
void clif_GMmessage(dumb_ptr<block_list> bl, XString mes, int flag);
void clif_resurrection(dumb_ptr<block_list> bl, int type);

int clif_specialeffect(dumb_ptr<block_list> bl, int type, int flag);    // special effects [Valaris]
void clif_message(dumb_ptr<block_list> bl, XString msg);   // messages (from mobs/npcs) [Valaris]

int clif_GM_kick(dumb_ptr<map_session_data> sd, dumb_ptr<map_session_data> tsd,
        int type);

int clif_foreachclient(std::function<void(dumb_ptr<map_session_data>)>);
// quest
void clif_sendallquest(dumb_ptr<map_session_data> sd);
void clif_sendquest(dumb_ptr<map_session_data> sd, QuestId questid, int value);

void do_init_clif(void);
} // namespace map
} // namespace tmwa
