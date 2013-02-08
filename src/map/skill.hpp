#ifndef SKILL_HPP
#define SKILL_HPP

#include "skill.t.hpp"

#include "map.hpp"

constexpr int MAX_SKILL_PRODUCE_DB = 150;
constexpr int MAX_SKILL_ARROW_DB = 150;
constexpr int MAX_SKILL_ABRA_DB = 350;

// スキルデータベース
struct skill_db
{
    int range[MAX_SKILL_LEVEL], hit, inf, pl, nk, max;
    SP stat;
    SkillFlags poolflags;
    int max_raise; // `max' is the global max, `max_raise' is the maximum attainable via skill-ups
    int num[MAX_SKILL_LEVEL];
    int cast[MAX_SKILL_LEVEL], delay[MAX_SKILL_LEVEL];
    int upkeep_time[MAX_SKILL_LEVEL], upkeep_time2[MAX_SKILL_LEVEL];
    int castcancel, cast_def_rate;
    int inf2, maxcount;
    int hp[MAX_SKILL_LEVEL], sp[MAX_SKILL_LEVEL], mhp[MAX_SKILL_LEVEL],
        hp_rate[MAX_SKILL_LEVEL], sp_rate[MAX_SKILL_LEVEL],
        zeny[MAX_SKILL_LEVEL];
    int weapon;
    int spiritball[MAX_SKILL_LEVEL];
    int itemid[10], amount[10];
    int castnodex[MAX_SKILL_LEVEL];
};
extern earray<struct skill_db, SkillID, SkillID::MAX_SKILL_DB> skill_db;

struct skill_name_db
{
    SkillID id;                    // skill id
    const char *name;                 // search strings
    const char *desc;                 // description that shows up for search's
};

// used only by @skillid for iteration - should be depublicized
extern struct skill_name_db skill_names[];

skill_name_db& skill_lookup_by_id(SkillID id);
skill_name_db& skill_lookup_by_name(const char *name);

struct block_list;
struct map_session_data;

int do_init_skill(void);

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
int skill_additional_effect(struct block_list *src, struct block_list *bl,
        SkillID skillid, int skilllv, BF attack_type,
        unsigned int tick);

int skill_castfix(struct block_list *bl, int time);
int skill_delayfix(struct block_list *bl, int time);

void skill_stop_dancing(struct block_list *src, int flag);

// 詠唱キャンセル
int skill_castcancel(struct block_list *bl, int type);

int skill_gangsterparadise(struct map_session_data *sd, int type);
void skill_devotion(struct map_session_data *md, int target);
int skill_devotion3(struct block_list *bl, int target);

// ステータス異常
int skill_status_effect(struct block_list *bl, StatusChange type,
        int val1, int val2, int val3, int val4,
        int tick, int flag, int spell_invocation);
int skill_status_change_start(struct block_list *bl, StatusChange type,
        int val1, int val2, int val3, int val4, int tick, int flag);
int skill_status_change_active(struct block_list *bl, StatusChange type);  // [fate]
int skill_status_change_end(struct block_list *bl, StatusChange type, int tid);
int skill_status_change_clear(struct block_list *bl, int type);

// mobスキルのため
int skill_castend_nodamage_id(struct block_list *src, struct block_list *bl,
        SkillID skillid, int skilllv, unsigned int tick,
        BCT flag);
int skill_castend_damage_id(struct block_list *src, struct block_list *bl,
        SkillID skillid, int skilllv, unsigned int tick,
        BCT flag);

int skill_update_heal_animation(struct map_session_data *sd); // [Fate]  Check whether the healing flag must be updated, do so if needed

void skill_reload(void);

// [Fate] Skill pools API

// Max. # of active entries in the skill pool
constexpr int MAX_SKILL_POOL = 3;
// Max. # of skills that may be classified as pool skills in db/skill_db.txt
constexpr int MAX_POOL_SKILLS = 128;

extern SkillID skill_pool_skills[MAX_POOL_SKILLS];  // All pool skills
extern int skill_pool_skills_size;  // Number of entries in skill_pool_skills

// Yields all active skills in the skill pool; no more than MAX_SKILL_POOL.  Return is number of skills.
int skill_pool(struct map_session_data *sd, SkillID *skills);
int skill_pool_size(struct map_session_data *sd);
int skill_pool_max(struct map_session_data *sd);  // Max. number of pool skills
// Skill into skill pool.  Return is zero iff okay.
int skill_pool_activate(struct map_session_data *sd, SkillID skill);
// Skill into skill pool.  Return is zero when activated.
bool skill_pool_is_activated(struct map_session_data *sd, SkillID skill);
// Skill out of skill pool.  Return is zero iff okay.
int skill_pool_deactivate(struct map_session_data *sd, SkillID skill);
// Yield configurable skill name
inline
const char *skill_name(SkillID skill)
{
    return skill_lookup_by_id(skill).desc;
}
// Yields the power of a skill.
// This is zero if the skill is unknown
//      or if it's a pool skill that is outside of the skill pool,
// otherwise a value from 0 to 255 (with 200 being the `normal maximum')
int skill_power(struct map_session_data *sd, SkillID skill);
int skill_power_bl(struct block_list *bl, SkillID skill);

// [Fate] Remember that a certain skill ID belongs to a pool skill
void skill_pool_register(SkillID id);
#endif // SKILL_HPP
