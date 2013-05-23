#include "battle.hpp"

#include <cstring>

#include <fstream>

#include "../common/cxxstdio.hpp"
#include "../common/random.hpp"
#include "../common/nullpo.hpp"

#include "clif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
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
int battle_counttargeted(struct block_list *bl, struct block_list *src,
        ATK target_lv)
{
    nullpo_ret(bl);
    if (bl->type == BL::PC)
        return pc_counttargeted((struct map_session_data *) bl, src,
                                 target_lv);
    else if (bl->type == BL::MOB)
        return mob_counttargeted((struct mob_data *) bl, src, target_lv);
    return 0;
}

/*==========================================
 * 対象のClassを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_class(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::MOB)
        return ((struct mob_data *) bl)->mob_class;
    else if (bl->type == BL::PC)
        return 0;
    else
        return 0;
}

/*==========================================
 * 対象の方向を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
DIR battle_get_dir(struct block_list *bl)
{
    nullpo_retr(DIR::S, bl);
    if (bl->type == BL::MOB)
        return ((struct mob_data *) bl)->dir;
    else if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->dir;
    else
        return DIR::S;
}

/*==========================================
 * 対象のレベルを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_lv(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::MOB)
        return ((struct mob_data *) bl)->stats[mob_stat::LV];
    else if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->status.base_level;
    else
        return 0;
}

/*==========================================
 * 対象の射程を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_range(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::MOB)
        return mob_db[((struct mob_data *) bl)->mob_class].range;
    else if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->attackrange;
    else
        return 0;
}

/*==========================================
 * 対象のHPを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_hp(struct block_list *bl)
{
    nullpo_retr(1, bl);
    if (bl->type == BL::MOB)
        return ((struct mob_data *) bl)->hp;
    else if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->status.hp;
    else
        return 1;
}

/*==========================================
 * 対象のMHPを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_max_hp(struct block_list *bl)
{
    nullpo_retr(1, bl);
    if (bl->type == BL::PC && ((struct map_session_data *) bl))
        return ((struct map_session_data *) bl)->status.max_hp;
    else
    {
        int max_hp = 1;
        if (bl->type == BL::MOB && ((struct mob_data *) bl))
        {
            max_hp = ((struct mob_data *) bl)->stats[mob_stat::MAX_HP];
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
int battle_get_str(struct block_list *bl)
{
    int str = 0;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::MOB && ((struct mob_data *) bl))
        str = ((struct mob_data *) bl)->stats[mob_stat::STR];
    else if (bl->type == BL::PC && ((struct map_session_data *) bl))
        return ((struct map_session_data *) bl)->paramc[ATTR::STR];

    if (str < 0)
        str = 0;
    return str;
}

/*==========================================
 * 対象のAgiを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */

int battle_get_agi(struct block_list *bl)
{
    int agi = 0;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::MOB)
        agi = ((struct mob_data *) bl)->stats[mob_stat::AGI];
    else if (bl->type == BL::PC)
        agi = ((struct map_session_data *) bl)->paramc[ATTR::AGI];

    if (agi < 0)
        agi = 0;
    return agi;
}

