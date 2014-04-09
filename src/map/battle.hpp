#ifndef TMWA_MAP_BATTLE_HPP
#define TMWA_MAP_BATTLE_HPP
//    battle.hpp - Not so scary code.
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

# include "../sanity.hpp"

# include "battle.t.hpp"

# include "../strings/fwd.hpp"

# include "../mmo/timer.t.hpp"

# include "magic-interpreter.t.hpp"
# include "map.t.hpp"
# include "skill.t.hpp"

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

eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> battle_get_sc_data(dumb_ptr<block_list> bl);
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
    int casting_rate, delay_rate, delay_dependon_dex;
    int skill_delay_attack_enable;
    int monster_skill_add_range;
    int player_damage_delay;
    int flooritem_lifetime;
    int item_auto_get;
    int item_first_get_time;
    int item_second_get_time;
    int item_third_get_time;
    int base_exp_rate, job_exp_rate;
    int death_penalty_type;
    int death_penalty_base, death_penalty_job;
    int restart_hp_rate;
    int restart_sp_rate;
    int monster_hp_rate;
    int monster_max_aspd;
    int atcommand_gm_only;
    int atcommand_spawn_quantity_limit;
    int gm_all_equipment;
    int monster_active_enable;
    int mob_skill_use;
    int mob_count_rate;
    int basic_skill_check;
    int player_invincible_time;
    int skill_min_damage;
    int natural_healhp_interval;
    int natural_healsp_interval;
    int natural_heal_skill_interval;
    int natural_heal_weight_rate;
    int arrow_decrement;
    int max_aspd;
    int max_hp;
    int max_sp;
    int max_lv;
    int max_parameter;
    int monster_skill_log;
    int battle_log;
    int save_log;
    int error_log;
    int etc_log;
    int save_clothcolor;
    int undead_detect_type;
    int agi_penaly_type;
    int agi_penaly_count;
    int agi_penaly_num;
    int vit_penaly_type;
    int vit_penaly_count;
    int vit_penaly_num;
    int mob_changetarget_byskill;
    int player_attack_direction_change;
    int monster_attack_direction_change;
    int display_delay_skill_fail;
    int dead_branch_active;
    int show_steal_in_same_party;

    int prevent_logout;

    int alchemist_summon_reward;
    int maximum_level;
    int drops_by_luk;
    int monsters_ignore_gm;
    int multi_level_up;
    int pk_mode;

    int agi_penaly_count_lv;
    int vit_penaly_count_lv;

    int hide_GM_session;
    int invite_request_check;
    int disp_experience;

    int hack_info_GM_level;
    int any_warp_GM_min_level;

    int min_hair_style;
    int max_hair_style;
    int min_hair_color;
    int max_hair_color;
    int min_cloth_color;
    int max_cloth_color;

    int castrate_dex_scale;
    int area_size;

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

    int drop_pickup_safety_zone;
    int itemheal_regeneration_factor;

    int mob_splash_radius;
} battle_config;

bool battle_config_read(ZString cfgName);
void battle_config_check();

#endif // TMWA_MAP_BATTLE_HPP
