#include "battle.hpp"
//    battle.cpp - Not so scary code.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2012 Vincent Petithory
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

#include <algorithm>

#include "../compat/nullpo.hpp"

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../generic/random.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/read.hpp"
#include "../io/span.hpp"

#include "../mmo/config_parse.hpp"
#include "../mmo/cxxstdio_enums.hpp"

#include "../high/utils.hpp"

#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "path.hpp"
#include "pc.hpp"
#include "skill.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
/*==========================================
 * 自分をロックしている対象の数を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
static
int battle_counttargeted(dumb_ptr<block_list> bl, dumb_ptr<block_list> src,
        ATK target_lv)
{
    nullpo_retz(bl);
    if (bl->bl_type == BL::PC)
        return pc_counttargeted(bl->is_player(), src,
                                 target_lv);
    else if (bl->bl_type == BL::MOB)
        return mob_counttargeted(bl->is_mob(), src, target_lv);
    return 0;
}

/*==========================================
 * 対象のClassを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
Species battle_get_class(dumb_ptr<block_list> bl)
{
    nullpo_retr(Species(), bl);
    if (bl->bl_type == BL::MOB)
        return bl->is_mob()->mob_class;
    else if (bl->bl_type == BL::NPC)
        return bl->is_npc()->npc_class;
    else if (bl->bl_type == BL::PC)
        return bl->is_player()->status.species;
    else
        return Species();
}

/*==========================================
 * 対象の方向を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
DIR battle_get_dir(dumb_ptr<block_list> bl)
{
    nullpo_retr(DIR::S, bl);
    if (bl->bl_type == BL::MOB)
        return bl->is_mob()->dir;
    else if (bl->bl_type == BL::PC)
        return bl->is_player()->dir;
    else
        return DIR::S;
}

/*==========================================
 * 対象のレベルを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_lv(dumb_ptr<block_list> bl)
{
    nullpo_retz(bl);
    if (bl->bl_type == BL::MOB)
        return bl->is_mob()->stats[mob_stat::LV];
    else if (bl->bl_type == BL::PC)
        return bl->is_player()->status.base_level;
    else
        return 0;
}

/*==========================================
 * 対象の射程を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_range(dumb_ptr<block_list> bl)
{
    nullpo_retz(bl);
    if (bl->bl_type == BL::MOB)
        return get_mob_db(bl->is_mob()->mob_class).range;
    else if (bl->bl_type == BL::PC)
        return (bl->is_player()->attack_spell_override
                    ? bl->is_player()->attack_spell_range
                    : bl->is_player()->attackrange);
    else
        return 0;
}

/*==========================================
 * 対象のHPを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_hp(dumb_ptr<block_list> bl)
{
    nullpo_retr(1, bl);
    if (bl->bl_type == BL::MOB)
        return bl->is_mob()->hp;
    else if (bl->bl_type == BL::PC)
        return bl->is_player()->status.hp;
    else
        return 1;
}

/*==========================================
 * 対象のMHPを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_max_hp(dumb_ptr<block_list> bl)
{
    nullpo_retr(1, bl);
    if (bl->bl_type == BL::PC)
        return bl->is_player()->status.max_hp;
    else
    {
        int max_hp = 1;
        if (bl->bl_type == BL::MOB)
        {
            max_hp = bl->is_mob()->stats[mob_stat::MAX_HP];
            {
                if (battle_config.monster_hp_rate != 100)
                    max_hp = (max_hp * battle_config.monster_hp_rate) / 100;
            }
        }
        if (max_hp < 1)
            max_hp = 1;
        return max_hp;
    }
}

VString<23> battle_get_name(dumb_ptr<block_list> bl)
{
    VString<23> name;
    nullpo_retr(name, bl);

    switch (bl->bl_type)
    {
        case BL::PC:
            name = bl->is_player()->status_key.name.to__actual();
            break;
        case BL::NPC:
            name = bl->is_npc()->name;
            break;
        case BL::MOB:
            name = bl->is_mob()->name;
            break;
    }

    return name;
}

/*==========================================
 * 対象のStrを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_str(dumb_ptr<block_list> bl)
{
    int str = 0;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        str = bl->is_mob()->stats[mob_stat::STR];
    else if (bl->bl_type == BL::PC)
        return bl->is_player()->paramc[ATTR::STR];

    if (str < 0)
        str = 0;
    return str;
}

/*==========================================
 * 対象のAgiを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */

int battle_get_agi(dumb_ptr<block_list> bl)
{
    int agi = 0;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        agi = bl->is_mob()->stats[mob_stat::AGI];
    else if (bl->bl_type == BL::PC)
        agi = bl->is_player()->paramc[ATTR::AGI];

    if (agi < 0)
        agi = 0;
    return agi;
}

/*==========================================
 * 対象のVitを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_vit(dumb_ptr<block_list> bl)
{
    int vit = 0;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        vit = bl->is_mob()->stats[mob_stat::VIT];
    else if (bl->bl_type == BL::PC)
        vit = bl->is_player()->paramc[ATTR::VIT];

    if (vit < 0)
        vit = 0;
    return vit;
}

/*==========================================
 * 対象のIntを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_int(dumb_ptr<block_list> bl)
{
    int int_ = 0;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        int_ = bl->is_mob()->stats[mob_stat::INT];
    else if (bl->bl_type == BL::PC)
        int_ = bl->is_player()->paramc[ATTR::INT];

    if (int_ < 0)
        int_ = 0;
    return int_;
}

/*==========================================
 * 対象のDexを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_dex(dumb_ptr<block_list> bl)
{
    int dex = 0;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        dex = bl->is_mob()->stats[mob_stat::DEX];
    else if (bl->bl_type == BL::PC)
        dex = bl->is_player()->paramc[ATTR::DEX];

    if (dex < 0)
        dex = 0;
    return dex;
}

/*==========================================
 * 対象のLukを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_luk(dumb_ptr<block_list> bl)
{
    int luk = 0;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        luk = bl->is_mob()->stats[mob_stat::LUK];
    else if (bl->bl_type == BL::PC)
        luk = bl->is_player()->paramc[ATTR::LUK];

    if (luk < 0)
        luk = 0;
    return luk;
}

/*==========================================
 * 対象のFleeを返す(汎用)
 * 戻りは整数で1以上
 *------------------------------------------
 */
int battle_get_flee(dumb_ptr<block_list> bl)
{
    int flee = 1;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        flee = bl->is_player()->flee;
    else
        flee = battle_get_agi(bl) + battle_get_lv(bl);

    if (sc_data)
    {
        if (battle_is_unarmed(bl))
            flee += (skill_power_bl(bl, SkillID::TMW_BRAWLING) >> 3);   // +25 for 200
        flee += skill_power_bl(bl, SkillID::TMW_SPEED) >> 3;
    }
    if (flee < 1)
        flee = 1;
    return flee;
}

/*==========================================
 * 対象のHitを返す(汎用)
 * 戻りは整数で1以上
 *------------------------------------------
 */
int battle_get_hit(dumb_ptr<block_list> bl)
{
    int hit = 1;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        hit = bl->is_player()->hit;
    else
        hit = battle_get_dex(bl) + battle_get_lv(bl);

    if (sc_data)
    {
        if (battle_is_unarmed(bl))
            hit += (skill_power_bl(bl, SkillID::TMW_BRAWLING) >> 4);    // +12 for 200
    }
    if (hit < 1)
        hit = 1;
    return hit;
}

