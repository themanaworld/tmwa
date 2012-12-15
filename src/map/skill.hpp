// $Id: skill.h,v 1.5 2004/09/25 05:32:19 MouseJstr Exp $
#ifndef SKILL_HPP
#define SKILL_HPP

#include "skill.t.hpp"

#include "../common/timer.hpp"

#include "map.hpp"
#include "magic.hpp"

#define MAX_SKILL_PRODUCE_DB     150
#define MAX_SKILL_ARROW_DB       150
#define MAX_SKILL_ABRA_DB        350

#define SKILL_POOL_FLAG         0x1 // is a pool skill
#define SKILL_POOL_ACTIVE       0x2 // is an active pool skill
#define SKILL_POOL_ACTIVATED    0x4 // pool skill has been activated (used for clif)

// スキルデータベース
struct skill_db
{
    int range[MAX_SKILL_LEVEL], hit, inf, pl, nk, max, stat, poolflags, max_raise; // `max' is the global max, `max_raise' is the maximum attainable via skill-ups
    int num[MAX_SKILL_LEVEL];
    int cast[MAX_SKILL_LEVEL], delay[MAX_SKILL_LEVEL];
    int upkeep_time[MAX_SKILL_LEVEL], upkeep_time2[MAX_SKILL_LEVEL];
    int castcancel, cast_def_rate;
    int inf2, maxcount, skill_type;
    int blewcount[MAX_SKILL_LEVEL];
    int hp[MAX_SKILL_LEVEL], sp[MAX_SKILL_LEVEL], mhp[MAX_SKILL_LEVEL],
        hp_rate[MAX_SKILL_LEVEL], sp_rate[MAX_SKILL_LEVEL],
        zeny[MAX_SKILL_LEVEL];
    int weapon;
    SkillState state;
    int spiritball[MAX_SKILL_LEVEL];
    int itemid[10], amount[10];
    int castnodex[MAX_SKILL_LEVEL];
};
extern earray<struct skill_db, SkillID, MAX_SKILL_DB> skill_db;

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
struct skill_unit;
struct skill_unit_group;

int do_init_skill(void);

// スキルデータベースへのアクセサ
int skill_get_hit(SkillID id);
int skill_get_inf(SkillID id);
int skill_get_pl(SkillID id);
int skill_get_nk(SkillID id);
int skill_get_max(SkillID id);
int skill_get_max_raise(SkillID id);
int skill_get_range(SkillID id, int lv);
int skill_get_hp(SkillID id, int lv);
int skill_get_mhp(SkillID id, int lv);
int skill_get_sp(SkillID id, int lv);
int skill_get_zeny(SkillID id, int lv);
int skill_get_num(SkillID id, int lv);
int skill_get_cast(SkillID id, int lv);
int skill_get_delay(SkillID id, int lv);
int skill_get_time(SkillID id, int lv);
int skill_get_time2(SkillID id, int lv);
int skill_get_castdef(SkillID id);
int skill_get_weapontype(SkillID id);
int skill_get_unit_id(SkillID id, int flag);
int skill_get_inf2(SkillID id);
int skill_get_maxcount(SkillID id);
int skill_get_blewcount(SkillID id, int lv);

// スキルの使用
int skill_use_id(struct map_session_data *sd, int target_id,
        SkillID skill_num, int skill_lv);
int skill_use_pos(struct map_session_data *sd,
                    int skill_x, int skill_y, SkillID skill_num, int skill_lv);

int skill_castend_map(struct map_session_data *sd, SkillID skill_num,
                        const char *map);

int skill_cleartimerskill(struct block_list *src);
int skill_addtimerskill(struct block_list *src, unsigned int tick,
                          int target, int x, int y, SkillID skill_id,
                          int skill_lv, int type, int flag);

// 追加効果
int skill_additional_effect(struct block_list *src, struct block_list *bl,
                              SkillID skillid, int skilllv, int attack_type,
                              unsigned int tick);

// ユニットスキル
struct skill_unit *skill_initunit(struct skill_unit_group *group, int idx,
                                   int x, int y);
int skill_delunit(struct skill_unit *unit);
struct skill_unit_group *skill_initunitgroup(struct block_list *src,
                                              int count, SkillID skillid,
                                              int skilllv, int unit_id);
int skill_delunitgroup(struct skill_unit_group *group);
struct skill_unit_group_tickset *skill_unitgrouptickset_search(struct
                                                                block_list
                                                                *bl,
                                                                int group_id);
int skill_unitgrouptickset_delete(struct block_list *bl, int group_id);
int skill_clear_unitgroup(struct block_list *src);

int skill_unit_ondamaged(struct skill_unit *src, struct block_list *bl,
                           int damage, unsigned int tick);