/*==========================================
 * 対象のVitを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_vit(struct block_list *bl)
{
    int vit = 0;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::MOB)
        vit = ((struct mob_data *) bl)->stats[mob_stat::VIT];
    else if (bl->type == BL::PC)
        vit = ((struct map_session_data *) bl)->paramc[ATTR::VIT];

    if (vit < 0)
        vit = 0;
    return vit;
}

/*==========================================
 * 対象のIntを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_int(struct block_list *bl)
{
    int int_ = 0;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::MOB)
        int_ = ((struct mob_data *) bl)->stats[mob_stat::INT];
    else if (bl->type == BL::PC)
        int_ = ((struct map_session_data *) bl)->paramc[ATTR::INT];

    if (int_ < 0)
        int_ = 0;
    return int_;
}

/*==========================================
 * 対象のDexを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_dex(struct block_list *bl)
{
    int dex = 0;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::MOB)
        dex = ((struct mob_data *) bl)->stats[mob_stat::DEX];
    else if (bl->type == BL::PC)
        dex = ((struct map_session_data *) bl)->paramc[ATTR::DEX];

    if (dex < 0)
        dex = 0;
    return dex;
}

/*==========================================
 * 対象のLukを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_luk(struct block_list *bl)
{
    int luk = 0;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::MOB)
        luk = ((struct mob_data *) bl)->stats[mob_stat::LUK];
    else if (bl->type == BL::PC)
        luk = ((struct map_session_data *) bl)->paramc[ATTR::LUK];

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
int battle_get_flee(struct block_list *bl)
{
    int flee = 1;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::PC)
        flee = ((struct map_session_data *) bl)->flee;
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
int battle_get_hit(struct block_list *bl)
{
    int hit = 1;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::PC)
        hit = ((struct map_session_data *) bl)->hit;
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
int battle_get_flee2(struct block_list *bl)
{
    int flee2 = 1;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::PC)
    {
        flee2 = battle_get_luk(bl) + 10;
        flee2 +=
            ((struct map_session_data *) bl)->flee2 -
            (((struct map_session_data *) bl)->paramc[ATTR::LUK] + 10);
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
int battle_get_critical(struct block_list *bl)
{
    int critical = 1;
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::PC)
    {
        critical = battle_get_luk(bl) * 2 + 10;
        critical +=
            ((struct map_session_data *) bl)->critical -
            ((((struct map_session_data *) bl)->paramc[ATTR::LUK] * 3) + 10);
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
int battle_get_baseatk(struct block_list *bl)
{
    eptr<struct status_change, StatusChange> sc_data;
    int batk = 1;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::PC)
        batk = ((struct map_session_data *) bl)->base_atk;  //設定されているbase_atk
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
int battle_get_atk(struct block_list *bl)
{
    eptr<struct status_change, StatusChange> sc_data;
    int atk = 0;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::PC)
        atk = ((struct map_session_data *) bl)->watk;
    else if (bl->type == BL::MOB)
        atk = ((struct mob_data *) bl)->stats[mob_stat::ATK1];

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
int battle_get_atk_(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->watk_;
    else
        return 0;
}

/*==========================================
 * 対象のAtk2を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
static
int battle_get_atk2(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->watk2;
    else
    {
        int atk2 = 0;
        if (bl->type == BL::MOB)
            atk2 = ((struct mob_data *) bl)->stats[mob_stat::ATK2];

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
int battle_get_atk_2(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->watk_2;
    else
        return 0;
}

/*==========================================
 * 対象のMAtk1を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
static
int battle_get_matk1(struct block_list *bl)
{
    eptr<struct status_change, StatusChange> sc_data;
    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::MOB)
    {
        int matk, int_ = battle_get_int(bl);
        matk = int_ + (int_ / 5) * (int_ / 5);

        return matk;
    }
    else if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->matk1;
    else
        return 0;
}

/*==========================================
 * 対象のMAtk2を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
static
int battle_get_matk2(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::MOB)
    {
        int matk, int_ = battle_get_int(bl);
        matk = int_ + (int_ / 7) * (int_ / 7);

        return matk;
    }
    else if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->matk2;
    else
        return 0;
}

/*==========================================
 * 対象のDefを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_get_def(struct block_list *bl)
{
    eptr<struct status_change, StatusChange> sc_data;
    int def = 0;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::PC)
    {
        def = ((struct map_session_data *) bl)->def;
    }
    else if (bl->type == BL::MOB)
    {
        def = ((struct mob_data *) bl)->stats[mob_stat::DEF];
    }

    if (def < 1000000)
    {
        if (sc_data)
        {
            //毒にかかっている時は減算
            if (sc_data[StatusChange::SC_POISON].timer
                && bl->type != BL::PC)
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
int battle_get_mdef(struct block_list *bl)
{
    eptr<struct status_change, StatusChange> sc_data;
    int mdef = 0;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::PC)
        mdef = ((struct map_session_data *) bl)->mdef;
    else if (bl->type == BL::MOB)
        mdef = ((struct mob_data *) bl)->stats[mob_stat::MDEF];

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
int battle_get_def2(struct block_list *bl)
{
    eptr<struct status_change, StatusChange> sc_data;
    int def2 = 1;

    nullpo_retr(1, bl);
    sc_data = battle_get_sc_data(bl);
    if (bl->type == BL::PC)
        def2 = ((struct map_session_data *) bl)->def2;
    else if (bl->type == BL::MOB)
        def2 = ((struct mob_data *) bl)->stats[mob_stat::VIT];

    if (sc_data)
    {
        if (sc_data[StatusChange::SC_POISON].timer
            && bl->type != BL::PC)
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
int battle_get_mdef2(struct block_list *bl)
{
    int mdef2 = 0;

    nullpo_ret(bl);
    if (bl->type == BL::MOB)
        mdef2 =
            ((struct mob_data *) bl)->stats[mob_stat::INT] +
            (((struct mob_data *) bl)->stats[mob_stat::VIT] >> 1);
    else if (bl->type == BL::PC)
        mdef2 =
            ((struct map_session_data *) bl)->mdef2 +
            (((struct map_session_data *) bl)->paramc[ATTR::VIT] >> 1);

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
interval_t battle_get_speed(struct block_list *bl)
{
    nullpo_retr(std::chrono::seconds(1), bl);
    if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->speed;
    else
    {
        interval_t speed = std::chrono::seconds(1);
        if (bl->type == BL::MOB)
            speed = static_cast<interval_t>(((struct mob_data *) bl)->stats[mob_stat::SPEED]);

        return std::max(speed, std::chrono::milliseconds(1));
    }
}

/*==========================================
 * 対象のaDelay(攻撃時ディレイ)を返す(汎用)
 * aDelayは小さいほうが攻撃速度が速い
 *------------------------------------------
 */
