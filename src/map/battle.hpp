#pragma once
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

#include "battle.t.hpp"

#include "fwd.hpp"

#include "../net/timer.t.hpp"

#include "../mmo/clif.t.hpp"
#include "map.t.hpp"
#include "../mmo/skill.t.hpp"


namespace tmwa
{
namespace map
{
// ダメージ
struct Damage
{
    int damage;
    DamageType type;
    int div_;
    interval_t amotion, dmotion;
    BF flag;
    ATK dmg_lv;
};

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

// 通常攻撃処理まとめ
ATK battle_weapon_attack(dumb_ptr<block_list> bl, dumb_ptr<block_list> target,
        tick_t tick);

int battle_is_unarmed(dumb_ptr<block_list> bl);
Species battle_get_class(dumb_ptr<block_list> bl);
VString<23> battle_get_name(dumb_ptr<block_list> bl);
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
int battle_get_atk(dumb_ptr<block_list> bl);
int battle_get_atk2(dumb_ptr<block_list> bl);
int battle_get_matk1(dumb_ptr<block_list> bl);
int battle_get_matk2(dumb_ptr<block_list> bl);
int battle_get_hit(dumb_ptr<block_list> bl);
int battle_get_flee(dumb_ptr<block_list> bl);
int battle_get_flee2(dumb_ptr<block_list> bl);
int battle_get_critical(dumb_ptr<block_list> bl);
int battle_get_baseatk(dumb_ptr<block_list> bl);
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
PartyId battle_get_party_id(dumb_ptr<block_list> bl);
Race battle_get_race(dumb_ptr<block_list> bl);
MobMode battle_get_mode(dumb_ptr<block_list> bl);
int battle_get_stat(SP stat_id, dumb_ptr<block_list> bl);

eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> battle_get_sc_data(dumb_ptr<block_list> bl);
Opt1 *battle_get_opt1(dumb_ptr<block_list> bl);
Opt2 *battle_get_opt2(dumb_ptr<block_list> bl);
Opt3 *battle_get_opt3(dumb_ptr<block_list> bl);
Opt0 *battle_get_option(dumb_ptr<block_list> bl);

bool battle_check_undead(Race race, Element element);
int battle_check_target(dumb_ptr<block_list> src, dumb_ptr<block_list> target,
        BCT flag);
int battle_check_range(dumb_ptr<block_list> src, dumb_ptr<block_list> bl,
        int range);
} // namespace map
} // namespace tmwa