/*==========================================
 * 対象の完全回避を返す(汎用)
 * 戻りは整数で1以上
 *------------------------------------------
 */
int battle_get_flee2(dumb_ptr<block_list> bl)
{
    int flee2 = 1;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = bl->is_player();
        flee2 = battle_get_luk(bl) + 10;
        flee2 += sd->flee2 - (sd->paramc[ATTR::LUK] + 10);
    }
    else
        flee2 = battle_get_luk(bl) + 1;

    {
        if (battle_is_unarmed(bl))
            flee2 += (skill_power_bl(bl, SkillID::TMW_BRAWLING) >> 3);  // +25 for 200
        flee2 += skill_power_bl(bl, SkillID::TMW_SPEED) >> 3;
    }
    if (flee2 < 1)
        flee2 = 1;
    return flee2;
}

/*==========================================
 * 対象のクリティカルを返す(汎用)
 * 戻りは整数で1以上
 *------------------------------------------
 */
int battle_get_critical(dumb_ptr<block_list> bl)
{
    int critical = 1;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = bl->is_player();
        critical = battle_get_luk(bl) * 2 + 10;
        critical += sd->critical - ((sd->paramc[ATTR::LUK] * 3) + 10);
    }
    else
        critical = battle_get_luk(bl) * 3 + 1;

    if (critical < 1)
        critical = 1;
    return critical;
}

/*==========================================
 * base_atkの取得
 * 戻りは整数で1以上
 *------------------------------------------
 */
int battle_get_baseatk(dumb_ptr<block_list> bl)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    int batk = 1;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        batk = bl->is_player()->base_atk;  //設定されているbase_atk
    else
    {                           //それ以外なら
        int str, dstr;
        str = battle_get_str(bl);  //STR
        dstr = str / 10;
        batk = dstr * dstr + str;   //base_atkを計算する
    }
    if (batk < 1)
        batk = 1;               //base_atkは最低でも1
    return batk;
}

/*==========================================
 * 対象のAtkを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_atk(dumb_ptr<block_list> bl)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    int atk = 0;

    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        atk = bl->is_player()->watk;
    else if (bl->bl_type == BL::MOB)
        atk = bl->is_mob()->stats[mob_stat::ATK1];

    if (atk < 0)
        atk = 0;
    return atk;
}

/*==========================================
 * 対象のAtk2を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_atk2(dumb_ptr<block_list> bl)
{
    nullpo_retz(bl);
    if (bl->bl_type == BL::PC)
        return bl->is_player()->watk2;
    else
    {
        int atk2 = 0;
        if (bl->bl_type == BL::MOB)
            atk2 = bl->is_mob()->stats[mob_stat::ATK2];

        if (atk2 < 0)
            atk2 = 0;
        return atk2;
    }
}

/*==========================================
 * 対象のMAtk1を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_matk1(dumb_ptr<block_list> bl)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
    {
        int matk, int_ = battle_get_int(bl);
        matk = int_ + (int_ / 5) * (int_ / 5);

        return matk;
    }
    else if (bl->bl_type == BL::PC)
        return bl->is_player()->matk1;
    else
        return 0;
}

/*==========================================
 * 対象のMAtk2を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_matk2(dumb_ptr<block_list> bl)
{
    nullpo_retz(bl);
    if (bl->bl_type == BL::MOB)
    {
        int matk, int_ = battle_get_int(bl);
        matk = int_ + (int_ / 7) * (int_ / 7);

        return matk;
    }
    else if (bl->bl_type == BL::PC)
        return bl->is_player()->matk2;
    else
        return 0;
}

/*==========================================
 * 対象のDefを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_def(dumb_ptr<block_list> bl)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    int def = 0;

    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
    {
        def = bl->is_player()->def;
    }
    else if (bl->bl_type == BL::MOB)
    {
        def = bl->is_mob()->stats[mob_stat::DEF];
    }

    if (def < 1000000)
    {
        if (sc_data)
        {
            //毒にかかっている時は減算
            if (sc_data[StatusChange::SC_POISON].timer
                && bl->bl_type != BL::PC)
                def = def * 75 / 100;
        }
    }
    if (def < 0)
        def = 0;
    return def;
}

/*==========================================
 * 対象のMDefを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_mdef(dumb_ptr<block_list> bl)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    int mdef = 0;

    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        mdef = bl->is_player()->mdef;
    else if (bl->bl_type == BL::MOB)
        mdef = bl->is_mob()->stats[mob_stat::MDEF];

    if (mdef < 1000000)
    {
        if (sc_data)
        {
            //バリアー状態時はMDEF100
            if (mdef < 90 && sc_data[StatusChange::SC_MBARRIER].timer)
            {
                mdef += sc_data[StatusChange::SC_MBARRIER].val1;
                if (mdef > 90)
                    mdef = 90;
            }
        }
    }
    if (mdef < 0)
        mdef = 0;
    return mdef;
}

/*==========================================
 * 対象のDef2を返す(汎用)
 * 戻りは整数で1以上
 *------------------------------------------
 */
int battle_get_def2(dumb_ptr<block_list> bl)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    int def2 = 1;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        def2 = bl->is_player()->def2;
    else if (bl->bl_type == BL::MOB)
        def2 = bl->is_mob()->stats[mob_stat::VIT];

    if (sc_data)
    {
        if (sc_data[StatusChange::SC_POISON].timer
            && bl->bl_type != BL::PC)
            def2 = def2 * 75 / 100;
    }
    if (def2 < 1)
        def2 = 1;
    return def2;
}

/*==========================================
 * 対象のMDef2を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_mdef2(dumb_ptr<block_list> bl)
{
    int mdef2 = 0;

    nullpo_retz(bl);
    if (bl->bl_type == BL::MOB)
    {
        dumb_ptr<mob_data> md = bl->is_mob();
        mdef2 = md->stats[mob_stat::INT] + (md->stats[mob_stat::VIT] >> 1);
    }
    else if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = bl->is_player();
        mdef2 = sd->mdef2 + (sd->paramc[ATTR::VIT] >> 1);
    }

    if (mdef2 < 0)
        mdef2 = 0;
    return mdef2;
}

/*==========================================
 * 対象のSpeed(移動速度)を返す(汎用)
 * 戻りは整数で1以上
 * Speedは小さいほうが移動速度が速い
 *------------------------------------------
 */
interval_t battle_get_speed(dumb_ptr<block_list> bl)
{
    nullpo_retr(1_s, bl);
    if (bl->bl_type == BL::PC)
        return bl->is_player()->speed;
    else
    {
        interval_t speed = 1_s;
        if (bl->bl_type == BL::MOB)
            speed = static_cast<interval_t>(bl->is_mob()->stats[mob_stat::SPEED]);

        return std::max(speed, 1_ms);
    }
}

/*==========================================
 * 対象のaDelay(攻撃時ディレイ)を返す(汎用)
 * aDelayは小さいほうが攻撃速度が速い
 *------------------------------------------
 */
// TODO figure out what all the doubling is about
interval_t battle_get_adelay(dumb_ptr<block_list> bl)
{
    nullpo_retr(4_s, bl);
    if (bl->bl_type == BL::PC)
        return bl->is_player()->aspd * 2;
    else
    {
        eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data = battle_get_sc_data(bl);
        interval_t adelay = 4_s;
        int aspd_rate = 100;
        if (bl->bl_type == BL::MOB)
            adelay = static_cast<interval_t>(bl->is_mob()->stats[mob_stat::ADELAY]);

        if (sc_data)
        {
            if (sc_data[StatusChange::SC_SPEEDPOTION0].timer)
                aspd_rate -= sc_data[StatusChange::SC_SPEEDPOTION0].val1;
            // Fate's `haste' spell works the same as the above
            if (sc_data[StatusChange::SC_HASTE].timer)
                aspd_rate -= sc_data[StatusChange::SC_HASTE].val1;
        }

        if (aspd_rate != 100)
            adelay = adelay * aspd_rate / 100;
        return std::max(adelay, battle_config.monster_max_aspd * 2);
    }
}