// TODO figure out what all the doubling is about
interval_t battle_get_adelay(struct block_list *bl)
{
    nullpo_retr(std::chrono::seconds(4), bl);
    if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->aspd * 2;
    else
    {
        eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
        interval_t adelay = std::chrono::seconds(4);
        int aspd_rate = 100;
        if (bl->type == BL::MOB)
            adelay = static_cast<interval_t>(((struct mob_data *) bl)->stats[mob_stat::ADELAY]);

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

interval_t battle_get_amotion(struct block_list *bl)
{
    nullpo_retr(std::chrono::seconds(2), bl);
    if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->amotion;
    else
    {
        eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
        interval_t amotion = std::chrono::seconds(2);
        int aspd_rate = 100;
        if (bl->type == BL::MOB)
            amotion = static_cast<interval_t>(mob_db[((struct mob_data *) bl)->mob_class].amotion);

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

interval_t battle_get_dmotion(struct block_list *bl)
{
    nullpo_retr(interval_t::zero(), bl);
    if (bl->type == BL::MOB)
    {
        return static_cast<interval_t>(mob_db[((struct mob_data *) bl)->mob_class].dmotion);
    }
    else if (bl->type == BL::PC)
    {
        return ((struct map_session_data *) bl)->dmotion;
    }
    else
        return std::chrono::seconds(2);
}

LevelElement battle_get_element(struct block_list *bl)
{
    LevelElement ret = {2, Element::neutral};

    nullpo_retr(ret, bl);
    if (bl->type == BL::MOB)   // 10の位＝Lv*2、１の位＝属性
        ret = ((struct mob_data *) bl)->def_ele;

    return ret;
}

int battle_get_party_id(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::PC)
        return ((struct map_session_data *) bl)->status.party_id;
    else if (bl->type == BL::MOB)
    {
        struct mob_data *md = (struct mob_data *) bl;
        if (md->master_id > 0)
            return -md->master_id;
        return -md->bl.id;
    }
    return 0;
}

Race battle_get_race(struct block_list *bl)
{
    nullpo_retr(Race::formless, bl);
    if (bl->type == BL::MOB)
        return mob_db[((struct mob_data *) bl)->mob_class].race;
    else if (bl->type == BL::PC)
        return Race::demihuman;
    else
        return Race::formless;
}

MobMode battle_get_mode(struct block_list *bl)
{
    nullpo_retr(MobMode::CAN_MOVE, bl);
    if (bl->type == BL::MOB)
        return mob_db[((struct mob_data *) bl)->mob_class].mode;
    // とりあえず動くということで1
    return MobMode::CAN_MOVE;
}

int battle_get_stat(SP stat_id, struct block_list *bl)
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
eptr<struct status_change, StatusChange> battle_get_sc_data(struct block_list *bl)
{
    nullpo_retr(nullptr, bl);

    switch (bl->type)
    {
    case BL::MOB:
        return ((struct mob_data *)(bl))->sc_data;
    case BL::PC:
        return ((struct map_session_data *)(bl))->sc_data;
    }
    return nullptr;
}

short *battle_get_sc_count(struct block_list *bl)
{
    nullpo_retr(NULL, bl);
    if (bl->type == BL::MOB)
        return &((struct mob_data *) bl)->sc_count;
    else if (bl->type == BL::PC)
        return &((struct map_session_data *) bl)->sc_count;
    return NULL;
}

Opt1 *battle_get_opt1(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::MOB)
        return &((struct mob_data *) bl)->opt1;
    else if (bl->type == BL::PC)
        return &((struct map_session_data *) bl)->opt1;
    else if (bl->type == BL::NPC && (struct npc_data *) bl)
        return &((struct npc_data *) bl)->opt1;
    return 0;
}

Opt2 *battle_get_opt2(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::MOB)
        return &((struct mob_data *) bl)->opt2;
    else if (bl->type == BL::PC)
        return &((struct map_session_data *) bl)->opt2;
    else if (bl->type == BL::NPC && (struct npc_data *) bl)
        return &((struct npc_data *) bl)->opt2;
    return 0;
}

Opt3 *battle_get_opt3(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::MOB)
        return &((struct mob_data *) bl)->opt3;
    else if (bl->type == BL::PC)
        return &((struct map_session_data *) bl)->opt3;
    else if (bl->type == BL::NPC && (struct npc_data *) bl)
        return &((struct npc_data *) bl)->opt3;
    return 0;
}

Option *battle_get_option(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::MOB)
        return &((struct mob_data *) bl)->option;
    else if (bl->type == BL::PC)
        return &((struct map_session_data *) bl)->status.option;
    else if (bl->type == BL::NPC && (struct npc_data *) bl)
        return &((struct npc_data *) bl)->option;
    return 0;
}

//-------------------------------------------------------------------

// ダメージの遅延
struct battle_delay_damage_
{
    struct block_list *src, *target;
    int damage;
    int flag;
};

// 実際にHPを操作
int battle_damage(struct block_list *bl, struct block_list *target,
                   int damage, int flag)
{
    nullpo_ret(target);    //blはNULLで呼ばれることがあるので他でチェック

    if (damage == 0)
        return 0;

    if (target->prev == NULL)
        return 0;

    if (bl)
    {
        if (bl->prev == NULL)
            return 0;
    }

    if (damage < 0)
        return battle_heal(bl, target, -damage, 0, flag);

    if (target->type == BL::MOB)
    {                           // MOB
        struct mob_data *md = (struct mob_data *) target;
        if (md && md->skilltimer && md->state.skillcastcancel)    // 詠唱妨害
            skill_castcancel(target, 0);
        return mob_damage(bl, md, damage, 0);
    }
    else if (target->type == BL::PC)
    {                           // PC

        struct map_session_data *tsd = (struct map_session_data *) target;

        return pc_damage(bl, tsd, damage);

    }
    return 0;
}

int battle_heal(struct block_list *bl, struct block_list *target, int hp,
                 int sp, int flag)
{
    nullpo_ret(target);    //blはNULLで呼ばれることがあるので他でチェック

    if (target->type == BL::PC
        && pc_isdead((struct map_session_data *) target))
        return 0;
    if (hp == 0 && sp == 0)
        return 0;

    if (hp < 0)
        return battle_damage(bl, target, -hp, flag);

    if (target->type == BL::MOB)
        return mob_heal((struct mob_data *) target, hp);
    else if (target->type == BL::PC)
        return pc_heal((struct map_session_data *) target, hp, sp);
    return 0;
}

// 攻撃停止
int battle_stopattack(struct block_list *bl)
{
    nullpo_ret(bl);
    if (bl->type == BL::MOB)
        return mob_stopattack((struct mob_data *) bl);
    else if (bl->type == BL::PC)
        return pc_stopattack((struct map_session_data *) bl);
    return 0;
}

// 移動停止
int battle_stopwalking(struct block_list *bl, int type)
{
    nullpo_ret(bl);
    if (bl->type == BL::MOB)
        return mob_stop_walking((struct mob_data *) bl, type);
    else if (bl->type == BL::PC)
        return pc_stop_walking((struct map_session_data *) bl, type);
    return 0;
}

/*==========================================
 * ダメージ最終計算
 *------------------------------------------
 */