int skill_castfix(struct block_list *bl, int time);
int skill_delayfix(struct block_list *bl, int time);
int skill_check_unit_range(int m, int x, int y, int range, SkillID skillid);
int skill_check_unit_range2(int m, int x, int y, int range);
// -- moonsoul  (added skill_check_unit_cell)
int skill_check_unit_cell(SkillID skillid, int m, int x, int y, int unit_id);
int skill_unit_out_all(struct block_list *bl, unsigned int tick, int range);
int skill_unit_move(struct block_list *bl, unsigned int tick, int range);
int skill_unit_move_unit_group(struct skill_unit_group *group, int m,
                                 int dx, int dy);

struct skill_unit_group *skill_check_dancing(struct block_list *src);
void skill_stop_dancing(struct block_list *src, int flag);

// 詠唱キャンセル
int skill_castcancel(struct block_list *bl, int type);

int skill_gangsterparadise(struct map_session_data *sd, int type);
void skill_brandishspear_first(struct square *tc, int dir, int x, int y);
void skill_brandishspear_dir(struct square *tc, int dir, int are);
int skill_autospell(struct map_session_data *md, SkillID skillid);
void skill_devotion(struct map_session_data *md, int target);
void skill_devotion2(struct block_list *bl, int crusader);
int skill_devotion3(struct block_list *bl, int target);
void skill_devotion_end(struct map_session_data *md,
                         struct map_session_data *sd, int target);

#define skill_calc_heal(bl,skill_lv) (( battle_get_lv(bl)+battle_get_int(bl) )/8 *(4+ skill_lv*8))

// その他
int skill_check_cloaking(struct block_list *bl);
int skill_is_danceskill(SkillID id);

// ステータス異常
int skill_status_effect(struct block_list *bl, StatusChange type,
        int val1, int val2, int val3, int val4,
        int tick, int flag, int spell_invocation);
int skill_status_change_start(struct block_list *bl, StatusChange type,
        int val1, int val2, int val3, int val4, int tick, int flag);
void skill_status_change_timer(timer_id, tick_t, custom_id_t, custom_data_t);
int skill_status_change_active(struct block_list *bl, StatusChange type);  // [fate]
int skill_encchant_eremental_end(struct block_list *bl, StatusChange type);
int skill_status_change_end(struct block_list *bl, StatusChange type, int tid);
int skill_status_change_clear(struct block_list *bl, int type);

// mobスキルのため
int skill_castend_nodamage_id(struct block_list *src, struct block_list *bl,
                                SkillID skillid, int skilllv, unsigned int tick,
                                int flag);
int skill_castend_damage_id(struct block_list *src, struct block_list *bl,
                              SkillID skillid, int skilllv, unsigned int tick,
                              int flag);
int skill_castend_pos2(struct block_list *src, int x, int y, SkillID skillid,
                         int skilllv, unsigned int tick, int flag);

// スキル攻撃一括処理
int skill_attack(int attack_type, struct block_list *src,
                   struct block_list *dsrc, struct block_list *bl,
                   SkillID skillid, int skilllv, unsigned int tick, int flag);

int skill_update_heal_animation(struct map_session_data *sd); // [Fate]  Check whether the healing flag must be updated, do so if needed

void skill_reload(void);

extern earray<StatusChange, SkillID, MAX_SKILL_DB> SkillStatusChangeTable;

// [Fate] Skill pools API

// Max. # of active entries in the skill pool
#define MAX_SKILL_POOL 3
// Max. # of skills that may be classified as pool skills in db/skill_db.txt
#define MAX_POOL_SKILLS 128

extern SkillID skill_pool_skills[MAX_POOL_SKILLS];  // All pool skills
extern int skill_pool_skills_size;  // Number of entries in skill_pool_skills

// Yields all active skills in the skill pool; no more than MAX_SKILL_POOL.  Return is number of skills.
int skill_pool(struct map_session_data *sd, SkillID *skills);
int skill_pool_size(struct map_session_data *sd);
int skill_pool_max(struct map_session_data *sd);  // Max. number of pool skills
void skill_pool_empty(struct map_session_data *sd);    // Deactivate all pool skills
// Skill into skill pool.  Return is zero iff okay.
int skill_pool_activate(struct map_session_data *sd, SkillID skill);
// Skill into skill pool.  Return is zero when activated.
int skill_pool_is_activated(struct map_session_data *sd, SkillID skill);
// Skill out of skill pool.  Return is zero iff okay.
int skill_pool_deactivate(struct map_session_data *sd, SkillID skill);
// Yield configurable skill name
inline
const char *skill_name(SkillID skill)
{
    return skill_lookup_by_id(skill).desc;
}
// Yields the stat associated with a skill.
// Returns zero if none, or SP_STR, SP_VIT, ... otherwise
int skill_stat(SkillID skill);
// Yields the power of a skill.
// This is zero if the skill is unknown
//      or if it's a pool skill that is outside of the skill pool,
// otherwise a value from 0 to 255 (with 200 being the `normal maximum')
int skill_power(struct map_session_data *sd, SkillID skill);
int skill_power_bl(struct block_list *bl, SkillID skill);

// [Fate] Remember that a certain skill ID belongs to a pool skill
void skill_pool_register(SkillID id);
#endif