interval_t battle_get_amotion(dumb_ptr<block_list> bl)
{
    nullpo_retr(2_s, bl);
    if (bl->bl_type == BL::PC)
        return bl->is_player()->amotion;
    else
    {
        eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data = battle_get_sc_data(bl);
        interval_t amotion = 2_s;
        int aspd_rate = 100;
        if (bl->bl_type == BL::MOB)
            amotion = get_mob_db(bl->is_mob()->mob_class).amotion;

        if (sc_data)
        {
            if (sc_data[StatusChange::SC_SPEEDPOTION0].timer)
                aspd_rate -= sc_data[StatusChange::SC_SPEEDPOTION0].val1;
            if (sc_data[StatusChange::SC_HASTE].timer)
                aspd_rate -= sc_data[StatusChange::SC_HASTE].val1;
        }

        if (aspd_rate != 100)
            amotion = amotion * aspd_rate / 100;
        return std::max(amotion, battle_config.monster_max_aspd);
    }
}

interval_t battle_get_dmotion(dumb_ptr<block_list> bl)
{
    nullpo_retr(interval_t::zero(), bl);
    if (bl->bl_type == BL::MOB)
    {
        return get_mob_db(bl->is_mob()->mob_class).dmotion;
    }
    else if (bl->bl_type == BL::PC)
    {
        return bl->is_player()->dmotion;
    }
    else
        return 2_s;
}

LevelElement battle_get_element(dumb_ptr<block_list> bl)
{
    LevelElement ret = {2, Element::neutral};

    nullpo_retr(ret, bl);
    if (bl->bl_type == BL::MOB)   // 10の位＝Lv*2、１の位＝属性
        ret = bl->is_mob()->def_ele;

    return ret;
}

PartyId battle_get_party_id(dumb_ptr<block_list> bl)
{
    nullpo_retr(PartyId(), bl);
    if (bl->bl_type == BL::PC)
        return bl->is_player()->status.party_id;
    else if (bl->bl_type == BL::MOB)
    {
        dumb_ptr<mob_data> md = bl->is_mob();
        if (md->master_id)
            return wrap<PartyId>(-unwrap<BlockId>(md->master_id));
        return wrap<PartyId>(-unwrap<BlockId>(md->bl_id));
    }
    return PartyId();
}

Race battle_get_race(dumb_ptr<block_list> bl)
{
    nullpo_retr(Race::formless, bl);
    if (bl->bl_type == BL::MOB)
        return get_mob_db(bl->is_mob()->mob_class).race;
    else if (bl->bl_type == BL::PC)
        return Race::demihuman;
    else
        return Race::formless;
}

MobMode battle_get_mode(dumb_ptr<block_list> bl)
{
    nullpo_retr(MobMode::CAN_MOVE, bl);
    if (bl->bl_type == BL::MOB)
        return get_mob_db(bl->is_mob()->mob_class).mode;
    // とりあえず動くということで1
    return MobMode::CAN_MOVE;
}

int battle_get_stat(SP stat_id, dumb_ptr<block_list> bl)
{
    switch (stat_id)
    {
        case SP::STR:
            return battle_get_str(bl);
        case SP::AGI:
            return battle_get_agi(bl);
        case SP::DEX:
            return battle_get_dex(bl);
        case SP::VIT:
            return battle_get_vit(bl);
        case SP::INT:
            return battle_get_int(bl);
        case SP::LUK:
            return battle_get_luk(bl);
        default:
            return 0;
    }
}

// StatusChange系の所得
eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> battle_get_sc_data(dumb_ptr<block_list> bl)
{
    nullpo_retr(nullptr, bl);

    switch (bl->bl_type)
    {
    case BL::MOB:
        return bl->is_mob()->sc_data;
    case BL::PC:
        return bl->is_player()->sc_data;
    }
    return nullptr;
}

Opt1 *battle_get_opt1(dumb_ptr<block_list> bl)
{
    nullpo_retn(bl);
    if (bl->bl_type == BL::MOB)
        return &bl->is_mob()->opt1;
    else if (bl->bl_type == BL::PC)
        return &bl->is_player()->opt1;
    else if (bl->bl_type == BL::NPC)
        return &bl->is_npc()->opt1;
    return nullptr;
}

Opt2 *battle_get_opt2(dumb_ptr<block_list> bl)
{
    nullpo_retn(bl);
    if (bl->bl_type == BL::MOB)
        return &bl->is_mob()->opt2;
    else if (bl->bl_type == BL::PC)
        return &bl->is_player()->opt2;
    else if (bl->bl_type == BL::NPC)
        return &bl->is_npc()->opt2;
    return nullptr;
}

Opt3 *battle_get_opt3(dumb_ptr<block_list> bl)
{
    nullpo_retn(bl);
    if (bl->bl_type == BL::MOB)
        return &bl->is_mob()->opt3;
    else if (bl->bl_type == BL::PC)
        return &bl->is_player()->opt3;
    else if (bl->bl_type == BL::NPC)
        return &bl->is_npc()->opt3;
    return nullptr;
}

Opt0 *battle_get_option(dumb_ptr<block_list> bl)
{
    nullpo_retn(bl);
    if (bl->bl_type == BL::MOB)
        return &bl->is_mob()->option;
    else if (bl->bl_type == BL::PC)
        return &bl->is_player()->status.option;
    else if (bl->bl_type == BL::NPC)
        return &bl->is_npc()->option;
    return nullptr;
}

//-------------------------------------------------------------------

// ダメージの遅延
struct battle_delay_damage_
{
    dumb_ptr<block_list> src, *target;
    int damage;
    int flag;
};

// 実際にHPを操作
int battle_damage(dumb_ptr<block_list> bl, dumb_ptr<block_list> target,
                   int damage, int flag)
{
    nullpo_retz(target);    //blはNULLで呼ばれることがあるので他でチェック

    if (damage == 0)
        return 0;

    if (target->bl_prev == nullptr)
        return 0;

    if (bl)
    {
        if (bl->bl_prev == nullptr)
            return 0;
    }

    if (damage < 0)
        return battle_heal(bl, target, -damage, 0, flag);

    if (target->bl_type == BL::MOB)
    {                           // MOB
        dumb_ptr<mob_data> md = target->is_mob();
        if (md && md->skilltimer && md->state.skillcastcancel)    // 詠唱妨害
            skill_castcancel(target, 0);
        return mob_damage(bl, md, damage, 0);
    }
    else if (target->bl_type == BL::PC)
    {                           // PC

        dumb_ptr<map_session_data> tsd = target->is_player();

        return pc_damage(bl, tsd, damage);

    }
    return 0;
}

int battle_heal(dumb_ptr<block_list> bl, dumb_ptr<block_list> target, int hp,
                 int sp, int flag)
{
    nullpo_retz(target);    //blはNULLで呼ばれることがあるので他でチェック

    if (target->bl_type == BL::PC
        && pc_isdead(target->is_player()))
        return 0;
    if (hp == 0 && sp == 0)
        return 0;

    if (hp < 0)
        return battle_damage(bl, target, -hp, flag);

    if (target->bl_type == BL::MOB)
        return mob_heal(target->is_mob(), hp);
    else if (target->bl_type == BL::PC)
        return pc_heal(target->is_player(), hp, sp);
    return 0;
}

