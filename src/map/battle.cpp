#include "battle.hpp"

#include <cstring>

#include <fstream>

#include "../strings/fstring.hpp"
#include "../strings/tstring.hpp"
#include "../strings/sstring.hpp"
#include "../strings/zstring.hpp"

#include "../common/cxxstdio.hpp"
#include "../common/io.hpp"
#include "../common/nullpo.hpp"
#include "../common/random.hpp"

#include "clif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "path.hpp"
#include "pc.hpp"
#include "skill.hpp"

#include "../poison.hpp"

struct Battle_Config battle_config;

/*==========================================
 * 自分をロックしている対象の数を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
static
int battle_counttargeted(dumb_ptr<block_list> bl, dumb_ptr<block_list> src,
        ATK target_lv)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::PC)
        return pc_counttargeted(bl->as_player(), src,
                                 target_lv);
    else if (bl->bl_type == BL::MOB)
        return mob_counttargeted(bl->as_mob(), src, target_lv);
    return 0;
}

/*==========================================
 * 対象のClassを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_class(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::MOB)
        return bl->as_mob()->mob_class;
    else if (bl->bl_type == BL::PC)
        return 0;
    else
        return 0;
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
        return bl->as_mob()->dir;
    else if (bl->bl_type == BL::PC)
        return bl->as_player()->dir;
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
    nullpo_ret(bl);
    if (bl->bl_type == BL::MOB)
        return bl->as_mob()->stats[mob_stat::LV];
    else if (bl->bl_type == BL::PC)
        return bl->as_player()->status.base_level;
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
    nullpo_ret(bl);
    if (bl->bl_type == BL::MOB)
        return mob_db[bl->as_mob()->mob_class].range;
    else if (bl->bl_type == BL::PC)
        return bl->as_player()->attackrange;
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
        return bl->as_mob()->hp;
    else if (bl->bl_type == BL::PC)
        return bl->as_player()->status.hp;
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
        return bl->as_player()->status.max_hp;
    else
    {
        int max_hp = 1;
        if (bl->bl_type == BL::MOB)
        {
            max_hp = bl->as_mob()->stats[mob_stat::MAX_HP];
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

/*==========================================
 * 対象のStrを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_str(dumb_ptr<block_list> bl)
{
    int str = 0;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        str = bl->as_mob()->stats[mob_stat::STR];
    else if (bl->bl_type == BL::PC)
        return bl->as_player()->paramc[ATTR::STR];

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
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        agi = bl->as_mob()->stats[mob_stat::AGI];
    else if (bl->bl_type == BL::PC)
        agi = bl->as_player()->paramc[ATTR::AGI];

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
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        vit = bl->as_mob()->stats[mob_stat::VIT];
    else if (bl->bl_type == BL::PC)
        vit = bl->as_player()->paramc[ATTR::VIT];

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
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        int_ = bl->as_mob()->stats[mob_stat::INT];
    else if (bl->bl_type == BL::PC)
        int_ = bl->as_player()->paramc[ATTR::INT];

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
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        dex = bl->as_mob()->stats[mob_stat::DEX];
    else if (bl->bl_type == BL::PC)
        dex = bl->as_player()->paramc[ATTR::DEX];

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
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
        luk = bl->as_mob()->stats[mob_stat::LUK];
    else if (bl->bl_type == BL::PC)
        luk = bl->as_player()->paramc[ATTR::LUK];

    if (luk < 0)
        luk = 0;
    return luk;
}

/*==========================================
 * 対象のFleeを返す(汎用)
 * 戻りは整数で1以上
 *------------------------------------------
 */