static
int battle_calc_damage(struct block_list *, struct block_list *bl,
                        int damage, int div_,
                        SkillID, int, BF flag)
{
    struct mob_data *md = NULL;

    nullpo_ret(bl);

    if (bl->type == BL::MOB)
        md = (struct mob_data *) bl;

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
struct Damage battle_calc_mob_weapon_attack(struct block_list *src,
                                                    struct block_list *target,
                                                    SkillID skill_num,
                                                    int skill_lv, int)
{
    struct map_session_data *tsd = NULL;
    struct mob_data *md = (struct mob_data *) src, *tmd = NULL;
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
    if (target->type == BL::PC)
        tsd = (struct map_session_data *) target;
    else if (target->type == BL::MOB)
        tmd = (struct mob_data *) target;
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

int battle_is_unarmed(struct block_list *bl)
{
    if (!bl)
        return 0;
    if (bl->type == BL::PC)
    {
        struct map_session_data *sd = (struct map_session_data *) bl;

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
struct Damage battle_calc_pc_weapon_attack(struct block_list *src,
                                                   struct block_list *target,
                                                   SkillID skill_num,
                                                   int skill_lv, int)
{
    struct map_session_data *sd = (struct map_session_data *) src, *tsd =
        NULL;
    struct mob_data *tmd = NULL;
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
    if (target->type == BL::PC)  //対象がPCなら
        tsd = (struct map_session_data *) target;   //tsdに代入(tmdはNULL)
    else if (target->type == BL::MOB)    //対象がMobなら
        tmd = (struct mob_data *) target;   //tmdに代入(tsdはNULL)
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
        int dx = abs(src->x - target->x);
        int dy = abs(src->y - target->y);
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
        damage = damage2 = battle_get_baseatk(&sd->bl);    //damega,damega2初登場、base_atkの取得
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
struct Damage battle_calc_weapon_attack(struct block_list *src,
                                         struct block_list *target,
                                         SkillID skill_num, int skill_lv,
                                         int wflag)
{
    struct Damage wd {};

    nullpo_retr(wd, src);
    nullpo_retr(wd, target);

    if (src->type == BL::PC)
        wd = battle_calc_pc_weapon_attack(src, target, skill_num, skill_lv, wflag);    // weapon breaking [Valaris]
    else if (src->type == BL::MOB)
        wd = battle_calc_mob_weapon_attack(src, target, skill_num, skill_lv, wflag);

    return wd;
}

/*==========================================
 * 魔法ダメージ計算
 *------------------------------------------
 */
static
struct Damage battle_calc_magic_attack(struct block_list *bl,
                                        struct block_list *target,
                                        SkillID skill_num, int skill_lv, int)
{
    int mdef1 = battle_get_mdef(target);
    int mdef2 = battle_get_mdef2(target);
    int matk1, matk2, damage = 0, div_ = 1;
    struct Damage md {};
    int normalmagic_flag = 1;
    struct map_session_data *sd = NULL;

    nullpo_retr(md, bl);
    nullpo_retr(md, target);

    matk1 = battle_get_matk1(bl);
    matk2 = battle_get_matk2(bl);
    MobMode t_mode = battle_get_mode(target);

    if (bl->type == BL::PC && (sd = (struct map_session_data *) bl))
    {
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
struct Damage battle_calc_misc_attack(struct block_list *bl,
                                       struct block_list *target,
                                       SkillID skill_num, int skill_lv, int)
{
    struct map_session_data *sd = NULL;
    int damage = 0, div_ = 1;
    struct Damage md {};
    int damagefix = 1;

    BF aflag = BF::MISC | BF::LONG | BF::SKILL;

    nullpo_retr(md, bl);
    nullpo_retr(md, target);

    if (bl->type == BL::PC && (sd = (struct map_session_data *) bl))
    {
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
                                  struct block_list *bl,
                                  struct block_list *target, SkillID skill_num,
                                  int skill_lv, int flag)
{
    struct Damage d;
    memset(&d, 0, sizeof(d));

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
ATK battle_weapon_attack(struct block_list *src, struct block_list *target,
        tick_t tick)
{
    struct map_session_data *sd = NULL;
    eptr<struct status_change, StatusChange> t_sc_data = battle_get_sc_data(target);
    struct Damage wd;

    nullpo_retr(ATK::ZERO, src);
    nullpo_retr(ATK::ZERO, target);

    if (src->type == BL::PC)
        sd = (struct map_session_data *) src;

    if (src->prev == NULL || target->prev == NULL)
        return ATK::ZERO;
    if (src->type == BL::PC && pc_isdead(sd))
        return ATK::ZERO;
    if (target->type == BL::PC
        && pc_isdead((struct map_session_data *) target))
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
            && target->type == BL::PC)
        {
            int reduction = t_sc_data[StatusChange::SC_PHYS_SHIELD].val1;
            if (reduction > wd.damage)
                reduction = wd.damage;

            wd.damage -= reduction;
            MAP_LOG_PC(((struct map_session_data *) target),
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

        if (src->type == BL::PC)
        {
            int weapon_index = sd->equip_index[EQUIP::WEAPON];
            int weapon = 0;
            if (sd->inventory_data[weapon_index]
                && bool(sd->status.inventory[weapon_index].equip & EPOS::WEAPON))
                weapon = sd->inventory_data[weapon_index]->nameid;

            MAP_LOG("PC%d %d:%d,%d WPNDMG %s%d %d FOR %d WPN %d",
                     sd->status.char_id, src->m, src->x, src->y,
                     (target->type == BL::PC) ? "PC" : "MOB",
                     (target->type ==
                      BL::PC) ? ((struct map_session_data *) target)->
                     status.char_id : target->id,
                     battle_get_class(target),
                     wd.damage + wd.damage2, weapon);
        }

        if (target->type == BL::PC)
        {
            struct map_session_data *sd2 = (struct map_session_data *) target;
            MAP_LOG("PC%d %d:%d,%d WPNINJURY %s%d %d FOR %d",
                     sd2->status.char_id, target->m, target->x, target->y,
                     (src->type == BL::PC) ? "PC" : "MOB",
                     (src->type == BL::PC)
                     ? ((struct map_session_data *) src)->status.char_id
                     : src->id,
                     battle_get_class(src),
                     wd.damage + wd.damage2);
        }

        battle_damage(src, target, (wd.damage + wd.damage2), 0);
        if (target->prev != NULL &&
            (target->type != BL::PC
             || (target->type == BL::PC
                 && !pc_isdead((struct map_session_data *) target))))
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
int battle_check_target(struct block_list *src, struct block_list *target,
        BCT flag)
{
    int s_p, t_p;
    struct block_list *ss = src;

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
        if (target->type == BL::MOB || target->type == BL::PC)
            return 1;
        else
            return -1;
    }

    if (target->type == BL::PC
        && ((struct map_session_data *) target)->invincible_timer)
        return -1;

    // Mobでmaster_idがあってspecial_mob_aiなら、召喚主を求める
    if (src->type == BL::MOB)
    {
        struct mob_data *md = (struct mob_data *) src;
        if (md && md->master_id > 0)
        {
            if (md->master_id == target->id)    // 主なら肯定
                return 1;
            if (md->state.special_mob_ai)
            {
                if (target->type == BL::MOB)
                {               //special_mob_aiで対象がMob
                    struct mob_data *tmd = (struct mob_data *) target;
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

    if (target->type == BL::PC
        && pc_isinvisible((struct map_session_data *) target))
        return -1;

    if (src->prev == NULL ||    // 死んでるならエラー
        (src->type == BL::PC && pc_isdead((struct map_session_data *) src)))
        return -1;

    if ((ss->type == BL::PC && target->type == BL::MOB) ||
        (ss->type == BL::MOB && target->type == BL::PC))
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

//PRINTF("ss:%d src:%d target:%d flag:0x%x %d %d ",ss->id,src->id,target->id,flag,src->type,target->type);
//PRINTF("p:%d %d g:%d %d\n",s_p,t_p,s_g,t_g);

    if (ss->type == BL::PC && target->type == BL::PC)
    {                           // 両方PVPモードなら否定（敵）
        if (map[ss->m].flag.pvp
            || pc_iskiller((struct map_session_data *) ss,
                            (struct map_session_data *) target))
        {                       // [MouseJstr]
            if (battle_config.pk_mode)
                return 1;       // prevent novice engagement in pk_mode [Valaris]
            else if (map[ss->m].flag.pvp_noparty && s_p > 0 && t_p > 0
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
int battle_check_range(struct block_list *src, struct block_list *bl,
                        int range)
{

    int dx, dy;
    struct walkpath_data wpd;
    int arange;

    nullpo_ret(src);
    nullpo_ret(bl);

    dx = abs(bl->x - src->x);
    dy = abs(bl->y - src->y);
    arange = ((dx > dy) ? dx : dy);

    if (src->m != bl->m)        // 違うマップ
        return 0;

    if (range > 0 && range < arange)    // 遠すぎる
        return 0;

    if (arange < 2)             // 同じマスか隣接
        return 1;

//  if(bl->type == BL_SKILL && ((struct skill_unit *)bl)->group->unit_id == 0x8d)
//      return 1;

    // 障害物判定
    wpd.path_len = 0;
    wpd.path_pos = 0;
    wpd.path_half = 0;
    if (path_search(&wpd, src->m, src->x, src->y, bl->x, bl->y, 0x10001) !=
        -1)
        return 1;

    dx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    dy = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
    return (path_search(&wpd, src->m, src->x + dx, src->y + dy,
                         bl->x - dx, bl->y - dy, 0x10001) != -1) ? 1 : 0;
}

/*==========================================
 * 設定ファイルを読み込む
 *------------------------------------------
 */
int battle_config_read(const char *cfgName)
{
    static int count = 0;

    if ((count++) == 0)
    {
        battle_config.warp_point_debug = 0;
        battle_config.enemy_critical = 0;
        battle_config.enemy_critical_rate = 100;
        battle_config.enemy_str = 1;
        battle_config.enemy_perfect_flee = 0;
        battle_config.cast_rate = 100;
        battle_config.delay_rate = 100;
        battle_config.delay_dependon_dex = 0;
        battle_config.sdelay_attack_enable = 0;
        battle_config.left_cardfix_to_right = 0;
        battle_config.pc_skill_add_range = 0;
        battle_config.skill_out_range_consume = 1;
        battle_config.mob_skill_add_range = 0;
        battle_config.pc_damage_delay = 1;
        battle_config.defnotenemy = 1;
        battle_config.random_monster_checklv = 1;
        battle_config.attr_recover = 1;
        battle_config.flooritem_lifetime = (int)std::chrono::duration_cast<std::chrono::milliseconds>(LIFETIME_FLOORITEM).count();
        battle_config.item_auto_get = 0;
        battle_config.drop_pickup_safety_zone = 20;
        battle_config.item_first_get_time = 3000;
        battle_config.item_second_get_time = 1000;
        battle_config.item_third_get_time = 1000;

        battle_config.base_exp_rate = 100;
        battle_config.job_exp_rate = 100;
        battle_config.pvp_exp = 1;
        battle_config.gtb_pvp_only = 0;
        battle_config.death_penalty_type = 0;
        battle_config.death_penalty_base = 0;
        battle_config.death_penalty_job = 0;
        battle_config.zeny_penalty = 0;
        battle_config.restart_hp_rate = 0;
        battle_config.restart_sp_rate = 0;
        battle_config.monster_hp_rate = 100;
        battle_config.monster_max_aspd = 199;
        battle_config.atc_gmonly = 0;
        battle_config.gm_allskill = 0;
        battle_config.gm_allequip = 0;
        battle_config.gm_skilluncond = 0;
        battle_config.skillfree = 0;
        battle_config.skillup_limit = 0;
        battle_config.wp_rate = 100;
        battle_config.pp_rate = 100;
        battle_config.monster_active_enable = 1;
        battle_config.monster_loot_type = 0;
        battle_config.mob_skill_use = 1;
        battle_config.mob_count_rate = 100;
        battle_config.quest_skill_learn = 0;
        battle_config.quest_skill_reset = 1;
        battle_config.basic_skill_check = 1;
        battle_config.pc_invincible_time = 5000;
        battle_config.skill_min_damage = 0;
        battle_config.finger_offensive_type = 0;
        battle_config.heal_exp = 0;
        battle_config.resurrection_exp = 0;
        battle_config.shop_exp = 0;
        battle_config.combo_delay_rate = 100;
        battle_config.item_check = 1;
        battle_config.wedding_modifydisplay = 0;
        battle_config.natural_healhp_interval = 6000;
        battle_config.natural_healsp_interval = 8000;
        battle_config.natural_heal_skill_interval = 10000;
        battle_config.natural_heal_weight_rate = 50;
        battle_config.itemheal_regeneration_factor = 1;
        battle_config.item_name_override_grffile = 1;
        battle_config.arrow_decrement = 1;
        battle_config.max_aspd = 199;
        battle_config.max_hp = 32500;
        battle_config.max_sp = 32500;
        battle_config.max_lv = 99;  // [MouseJstr]
        battle_config.max_parameter = 99;
        battle_config.max_cart_weight = 8000;
        battle_config.pc_skill_log = 0;
        battle_config.mob_skill_log = 0;
        battle_config.battle_log = 0;
        battle_config.save_log = 0;
        battle_config.error_log = 1;
        battle_config.etc_log = 1;
        battle_config.save_clothcolor = 0;
        battle_config.undead_detect_type = 0;
        battle_config.pc_auto_counter_type = 1;
        battle_config.monster_auto_counter_type = 1;
        battle_config.agi_penaly_type = 0;
        battle_config.agi_penaly_count = 3;
        battle_config.agi_penaly_num = 0;
        battle_config.agi_penaly_count_lv = static_cast<int>(ATK::FLEE); // FIXME
        battle_config.vit_penaly_type = 0;
        battle_config.vit_penaly_count = 3;
        battle_config.vit_penaly_num = 0;
        battle_config.vit_penaly_count_lv = static_cast<int>(ATK::DEF); // FIXME
        battle_config.pc_skill_reiteration = 0;
        battle_config.monster_skill_reiteration = 0;
        battle_config.pc_skill_nofootset = 0;
        battle_config.monster_skill_nofootset = 0;
        battle_config.pc_cloak_check_type = 0;
        battle_config.monster_cloak_check_type = 0;
        battle_config.mob_changetarget_byskill = 0;
        battle_config.pc_attack_direction_change = 1;
        battle_config.monster_attack_direction_change = 1;
        battle_config.pc_undead_nofreeze = 0;
        battle_config.pc_land_skill_limit = 1;
        battle_config.monster_land_skill_limit = 1;
        battle_config.party_skill_penaly = 1;
        battle_config.monster_class_change_full_recover = 0;
        battle_config.produce_item_name_input = 1;
        battle_config.produce_potion_name_input = 1;
        battle_config.making_arrow_name_input = 1;
        battle_config.holywater_name_input = 1;
        battle_config.display_delay_skill_fail = 1;
        battle_config.chat_warpportal = 0;
        battle_config.mob_warpportal = 0;
        battle_config.dead_branch_active = 0;
        battle_config.show_steal_in_same_party = 0;
        battle_config.enable_upper_class = 0;
        battle_config.pc_attack_attr_none = 0;
        battle_config.mob_attack_attr_none = 1;
        battle_config.mob_ghostring_fix = 0;
        battle_config.gx_allhit = 0;
        battle_config.gx_cardfix = 0;
        battle_config.gx_dupele = 1;
        battle_config.gx_disptype = 1;
        battle_config.player_skill_partner_check = 1;
        battle_config.hide_GM_session = 0;
        battle_config.unit_movement_type = 0;
        battle_config.invite_request_check = 1;
        battle_config.skill_removetrap_type = 0;
        battle_config.disp_experience = 0;
        battle_config.prevent_logout = 1;   // Added by RoVeRT
        battle_config.maximum_level = 255;  // Added by Valaris
        battle_config.drops_by_luk = 0; // [Valaris]
        battle_config.pk_mode = 0;  // [Valaris]
        battle_config.multi_level_up = 0;   // [Valaris]
        battle_config.backstab_bow_penalty = 0; // Akaru
        battle_config.show_mob_hp = 0;  // [Valaris]
        battle_config.hack_info_GM_level = 60;  // added by [Yor] (default: 60, GM level)
        battle_config.any_warp_GM_min_level = 20;   // added by [Yor]
        battle_config.packet_ver_flag = 63; // added by [Yor]
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

    std::ifstream in(cfgName);
    if (!in.is_open())
    {
        PRINTF("file not found: %s\n", cfgName);
        return 1;
    }

    std::string line;
    while (std::getline(in, line))
    {
        // s/{"\([a-zA-Z_0-9]*\)", &battle_config.\1}/BATTLE_CONFIG_VAR(\1)/
        const struct
        {
            const char *str;
            int *val;
        } data[] =
        {
            {"warp_point_debug", &battle_config.warp_point_debug},
            {"enemy_critical", &battle_config.enemy_critical},
            {"enemy_critical_rate", &battle_config.enemy_critical_rate},
            {"enemy_str", &battle_config.enemy_str},
            {"enemy_perfect_flee", &battle_config.enemy_perfect_flee},
            {"casting_rate", &battle_config.cast_rate},
            {"delay_rate", &battle_config.delay_rate},
            {"delay_dependon_dex", &battle_config.delay_dependon_dex},
            {"skill_delay_attack_enable", &battle_config.sdelay_attack_enable},
            {"left_cardfix_to_right", &battle_config.left_cardfix_to_right},
            {"player_skill_add_range", &battle_config.pc_skill_add_range},
            {"skill_out_range_consume", &battle_config.skill_out_range_consume},
            {"monster_skill_add_range", &battle_config.mob_skill_add_range},
            {"player_damage_delay", &battle_config.pc_damage_delay},
            {"defunit_not_enemy", &battle_config.defnotenemy},
            {"random_monster_checklv", &battle_config.random_monster_checklv},
            {"attribute_recover", &battle_config.attr_recover},
            {"flooritem_lifetime", &battle_config.flooritem_lifetime},
            {"item_auto_get", &battle_config.item_auto_get},
            {"drop_pickup_safety_zone", &battle_config.drop_pickup_safety_zone},
            {"item_first_get_time", &battle_config.item_first_get_time},
            {"item_second_get_time", &battle_config.item_second_get_time},
            {"item_third_get_time", &battle_config.item_third_get_time},
            {"base_exp_rate", &battle_config.base_exp_rate},
            {"job_exp_rate", &battle_config.job_exp_rate},
            {"pvp_exp", &battle_config.pvp_exp},
            {"gtb_pvp_only", &battle_config.gtb_pvp_only},
            {"death_penalty_type", &battle_config.death_penalty_type},
            {"death_penalty_base", &battle_config.death_penalty_base},
            {"death_penalty_job", &battle_config.death_penalty_job},
            {"zeny_penalty", &battle_config.zeny_penalty},
            {"restart_hp_rate", &battle_config.restart_hp_rate},
            {"restart_sp_rate", &battle_config.restart_sp_rate},
            {"monster_hp_rate", &battle_config.monster_hp_rate},
            {"monster_max_aspd", &battle_config.monster_max_aspd},
            {"atcommand_gm_only", &battle_config.atc_gmonly},
            {"atcommand_spawn_quantity_limit", &battle_config.atc_spawn_quantity_limit},
            {"gm_all_skill", &battle_config.gm_allskill},
            {"gm_all_skill_add_abra", &battle_config.gm_allskill_addabra},
            {"gm_all_equipment", &battle_config.gm_allequip},
            {"gm_skill_unconditional", &battle_config.gm_skilluncond},
            {"player_skillfree", &battle_config.skillfree},
            {"player_skillup_limit", &battle_config.skillup_limit},
            {"weapon_produce_rate", &battle_config.wp_rate},
            {"potion_produce_rate", &battle_config.pp_rate},
            {"monster_active_enable", &battle_config.monster_active_enable},
            {"monster_loot_type", &battle_config.monster_loot_type},
            {"mob_skill_use", &battle_config.mob_skill_use},
            {"mob_count_rate", &battle_config.mob_count_rate},
            {"quest_skill_learn", &battle_config.quest_skill_learn},
            {"quest_skill_reset", &battle_config.quest_skill_reset},
            {"basic_skill_check", &battle_config.basic_skill_check},
            {"player_invincible_time", &battle_config.pc_invincible_time},
            {"skill_min_damage", &battle_config.skill_min_damage},
            {"finger_offensive_type", &battle_config.finger_offensive_type},
            {"heal_exp", &battle_config.heal_exp},
            {"resurrection_exp", &battle_config.resurrection_exp},
            {"shop_exp", &battle_config.shop_exp},
            {"combo_delay_rate", &battle_config.combo_delay_rate},
            {"item_check", &battle_config.item_check},
            {"wedding_modifydisplay", &battle_config.wedding_modifydisplay},
            {"natural_healhp_interval", &battle_config.natural_healhp_interval},
            {"natural_healsp_interval", &battle_config.natural_healsp_interval},
            {"natural_heal_skill_interval", &battle_config.natural_heal_skill_interval},
            {"natural_heal_weight_rate", &battle_config.natural_heal_weight_rate},
            {"itemheal_regeneration_factor", &battle_config.itemheal_regeneration_factor},
            {"item_name_override_grffile", &battle_config.item_name_override_grffile},
            {"arrow_decrement", &battle_config.arrow_decrement},
            {"max_aspd", &battle_config.max_aspd},
            {"max_hp", &battle_config.max_hp},
            {"max_sp", &battle_config.max_sp},
            {"max_lv", &battle_config.max_lv},
            {"max_parameter", &battle_config.max_parameter},
            {"max_cart_weight", &battle_config.max_cart_weight},
            {"player_skill_log", &battle_config.pc_skill_log},
            {"monster_skill_log", &battle_config.mob_skill_log},
            {"battle_log", &battle_config.battle_log},
            {"save_log", &battle_config.save_log},
            {"error_log", &battle_config.error_log},
            {"etc_log", &battle_config.etc_log},
            {"save_clothcolor", &battle_config.save_clothcolor},
            {"undead_detect_type", &battle_config.undead_detect_type},
            {"player_auto_counter_type", &battle_config.pc_auto_counter_type},
            {"monster_auto_counter_type", &battle_config.monster_auto_counter_type},
            {"agi_penaly_type", &battle_config.agi_penaly_type},
            {"agi_penaly_count", &battle_config.agi_penaly_count},
            {"agi_penaly_num", &battle_config.agi_penaly_num},
            {"agi_penaly_count_lv", &battle_config.agi_penaly_count_lv},
            {"vit_penaly_type", &battle_config.vit_penaly_type},
            {"vit_penaly_count", &battle_config.vit_penaly_count},
            {"vit_penaly_num", &battle_config.vit_penaly_num},
            {"vit_penaly_count_lv", &battle_config.vit_penaly_count_lv},
            {"player_skill_reiteration", &battle_config.pc_skill_reiteration},
            {"monster_skill_reiteration", &battle_config.monster_skill_reiteration},
            {"player_skill_nofootset", &battle_config.pc_skill_nofootset},
            {"monster_skill_nofootset", &battle_config.monster_skill_nofootset},
            {"player_cloak_check_type", &battle_config.pc_cloak_check_type},
            {"monster_cloak_check_type", &battle_config.monster_cloak_check_type},
            {"mob_changetarget_byskill", &battle_config.mob_changetarget_byskill},
            {"player_attack_direction_change", &battle_config.pc_attack_direction_change},
            {"monster_attack_direction_change", &battle_config.monster_attack_direction_change},
            {"player_land_skill_limit", &battle_config.pc_land_skill_limit},
            {"monster_land_skill_limit", &battle_config.monster_land_skill_limit},
            {"party_skill_penaly", &battle_config.party_skill_penaly},
            {"monster_class_change_full_recover", &battle_config.monster_class_change_full_recover},
            {"produce_item_name_input", &battle_config.produce_item_name_input},
            {"produce_potion_name_input", &battle_config.produce_potion_name_input},
            {"making_arrow_name_input", &battle_config.making_arrow_name_input},
            {"holywater_name_input", &battle_config.holywater_name_input},
            {"display_delay_skill_fail", &battle_config.display_delay_skill_fail},
            {"chat_warpportal", &battle_config.chat_warpportal},
            {"mob_warpportal", &battle_config.mob_warpportal},
            {"dead_branch_active", &battle_config.dead_branch_active},
            {"show_steal_in_same_party", &battle_config.show_steal_in_same_party},
            {"enable_upper_class", &battle_config.enable_upper_class},
            {"mob_attack_attr_none", &battle_config.mob_attack_attr_none},
            {"mob_ghostring_fix", &battle_config.mob_ghostring_fix},
            {"pc_attack_attr_none", &battle_config.pc_attack_attr_none},
            {"gx_allhit", &battle_config.gx_allhit},
            {"gx_cardfix", &battle_config.gx_cardfix},
            {"gx_dupele", &battle_config.gx_dupele},
            {"gx_disptype", &battle_config.gx_disptype},
            {"player_skill_partner_check", &battle_config.player_skill_partner_check},
            {"hide_GM_session", &battle_config.hide_GM_session},
            {"unit_movement_type", &battle_config.unit_movement_type},
            {"invite_request_check", &battle_config.invite_request_check},
            {"skill_removetrap_type", &battle_config.skill_removetrap_type},
            {"disp_experience", &battle_config.disp_experience},
            {"riding_weight", &battle_config.riding_weight},
            {"prevent_logout", &battle_config.prevent_logout},   // Added by RoVeRT
            {"alchemist_summon_reward", &battle_config.alchemist_summon_reward}, // [Valaris]
            {"maximum_level", &battle_config.maximum_level}, // [Valaris]
            {"drops_by_luk", &battle_config.drops_by_luk},   // [Valaris]
            {"monsters_ignore_gm", &battle_config.monsters_ignore_gm},   // [Valaris]
            {"pk_mode", &battle_config.pk_mode}, // [Valaris]
            {"multi_level_up", &battle_config.multi_level_up},   // [Valaris]
            {"backstab_bow_penalty", &battle_config.backstab_bow_penalty},
            {"show_mob_hp", &battle_config.show_mob_hp}, // [Valaris]
            {"hack_info_GM_level", &battle_config.hack_info_GM_level},   // added by [Yor]
            {"any_warp_GM_min_level", &battle_config.any_warp_GM_min_level}, // added by [Yor]
            {"packet_ver_flag", &battle_config.packet_ver_flag}, // added by [Yor]
            {"min_hair_style", &battle_config.min_hair_style},   // added by [MouseJstr]
            {"max_hair_style", &battle_config.max_hair_style},   // added by [MouseJstr]
            {"min_hair_color", &battle_config.min_hair_color},   // added by [MouseJstr]
            {"max_hair_color", &battle_config.max_hair_color},   // added by [MouseJstr]
            {"min_cloth_color", &battle_config.min_cloth_color}, // added by [MouseJstr]
            {"max_cloth_color", &battle_config.max_cloth_color}, // added by [MouseJstr]
            {"castrate_dex_scale", &battle_config.castrate_dex_scale},   // added by [MouseJstr]
            {"area_size", &battle_config.area_size}, // added by [MouseJstr]
            {"chat_lame_penalty", &battle_config.chat_lame_penalty},
            {"chat_spam_threshold", &battle_config.chat_spam_threshold},
            {"chat_spam_flood", &battle_config.chat_spam_flood},
            {"chat_spam_ban", &battle_config.chat_spam_ban},
            {"chat_spam_warn", &battle_config.chat_spam_warn},
            {"chat_maxline", &battle_config.chat_maxline},
            {"packet_spam_threshold", &battle_config.packet_spam_threshold},
            {"packet_spam_flood", &battle_config.packet_spam_flood},
            {"packet_spam_kick", &battle_config.packet_spam_kick},
            {"mask_ip_gms", &battle_config.mask_ip_gms},
            {"mob_splash_radius", &battle_config.mob_splash_radius},
        };

        std::string w1, w2;
        if (!split_key_value(line, &w1, &w2))
            continue;

        if (w1 == "import")
        {
            battle_config_read(w2.c_str());
            continue;
        }

        for (auto datum : data)
            if (w1 == datum.str)
            {
                *datum.val = config_switch(w2.c_str());
                goto continue_outer;
            }

        PRINTF("WARNING: unknown battle conf key: %s\n", w1);

    continue_outer:
        ;
    }

    if (--count == 0)
    {
        if (static_cast<interval_t>(battle_config.flooritem_lifetime) < std::chrono::seconds(1))
            battle_config.flooritem_lifetime = (int)std::chrono::duration_cast<std::chrono::milliseconds>(LIFETIME_FLOORITEM).count();
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

        // at least 1 client must be accepted
        if ((battle_config.packet_ver_flag & 63) == 0)  // added by [Yor]
            battle_config.packet_ver_flag = 63; // accept all clients

    }

    return 0;
}