// 攻撃停止
int battle_stopattack(dumb_ptr<block_list> bl)
{
    nullpo_retz(bl);
    if (bl->bl_type == BL::MOB)
        return mob_stopattack(bl->is_mob());
    else if (bl->bl_type == BL::PC)
        return pc_stopattack(bl->is_player());
    return 0;
}

/*==========================================
 * ダメージ最終計算
 *------------------------------------------
 */
static
int battle_calc_damage(dumb_ptr<block_list>, dumb_ptr<block_list> bl,
                        int damage, int div_,
                        SkillID, int, BF flag)
{
    dumb_ptr<mob_data> md = nullptr;

    nullpo_retz(bl);

    if (bl->bl_type == BL::MOB)
        md = bl->is_mob();

    if (battle_config.skill_min_damage
        || bool(flag & BF::MISC))
    {
        if (div_ < 255)
        {
            if (damage > 0 && damage < div_)
                damage = div_;
        }
        else if (damage > 0 && damage < 3)
            damage = 3;
    }

    if (md != nullptr && md->hp > 0 && damage > 0) // 反撃などのMOBスキル判定
        mobskill_event(md, flag);

    return damage;
}

static
struct Damage battle_calc_mob_weapon_attack(dumb_ptr<block_list> src,
                                                    dumb_ptr<block_list> target,
                                                    SkillID skill_num,
                                                    int skill_lv, int)
{
    dumb_ptr<map_session_data> tsd = nullptr;
    dumb_ptr<mob_data> md = src->is_mob(), tmd = nullptr;
    int hitrate, flee, cri = 0, atkmin, atkmax;
    int target_count = 1;
    int def1 = battle_get_def(target);
    int def2 = battle_get_def2(target);
    int t_vit = battle_get_vit(target);
    struct Damage wd {};
    int damage;
    DamageType type;
    int div_;
    BF flag;
    int ac_flag = 0;
    ATK dmg_lv = ATK::ZERO;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data, t_sc_data;

    nullpo_retr(wd, src);
    nullpo_retr(wd, target);
    nullpo_retr(wd, md);

    sc_data = battle_get_sc_data(src);

    // ターゲット
    if (target->bl_type == BL::PC)
        tsd = target->is_player();
    else if (target->bl_type == BL::MOB)
        tmd = target->is_mob();
    MobMode t_mode = battle_get_mode(target);
    t_sc_data = battle_get_sc_data(target);

    flag = BF::SHORT | BF::WEAPON | BF::NORMAL;    // 攻撃の種類の設定

    // 回避率計算、回避判定は後で
    flee = battle_get_flee(target);
    if (battle_config.agi_penaly_type > 0
        || battle_config.vit_penaly_type > 0)
        target_count +=
            battle_counttargeted(target, src,
                    battle_config.agi_penaly_count_lv);
    if (battle_config.agi_penaly_type > 0)
    {
        if (target_count >= battle_config.agi_penaly_count)
        {
            if (battle_config.agi_penaly_type == 1)
                flee =
                    (flee *
                     (100 -
                      (target_count -
                       (battle_config.agi_penaly_count -
                        1)) * battle_config.agi_penaly_num)) / 100;
            else if (battle_config.agi_penaly_type == 2)
                flee -=
                    (target_count -
                     (battle_config.agi_penaly_count -
                      1)) * battle_config.agi_penaly_num;
            if (flee < 1)
                flee = 1;
        }
    }
    hitrate = battle_get_hit(src) - flee + 80;

    type = DamageType::NORMAL;
    div_ = 1;                   // single attack

    if (battle_config.enemy_str)
        damage = battle_get_baseatk(src);
    else
        damage = 0;
    {
        atkmin = battle_get_atk(src);
        atkmax = battle_get_atk2(src);
    }
    if (get_mob_db(md->mob_class).range > 3)
        flag = (flag & ~BF::RANGEMASK) | BF::LONG;

    if (atkmin > atkmax)
        atkmin = atkmax;

    cri = battle_get_critical(src);
    cri -= battle_get_luk(target) * 3;
    if (battle_config.enemy_critical_rate != 100)
    {
        cri = cri * battle_config.enemy_critical_rate / 100;
        if (cri < 1)
            cri = 1;
    }

    if (ac_flag)
        cri = 1000;

    if (tsd && tsd->critical_def)
        cri = cri * (100 - tsd->critical_def) / 100;

    if ((skill_num == SkillID::ZERO)
        && skill_lv >= 0 && battle_config.enemy_critical
        && random_::chance({cri, 1000}))
        // 判定（スキルの場合は無視）
        // 敵の判定
    {
        damage += atkmax;
        type = DamageType::CRITICAL;
    }
    else
    {
        int vitbonusmax;

        if (atkmax > atkmin)
            damage += random_::in(atkmin, atkmax);
        else
            damage += atkmin;

        if (skill_num != SkillID::ZERO && skill_num != SkillID::NEGATIVE)
        {
            flag = (flag & ~BF::SKILLMASK) | BF::SKILL;
        }

        {
            // 対 象の防御力によるダメージの減少
            // ディバインプロテクション（ここでいいのかな？）
            if (def1 < 1000000)
            {                   //DEF, VIT無視
                int t_def;
                target_count =
                    1 + battle_counttargeted(target, src,
                            battle_config.vit_penaly_count_lv);
                if (battle_config.vit_penaly_type > 0)
                {
                    if (target_count >= battle_config.vit_penaly_count)
                    {
                        if (battle_config.vit_penaly_type == 1)
                        {
                            def1 =
                                (def1 *
                                 (100 -
                                  (target_count -
                                   (battle_config.vit_penaly_count -
                                    1)) * battle_config.vit_penaly_num)) /
                                100;
                            def2 =
                                (def2 *
                                 (100 -
                                  (target_count -
                                   (battle_config.vit_penaly_count -
                                    1)) * battle_config.vit_penaly_num)) /
                                100;
                            t_vit =
                                (t_vit *
                                 (100 -
                                  (target_count -
                                   (battle_config.vit_penaly_count -
                                    1)) * battle_config.vit_penaly_num)) /
                                100;
                        }
                        else if (battle_config.vit_penaly_type == 2)
                        {
                            def1 -=
                                (target_count -
                                 (battle_config.vit_penaly_count -
                                  1)) * battle_config.vit_penaly_num;
                            def2 -=
                                (target_count -
                                 (battle_config.vit_penaly_count -
                                  1)) * battle_config.vit_penaly_num;
                            t_vit -=
                                (target_count -
                                 (battle_config.vit_penaly_count -
                                  1)) * battle_config.vit_penaly_num;
                        }
                        if (def1 < 0)
                            def1 = 0;
                        if (def2 < 1)
                            def2 = 1;
                        if (t_vit < 1)
                            t_vit = 1;
                    }
                }
                t_def = def2 * 8 / 10;

                vitbonusmax = (t_vit / 20) * (t_vit / 20) - 1;
                {
                    damage = damage * (100 - def1) / 100;
                    damage -= t_def;
                    if (vitbonusmax > 0)
                       damage -= random_::in(0, vitbonusmax);
                }
            }
        }
    }

    // 0未満だった場合1に補正
    if (damage < 1)
        damage = 1;

    // 回避修正
    if (hitrate < 1000000)
        hitrate = ((hitrate > 95) ? 95 : ((hitrate < 5) ? 5 : hitrate));

    if (type == DamageType::NORMAL && !random_::chance({hitrate, 100}))
    {
        damage = 0;
        dmg_lv = ATK::FLEE;
    }
    else
    {
        dmg_lv = ATK::DEF;
    }

    if (damage < 0)
        damage = 0;

    // 完全回避の判定
    if (skill_num == SkillID::ZERO && skill_lv >= 0 && tsd != nullptr
        && random_::chance({battle_get_flee2(target), 1000}))
    {
        damage = 0;
        type = DamageType::FLEE2;
        dmg_lv = ATK::LUCKY;
    }

    if (battle_config.enemy_perfect_flee)
    {
        if (skill_num == SkillID::ZERO && skill_lv >= 0 && tmd != nullptr
            && random_::chance({battle_get_flee2(target), 1000}))
        {
            damage = 0;
            type = DamageType::FLEE2;
            dmg_lv = ATK::LUCKY;
        }
    }

//  if(def1 >= 1000000 && damage > 0)
    if (bool(t_mode & MobMode::PLANT) && damage > 0)
        damage = 1;

    damage = battle_calc_damage(src, target, damage, div_,
            skill_num, skill_lv, flag);

    wd.damage = damage;
    wd.type = type;
    wd.div_ = div_;
    wd.amotion = battle_get_amotion(src);
    wd.dmotion = battle_get_dmotion(target);
    wd.flag = flag;
    wd.dmg_lv = dmg_lv;
    return wd;
}

