#ifndef BATTLE_HPP
#define BATTLE_HPP

#include "battle.t.hpp"

#include "../common/timer.t.hpp"

#include "magic-interpreter.t.hpp"
#include "map.t.hpp"
#include "skill.t.hpp"

// ダメージ
struct Damage
{
    int damage, damage2;
    DamageType type;
    int div_;
    interval_t amotion, dmotion;
    BF flag;
    ATK dmg_lv;
};

struct map_session_data;
struct mob_data;
struct block_list;

// ダメージ計算

struct Damage battle_calc_attack(BF attack_type,
        dumb_ptr<block_list> bl, dumb_ptr<block_list> target,
        SkillID skill_num, int skill_lv, int flag);

// 実際にHPを増減
int battle_damage(dumb_ptr<block_list> bl, dumb_ptr<block_list> target,
        int damage, int flag);
int battle_heal(dumb_ptr<block_list> bl, dumb_ptr<block_list> target, int hp,
        int sp, int flag);

// 攻撃や移動を止める
int battle_stopattack(dumb_ptr<block_list> bl);
int battle_stopwalking(dumb_ptr<block_list> bl, int type);

// 通常攻撃処理まとめ
ATK battle_weapon_attack(dumb_ptr<block_list> bl, dumb_ptr<block_list> target,
        tick_t tick);

int battle_is_unarmed(dumb_ptr<block_list> bl);
int battle_get_class(dumb_ptr<block_list> bl);
DIR battle_get_dir(dumb_ptr<block_list> bl);
int battle_get_lv(dumb_ptr<block_list> bl);
int battle_get_range(dumb_ptr<block_list> bl);
int battle_get_hp(dumb_ptr<block_list> bl);
int battle_get_max_hp(dumb_ptr<block_list> bl);
int battle_get_str(dumb_ptr<block_list> bl);
int battle_get_agi(dumb_ptr<block_list> bl);
int battle_get_vit(dumb_ptr<block_list> bl);
int battle_get_int(dumb_ptr<block_list> bl);
int battle_get_dex(dumb_ptr<block_list> bl);
int battle_get_luk(dumb_ptr<block_list> bl);
int battle_get_def(dumb_ptr<block_list> bl);
int battle_get_mdef(dumb_ptr<block_list> bl);
int battle_get_def2(dumb_ptr<block_list> bl);
int battle_get_mdef2(dumb_ptr<block_list> bl);
interval_t battle_get_speed(dumb_ptr<block_list> bl);
interval_t battle_get_adelay(dumb_ptr<block_list> bl);
interval_t battle_get_amotion(dumb_ptr<block_list> bl);
interval_t battle_get_dmotion(dumb_ptr<block_list> bl);
LevelElement battle_get_element(dumb_ptr<block_list> bl);
inline
Element battle_get_elem_type(dumb_ptr<block_list> bl)
{
    return battle_get_element(bl).element;
}
int battle_get_party_id(dumb_ptr<block_list> bl);
Race battle_get_race(dumb_ptr<block_list> bl);
MobMode battle_get_mode(dumb_ptr<block_list> bl);
int battle_get_stat(SP stat_id, dumb_ptr<block_list> bl);

eptr<struct status_change, StatusChange> battle_get_sc_data(dumb_ptr<block_list> bl);
short *battle_get_sc_count(dumb_ptr<block_list> bl);
Opt1 *battle_get_opt1(dumb_ptr<block_list> bl);
Opt2 *battle_get_opt2(dumb_ptr<block_list> bl);
Opt3 *battle_get_opt3(dumb_ptr<block_list> bl);
Option *battle_get_option(dumb_ptr<block_list> bl);

bool battle_check_undead(Race race, Element element);
int battle_check_target(dumb_ptr<block_list> src, dumb_ptr<block_list> target,
        BCT flag);
int battle_check_range(dumb_ptr<block_list> src, dumb_ptr<block_list> bl,
        int range);

