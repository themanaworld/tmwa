#pragma once
//    skill.hpp - Old-style skills.
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

#include "../mmo/skill.t.hpp"

#include "fwd.hpp"

#include "skill-pools.hpp"

#include "../strings/rstring.hpp"
#include "../strings/literal.hpp"

#include "../generic/array.hpp"

#include "map.hpp"


namespace tmwa
{
namespace map
{
constexpr int MAX_SKILL_PRODUCE_DB = 150;
constexpr int MAX_SKILL_ARROW_DB = 150;
constexpr int MAX_SKILL_ABRA_DB = 350;

// スキルデータベース
struct skill_db_
{
    int range_k, hit, inf, pl, nk, max;
    SP stat;
    SkillFlags poolflags;
    int max_raise; // `max' is the global max, `max_raise' is the maximum attainable via skill-ups
    int num_k;
    Array<int, MAX_SKILL_LEVEL> cast, delay;
    Array<int, MAX_SKILL_LEVEL> upkeep_time, upkeep_time2;
    bool castcancel;
    int cast_def_rate;
    int inf2, maxcount;
    Array<int, MAX_SKILL_LEVEL> hp, sp, mhp,
        hprate, sp_rate,
        zeny;
    int weapon;
    Array<int, MAX_SKILL_LEVEL> castnodex;
};

struct skill_name_db
{
    SkillID id;                    // skill id
    LString name;                 // search strings
    RString desc;                 // description that shows up for searches

    // this makes const char(&)[] not decay into const char * in {}
    skill_name_db(SkillID i, LString n, LString d)
    : id(i), name(n), desc(d)
    {}
};

skill_name_db& skill_lookup_by_id(SkillID id);
skill_name_db& skill_lookup_by_name(XString name);

bool skill_readdb(ZString filename);

// スキルデータベースへのアクセサ
int skill_get_hit(SkillID id);
int skill_get_inf(SkillID id);
int skill_get_nk(SkillID id);
int skill_get_max(SkillID id);
int skill_get_max_raise(SkillID id);
int skill_get_range(SkillID id, int lv);
int skill_get_sp(SkillID id, int lv);
int skill_get_num(SkillID id, int lv);
int skill_get_cast(SkillID id, int lv);
int skill_get_delay(SkillID id, int lv);
int skill_get_inf2(SkillID id);
int skill_get_maxcount(SkillID id);

// 追加効果
int skill_additional_effect(dumb_ptr<block_list> src, dumb_ptr<block_list> bl,
        SkillID skillid, int skilllv);

interval_t skill_castfix(dumb_ptr<block_list> bl, interval_t time);
interval_t skill_delayfix(dumb_ptr<block_list> bl, interval_t time);

void skill_stop_dancing(dumb_ptr<block_list> src, int flag);

// 詠唱キャンセル
int skill_castcancel(dumb_ptr<block_list> bl, int type);

// ステータス異常
int skill_status_effect(dumb_ptr<block_list> bl, StatusChange type,
        int val1,
        interval_t tick);
int skill_status_change_start(dumb_ptr<block_list> bl, StatusChange type,
        int val1,
        interval_t tick);
int skill_status_change_active(dumb_ptr<block_list> bl, StatusChange type);  // [fate]
void skill_status_change_end(dumb_ptr<block_list> bl, StatusChange type, TimerData *tid);
int skill_status_change_clear(dumb_ptr<block_list> bl, int type);

// mobスキルのため
int skill_castend_nodamage_id(dumb_ptr<block_list> src, dumb_ptr<block_list> bl,
        SkillID skillid, int skilllv);
int skill_castend_damage_id(dumb_ptr<block_list> src, dumb_ptr<block_list> bl,
        SkillID skillid, int skilllv, tick_t tick,
        BCT flag);

int skill_update_heal_animation(dumb_ptr<map_session_data> sd); // [Fate]  Check whether the healing flag must be updated, do so if needed

void skill_reload(void);

// [Fate] Skill pools API

// Max. # of active entries in the skill pool
constexpr int MAX_SKILL_POOL = 3;

// Yields all active skills in the skill pool; no more than MAX_SKILL_POOL.  Return is number of skills.
int skill_pool(dumb_ptr<map_session_data> sd, SkillID *skills);
int skill_pool_size(dumb_ptr<map_session_data> sd);
int skill_pool_max(dumb_ptr<map_session_data> sd);  // Max. number of pool skills
// Skill into skill pool.  Return is zero iff okay.
int skill_pool_activate(dumb_ptr<map_session_data> sd, SkillID skill);
// Skill into skill pool.  Return is zero when activated.
bool skill_pool_is_activated(dumb_ptr<map_session_data> sd, SkillID skill);
// Skill out of skill pool.  Return is zero iff okay.
int skill_pool_deactivate(dumb_ptr<map_session_data> sd, SkillID skill);
// Yield configurable skill name
inline
const RString& skill_name(SkillID skill)
{
    return skill_lookup_by_id(skill).desc;
}
// Yields the power of a skill.
// This is zero if the skill is unknown
//      or if it's a pool skill that is outside of the skill pool,
// otherwise a value from 0 to 255 (with 200 being the `normal maximum')
int skill_power(dumb_ptr<map_session_data> sd, SkillID skill);
int skill_power_bl(dumb_ptr<block_list> bl, SkillID skill);

// [Fate] Remember that a certain skill ID belongs to a pool skill
void skill_pool_register(SkillID id);
} // namespace map
} // namespace tmwa