int battle_is_unarmed(dumb_ptr<block_list> bl)
{
    if (!bl)
        return 0;
    if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = bl->is_player();

        IOff0 sidx = sd->equip_index_maybe[EQUIP::SHIELD];
        IOff0 widx = sd->equip_index_maybe[EQUIP::WEAPON];
        return !sidx.ok() && !widx.ok();
    }
    else
        return 0;
}

/*
 * =========================================================================
 * PCの武器による攻撃
 *-------------------------------------------------------------------------
 */
static
struct Damage battle_calc_pc_weapon_attack(dumb_ptr<block_list> src,
                                                   dumb_ptr<block_list> target,
                                                   SkillID skill_num,
                                                   int skill_lv, int)
{
    dumb_ptr<map_session_data> sd = src->is_player(), tsd = nullptr;
    dumb_ptr<mob_data> tmd = nullptr;
    int hitrate, flee, cri = 0, atkmin, atkmax;
    int dex, target_count = 1;
    int def1 = battle_get_def(target);
    int def2 = battle_get_def2(target);
    int t_vit = battle_get_vit(target);
    struct Damage wd {};
    int damage;
    DamageType type;
    int div_;
    BF flag;
    ATK dmg_lv = ATK::ZERO;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data, t_sc_data;
    int watk;
    bool da = false;
    int ac_flag = 0;
    int target_distance;

    nullpo_retr(wd, src);
    nullpo_retr(wd, target);
    nullpo_retr(wd, sd);

    // アタッカー
    sc_data = battle_get_sc_data(src); //ステータス異常

    sd->state.attack_type = BF::WEAPON;  //攻撃タイプは武器攻撃

    // ターゲット
    if (target->bl_type == BL::PC)  //対象がPCなら
        tsd = target->is_player();   //tsdに代入(tmdはNULL)
    else if (target->bl_type == BL::MOB)    //対象がMobなら
        tmd = target->is_mob();   //tmdに代入(tsdはNULL)
    MobMode t_mode = battle_get_mode(target);  //対象のMode
    t_sc_data = battle_get_sc_data(target);    //対象のステータス異常

    flag = BF::SHORT | BF::WEAPON | BF::NORMAL;    // 攻撃の種類の設定

    // 回避率計算、回避判定は後で
    flee = battle_get_flee(target);
    if (battle_config.agi_penaly_type > 0 || battle_config.vit_penaly_type > 0) //AGI、VITペナルティ設定が有効
        target_count += battle_counttargeted(target, src,
                battle_config.agi_penaly_count_lv);  //対象の数を算出
    if (battle_config.agi_penaly_type > 0)
    {
        if (target_count >= battle_config.agi_penaly_count)
        {                       //ペナルティ設定より対象が多い
            if (battle_config.agi_penaly_type == 1) //回避率がagi_penaly_num%ずつ減少
                flee =
                    (flee *
                     (100 -
                      (target_count -
                       (battle_config.agi_penaly_count -
                        1)) * battle_config.agi_penaly_num)) / 100;
            else if (battle_config.agi_penaly_type == 2)    //回避率がagi_penaly_num分減少
                flee -=
                    (target_count -
                     (battle_config.agi_penaly_count -
                      1)) * battle_config.agi_penaly_num;
            if (flee < 1)
                flee = 1;       //回避率は最低でも1
        }
    }
    hitrate = battle_get_hit(src) - flee + 80; //命中率計算

    {                           // [fate] Reduce hit chance by distance
        int dx = abs(src->bl_x - target->bl_x);
        int dy = abs(src->bl_y - target->bl_y);
        int malus_dist;

        target_distance = std::max(dx, dy);
        malus_dist =
            std::max(0, target_distance - (skill_power(sd, SkillID::AC_OWL) / 75));
        hitrate -= (malus_dist * (malus_dist + 1));
    }

    dex = battle_get_dex(src); //DEX
    watk = battle_get_atk(src);    //ATK

    type = DamageType::NORMAL;
    div_ = 1;                   // single attack

    {
        damage = battle_get_baseatk(sd);    //damega,damega2初登場、base_atkの取得
    }
    if (sd->attackrange > 2)
    {                           // [fate] ranged weapon?
        const int range_damage_bonus = 80;  // up to 31.25% bonus for long-range hit
        damage =
            damage * (256 +
                      ((range_damage_bonus * target_distance) /
                       sd->attackrange)) >> 8;
    }

    atkmin = dex;     //最低ATKはDEXで初期化？
    sd->state.arrow_atk = 0;    //arrow_atk初期化

    IOff0 widx = sd->equip_index_maybe[EQUIP::WEAPON];

    if (widx.ok())
    {
        OMATCH_BEGIN_SOME (sdidw, sd->inventory_data[widx])
        {
            atkmin = atkmin * (80 + sdidw->wlv * 20) / 100;
        }
        OMATCH_END ();
    }
    if (sd->status.weapon == ItemLook::BOW)
    {                           //武器が弓矢の場合
        atkmin = watk * ((atkmin < watk) ? atkmin : watk) / 100;    //弓用最低ATK計算
        flag = (flag & ~BF::RANGEMASK) | BF::LONG;    //遠距離攻撃フラグを有効
        sd->state.arrow_atk = 1;    //arrow_atk有効化
    }

    {
        atkmax = watk;
    }

    if (atkmin > atkmax && !(sd->state.arrow_atk))
        atkmin = atkmax;        //弓は最低が上回る場合あり

    if (sd->double_rate > 0 && skill_num == SkillID::ZERO && skill_lv >= 0)
        da = random_::chance({sd->double_rate, 100});

    if (!da)
    {                           //ダブルアタックが発動していない
        // クリティカル計算
        cri = battle_get_critical(src);

        if (sd->state.arrow_atk)
            cri += sd->arrow_cri;
        cri -= battle_get_luk(target) * 3;
        if (ac_flag)
            cri = 1000;
    }

    if (tsd && tsd->critical_def)
        cri = cri * (100 - tsd->critical_def) / 100;

    //ダブルアタックが発動していない
    // 判定（スキルの場合は無視）
    if (!da && skill_num == SkillID::ZERO && skill_lv >= 0
        && random_::chance({cri, 1000}))
    {
        damage += atkmax;
        if (sd->atk_rate != 100)
        {
            damage = (damage * sd->atk_rate) / 100;
        }
        if (sd->state.arrow_atk)
            damage += sd->arrow_atk;
        type = DamageType::CRITICAL;
    }
    else
    {
        int vitbonusmax;

        if (atkmax > atkmin)
            damage += random_::in(atkmin, atkmax);
        else
            damage += atkmin;
        if (sd->atk_rate != 100)
        {
            damage = (damage * sd->atk_rate) / 100;
        }

        if (sd->state.arrow_atk)
        {
            if (sd->arrow_atk > 0)
                damage += random_::in(0, sd->arrow_atk);
            hitrate += sd->arrow_hit;
        }

        if (skill_num != SkillID::ZERO && skill_num != SkillID::NEGATIVE)
        {
            flag = (flag & ~BF::SKILLMASK) | BF::SKILL;
        }

        {
            // 対 象の防御力によるダメージの減少
            // ディバインプロテクション（ここでいいのかな？）
            if (def1 < 1000000)
            {                   //DEF, VIT無視
                int t_def;
                target_count =
                    1 + battle_counttargeted(target, src,
                            battle_config.vit_penaly_count_lv);
                if (battle_config.vit_penaly_type > 0)
                {
                    if (target_count >= battle_config.vit_penaly_count)
                    {
                        if (battle_config.vit_penaly_type == 1)
                        {
                            def1 =
                                (def1 *
                                 (100 -
                                  (target_count -
                                   (battle_config.vit_penaly_count -
                                    1)) * battle_config.vit_penaly_num)) /
                                100;
                            def2 =
                                (def2 *
                                 (100 -
                                  (target_count -
                                   (battle_config.vit_penaly_count -
                                    1)) * battle_config.vit_penaly_num)) /
                                100;
                            t_vit =
                                (t_vit *
                                 (100 -
                                  (target_count -
                                   (battle_config.vit_penaly_count -
                                    1)) * battle_config.vit_penaly_num)) /
                                100;
                        }
                        else if (battle_config.vit_penaly_type == 2)
                        {
                            def1 -=
                                (target_count -
                                 (battle_config.vit_penaly_count -
                                  1)) * battle_config.vit_penaly_num;
                            def2 -=
                                (target_count -
                                 (battle_config.vit_penaly_count -
                                  1)) * battle_config.vit_penaly_num;
                            t_vit -=
                                (target_count -
                                 (battle_config.vit_penaly_count -
                                  1)) * battle_config.vit_penaly_num;
                        }
                        if (def1 < 0)
                            def1 = 0;
                        if (def2 < 1)
                            def2 = 1;
                        if (t_vit < 1)
                            t_vit = 1;
                    }
                }
                t_def = def2 * 8 / 10;
                vitbonusmax = (t_vit / 20) * (t_vit / 20) - 1;

                {
                    {
                        damage = damage * (100 - def1) / 100;
                        damage -= t_def;
                        if (vitbonusmax > 0)
                            damage -= random_::in(0, vitbonusmax);
                    }
                }
            }
        }
    }
    // 精錬ダメージの追加
    {                           //DEF, VIT無視
        damage += battle_get_atk2(src);
    }

    // 0未満だった場合1に補正
    if (damage < 1)
        damage = 1;

    // スキル修正２（修練系）
    // 修練ダメージ(右手のみ) ソニックブロー時は別処理（1撃に付き1/8適応)
    {                           //修練ダメージ無視
    }

    if (sd->perfect_hit > 0)
    {
        if (random_::chance({sd->perfect_hit, 100}))
            hitrate = 1000000;
    }

    // 回避修正
    hitrate = (hitrate < 5) ? 5 : hitrate;
    if (type == DamageType::NORMAL && !random_::chance({hitrate, 100}))
    {
        damage = 0;
        dmg_lv = ATK::FLEE;
    }
    else
    {
        dmg_lv = ATK::DEF;
    }

    if (damage < 0)
        damage = 0;

    // 右手,短剣のみ
    if (da)
    {                           //ダブルアタックが発動しているか
        div_ = 2;
        damage += damage;
        type = DamageType::DOUBLED;
    }

    // 完全回避の判定
    if (skill_num == SkillID::ZERO && skill_lv >= 0 && tsd != nullptr && div_ < 255
        && random_::chance({battle_get_flee2(target), 1000}))
    {
        damage = 0;
        type = DamageType::FLEE2;
        dmg_lv = ATK::LUCKY;
    }

    // 対象が完全回避をする設定がONなら
    if (battle_config.enemy_perfect_flee)
    {
        if (skill_num == SkillID::ZERO && skill_lv >= 0 && tmd != nullptr && div_ < 255
            && random_::chance({battle_get_flee2(target), 1000}))
        {
            damage = 0;
            type = DamageType::FLEE2;
            dmg_lv = ATK::LUCKY;
        }
    }

    //MobのModeに頑強フラグが立っているときの処理
    if (bool(t_mode & MobMode::PLANT))
    {
        if (damage > 0)
            damage = 1;
    }

    if (damage > 0)
    {
        {
            damage =
                battle_calc_damage(src, target, damage, div_, skill_num,
                                    skill_lv, flag);
        }
    }

    wd.damage = damage;
    wd.type = type;
    wd.div_ = div_;
    wd.amotion = battle_get_amotion(src);
    wd.dmotion = battle_get_dmotion(target);
    wd.flag = flag;
    wd.dmg_lv = dmg_lv;

    return wd;
}