static
int battle_get_flee(dumb_ptr<block_list> bl)
{
    int flee = 1;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        flee = bl->as_player()->flee;
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
static
int battle_get_hit(dumb_ptr<block_list> bl)
{
    int hit = 1;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        hit = bl->as_player()->hit;
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
static
int battle_get_flee2(dumb_ptr<block_list> bl)
{
    int flee2 = 1;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = bl->as_player();
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
static
int battle_get_critical(dumb_ptr<block_list> bl)
{
    int critical = 1;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = bl->as_player();
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
static
int battle_get_baseatk(dumb_ptr<block_list> bl)
{
    eptr<struct status_change, StatusChange> sc_data;
    int batk = 1;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        batk = bl->as_player()->base_atk;  //設定されているbase_atk
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
static
int battle_get_atk(dumb_ptr<block_list> bl)
{
    eptr<struct status_change, StatusChange> sc_data;
    int atk = 0;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        atk = bl->as_player()->watk;
    else if (bl->bl_type == BL::MOB)
        atk = bl->as_mob()->stats[mob_stat::ATK1];

    if (atk < 0)
        atk = 0;
    return atk;
}

/*==========================================
 * 対象の左手Atkを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
static
int battle_get_atk_(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::PC)
        return bl->as_player()->watk_;
    else
        return 0;
}

/*==========================================
 * 対象のAtk2を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
static
int battle_get_atk2(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::PC)
        return bl->as_player()->watk2;
    else
    {
        int atk2 = 0;
        if (bl->bl_type == BL::MOB)
            atk2 = bl->as_mob()->stats[mob_stat::ATK2];

        if (atk2 < 0)
            atk2 = 0;
        return atk2;
    }
}

/*==========================================
 * 対象の左手Atk2を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
static
int battle_get_atk_2(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::PC)
        return bl->as_player()->watk_2;
    else
        return 0;
}

/*==========================================
 * 対象のMAtk1を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
static
int battle_get_matk1(dumb_ptr<block_list> bl)
{
    eptr<struct status_change, StatusChange> sc_data;
    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::MOB)
    {
        int matk, int_ = battle_get_int(bl);
        matk = int_ + (int_ / 5) * (int_ / 5);

        return matk;
    }
    else if (bl->bl_type == BL::PC)
        return bl->as_player()->matk1;
    else
        return 0;
}

/*==========================================
 * 対象のMAtk2を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
static
int battle_get_matk2(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::MOB)
    {
        int matk, int_ = battle_get_int(bl);
        matk = int_ + (int_ / 7) * (int_ / 7);

        return matk;
    }
    else if (bl->bl_type == BL::PC)
        return bl->as_player()->matk2;
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
    eptr<struct status_change, StatusChange> sc_data;
    int def = 0;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
    {
        def = bl->as_player()->def;
    }
    else if (bl->bl_type == BL::MOB)
    {
        def = bl->as_mob()->stats[mob_stat::DEF];
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
    eptr<struct status_change, StatusChange> sc_data;
    int mdef = 0;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        mdef = bl->as_player()->mdef;
    else if (bl->bl_type == BL::MOB)
        mdef = bl->as_mob()->stats[mob_stat::MDEF];

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
    eptr<struct status_change, StatusChange> sc_data;
    int def2 = 1;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->bl_type == BL::PC)
        def2 = bl->as_player()->def2;
    else if (bl->bl_type == BL::MOB)
        def2 = bl->as_mob()->stats[mob_stat::VIT];

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

    nullpo_ret(bl);
    if (bl->bl_type == BL::MOB)
    {
        dumb_ptr<mob_data> md = bl->as_mob();
        mdef2 = md->stats[mob_stat::INT] + (md->stats[mob_stat::VIT] >> 1);
    }
    else if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = bl->as_player();
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
    nullpo_retr(std::chrono::seconds(1), bl);
    if (bl->bl_type == BL::PC)
        return bl->as_player()->speed;
    else
    {
        interval_t speed = std::chrono::seconds(1);
        if (bl->bl_type == BL::MOB)
            speed = static_cast<interval_t>(bl->as_mob()->stats[mob_stat::SPEED]);

        return std::max(speed, std::chrono::milliseconds(1));
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
    nullpo_retr(std::chrono::seconds(4), bl);
    if (bl->bl_type == BL::PC)
        return bl->as_player()->aspd * 2;
    else
    {
        eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
        interval_t adelay = std::chrono::seconds(4);
        int aspd_rate = 100;
        if (bl->bl_type == BL::MOB)
            adelay = static_cast<interval_t>(bl->as_mob()->stats[mob_stat::ADELAY]);

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
        return std::max(adelay, static_cast<interval_t>(battle_config.monster_max_aspd) * 2);
    }
}

interval_t battle_get_amotion(dumb_ptr<block_list> bl)
{
    nullpo_retr(std::chrono::seconds(2), bl);
    if (bl->bl_type == BL::PC)
        return bl->as_player()->amotion;
    else
    {
        eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
        interval_t amotion = std::chrono::seconds(2);
        int aspd_rate = 100;
        if (bl->bl_type == BL::MOB)
            amotion = static_cast<interval_t>(mob_db[bl->as_mob()->mob_class].amotion);

        if (sc_data)
        {
            if (sc_data[StatusChange::SC_SPEEDPOTION0].timer)
                aspd_rate -= sc_data[StatusChange::SC_SPEEDPOTION0].val1;
            if (sc_data[StatusChange::SC_HASTE].timer)
                aspd_rate -= sc_data[StatusChange::SC_HASTE].val1;
        }

        if (aspd_rate != 100)
            amotion = amotion * aspd_rate / 100;
        return std::max(amotion, static_cast<interval_t>(battle_config.monster_max_aspd));
    }
}

interval_t battle_get_dmotion(dumb_ptr<block_list> bl)
{
    nullpo_retr(interval_t::zero(), bl);
    if (bl->bl_type == BL::MOB)
    {
        return static_cast<interval_t>(mob_db[bl->as_mob()->mob_class].dmotion);
    }
    else if (bl->bl_type == BL::PC)
    {
        return bl->as_player()->dmotion;
    }
    else
        return std::chrono::seconds(2);
}

LevelElement battle_get_element(dumb_ptr<block_list> bl)
{
    LevelElement ret = {2, Element::neutral};

    nullpo_retr(ret, bl);
    if (bl->bl_type == BL::MOB)   // 10の位＝Lv*2、１の位＝属性
        ret = bl->as_mob()->def_ele;

    return ret;
}

int battle_get_party_id(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::PC)
        return bl->as_player()->status.party_id;
    else if (bl->bl_type == BL::MOB)
    {
        dumb_ptr<mob_data> md = bl->as_mob();
        if (md->master_id > 0)
            return -md->master_id;
        return -md->bl_id;
    }
    return 0;
}

Race battle_get_race(dumb_ptr<block_list> bl)
{
    nullpo_retr(Race::formless, bl);
    if (bl->bl_type == BL::MOB)
        return mob_db[bl->as_mob()->mob_class].race;
    else if (bl->bl_type == BL::PC)
        return Race::demihuman;
    else
        return Race::formless;
}

MobMode battle_get_mode(dumb_ptr<block_list> bl)
{
    nullpo_retr(MobMode::CAN_MOVE, bl);
    if (bl->bl_type == BL::MOB)
        return mob_db[bl->as_mob()->mob_class].mode;
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
eptr<struct status_change, StatusChange> battle_get_sc_data(dumb_ptr<block_list> bl)
{
    nullpo_retr(nullptr, bl);

    switch (bl->bl_type)
    {
    case BL::MOB:
        return bl->as_mob()->sc_data;
    case BL::PC:
        return bl->as_player()->sc_data;
    }
    return nullptr;
}

short *battle_get_sc_count(dumb_ptr<block_list> bl)
{
    nullpo_retr(NULL, bl);
    if (bl->bl_type == BL::MOB)
        return &bl->as_mob()->sc_count;
    else if (bl->bl_type == BL::PC)
        return &bl->as_player()->sc_count;
    return NULL;
}

Opt1 *battle_get_opt1(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::MOB)
        return &bl->as_mob()->opt1;
    else if (bl->bl_type == BL::PC)
        return &bl->as_player()->opt1;
    else if (bl->bl_type == BL::NPC)
        return &bl->as_npc()->opt1;
    return 0;
}

Opt2 *battle_get_opt2(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::MOB)
        return &bl->as_mob()->opt2;
    else if (bl->bl_type == BL::PC)
        return &bl->as_player()->opt2;
    else if (bl->bl_type == BL::NPC)
        return &bl->as_npc()->opt2;
    return 0;
}

Opt3 *battle_get_opt3(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::MOB)
        return &bl->as_mob()->opt3;
    else if (bl->bl_type == BL::PC)
        return &bl->as_player()->opt3;
    else if (bl->bl_type == BL::NPC)
        return &bl->as_npc()->opt3;
    return 0;
}

Option *battle_get_option(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::MOB)
        return &bl->as_mob()->option;
    else if (bl->bl_type == BL::PC)
        return &bl->as_player()->status.option;
    else if (bl->bl_type == BL::NPC)
        return &bl->as_npc()->option;
    return 0;
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
    nullpo_ret(target);    //blはNULLで呼ばれることがあるので他でチェック

    if (damage == 0)
        return 0;

    if (target->bl_prev == NULL)
        return 0;

    if (bl)
    {
        if (bl->bl_prev == NULL)
            return 0;
    }

    if (damage < 0)
        return battle_heal(bl, target, -damage, 0, flag);

    if (target->bl_type == BL::MOB)
    {                           // MOB
        dumb_ptr<mob_data> md = target->as_mob();
        if (md && md->skilltimer && md->state.skillcastcancel)    // 詠唱妨害
            skill_castcancel(target, 0);
        return mob_damage(bl, md, damage, 0);
    }
    else if (target->bl_type == BL::PC)
    {                           // PC

        dumb_ptr<map_session_data> tsd = target->as_player();

        return pc_damage(bl, tsd, damage);

    }
    return 0;
}

int battle_heal(dumb_ptr<block_list> bl, dumb_ptr<block_list> target, int hp,
                 int sp, int flag)
{
    nullpo_ret(target);    //blはNULLで呼ばれることがあるので他でチェック

    if (target->bl_type == BL::PC
        && pc_isdead(target->as_player()))
        return 0;
    if (hp == 0 && sp == 0)
        return 0;

    if (hp < 0)
        return battle_damage(bl, target, -hp, flag);

    if (target->bl_type == BL::MOB)
        return mob_heal(target->as_mob(), hp);
    else if (target->bl_type == BL::PC)
        return pc_heal(target->as_player(), hp, sp);
    return 0;
}

// 攻撃停止
int battle_stopattack(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::MOB)
        return mob_stopattack(bl->as_mob());
    else if (bl->bl_type == BL::PC)
        return pc_stopattack(bl->as_player());
    return 0;
}

// 移動停止
int battle_stopwalking(dumb_ptr<block_list> bl, int type)
{
    nullpo_ret(bl);
    if (bl->bl_type == BL::MOB)
        return mob_stop_walking(bl->as_mob(), type);
    else if (bl->bl_type == BL::PC)
        return pc_stop_walking(bl->as_player(), type);
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
    dumb_ptr<mob_data> md = NULL;

    nullpo_ret(bl);

    if (bl->bl_type == BL::MOB)
        md = bl->as_mob();

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

    if (md != NULL && md->hp > 0 && damage > 0) // 反撃などのMOBスキル判定
        mobskill_event(md, flag);

    return damage;
}

static
struct Damage battle_calc_mob_weapon_attack(dumb_ptr<block_list> src,
                                                    dumb_ptr<block_list> target,
                                                    SkillID skill_num,
                                                    int skill_lv, int)
{
    dumb_ptr<map_session_data> tsd = NULL;
    dumb_ptr<mob_data> md = src->as_mob(), tmd = NULL;
    int hitrate, flee, cri = 0, atkmin, atkmax;
    int target_count = 1;
    int def1 = battle_get_def(target);
    int def2 = battle_get_def2(target);
    int t_vit = battle_get_vit(target);
    struct Damage wd {};
    int damage, damage2 = 0;
    DamageType type;
    int div_;
    BF flag;
    int ac_flag = 0;
    ATK dmg_lv = ATK::ZERO;
    eptr<struct status_change, StatusChange> sc_data, t_sc_data;

    nullpo_retr(wd, src);
    nullpo_retr(wd, target);
    nullpo_retr(wd, md);

    sc_data = battle_get_sc_data(src);

    // ターゲット
    if (target->bl_type == BL::PC)
        tsd = target->as_player();
    else if (target->bl_type == BL::MOB)
        tmd = target->as_mob();
    MobMode t_mode = battle_get_mode(target);
    t_sc_data = battle_get_sc_data(target);

    flag = BF::SHORT | BF::WEAPON | BF::NORMAL;    // 攻撃の種類の設定

    // 回避率計算、回避判定は後で
    flee = battle_get_flee(target);
    if (battle_config.agi_penaly_type > 0
        || battle_config.vit_penaly_type > 0)
        target_count +=
            battle_counttargeted(target, src,
                    ATK(battle_config.agi_penaly_count_lv)); // FIXME
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
    if (mob_db[md->mob_class].range > 3)
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
                            ATK(battle_config.vit_penaly_count_lv)); // FIXME
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
        damage = damage2 = 0;
        dmg_lv = ATK::FLEE;
    }
    else
    {
        dmg_lv = ATK::DEF;
    }

    if (damage < 0)
        damage = 0;

    // 完全回避の判定
    if (skill_num == SkillID::ZERO && skill_lv >= 0 && tsd != NULL
        && random_::chance({battle_get_flee2(target), 1000}))
    {
        damage = 0;
        type = DamageType::FLEE2;
        dmg_lv = ATK::LUCKY;
    }

    if (battle_config.enemy_perfect_flee)
    {
        if (skill_num == SkillID::ZERO && skill_lv >= 0 && tmd != NULL
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
    wd.damage2 = 0;
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
        dumb_ptr<map_session_data> sd = bl->as_player();

        return (sd->equip_index[EQUIP::SHIELD] == -1
                && sd->equip_index[EQUIP::WEAPON] == -1);
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
    dumb_ptr<map_session_data> sd = src->as_player(), tsd = NULL;
    dumb_ptr<mob_data> tmd = NULL;
    int hitrate, flee, cri = 0, atkmin, atkmax;
    int dex, target_count = 1;
    int def1 = battle_get_def(target);
    int def2 = battle_get_def2(target);
    int t_vit = battle_get_vit(target);
    struct Damage wd {};
    int damage, damage2;
    DamageType type;
    int div_;
    BF flag;
    ATK dmg_lv = ATK::ZERO;
    eptr<struct status_change, StatusChange> sc_data, t_sc_data;
    int atkmax_ = 0, atkmin_ = 0;  //二刀流用
    int watk, watk_;
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
        tsd = target->as_player();   //tsdに代入(tmdはNULL)
    else if (target->bl_type == BL::MOB)    //対象がMobなら
        tmd = target->as_mob();   //tmdに代入(tsdはNULL)
    MobMode t_mode = battle_get_mode(target);  //対象のMode
    t_sc_data = battle_get_sc_data(target);    //対象のステータス異常

    flag = BF::SHORT | BF::WEAPON | BF::NORMAL;    // 攻撃の種類の設定

    // 回避率計算、回避判定は後で
    flee = battle_get_flee(target);
    if (battle_config.agi_penaly_type > 0 || battle_config.vit_penaly_type > 0) //AGI、VITペナルティ設定が有効
        target_count += battle_counttargeted(target, src,
                ATK(battle_config.agi_penaly_count_lv));  //対象の数を算出
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

        target_distance = max(dx, dy);
        malus_dist =
            max(0, target_distance - (skill_power(sd, SkillID::AC_OWL) / 75));
        hitrate -= (malus_dist * (malus_dist + 1));
    }

    dex = battle_get_dex(src); //DEX
    watk = battle_get_atk(src);    //ATK
    watk_ = battle_get_atk_(src);  //ATK左手

    type = DamageType::NORMAL;
    div_ = 1;                   // single attack

    {
        damage = damage2 = battle_get_baseatk(sd);    //damega,damega2初登場、base_atkの取得
    }
    if (sd->attackrange > 2)
    {                           // [fate] ranged weapon?
        const int range_damage_bonus = 80;  // up to 31.25% bonus for long-range hit
        damage =
            damage * (256 +
                      ((range_damage_bonus * target_distance) /
                       sd->attackrange)) >> 8;
        damage2 =
            damage2 * (256 +
                       ((range_damage_bonus * target_distance) /
                        sd->attackrange)) >> 8;
    }

    atkmin = atkmin_ = dex;     //最低ATKはDEXで初期化？
    sd->state.arrow_atk = 0;    //arrow_atk初期化
    if (sd->equip_index[EQUIP::WEAPON] >= 0 && sd->inventory_data[sd->equip_index[EQUIP::WEAPON]])
        atkmin =
            atkmin * (80 +
                      sd->inventory_data[sd->equip_index[EQUIP::WEAPON]]->wlv * 20) / 100;
    if (sd->equip_index[EQUIP::SHIELD] >= 0 && sd->inventory_data[sd->equip_index[EQUIP::SHIELD]])
        atkmin_ =
            atkmin_ * (80 +
                       sd->inventory_data[sd->equip_index[EQUIP::SHIELD]]->wlv * 20) /
            100;
    if (sd->status.weapon == ItemLook::BOW)
    {                           //武器が弓矢の場合
        atkmin = watk * ((atkmin < watk) ? atkmin : watk) / 100;    //弓用最低ATK計算
        flag = (flag & ~BF::RANGEMASK) | BF::LONG;    //遠距離攻撃フラグを有効
        sd->state.arrow_atk = 1;    //arrow_atk有効化
    }

    {
        atkmax = watk;
        atkmax_ = watk_;
    }

    if (atkmin > atkmax && !(sd->state.arrow_atk))
        atkmin = atkmax;        //弓は最低が上回る場合あり
    if (atkmin_ > atkmax_)
        atkmin_ = atkmax_;

    if (sd->double_rate > 0 && skill_num == SkillID::ZERO && skill_lv >= 0)
        da = random_::chance({sd->double_rate, 100});

    // 過剰精錬ボーナス
    if (sd->overrefine > 0)
        damage += random_::in(1, sd->overrefine);
    if (sd->overrefine_ > 0)
        damage2 += random_::in(1, sd->overrefine_);

    if (!da)
    {                           //ダブルアタックが発動していない
        // クリティカル計算
        cri = battle_get_critical(src);

        if (sd->state.arrow_atk)
            cri += sd->arrow_cri;
        if (sd->status.weapon == ItemLook::_16)
            // カタールの場合、クリティカルを倍に
            cri <<= 1;
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
        damage2 += atkmax_;
        if (sd->atk_rate != 100)
        {
            damage = (damage * sd->atk_rate) / 100;
            damage2 = (damage2 * sd->atk_rate) / 100;
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
        if (atkmax_ > atkmin_)
            damage2 += random_::in(atkmin_, atkmax_);
        else
            damage2 += atkmin_;
        if (sd->atk_rate != 100)
        {
            damage = (damage * sd->atk_rate) / 100;
            damage2 = (damage2 * sd->atk_rate) / 100;
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
                            ATK(battle_config.vit_penaly_count_lv)); // FIXME
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
                {
                    {
                        damage2 = damage2 * (100 - def1) / 100;
                        damage2 -= t_def;
                        if (vitbonusmax > 0)
                            damage2 -= random_::in(0, vitbonusmax);
                    }
                }
            }
        }
    }
    // 精錬ダメージの追加
    {                           //DEF, VIT無視
        damage += battle_get_atk2(src);
        damage2 += battle_get_atk_2(src);
    }

    // 0未満だった場合1に補正
    if (damage < 1)
        damage = 1;
    if (damage2 < 1)
        damage2 = 1;

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
        damage = damage2 = 0;
        dmg_lv = ATK::FLEE;
    }
    else
    {
        dmg_lv = ATK::DEF;
    }

    if (damage < 0)
        damage = 0;
    if (damage2 < 0)
        damage2 = 0;

    // 星のかけら、気球の適用
    damage += sd->star;
    damage2 += sd->star_;

    // >二刀流の左右ダメージ計算誰かやってくれぇぇぇぇえええ！
    // >map_session_data に左手ダメージ(atk,atk2)追加して
    // >pc_calcstatus()でやるべきかな？
    // map_session_data に左手武器(atk,atk2,ele,star,atkmods)追加して
    // pc_calcstatus()でデータを入力しています

    //左手のみ武器装備
    if (sd->weapontype1 == ItemLook::NONE
        && sd->weapontype2 != ItemLook::NONE)
    {
        damage = damage2;
        damage2 = 0;
    }
    // 右手、左手修練の適用
    if (sd->status.weapon >= ItemLook::SINGLE_HANDED_COUNT)
    {                           // 二刀流か?
        int dmg = damage, dmg2 = damage2;
        // 右手修練(60% 〜 100%) 右手全般
        damage = damage * 50 / 100;
        if (dmg > 0 && damage < 1)
            damage = 1;
        // 左手修練(40% 〜 80%) 左手全般
        damage2 = damage2 * 30 / 100;
        if (dmg2 > 0 && damage2 < 1)
            damage2 = 1;
    }
    else                        //二刀流でなければ左手ダメージは0
        damage2 = 0;

    // 右手,短剣のみ
    if (da)
    {                           //ダブルアタックが発動しているか
        div_ = 2;
        damage += damage;
        type = DamageType::DOUBLED;
    }

    if (sd->status.weapon == ItemLook::_16)
    {
        // カタール追撃ダメージ
        damage2 = damage * 1 / 100;
        if (damage > 0 && damage2 < 1)
            damage2 = 1;
    }

    // 完全回避の判定
    if (skill_num == SkillID::ZERO && skill_lv >= 0 && tsd != NULL && div_ < 255
        && random_::chance({battle_get_flee2(target), 1000}))
    {
        damage = damage2 = 0;
        type = DamageType::FLEE2;
        dmg_lv = ATK::LUCKY;
    }

    // 対象が完全回避をする設定がONなら
    if (battle_config.enemy_perfect_flee)
    {
        if (skill_num == SkillID::ZERO && skill_lv >= 0 && tmd != NULL && div_ < 255
            && random_::chance({battle_get_flee2(target), 1000}))
        {
            damage = damage2 = 0;
            type = DamageType::FLEE2;
            dmg_lv = ATK::LUCKY;
        }
    }

    //MobのModeに頑強フラグが立っているときの処理
    if (bool(t_mode & MobMode::PLANT))
    {
        if (damage > 0)
            damage = 1;
        if (damage2 > 0)
            damage2 = 1;
    }

    if (damage > 0 || damage2 > 0)
    {
        if (damage2 < 1)        // ダメージ最終修正
            damage =
                battle_calc_damage(src, target, damage, div_, skill_num,
                                    skill_lv, flag);
        else if (damage < 1)    // 右手がミス？
            damage2 =
                battle_calc_damage(src, target, damage2, div_, skill_num,
                                    skill_lv, flag);
        else
        {                       // 両 手/カタールの場合はちょっと計算ややこしい
            int d1 = damage + damage2, d2 = damage2;
            damage =
                battle_calc_damage(src, target, damage + damage2, div_,
                                    skill_num, skill_lv, flag);
            damage2 = (d2 * 100 / d1) * damage / 100;
            if (damage > 1 && damage2 < 1)
                damage2 = 1;
            damage -= damage2;
        }
    }

    wd.damage = damage;
    wd.damage2 = damage2;
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
    dumb_ptr<map_session_data> sd = NULL;

    nullpo_retr(md, bl);
    nullpo_retr(md, target);

    matk1 = battle_get_matk1(bl);
    matk2 = battle_get_matk2(bl);
    MobMode t_mode = battle_get_mode(target);

    if (bl->bl_type == BL::PC)
    {
        sd = bl->as_player();
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
    md.damage2 = 0;
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
    dumb_ptr<map_session_data> sd = NULL;
    int damage = 0, div_ = 1;
    struct Damage md {};
    int damagefix = 1;

    BF aflag = BF::MISC | BF::LONG | BF::SKILL;

    nullpo_retr(md, bl);
    nullpo_retr(md, target);

    if (bl->bl_type == BL::PC)
    {
        sd = bl->as_player();
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
    md.damage2 = 0;
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
                PRINTF("battle_calc_attack: unknwon attack type ! %d\n",
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
    dumb_ptr<map_session_data> sd = NULL;
    eptr<struct status_change, StatusChange> t_sc_data = battle_get_sc_data(target);
    struct Damage wd;

    nullpo_retr(ATK::ZERO, src);
    nullpo_retr(ATK::ZERO, target);

    if (src->bl_type == BL::PC)
        sd = src->as_player();

    if (src->bl_prev == NULL || target->bl_prev == NULL)
        return ATK::ZERO;
    if (src->bl_type == BL::PC && pc_isdead(sd))
        return ATK::ZERO;
    if (target->bl_type == BL::PC
        && pc_isdead(target->as_player()))
        return ATK::ZERO;

    Opt1 *opt1 = battle_get_opt1(src);
    if (opt1 != NULL && bool(*opt1))
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
            if (sd->equip_index[EQUIP::ARROW] >= 0)
            {
                if (battle_config.arrow_decrement)
                    pc_delitem(sd, sd->equip_index[EQUIP::ARROW], 1, 0);
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
            MAP_LOG_PC(target->as_player(),
                        "MAGIC-ABSORB-DMG %d", reduction);
        }

        {
            clif_damage(src, target, tick, wd.amotion, wd.dmotion,
                         wd.damage, wd.div_, wd.type, wd.damage2);
            if (sd
                    && (sd->status.weapon == ItemLook::_16
                        || sd->status.weapon >= ItemLook::SINGLE_HANDED_COUNT)
                    && wd.damage2 == 0)
                clif_damage(src, target, tick + std::chrono::milliseconds(10),
                        wd.amotion, wd.dmotion, 0, 1, DamageType::NORMAL, 0);
        }

        MapBlockLock lock;

        if (src->bl_type == BL::PC)
        {
            int weapon_index = sd->equip_index[EQUIP::WEAPON];
            int weapon = 0;
            if (sd->inventory_data[weapon_index]
                && bool(sd->status.inventory[weapon_index].equip & EPOS::WEAPON))
                weapon = sd->inventory_data[weapon_index]->nameid;

            MAP_LOG("PC%d %s:%d,%d WPNDMG %s%d %d FOR %d WPN %d",
                     sd->status.char_id, src->bl_m->name_, src->bl_x, src->bl_y,
                     (target->bl_type == BL::PC) ? "PC" : "MOB",
                     (target->bl_type == BL::PC)
                     ? target->as_player()-> status.char_id
                     : target->bl_id,
                     battle_get_class(target),
                     wd.damage + wd.damage2, weapon);
        }

        if (target->bl_type == BL::PC)
        {
            dumb_ptr<map_session_data> sd2 = target->as_player();
            MAP_LOG("PC%d %s:%d,%d WPNINJURY %s%d %d FOR %d",
                     sd2->status.char_id, target->bl_m->name_, target->bl_x, target->bl_y,
                     (src->bl_type == BL::PC) ? "PC" : "MOB",
                     (src->bl_type == BL::PC)
                     ? src->as_player()->status.char_id
                     : src->bl_id,
                     battle_get_class(src),
                     wd.damage + wd.damage2);
        }

        battle_damage(src, target, (wd.damage + wd.damage2), 0);
        if (target->bl_prev != NULL &&
            (target->bl_type != BL::PC
             || (target->bl_type == BL::PC
                 && !pc_isdead(target->as_player()))))
        {
            if (wd.damage > 0 || wd.damage2 > 0)
            {
                skill_additional_effect(src, target, SkillID::ZERO, 0);
            }
        }
        if (sd)
        {
            if (bool(wd.flag & BF::WEAPON)
                && src != target
                && (wd.damage > 0 || wd.damage2 > 0))
            {
                int hp = 0, sp = 0;
                if (sd->hp_drain_rate && wd.damage > 0
                    && random_::chance({sd->hp_drain_rate, 100}))
                {
                    hp += (wd.damage * sd->hp_drain_per) / 100;
                }
                if (sd->hp_drain_rate_ && wd.damage2 > 0
                    && random_::chance({sd->hp_drain_rate_, 100}))
                {
                    hp += (wd.damage2 * sd->hp_drain_per_) / 100;
                }
                if (sd->sp_drain_rate && wd.damage > 0
                    && random_::chance({sd->sp_drain_rate, 100}))
                {
                    sp += (wd.damage * sd->sp_drain_per) / 100;
                }
                if (sd->sp_drain_rate_ && wd.damage2 > 0
                    && random_::chance({sd->sp_drain_rate_, 100}))
                {
                    sp += (wd.damage2 * sd->sp_drain_per_) / 100;
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
    int s_p, t_p;
    dumb_ptr<block_list> ss = src;

    nullpo_ret(src);
    nullpo_ret(target);

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
        && target->as_player()->invincible_timer)
        return -1;

    // Mobでmaster_idがあってspecial_mob_aiなら、召喚主を求める
    if (src->bl_type == BL::MOB)
    {
        dumb_ptr<mob_data> md = src->as_mob();
        if (md && md->master_id > 0)
        {
            if (md->master_id == target->bl_id)    // 主なら肯定
                return 1;
            if (md->state.special_mob_ai)
            {
                if (target->bl_type == BL::MOB)
                {               //special_mob_aiで対象がMob
                    dumb_ptr<mob_data> tmd = target->as_mob();
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
            if ((ss = map_id2bl(md->master_id)) == NULL)
                return -1;
        }
    }

    if (src == target || ss == target)  // 同じなら肯定
        return 1;

    if (target->bl_type == BL::PC
        && pc_isinvisible(target->as_player()))
        return -1;

    if (src->bl_prev == NULL ||    // 死んでるならエラー
        (src->bl_type == BL::PC && pc_isdead(src->as_player())))
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

//PRINTF("ss:%d src:%d target:%d flag:0x%x %d %d ",ss->bl_id,src->bl_id,target->bl_id,flag,src->bl_type,target->bl_type);
//PRINTF("p:%d %d g:%d %d\n",s_p,t_p,s_g,t_g);

    if (ss->bl_type == BL::PC && target->bl_type == BL::PC)
    {                           // 両方PVPモードなら否定（敵）
        if (ss->bl_m->flag.pvp
            || pc_iskiller(ss->as_player(), target->as_player()))
        {                       // [MouseJstr]
            if (battle_config.pk_mode)
                return 1;       // prevent novice engagement in pk_mode [Valaris]
            else if (ss->bl_m->flag.pvp_noparty && s_p > 0 && t_p > 0
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

    int dx, dy;
    struct walkpath_data wpd;
    int arange;

    nullpo_ret(src);
    nullpo_ret(bl);

    dx = abs(bl->bl_x - src->bl_x);
    dy = abs(bl->bl_y - src->bl_y);
    arange = ((dx > dy) ? dx : dy);

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

/*==========================================
 * 設定ファイルを読み込む
 *------------------------------------------
 */
int battle_config_read(ZString cfgName)
{
    static int count = 0;

    if ((count++) == 0)
    {
        battle_config.warp_point_debug = 0;
        battle_config.enemy_critical = 0;
        battle_config.enemy_critical_rate = 100;
        battle_config.enemy_str = 1;
        battle_config.enemy_perfect_flee = 0;
        battle_config.casting_rate = 100;
        battle_config.delay_rate = 100;
        battle_config.delay_dependon_dex = 0;
        battle_config.skill_delay_attack_enable = 0;
        battle_config.monster_skill_add_range = 0;
        battle_config.player_damage_delay = 1;
        battle_config.flooritem_lifetime = std::chrono::duration_cast<std::chrono::milliseconds>(LIFETIME_FLOORITEM).count();
        battle_config.item_auto_get = 0;
        battle_config.drop_pickup_safety_zone = 20;
        battle_config.item_first_get_time = 3000;
        battle_config.item_second_get_time = 1000;
        battle_config.item_third_get_time = 1000;

        battle_config.base_exp_rate = 100;
        battle_config.job_exp_rate = 100;
        battle_config.death_penalty_type = 0;
        battle_config.death_penalty_base = 0;
        battle_config.death_penalty_job = 0;
        battle_config.restart_hp_rate = 0;
        battle_config.restart_sp_rate = 0;
        battle_config.monster_hp_rate = 100;
        battle_config.monster_max_aspd = 199;
        battle_config.atcommand_gm_only = 0;
        battle_config.gm_all_equipment = 0;
        battle_config.monster_active_enable = 1;
        battle_config.mob_skill_use = 1;
        battle_config.mob_count_rate = 100;
        battle_config.basic_skill_check = 1;
        battle_config.player_invincible_time = 5000;
        battle_config.skill_min_damage = 0;
        battle_config.natural_healhp_interval = 6000;
        battle_config.natural_healsp_interval = 8000;
        battle_config.natural_heal_skill_interval = 10000;
        battle_config.natural_heal_weight_rate = 50;
        battle_config.itemheal_regeneration_factor = 1;
        battle_config.arrow_decrement = 1;
        battle_config.max_aspd = 199;
        battle_config.max_hp = 32500;
        battle_config.max_sp = 32500;
        battle_config.max_lv = 99;  // [MouseJstr]
        battle_config.max_parameter = 99;
        battle_config.max_cart_weight = 8000;
        battle_config.monster_skill_log = 0;
        battle_config.battle_log = 0;
        battle_config.save_log = 0;
        battle_config.error_log = 1;
        battle_config.etc_log = 1;
        battle_config.save_clothcolor = 0;
        battle_config.undead_detect_type = 0;
        battle_config.agi_penaly_type = 0;
        battle_config.agi_penaly_count = 3;
        battle_config.agi_penaly_num = 0;
        battle_config.agi_penaly_count_lv = static_cast<int>(ATK::FLEE); // FIXME
        battle_config.vit_penaly_type = 0;
        battle_config.vit_penaly_count = 3;
        battle_config.vit_penaly_num = 0;
        battle_config.vit_penaly_count_lv = static_cast<int>(ATK::DEF); // FIXME
        battle_config.mob_changetarget_byskill = 0;
        battle_config.player_attack_direction_change = 1;
        battle_config.monster_attack_direction_change = 1;
        battle_config.display_delay_skill_fail = 1;
        battle_config.dead_branch_active = 0;
        battle_config.show_steal_in_same_party = 0;
        battle_config.hide_GM_session = 0;
        battle_config.invite_request_check = 1;
        battle_config.disp_experience = 0;
        battle_config.prevent_logout = 1;   // Added by RoVeRT
        battle_config.maximum_level = 255;  // Added by Valaris
        battle_config.drops_by_luk = 0; // [Valaris]
        battle_config.pk_mode = 0;  // [Valaris]
        battle_config.multi_level_up = 0;   // [Valaris]
        battle_config.hack_info_GM_level = 60;  // added by [Yor] (default: 60, GM level)
        battle_config.any_warp_GM_min_level = 20;   // added by [Yor]
        battle_config.min_hair_style = 0;
        battle_config.max_hair_style = 20;
        battle_config.min_hair_color = 0;
        battle_config.max_hair_color = 9;
        battle_config.min_cloth_color = 0;
        battle_config.max_cloth_color = 4;

        battle_config.castrate_dex_scale = 150;

        battle_config.area_size = 14;

        battle_config.chat_lame_penalty = 2;
        battle_config.chat_spam_threshold = 10;
        battle_config.chat_spam_flood = 10;
        battle_config.chat_spam_ban = 1;
        battle_config.chat_spam_warn = 8;
        battle_config.chat_maxline = 255;

        battle_config.packet_spam_threshold = 2;
        battle_config.packet_spam_flood = 30;
        battle_config.packet_spam_kick = 1;

        battle_config.mask_ip_gms = 1;

        battle_config.mob_splash_radius = -1;
    }

    std::ifstream in(cfgName.c_str());
    if (!in.is_open())
    {
        PRINTF("file not found: %s\n", cfgName);
        return 1;
    }

    FString line;
    while (io::getline(in, line))
    {
#define BATTLE_CONFIG_VAR(name) {{#name}, &battle_config.name}
        const struct
        {
            ZString str;
            int *val;
        } data[] =
        {
            BATTLE_CONFIG_VAR(warp_point_debug),
            BATTLE_CONFIG_VAR(enemy_critical),
            BATTLE_CONFIG_VAR(enemy_critical_rate),
            BATTLE_CONFIG_VAR(enemy_str),
            BATTLE_CONFIG_VAR(enemy_perfect_flee),
            BATTLE_CONFIG_VAR(casting_rate),
            BATTLE_CONFIG_VAR(delay_rate),
            BATTLE_CONFIG_VAR(delay_dependon_dex),
            BATTLE_CONFIG_VAR(skill_delay_attack_enable),
            BATTLE_CONFIG_VAR(monster_skill_add_range),
            BATTLE_CONFIG_VAR(player_damage_delay),
            BATTLE_CONFIG_VAR(flooritem_lifetime),
            BATTLE_CONFIG_VAR(item_auto_get),
            BATTLE_CONFIG_VAR(drop_pickup_safety_zone),
            BATTLE_CONFIG_VAR(item_first_get_time),
            BATTLE_CONFIG_VAR(item_second_get_time),
            BATTLE_CONFIG_VAR(item_third_get_time),
            BATTLE_CONFIG_VAR(base_exp_rate),
            BATTLE_CONFIG_VAR(job_exp_rate),
            BATTLE_CONFIG_VAR(death_penalty_type),
            BATTLE_CONFIG_VAR(death_penalty_base),
            BATTLE_CONFIG_VAR(death_penalty_job),
            BATTLE_CONFIG_VAR(restart_hp_rate),
            BATTLE_CONFIG_VAR(restart_sp_rate),
            BATTLE_CONFIG_VAR(monster_hp_rate),
            BATTLE_CONFIG_VAR(monster_max_aspd),
            BATTLE_CONFIG_VAR(atcommand_gm_only),
            BATTLE_CONFIG_VAR(atcommand_spawn_quantity_limit),
            BATTLE_CONFIG_VAR(gm_all_equipment),
            BATTLE_CONFIG_VAR(monster_active_enable),
            BATTLE_CONFIG_VAR(mob_skill_use),
            BATTLE_CONFIG_VAR(mob_count_rate),
            BATTLE_CONFIG_VAR(basic_skill_check),
            BATTLE_CONFIG_VAR(player_invincible_time),
            BATTLE_CONFIG_VAR(skill_min_damage),
            BATTLE_CONFIG_VAR(natural_healhp_interval),
            BATTLE_CONFIG_VAR(natural_healsp_interval),
            BATTLE_CONFIG_VAR(natural_heal_skill_interval),
            BATTLE_CONFIG_VAR(natural_heal_weight_rate),
            BATTLE_CONFIG_VAR(itemheal_regeneration_factor),
            BATTLE_CONFIG_VAR(arrow_decrement),
            BATTLE_CONFIG_VAR(max_aspd),
            BATTLE_CONFIG_VAR(max_hp),
            BATTLE_CONFIG_VAR(max_sp),
            BATTLE_CONFIG_VAR(max_lv),
            BATTLE_CONFIG_VAR(max_parameter),
            BATTLE_CONFIG_VAR(max_cart_weight),
            BATTLE_CONFIG_VAR(monster_skill_log),
            BATTLE_CONFIG_VAR(battle_log),
            BATTLE_CONFIG_VAR(save_log),
            BATTLE_CONFIG_VAR(error_log),
            BATTLE_CONFIG_VAR(etc_log),
            BATTLE_CONFIG_VAR(save_clothcolor),
            BATTLE_CONFIG_VAR(undead_detect_type),
            BATTLE_CONFIG_VAR(agi_penaly_type),
            BATTLE_CONFIG_VAR(agi_penaly_count),
            BATTLE_CONFIG_VAR(agi_penaly_num),
            BATTLE_CONFIG_VAR(agi_penaly_count_lv),
            BATTLE_CONFIG_VAR(vit_penaly_type),
            BATTLE_CONFIG_VAR(vit_penaly_count),
            BATTLE_CONFIG_VAR(vit_penaly_num),
            BATTLE_CONFIG_VAR(vit_penaly_count_lv),
            BATTLE_CONFIG_VAR(mob_changetarget_byskill),
            BATTLE_CONFIG_VAR(player_attack_direction_change),
            BATTLE_CONFIG_VAR(monster_attack_direction_change),
            BATTLE_CONFIG_VAR(display_delay_skill_fail),
            BATTLE_CONFIG_VAR(dead_branch_active),
            BATTLE_CONFIG_VAR(show_steal_in_same_party),
            BATTLE_CONFIG_VAR(hide_GM_session),
            BATTLE_CONFIG_VAR(invite_request_check),
            BATTLE_CONFIG_VAR(disp_experience),
            BATTLE_CONFIG_VAR(prevent_logout),   // Added by RoVeRT
            BATTLE_CONFIG_VAR(alchemist_summon_reward), // [Valaris]
            BATTLE_CONFIG_VAR(maximum_level), // [Valaris]
            BATTLE_CONFIG_VAR(drops_by_luk),   // [Valaris]
            BATTLE_CONFIG_VAR(monsters_ignore_gm),   // [Valaris]
            BATTLE_CONFIG_VAR(pk_mode), // [Valaris]
            BATTLE_CONFIG_VAR(multi_level_up),   // [Valaris]
            BATTLE_CONFIG_VAR(hack_info_GM_level),   // added by [Yor]
            BATTLE_CONFIG_VAR(any_warp_GM_min_level), // added by [Yor]
            BATTLE_CONFIG_VAR(min_hair_style),   // added by [MouseJstr]
            BATTLE_CONFIG_VAR(max_hair_style),   // added by [MouseJstr]
            BATTLE_CONFIG_VAR(min_hair_color),   // added by [MouseJstr]
            BATTLE_CONFIG_VAR(max_hair_color),   // added by [MouseJstr]
            BATTLE_CONFIG_VAR(min_cloth_color), // added by [MouseJstr]
            BATTLE_CONFIG_VAR(max_cloth_color), // added by [MouseJstr]
            BATTLE_CONFIG_VAR(castrate_dex_scale),   // added by [MouseJstr]
            BATTLE_CONFIG_VAR(area_size), // added by [MouseJstr]
            BATTLE_CONFIG_VAR(chat_lame_penalty),
            BATTLE_CONFIG_VAR(chat_spam_threshold),
            BATTLE_CONFIG_VAR(chat_spam_flood),
            BATTLE_CONFIG_VAR(chat_spam_ban),
            BATTLE_CONFIG_VAR(chat_spam_warn),
            BATTLE_CONFIG_VAR(chat_maxline),
            BATTLE_CONFIG_VAR(packet_spam_threshold),
            BATTLE_CONFIG_VAR(packet_spam_flood),
            BATTLE_CONFIG_VAR(packet_spam_kick),
            BATTLE_CONFIG_VAR(mask_ip_gms),
            BATTLE_CONFIG_VAR(mob_splash_radius),
        };

        SString w1;
        TString w2;
        if (!split_key_value(line, &w1, &w2))
            continue;

        if (w1 == "import")
        {
            battle_config_read(w2);
            continue;
        }

        for (auto datum : data)
            if (w1 == datum.str)
            {
                *datum.val = config_switch(w2);
                goto continue_outer;
            }

        PRINTF("WARNING: unknown battle conf key: %s\n", FString(w1));

    continue_outer:
        ;
    }

    if (--count == 0)
    {
        if (static_cast<interval_t>(battle_config.flooritem_lifetime) < std::chrono::seconds(1))
            battle_config.flooritem_lifetime = std::chrono::duration_cast<std::chrono::milliseconds>(LIFETIME_FLOORITEM).count();
        if (battle_config.restart_hp_rate < 0)
            battle_config.restart_hp_rate = 0;
        else if (battle_config.restart_hp_rate > 100)
            battle_config.restart_hp_rate = 100;
        if (battle_config.restart_sp_rate < 0)
            battle_config.restart_sp_rate = 0;
        else if (battle_config.restart_sp_rate > 100)
            battle_config.restart_sp_rate = 100;
        if (battle_config.natural_healhp_interval < NATURAL_HEAL_INTERVAL.count())
            battle_config.natural_healhp_interval = NATURAL_HEAL_INTERVAL.count();
        if (battle_config.natural_healsp_interval < NATURAL_HEAL_INTERVAL.count())
            battle_config.natural_healsp_interval = NATURAL_HEAL_INTERVAL.count();
        if (battle_config.natural_heal_skill_interval < NATURAL_HEAL_INTERVAL.count())
            battle_config.natural_heal_skill_interval = NATURAL_HEAL_INTERVAL.count();
        if (battle_config.natural_heal_weight_rate < 50)
            battle_config.natural_heal_weight_rate = 50;
        if (battle_config.natural_heal_weight_rate > 101)
            battle_config.natural_heal_weight_rate = 101;
        battle_config.monster_max_aspd =
            2000 - battle_config.monster_max_aspd * 10;
        if (battle_config.monster_max_aspd < 10)
            battle_config.monster_max_aspd = 10;
        if (battle_config.monster_max_aspd > 1000)
            battle_config.monster_max_aspd = 1000;
        battle_config.max_aspd = 2000 - battle_config.max_aspd * 10;
        if (battle_config.max_aspd < 10)
            battle_config.max_aspd = 10;
        if (battle_config.max_aspd > 1000)
            battle_config.max_aspd = 1000;
        if (battle_config.max_hp > 1000000)
            battle_config.max_hp = 1000000;
        if (battle_config.max_hp < 100)
            battle_config.max_hp = 100;
        if (battle_config.max_sp > 1000000)
            battle_config.max_sp = 1000000;
        if (battle_config.max_sp < 100)
            battle_config.max_sp = 100;
        if (battle_config.max_parameter < 10)
            battle_config.max_parameter = 10;
        if (battle_config.max_parameter > 10000)
            battle_config.max_parameter = 10000;
        if (battle_config.max_cart_weight > 1000000)
            battle_config.max_cart_weight = 1000000;
        if (battle_config.max_cart_weight < 100)
            battle_config.max_cart_weight = 100;
        battle_config.max_cart_weight *= 10;

        if (battle_config.agi_penaly_count < 2)
            battle_config.agi_penaly_count = 2;
        if (battle_config.vit_penaly_count < 2)
            battle_config.vit_penaly_count = 2;

        if (battle_config.hack_info_GM_level < 0)   // added by [Yor]
            battle_config.hack_info_GM_level = 0;
        else if (battle_config.hack_info_GM_level > 100)
            battle_config.hack_info_GM_level = 100;

        if (battle_config.any_warp_GM_min_level < 0)    // added by [Yor]
            battle_config.any_warp_GM_min_level = 0;
        else if (battle_config.any_warp_GM_min_level > 100)
            battle_config.any_warp_GM_min_level = 100;

        if (battle_config.chat_spam_ban < 0)
            battle_config.chat_spam_ban = 0;
        else if (battle_config.chat_spam_ban > 32767)
            battle_config.chat_spam_ban = 32767;

        if (battle_config.chat_spam_flood < 0)
            battle_config.chat_spam_flood = 0;
        else if (battle_config.chat_spam_flood > 32767)
            battle_config.chat_spam_flood = 32767;

        if (battle_config.chat_spam_warn < 0)
            battle_config.chat_spam_warn = 0;
        else if (battle_config.chat_spam_warn > 32767)
            battle_config.chat_spam_warn = 32767;

        if (battle_config.chat_spam_threshold < 0)
            battle_config.chat_spam_threshold = 0;
        else if (battle_config.chat_spam_threshold > 32767)
            battle_config.chat_spam_threshold = 32767;

        if (battle_config.chat_maxline < 1)
            battle_config.chat_maxline = 1;
        else if (battle_config.chat_maxline > 512)
            battle_config.chat_maxline = 512;

        if (battle_config.packet_spam_threshold < 0)
            battle_config.packet_spam_threshold = 0;
        else if (battle_config.packet_spam_threshold > 32767)
            battle_config.packet_spam_threshold = 32767;

        if (battle_config.packet_spam_flood < 0)
            battle_config.packet_spam_flood = 0;
        else if (battle_config.packet_spam_flood > 32767)
            battle_config.packet_spam_flood = 32767;

        if (battle_config.packet_spam_kick < 0)
            battle_config.packet_spam_kick = 0;
        else if (battle_config.packet_spam_kick > 1)
            battle_config.packet_spam_kick = 1;

        if (battle_config.mask_ip_gms < 0)
            battle_config.mask_ip_gms = 0;
        else if (battle_config.mask_ip_gms > 1)
            battle_config.mask_ip_gms = 1;
    }

    return 0;
}
