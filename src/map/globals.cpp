#include "globals.hpp"
//    globals.cpp - Evil global variables for tmwa-map.
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

#include "../generic/intern-pool.hpp"

#include "../io/write.hpp"

#include "../proto2/net-Storage.hpp"

#include "battle_conf.hpp"
#include "itemdb.hpp"
#include "quest.hpp"
#include "map_conf.hpp"
#include "mob.hpp"
#include "npc-internal.hpp"
#include "script-parse-internal.hpp"
#include "skill.hpp"

#include "../poison.hpp"


namespace tmwa
{
    namespace map
    {
        BattleConf battle_config;
        MapConf map_conf;

        // only used by intif.cpp
        // and clif.cpp for the new on_delete stuff ...
        Session *char_session;
        int chrif_state;
        std::map<MapName, RString> resnametable;
        Map<ItemNameId, item_data> item_db;
        Map<QuestId, quest_data> quest_db;

        DMap<BlockId, dumb_ptr<block_list>> id_db;
        UPMap<MapName, map_abstract> maps_db;
        DMap<CharName, dumb_ptr<map_session_data>> nick_db;
        Map<CharId, charid2nick> charid_db;
        int world_user_count = 0;
        Array<dumb_ptr<block_list>, unwrap<BlockId>(MAX_FLOORITEM)> object;
        BlockId first_free_object_id = BlockId();
        int save_settings = 0xFFFF;
        int block_free_lock = 0;
        std::vector<dumb_ptr<block_list>> block_free;
        /// This is a dummy entry that is shared by all the linked lists,
        /// so that any entry can unlink itself without worrying about
        /// whether it was the the head of the list.
        block_list bl_head;
        std::unique_ptr<io::AppendFile> map_logfile;
        long map_logfile_index;
        mob_db_ mob_db[2001];
        std::list<AString> npc_srcs;
        int npc_warp, npc_shop, npc_script, npc_mob;
        BlockId npc_id = START_NPC_NUM;
        Map<NpcEvent, struct event_data> ev_db;
        DMap<NpcName, dumb_ptr<npc_data>> npcs_by_name;
        DMap<RString, NpcEvent> spells_by_events;
        // used for clock-based event triggers
        // only tm_min, tm_hour, and tm_mday are used
        tm ev_tm_b =
        {
            .tm_sec= 0,
            .tm_min= -1,
            .tm_hour= -1,
            .tm_mday= -1,
            .tm_mon= 0,
            .tm_year= 0,
            .tm_wday= 0,
            .tm_yday= 0,
            .tm_isdst= 0,
        };
        Map<PartyId, PartyMost> party_db;
        std::map<AccountId, GmLevel> gm_accountm;
        tick_t natural_heal_tick, natural_heal_prev_tick;
        interval_t natural_heal_diff_tick;
        int last_save_fd;
        bool save_flag;
        Map<AccountId, Storage> storage_db;

        Map<RString, str_data_t> str_datam;
        str_data_t LABEL_NEXTLINE_;
        Map<ScriptLabel, int> scriptlabel_db;
        std::set<ScriptLabel> probable_labels;
        UPMap<RString, const ScriptBuffer> userfunc_db;
        int parse_cmd_if = 0;
        Option<Borrowed<str_data_t>> parse_cmdp = None;
        InternPool variable_names;
        // TODO: replace this whole mess with some sort of input stream that works
        // a line at a time.
        ZString startptr;
        int startline;
        int script_errors = 0;
        DMap<SIR, int> mapreg_db;
        Map<SIR, RString> mapregstr_db;
        int mapreg_dirty = -1;

        std::vector<SkillID> skill_pool_skills;
        earray<skill_db_, SkillID, SkillID::MAX_SKILL_DB> skill_db;
        // these variables are set in the 'else' branches,
        // and used in the (recursive) 'if' branch
        // TODO kill it, kill it with fire.
        BlockId skill_area_temp_id;
        int skill_area_temp_hp;

        // Some other globals are not moved here, because they are
        // large and initialized in-place and then *mostly* unmodified.
        //
        //  src/map/atcommand.cpp:
        //      Map<XString, AtCommandInfo> atcommand_info;
        //  src/map/script-fun.cpp:
        //      BuiltinFunction builtin_functions[];
        //  src/map/clif.cpp:
        //      func_table clif_parse_func_table[0x0220];
    } // namespace map
} // namespace tmwa