/*==========================================
 * 武器ダメージ計算
 *------------------------------------------
 */
static
struct Damage battle_calc_weapon_attack(dumb_ptr<block_list> src,
                                         dumb_ptr<block_list> target,
                                         SkillID skill_num, int skill_lv,
                                         int wflag)
{
    struct Damage wd {};

    nullpo_retr(wd, src);
    nullpo_retr(wd, target);

    if (src->bl_type == BL::PC)
        wd = battle_calc_pc_weapon_attack(src, target, skill_num, skill_lv, wflag);    // weapon breaking [Valaris]
    else if (src->bl_type == BL::MOB)
        wd = battle_calc_mob_weapon_attack(src, target, skill_num, skill_lv, wflag);

    return wd;
}

/*==========================================
 * 魔法ダメージ計算
 *------------------------------------------
 */
static
struct Damage battle_calc_magic_attack(dumb_ptr<block_list> bl,
                                        dumb_ptr<block_list> target,
                                        SkillID skill_num, int skill_lv, int)
{
    int mdef1 = battle_get_mdef(target);
    int mdef2 = battle_get_mdef2(target);
    int matk1, matk2, damage = 0, div_ = 1;
    struct Damage md {};
    int normalmagic_flag = 1;
    dumb_ptr<map_session_data> sd = nullptr;

    nullpo_retr(md, bl);
    nullpo_retr(md, target);

    matk1 = battle_get_matk1(bl);
    matk2 = battle_get_matk2(bl);
    MobMode t_mode = battle_get_mode(target);

    if (bl->bl_type == BL::PC)
    {
        sd = bl->is_player();
        sd->state.attack_type = BF::MAGIC;
        if (sd->matk_rate != 100)
        {
            matk1 = matk1 * sd->matk_rate / 100;
            matk2 = matk2 * sd->matk_rate / 100;
        }
        sd->state.arrow_atk = 0;
    }

    BF aflag = BF::MAGIC | BF::LONG | BF::SKILL;

    if (normalmagic_flag)
    {
        // 一般魔法ダメージ計算
        if (matk1 > matk2)
            damage = random_::in(matk2, matk1);
        else
            damage = matk2;

        {
            {
                damage = (damage * (100 - mdef1)) / 100;
                damage -= mdef2;
            }
        }

        if (damage < 1)
            damage = 1;
    }

    if (damage < 0)
        damage = 0;

    div_ = skill_get_num(skill_num, skill_lv);

    if (div_ > 1)
        damage *= div_;

//  if(mdef1 >= 1000000 && damage > 0)
    if (bool(t_mode & MobMode::PLANT) && damage > 0)
        damage = 1;

    damage = battle_calc_damage(bl, target, damage, div_, skill_num, skill_lv, aflag); // 最終修正

    md.damage = damage;
    md.div_ = div_;
    md.amotion = battle_get_amotion(bl);
    md.dmotion = battle_get_dmotion(target);
    md.type = DamageType::NORMAL;
    md.flag = aflag;

    return md;
}

