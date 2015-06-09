#pragma once
//    globals.hpp - Evil global variables for tmwa-map.
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "fwd.hpp"

#include <ctime>

#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "../ints/wrap.hpp"

#include "../net/timer.t.hpp"

#include "../mmo/skill.t.hpp"

#include "consts.hpp"
#include "script-buffer.hpp"


namespace tmwa
{
    namespace map
    {
        extern BattleConf battle_config;
        extern MapConf map_conf;
        extern Session *char_session;
        extern int chrif_state;
        extern std::map<MapName, RString> resnametable;
        extern Map<ItemNameId, item_data> item_db;
        extern Map<QuestId, quest_data> quest_db;
        extern DMap<BlockId, dumb_ptr<block_list>> id_db;
        extern UPMap<MapName, map_abstract> maps_db;
        extern DMap<CharName, dumb_ptr<map_session_data>> nick_db;
        extern Map<CharId, charid2nick> charid_db;
        extern int world_user_count;
        extern Array<dumb_ptr<block_list>, unwrap<BlockId>(MAX_FLOORITEM)> object;
        extern BlockId first_free_object_id;
        extern int save_settings;
        extern int block_free_lock;
        extern std::vector<dumb_ptr<block_list>> block_free;
        extern block_list bl_head;
        extern std::unique_ptr<io::AppendFile> map_logfile;
        extern long map_logfile_index;
        extern mob_db_ mob_db[2001];
        extern std::list<AString> npc_srcs;
        extern int npc_warp, npc_shop, npc_script, npc_mob;
        extern BlockId npc_id;
        extern Map<NpcEvent, event_data> ev_db;
        extern DMap<NpcName, dumb_ptr<npc_data>> npcs_by_name;
        extern DMap<RString, NpcName> spells_by_name;
        extern DMap<RString, NpcEvent> spells_by_events;
        extern tm ev_tm_b;
        extern Map<PartyId, PartyMost> party_db;
        extern std::map<AccountId, GmLevel> gm_accountm;
        extern tick_t natural_heal_tick, natural_heal_prev_tick;
        extern interval_t natural_heal_diff_tick;
        extern int last_save_fd;
        extern bool save_flag;
        extern Map<AccountId, Storage> storage_db;
        extern Map<RString, str_data_t> str_datam;
        extern str_data_t LABEL_NEXTLINE_;
        extern Map<ScriptLabel, int> scriptlabel_db;
        extern std::set<ScriptLabel> probable_labels;
        extern UPMap<RString, const ScriptBuffer> userfunc_db;
        extern int parse_cmd_if;
        extern Option<Borrowed<str_data_t>> parse_cmdp;
        extern InternPool variable_names;
        extern ZString startptr;
        extern int startline;
        extern int script_errors;
        extern DMap<SIR, int> mapreg_db;
        extern Map<SIR, RString> mapregstr_db;
        extern int mapreg_dirty;
        extern std::vector<SkillID> skill_pool_skills;
        extern earray<skill_db_, SkillID, SkillID::MAX_SKILL_DB> skill_db;
        extern BlockId skill_area_temp_id;
        extern int skill_area_temp_hp;
    } // namespace map
} // namespace tmwa