extern struct Battle_Config
{
    int warp_point_debug;
    int enemy_critical;
    int enemy_critical_rate;
    int enemy_str;
    int enemy_perfect_flee;
    int cast_rate, delay_rate, delay_dependon_dex;
    int sdelay_attack_enable;
    int left_cardfix_to_right;
    int pc_skill_add_range;
    int skill_out_range_consume;
    int mob_skill_add_range;
    int pc_damage_delay;
    int defnotenemy;
    int random_monster_checklv;
    int attr_recover;
    int flooritem_lifetime;
    int item_auto_get;
    int item_first_get_time;
    int item_second_get_time;
    int item_third_get_time;
    int base_exp_rate, job_exp_rate;
    int death_penalty_type;
    int death_penalty_base, death_penalty_job;
    int pvp_exp;               // [MouseJstr]
    int gtb_pvp_only;          // [MouseJstr]
    int zeny_penalty;
    int restart_hp_rate;
    int restart_sp_rate;
    int monster_hp_rate;
    int monster_max_aspd;
    int atc_gmonly;
    int atc_spawn_quantity_limit;
    int gm_allskill;
    int gm_allskill_addabra;
    int gm_allequip;
    int gm_skilluncond;
    int skillfree;
    int skillup_limit;
    int wp_rate;
    int pp_rate;
    int monster_active_enable;
    int mob_skill_use;
    int mob_count_rate;
    int quest_skill_learn;
    int quest_skill_reset;
    int basic_skill_check;
    int pc_invincible_time;
    int skill_min_damage;
    int finger_offensive_type;
    int heal_exp;
    int resurrection_exp;
    int shop_exp;
    int combo_delay_rate;
    int item_check;
    int wedding_modifydisplay;
    int natural_healhp_interval;
    int natural_healsp_interval;
    int natural_heal_skill_interval;
    int natural_heal_weight_rate;
    int item_name_override_grffile;
    int arrow_decrement;
    int max_aspd;
    int max_hp;
    int max_sp;
    int max_lv;
    int max_parameter;
    int max_cart_weight;
    int pc_skill_log;
    int mob_skill_log;
    int battle_log;
    int save_log;
    int error_log;
    int etc_log;
    int save_clothcolor;
    int undead_detect_type;
    int pc_auto_counter_type;
    int monster_auto_counter_type;
    int agi_penaly_type;
    int agi_penaly_count;
    int agi_penaly_num;
    int vit_penaly_type;
    int vit_penaly_count;
    int vit_penaly_num;
    int pc_skill_reiteration;
    int monster_skill_reiteration;
    int pc_skill_nofootset;
    int monster_skill_nofootset;
    int pc_cloak_check_type;
    int monster_cloak_check_type;
    int mob_changetarget_byskill;
    int pc_attack_direction_change;
    int monster_attack_direction_change;
    int pc_undead_nofreeze;
    int pc_land_skill_limit;
    int monster_land_skill_limit;
    int party_skill_penaly;
    int monster_class_change_full_recover;
    int produce_item_name_input;
    int produce_potion_name_input;
    int making_arrow_name_input;
    int holywater_name_input;
    int display_delay_skill_fail;
    int chat_warpportal;
    int mob_warpportal;
    int dead_branch_active;
    int show_steal_in_same_party;
    int enable_upper_class;
    int mob_attack_attr_none;
    int mob_ghostring_fix;
    int pc_attack_attr_none;

    int prevent_logout;        // Added by RoVeRT

    int alchemist_summon_reward;   // [Valaris]
    int maximum_level;
    int drops_by_luk;
    int monsters_ignore_gm;
    int multi_level_up;
    int pk_mode;
    int show_mob_hp;           // end additions [Valaris]

    int agi_penaly_count_lv;
    int vit_penaly_count_lv;

    int gx_allhit;
    int gx_cardfix;
    int gx_dupele;
    int gx_disptype;
    int player_skill_partner_check;
    int hide_GM_session;
    int unit_movement_type;
    int invite_request_check;
    int skill_removetrap_type;
    int disp_experience;
    int riding_weight;
    int backstab_bow_penalty;

    int hack_info_GM_level;    // added by [Yor]
    int any_warp_GM_min_level; // added by [Yor]
    int packet_ver_flag;       // added by [Yor]

    int min_hair_style;        // added by [MouseJstr]
    int max_hair_style;        // added by [MouseJstr]
    int min_hair_color;        // added by [MouseJstr]
    int max_hair_color;        // added by [MouseJstr]
    int min_cloth_color;       // added by [MouseJstr]
    int max_cloth_color;       // added by [MouseJstr]

    int castrate_dex_scale;    // added by [MouseJstr]
    int area_size;             // added by [MouseJstr]

    int chat_lame_penalty;
    int chat_spam_threshold;
    int chat_spam_flood;
    int chat_spam_ban;
    int chat_spam_warn;
    int chat_maxline;

    int packet_spam_threshold;
    int packet_spam_flood;
    int packet_spam_kick;

    int mask_ip_gms;

    int drop_pickup_safety_zone;   // [Fate] Max. distance to an object dropped by a kill by self in which dropsteal protection works
    int itemheal_regeneration_factor;  // [Fate] itemheal speed factor

    int mob_splash_radius;
} battle_config;

int battle_config_read(const char *cfgName);

#endif // BATTLE_HPP