/*==========================================
 * その他ダメージ計算
 *------------------------------------------
 */
static
struct Damage battle_calc_misc_attack(dumb_ptr<block_list> bl,
                                       dumb_ptr<block_list> target,
                                       SkillID skill_num, int skill_lv, int)
{
    dumb_ptr<map_session_data> sd = nullptr;
    int damage = 0, div_ = 1;
    struct Damage md {};
    int damagefix = 1;

    BF aflag = BF::MISC | BF::LONG | BF::SKILL;

    nullpo_retr(md, bl);
    nullpo_retr(md, target);

    if (bl->bl_type == BL::PC)
    {
        sd = bl->is_player();
        sd->state.attack_type = BF::MISC;
        sd->state.arrow_atk = 0;
    }

    switch (skill_num)
    {
        case SkillID::NPC_SELFDESTRUCTION:  // 自爆
            damage = battle_get_hp(bl) - (bl == target ? 1 : 0);
            damagefix = 0;
            break;
    }

    if (damagefix)
    {
        if (damage < 1)
            damage = 1;
    }

    div_ = skill_get_num(skill_num, skill_lv);
    if (div_ > 1)
        damage *= div_;

    if (damage > 0
        && (damage < div_
            || (battle_get_def(target) >= 1000000
                && battle_get_mdef(target) >= 1000000)))
    {
        damage = div_;
    }

    damage = battle_calc_damage(bl, target, damage, div_, skill_num, skill_lv, aflag); // 最終修正

    md.damage = damage;
    md.div_ = div_;
    md.amotion = battle_get_amotion(bl);
    md.dmotion = battle_get_dmotion(target);
    md.type = DamageType::NORMAL;
    md.flag = aflag;
    return md;

}

/*==========================================
 * ダメージ計算一括処理用
 *------------------------------------------
 */
struct Damage battle_calc_attack(BF attack_type,
                                  dumb_ptr<block_list> bl,
                                  dumb_ptr<block_list> target, SkillID skill_num,
                                  int skill_lv, int flag)
{
    struct Damage d {};

    switch (attack_type)
    {
        case BF::WEAPON:
            return battle_calc_weapon_attack(bl, target, skill_num, skill_lv,
                                              flag);
        case BF::MAGIC:
            return battle_calc_magic_attack(bl, target, skill_num, skill_lv,
                                             flag);
        case BF::MISC:
            return battle_calc_misc_attack(bl, target, skill_num, skill_lv,
                                            flag);
        default:
            if (battle_config.error_log)
                PRINTF("battle_calc_attack: unknwon attack type ! %d\n"_fmt,
                        attack_type);
            break;
    }
    return d;
}

/*==========================================
 * 通常攻撃処理まとめ
 *------------------------------------------
 */
ATK battle_weapon_attack(dumb_ptr<block_list> src, dumb_ptr<block_list> target,
        tick_t tick)
{
    dumb_ptr<map_session_data> sd = nullptr;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> t_sc_data = battle_get_sc_data(target);
    struct Damage wd;

    nullpo_retr(ATK::ZERO, src);
    nullpo_retr(ATK::ZERO, target);

    if (src->bl_type == BL::PC)
        sd = src->is_player();

    if (src->bl_prev == nullptr || target->bl_prev == nullptr)
        return ATK::ZERO;
    if (src->bl_type == BL::PC && pc_isdead(sd))
        return ATK::ZERO;
    if (target->bl_type == BL::PC
        && pc_isdead(target->is_player()))
        return ATK::ZERO;

    Opt1 *opt1 = battle_get_opt1(src);
    if (opt1 != nullptr && bool(*opt1))
    {
        battle_stopattack(src);
        return ATK::ZERO;
    }

    if (battle_check_target(src, target, BCT_ENEMY) > 0 &&
        battle_check_range(src, target, 0))
    {
        // 攻撃対象となりうるので攻撃
        if (sd && sd->status.weapon == ItemLook::BOW)
        {
            IOff0 aidx = sd->equip_index_maybe[EQUIP::ARROW];
            if (aidx.ok())
            {
                if (battle_config.arrow_decrement)
                    pc_delitem(sd, aidx, 1, 0);
            }
            else
            {
                clif_arrow_fail(sd, 0);
                return ATK::ZERO;
            }
        }
        wd = battle_calc_weapon_attack(src, target, SkillID::ZERO, 0, 0);

        // significantly increase injuries for hasted characters
        if (wd.damage > 0 && t_sc_data[StatusChange::SC_HASTE].timer)
        {
            wd.damage = (wd.damage * (16 + t_sc_data[StatusChange::SC_HASTE].val1)) >> 4;
        }

        if (wd.damage > 0
            && t_sc_data[StatusChange::SC_PHYS_SHIELD].timer
            && target->bl_type == BL::PC)
        {
            int reduction = t_sc_data[StatusChange::SC_PHYS_SHIELD].val1;
            if (reduction > wd.damage)
                reduction = wd.damage;

            wd.damage -= reduction;
            MAP_LOG_PC(target->is_player(),
                    "MAGIC-ABSORB-DMG %d"_fmt, reduction);
        }

        {
            clif_damage(src, target, tick, wd.amotion, wd.dmotion,
                         wd.damage, wd.div_, wd.type);
        }

        MapBlockLock lock;

        if (src->bl_type == BL::PC)
        {
            IOff0 weapon_index = sd->equip_index_maybe[EQUIP::WEAPON];
            ItemNameId weapon;
            if (weapon_index.ok())
            {
                OMATCH_BEGIN_SOME (sdidw, sd->inventory_data[weapon_index])
                {
                    if (bool(sd->status.inventory[weapon_index].equip & EPOS::WEAPON))
                    {
                        weapon = sdidw->nameid;
                    }
                }
                OMATCH_END ();
            }

            MAP_LOG("PC%d %s:%d,%d WPNDMG %s%d %d FOR %d WPN %d"_fmt,
                    sd->status_key.char_id, src->bl_m->name_, src->bl_x, src->bl_y,
                    (target->bl_type == BL::PC) ? "PC"_s : "MOB"_s,
                    (target->bl_type == BL::PC)
                    ? unwrap<CharId>(target->is_player()->status_key.char_id)
                    : unwrap<BlockId>(target->bl_id),
                    battle_get_class(target),
                    wd.damage, weapon);
        }

        if (target->bl_type == BL::PC)
        {
            dumb_ptr<map_session_data> sd2 = target->is_player();
            MAP_LOG("PC%d %s:%d,%d WPNINJURY %s%d %d FOR %d"_fmt,
                    sd2->status_key.char_id, target->bl_m->name_, target->bl_x, target->bl_y,
                    (src->bl_type == BL::PC) ? "PC"_s : "MOB"_s,
                    (src->bl_type == BL::PC)
                    ? unwrap<CharId>(src->is_player()->status_key.char_id)
                    : unwrap<BlockId>(src->bl_id),
                    battle_get_class(src),
                    wd.damage);
        }

        battle_damage(src, target, (wd.damage), 0);
        if (target->bl_prev != nullptr &&
            (target->bl_type != BL::PC
             || (target->bl_type == BL::PC
                 && !pc_isdead(target->is_player()))))
        {
            if (wd.damage > 0)
            {
                skill_additional_effect(src, target, SkillID::ZERO, 0);
            }
        }
        if (sd)
        {
            if (bool(wd.flag & BF::WEAPON)
                && src != target
                && (wd.damage > 0))
            {
                int hp = 0, sp = 0;
                if (sd->hp_drain_rate && wd.damage > 0
                    && random_::chance({sd->hp_drain_rate, 100}))
                {
                    hp += (wd.damage * sd->hp_drain_per) / 100;
                }
                if (sd->sp_drain_rate && wd.damage > 0
                    && random_::chance({sd->sp_drain_rate, 100}))
                {
                    sp += (wd.damage * sd->sp_drain_per) / 100;
                }
                if (hp || sp)
                    pc_heal(sd, hp, sp);
            }
        }
    }
    return wd.dmg_lv;
}

bool battle_check_undead(Race race, Element element)
{
    if (battle_config.undead_detect_type == 0)
    {
        return element == Element::undead;
    }
    else if (battle_config.undead_detect_type == 1)
    {
        return race == Race::undead;
    }
    else
    {
        return element == Element::undead || race == Race::undead;
    }
}

/*==========================================
 * 敵味方判定(1=肯定,0=否定,-1=エラー)
 * flag&0xf0000 = 0x00000:敵じゃないか判定（ret:1＝敵ではない）
 *                              = 0x10000:パーティー判定（ret:1=パーティーメンバ)
 *                              = 0x20000:全て(ret:1=敵味方両方)
 *                              = 0x40000:敵か判定(ret:1=敵)
 *                              = 0x50000:パーティーじゃないか判定(ret:1=パーティでない)
 *------------------------------------------
 */
int battle_check_target(dumb_ptr<block_list> src, dumb_ptr<block_list> target,
        BCT flag)
{
    PartyId s_p, t_p;
    dumb_ptr<block_list> ss = src;

    nullpo_retz(src);
    nullpo_retz(target);

    if (flag & BCT_ENEMY)
    {                           // 反転フラグ
        int ret = battle_check_target(src, target, flag & (BCT_PARTY | BCT_ALL));
        if (ret != -1)
            return !ret;
        return -1;
    }

    if (flag & BCT_ALL)
    {
        if (target->bl_type == BL::MOB || target->bl_type == BL::PC)
            return 1;
        else
            return -1;
    }

    if (target->bl_type == BL::PC
        && target->is_player()->invincible_timer)
        return -1;

    // Mobでmaster_idがあってspecial_mob_aiなら、召喚主を求める
    if (src->bl_type == BL::MOB)
    {
        dumb_ptr<mob_data> md = src->is_mob();
        if (md && md->master_id)
        {
            if (md->master_id == target->bl_id)    // 主なら肯定
                return 1;
            if (md->state.special_mob_ai)
            {
                if (target->bl_type == BL::MOB)
                {               //special_mob_aiで対象がMob
                    dumb_ptr<mob_data> tmd = target->is_mob();
                    if (tmd)
                    {
                        if (tmd->master_id != md->master_id)    //召喚主が一緒でなければ否定
                            return 0;
                        else
                        {       //召喚主が一緒なので肯定したいけど自爆は否定
                            if (md->state.special_mob_ai > 2)
                                return 0;
                            else
                                return 1;
                        }
                    }
                }
            }
            if ((ss = map_id2bl(md->master_id)) == nullptr)
                return -1;
        }
    }

    if (src == target || ss == target)  // 同じなら肯定
        return 1;

    if (target->bl_type == BL::PC
        && pc_isinvisible(target->is_player()))
        return -1;

    if (src->bl_prev == nullptr ||    // 死んでるならエラー
        (src->bl_type == BL::PC && pc_isdead(src->is_player())))
        return -1;

    if ((ss->bl_type == BL::PC && target->bl_type == BL::MOB) ||
        (ss->bl_type == BL::MOB && target->bl_type == BL::PC))
        return 0;               // PCvsMOBなら否定

    s_p = battle_get_party_id(ss);

    t_p = battle_get_party_id(target);

    if (flag & BCT_PARTY)
    {
        if (s_p && t_p && s_p == t_p)   // 同じパーティなら肯定（味方）
            return 1;
        else                    // パーティ検索なら同じパーティじゃない時点で否定
            return 0;
    }

    if (ss->bl_type == BL::PC && target->bl_type == BL::PC)
    {                           // 両方PVPモードなら否定（敵）
        if (ss->bl_m->flag.get(MapFlag::PVP)
            || pc_iskiller(ss->is_player(), target->is_player()))
        {                       // [MouseJstr]
            if (battle_config.pk_mode)
                return 1;       // prevent novice engagement in pk_mode [Valaris]
            else if (ss->bl_m->flag.get(MapFlag::PVP_NOPARTY) && s_p && t_p
                     && s_p == t_p)
                return 1;
            return 0;
        }
    }

    return 1;                   // 該当しないので無関係人物（まあ敵じゃないので味方）
}

/*==========================================
 * 射程判定
 *------------------------------------------
 */
int battle_check_range(dumb_ptr<block_list> src, dumb_ptr<block_list> bl,
                        int range)
{

    int dx, dy, rangex, rangey;
    struct walkpath_data wpd;
    int arange;

    nullpo_retz(src);
    nullpo_retz(bl);

    dx = (bl->bl_x - src->bl_x);
    dy = (bl->bl_y - src->bl_y);
    rangex = abs(dx);
    rangey = abs(dy);
    arange = ((rangex > rangey) ? rangex : rangey);

    if (src->bl_m != bl->bl_m)        // 違うマップ
        return 0;

    if (range > 0 && range < arange)    // 遠すぎる
        return 0;

    if (arange < 2)             // 同じマスか隣接
        return 1;

//  if(bl->bl_type == BL_SKILL && ((struct skill_unit *)bl)->group->unit_id == 0x8d)
//      return 1;

    // 障害物判定
    wpd.path_len = 0;
    wpd.path_pos = 0;
    wpd.path_half = 0;
    if (path_search(&wpd, src->bl_m, src->bl_x, src->bl_y, bl->bl_x, bl->bl_y, 0x10001) !=
        -1)
        return 1;

    dx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    dy = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
    return (path_search(&wpd, src->bl_m, src->bl_x + dx, src->bl_y + dy,
                         bl->bl_x - dx, bl->bl_y - dy, 0x10001) != -1) ? 1 : 0;
}
} // namespace map
} // namespace tmwa
