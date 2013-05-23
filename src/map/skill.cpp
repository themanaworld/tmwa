#include "skill.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "../common/cxxstdio.hpp"
#include "../common/random.hpp"
#include "../common/nullpo.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"

#include "battle.hpp"
#include "clif.hpp"
#include "magic.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "pc.hpp"

#include "../poison.hpp"

struct skill_name_db skill_names[] =
{
    {SkillID::AC_OWL, "OWL", "Owl's_Eye"},

    {SkillID::NPC_EMOTION, "EMOTION", "NPC_EMOTION"},
    {SkillID::NPC_POISON, "POISON", "NPC_POISON"},
    {SkillID::NPC_SELFDESTRUCTION, "SELFDESTRUCTION", "Kabooooom!"},
    {SkillID::NPC_SUMMONSLAVE, "SUMMONSLAVE", "NPC_SUMMONSLAVE"},

    {SkillID::NV_EMOTE, "EMOTE", "Emote_Skill"},
    {SkillID::NV_TRADE, "TRADE", "Trade_Skill"},
    {SkillID::NV_PARTY, "PARTY", "Party_Skill"},

    {SkillID::TMW_MAGIC, "MAGIC", "General Magic"},
    {SkillID::TMW_MAGIC_LIFE, "MAGIC_LIFE", "Life Magic"},
    {SkillID::TMW_MAGIC_WAR, "MAGIC_WAR", "War Magic"},
    {SkillID::TMW_MAGIC_TRANSMUTE, "MAGIC_TRANSMUTE", "Transmutation Magic"},
    {SkillID::TMW_MAGIC_NATURE, "MAGIC_NATURE", "Nature Magic"},
    {SkillID::TMW_MAGIC_ETHER, "MAGIC_ETHER", "Astral Magic"},
    {SkillID::TMW_MAGIC_DARK, "MAGIC_DARK", "Dark Magic"},
    {SkillID::TMW_MAGIC_LIGHT, "MAGIC_LIGHT", "Light Magic"},

    {SkillID::TMW_BRAWLING, "BRAWLING", "Brawling"},
    {SkillID::TMW_LUCKY_COUNTER, "LUCKY_COUNTER", "Lucky Counter"},
    {SkillID::TMW_SPEED, "SPEED", "Speed"},
    {SkillID::TMW_RESIST_POISON, "RESIST_POISON", "Resist Poison"},
    {SkillID::TMW_ASTRAL_SOUL, "ASTRAL_SOUL", "Astral Soul"},
    {SkillID::TMW_RAGING, "RAGING", "Raging"},

    {SkillID::ZERO, nullptr, nullptr}
};

earray<struct skill_db, SkillID, SkillID::MAX_SKILL_DB> skill_db;


static
int skill_attack(BF attack_type, struct block_list *src,
        struct block_list *dsrc, struct block_list *bl,
        SkillID skillid, int skilllv, tick_t tick, BCT flag);
static
void skill_devotion_end(struct map_session_data *md,
        struct map_session_data *sd, int target);
static
void skill_status_change_timer(TimerData *tid, tick_t tick,
        int id, StatusChange type);

int skill_get_hit(SkillID id)
{
    return skill_db[id].hit;
}

int skill_get_inf(SkillID id)
{
    return skill_db[id].inf;
}

int skill_get_nk(SkillID id)
{
    return skill_db[id].nk;
}

int skill_get_max(SkillID id)
{
    return skill_db[id].max;
}

int skill_get_max_raise(SkillID id)
{
    return skill_db[id].max_raise;
}

int skill_get_range(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].range[lv - 1];
}

int skill_get_sp(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].sp[lv - 1];
}

int skill_get_num(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].num[lv - 1];
}

int skill_get_cast(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].cast[lv - 1];
}

int skill_get_delay(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].delay[lv - 1];
}

int skill_get_inf2(SkillID id)
{
    return skill_db[id].inf2;
}

int skill_get_maxcount(SkillID id)
{
    return skill_db[id].maxcount;
}

static
int skill_get_castnodex(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].castnodex[lv - 1];
}

static
int distance(int x0, int y0, int x1, int y1)
{
    int dx, dy;

    dx = abs(x0 - x1);
    dy = abs(y0 - y1);
    return dx > dy ? dx : dy;
}

/*==========================================
 * スキル追加効果
 *------------------------------------------
 */
int skill_additional_effect(struct block_list *src, struct block_list *bl,
        SkillID skillid, int skilllv)
{
    struct map_session_data *sd = NULL;
    struct mob_data *md = NULL;

    int luk;

    int sc_def_mdef, sc_def_vit, sc_def_int, sc_def_luk;
    int sc_def_phys_shield_spell;

    nullpo_ret(src);
    nullpo_ret(bl);

    if (skilllv < 0)
        return 0;

    if (src->type == BL::PC)
    {
        sd = (struct map_session_data *) src;
        nullpo_ret(sd);
    }
    else if (src->type == BL::MOB)
    {
        md = (struct mob_data *) src;
        nullpo_ret(md);  //未使用？
    }

    sc_def_phys_shield_spell = 0;
    if (battle_get_sc_data(bl)[StatusChange::SC_PHYS_SHIELD].timer)
        sc_def_phys_shield_spell =
            battle_get_sc_data(bl)[StatusChange::SC_PHYS_SHIELD].val1;

    //対象の耐性
    luk = battle_get_luk(bl);
    sc_def_mdef = 100 - (3 + battle_get_mdef(bl) + luk / 3);
    sc_def_vit = 100 - (3 + battle_get_vit(bl) + luk / 3);
    sc_def_int = 100 - (3 + battle_get_int(bl) + luk / 3);
    sc_def_luk = 100 - (3 + luk);
    //自分の耐性
    luk = battle_get_luk(src);

    if (bl->type == BL::MOB)
    {
        if (sc_def_mdef > 50)
            sc_def_mdef = 50;
        if (sc_def_vit > 50)
            sc_def_vit = 50;
        if (sc_def_int > 50)
            sc_def_int = 50;
        if (sc_def_luk > 50)
            sc_def_luk = 50;
    }
    if (sc_def_mdef < 0)
        sc_def_mdef = 0;
    if (sc_def_vit < 0)
        sc_def_vit = 0;
    if (sc_def_int < 0)
        sc_def_int = 0;

    switch (skillid)
    {
        case SkillID::NPC_POISON:
            // blame Fate for this
            if (random_::chance({50 - (sc_def_vit >> 2) - (sc_def_phys_shield_spell) + (skilllv >> 2), 100}))
                skill_status_change_start(bl, StatusChange::SC_POISON, skilllv, static_cast<interval_t>(skilllv));
            break;
    }

    return 0;
}

/*
 * =========================================================================
 * スキル攻撃効果処理まとめ
 * flagの説明。16進図
 *      00XRTTff
 *  ff  = magicで計算に渡される）
 *      TT      = パケットのtype部分(0でデフォルト）
 *  X   = パケットのスキルLv
 *  R   = 予約（skill_area_subで使用する）
 *-------------------------------------------------------------------------
 */

int skill_attack(BF attack_type, struct block_list *src,
        struct block_list *dsrc, struct block_list *bl,
        SkillID skillid, int skilllv, tick_t tick, BCT flag)
{
    struct Damage dmg;
    eptr<struct status_change, StatusChange> sc_data;
    int type, lv, damage;

    nullpo_ret(src);
    nullpo_ret(dsrc);
    nullpo_ret(bl);

    sc_data = battle_get_sc_data(bl);

//何もしない判定ここから
    if (dsrc->m != bl->m)       //対象が同じマップにいなければ何もしない
        return 0;
    if (src->prev == NULL || dsrc->prev == NULL || bl->prev == NULL)    //prevよくわからない※
        return 0;
    if (src->type == BL::PC && pc_isdead((struct map_session_data *) src))  //術者？がPCですでに死んでいたら何もしない
        return 0;
    if (dsrc->type == BL::PC && pc_isdead((struct map_session_data *) dsrc))    //術者？がPCですでに死んでいたら何もしない
        return 0;
    if (bl->type == BL::PC && pc_isdead((struct map_session_data *) bl))    //対象がPCですでに死んでいたら何もしない
        return 0;

//何もしない判定ここまで

    type = -1;
    lv = flag.level;
    dmg = battle_calc_attack(attack_type, src, bl, skillid, skilllv, flag.lo); //ダメージ計算

    damage = dmg.damage + dmg.damage2;

    if (lv == 15)
        lv = -1;

    if (flag.mid)
        type = flag.mid;

    switch (skillid)
    {
        case SkillID::NPC_SELFDESTRUCTION:
            break;
        default:
            clif_skill_damage(dsrc, bl, tick, dmg.amotion, dmg.dmotion,
                               damage, dmg.div_, skillid,
                               (lv != 0) ? lv : skilllv,
                               (skillid == SkillID::ZERO) ? 5 : type);
    }

    MapBlockLock lock;
    /* 実際にダメージ処理を行う */
    battle_damage(src, bl, damage, 0);

    /* ダメージがあるなら追加効果判定 */
    if (bl->prev != NULL)
    {
        struct map_session_data *sd = (struct map_session_data *) bl;
        nullpo_ret(sd);
        if (bl->type != BL::PC || (sd && !pc_isdead(sd)))
        {
            if (damage > 0)
                skill_additional_effect(src, bl, skillid, skilllv);
            if (bl->type == BL::MOB && src != bl)    /* スキル使用条件のMOBスキル */
            {
                struct mob_data *md = (struct mob_data *) bl;
                nullpo_ret(md);
                if (battle_config.mob_changetarget_byskill == 1)
                {
                    int target;
                    target = md->target_id;
                    if (src->type == BL::PC)
                        md->target_id = src->id;
                    mobskill_use(md, tick, MobSkillCondition::ANY);
                    md->target_id = target;
                }
                else
                    mobskill_use(md, tick, MobSkillCondition::ANY);
            }
        }
    }

    if (src->type == BL::PC
        && bool(dmg.flag & BF::WEAPON)
        && src != bl
        && src == dsrc
        && damage > 0)
    {
        struct map_session_data *sd = (struct map_session_data *) src;
        int hp = 0, sp = 0;
        nullpo_ret(sd);
        if (sd->hp_drain_rate && dmg.damage > 0
            && random_::chance({sd->hp_drain_rate, 100}))
        {
            hp += (dmg.damage * sd->hp_drain_per) / 100;
        }
        if (sd->hp_drain_rate_ && dmg.damage2 > 0
            && random_::chance({sd->hp_drain_rate_, 100}))
        {
            hp += (dmg.damage2 * sd->hp_drain_per_) / 100;
        }
        if (sd->sp_drain_rate > 0 && dmg.damage > 0
            && random_::chance({sd->sp_drain_rate, 100}))
        {
            sp += (dmg.damage * sd->sp_drain_per) / 100;
        }
        if (sd->sp_drain_rate_ > 0 && dmg.damage2 > 0
            && random_::chance({sd->sp_drain_rate_, 100}))
        {
            sp += (dmg.damage2 * sd->sp_drain_per_) / 100;
        }
        if (hp || sp)
            pc_heal(sd, hp, sp);
    }

    return (dmg.damage + dmg.damage2);  /* 与ダメを返す */
}

typedef int(*SkillFunc)(struct block_list *, struct block_list *,
        SkillID, int,
        tick_t, BCT);

static
void skill_area_sub(struct block_list *bl,
        struct block_list *src, SkillID skill_id, int skill_lv,
        tick_t tick, BCT flag, SkillFunc func)
{
    nullpo_retv(bl);

    if (bl->type != BL::PC && bl->type != BL::MOB)
        return;

    if (battle_check_target(src, bl, flag) > 0)
        func(src, bl, skill_id, skill_lv, tick, flag);
}


/* 範囲スキル使用処理小分けここまで
 * -------------------------------------------------------------------------
 */

// these variables are set in the 'else' branches,
// and used in the (recursive) 'if' branch
static int skill_area_temp_id, skill_area_temp_hp;


/*==========================================
 * スキル使用（詠唱完了、ID指定攻撃系）
 * （スパゲッティに向けて１歩前進！(ダメポ)）
 *------------------------------------------
 */
int skill_castend_damage_id(struct block_list *src, struct block_list *bl,
        SkillID skillid, int skilllv,
        tick_t tick, BCT flag)
{
    struct map_session_data *sd = NULL;

    nullpo_retr(1, src);
    nullpo_retr(1, bl);

    if (src->type == BL::PC)
        sd = (struct map_session_data *) src;
    if (sd && pc_isdead(sd))
        return 1;

    if (bl->prev == NULL)
        return 1;
    if (bl->type == BL::PC && pc_isdead((struct map_session_data *) bl))
        return 1;

    MapBlockLock lock;
    switch (skillid)
    {
        case SkillID::NPC_POISON:
            skill_attack(BF::WEAPON, src, src, bl, skillid, skilllv, tick,
                          flag);
            break;

        case SkillID::NPC_SELFDESTRUCTION:  /* 自爆 */
            if (flag.lo & 1)
            {
                /* 個別にダメージを与える */
                if (src->type == BL::MOB)
                {
                    struct mob_data *mb = (struct mob_data *) src;
                    nullpo_retr(1, mb);
                    mb->hp = skill_area_temp_hp;
                    if (bl->id != skill_area_temp_id)
                        skill_attack(BF::MISC, src, src, bl,
                                      SkillID::NPC_SELFDESTRUCTION, skilllv, tick,
                                      flag);
                    mb->hp = 1;
                }
            }
            else
            {
                struct mob_data *md;
                if ((md = (struct mob_data *) src))
                {
                    skill_area_temp_id = bl->id;
                    skill_area_temp_hp = battle_get_hp(src);
                    map_foreachinarea(std::bind(skill_area_sub, ph::_1, src, skillid, skilllv,
                                tick, flag | BCT_ENEMY | BCT_lo_x01, skill_castend_damage_id),
                            bl->m, bl->x - 5, bl->y - 5,
                            bl->x + 5, bl->y + 5, BL::NUL);
                    battle_damage(src, src, md->hp, 0);
                }
            }
            break;

            /* HP吸収/HP吸収魔法 */
        case SkillID::ZERO:
            if (sd)
            {
                if (flag.lo & 3)
                {
                    if (bl->id != skill_area_temp_id)
                        skill_attack(BF::WEAPON, src, src, bl, skillid,
                                      skilllv, tick, BCT_mid_x05);
                }
                else
                {
                    // TODO does this happen?
                    skill_area_temp_id = bl->id;
                    map_foreachinarea(std::bind(skill_area_sub, ph::_1, src, skillid, skilllv,
                                tick, flag | BCT_ENEMY | BCT_lo_x01, skill_castend_damage_id),
                            bl->m, bl->x - 0, bl->y - 0,
                            bl->x + 0, bl->y + 0, BL::NUL);
                }
            }
            break;

        default:
            return 1;
    }

    return 0;
}

/*==========================================
 * スキル使用（詠唱完了、ID指定支援系）
 *------------------------------------------
 */
// skillid.nk == 1
// so skillid in (SkillID::NPC_SUMMONSLAVE, SkillID::NPC_EMOTION)
int skill_castend_nodamage_id(struct block_list *src, struct block_list *bl,
        SkillID skillid, int skilllv)
{
    struct map_session_data *sd = NULL;
    struct map_session_data *dstsd = NULL;
    struct mob_data *md = NULL;
    struct mob_data *dstmd = NULL;
    int sc_def_vit, sc_def_mdef, strip_fix;

    nullpo_retr(1, src);
    nullpo_retr(1, bl);

    if (src->type == BL::PC)
        sd = (struct map_session_data *) src;
    else if (src->type == BL::MOB)
        md = (struct mob_data *) src;

    sc_def_vit = 100 - (3 + battle_get_vit(bl) + battle_get_luk(bl) / 3);
    sc_def_vit = 100 - (3 + battle_get_vit(bl) + battle_get_luk(bl) / 3);
    sc_def_mdef = 100 - (3 + battle_get_mdef(bl) + battle_get_luk(bl) / 3);
    strip_fix = battle_get_dex(src) - battle_get_dex(bl);

    if (bl->type == BL::PC)
    {
        dstsd = (struct map_session_data *) bl;
        nullpo_retr(1, dstsd);
    }
    else if (bl->type == BL::MOB)
    {
        dstmd = (struct mob_data *) bl;
        nullpo_retr(1, dstmd);
        if (sc_def_vit > 50)
            sc_def_vit = 50;
        if (sc_def_mdef > 50)
            sc_def_mdef = 50;
    }
    if (sc_def_vit < 0)
        sc_def_vit = 0;
    if (sc_def_mdef < 0)
        sc_def_mdef = 0;
    if (strip_fix < 0)
        strip_fix = 0;

    if (bl == NULL || bl->prev == NULL)
        return 1;
    if (sd && pc_isdead(sd))
        return 1;
    if (dstsd && pc_isdead(dstsd))
        return 1;

    MapBlockLock lock;
    switch (skillid)
    {
        case SkillID::NPC_SUMMONSLAVE:
            if (md && !md->master_id)
            {
                mob_summonslave(md,
                        mob_db[md->mob_class].skill[md->skillidx].val,
                        skilllv,
                        1);
            }
            break;

        case SkillID::NPC_EMOTION:
            if (md)
                clif_emotion(&md->bl,
                        mob_db[md->mob_class].skill[md->skillidx].val[0]);
            break;
    }

    return 0;
}

/*==========================================
 * 詠唱時間計算
 *------------------------------------------
 */
interval_t skill_castfix(struct block_list *bl, interval_t interval)
{
    struct mob_data *md;        // [Valaris]
    eptr<struct status_change, StatusChange> sc_data;
    int dex;
    int castrate = 100;
    SkillID skill;
    int lv, castnodex;

    nullpo_retr(interval_t::zero(), bl);

    if (bl->type == BL::MOB)
    {                           // Crash fix [Valaris]
        md = (struct mob_data *) bl;
        skill = md->skillid;
        lv = md->skilllv;
    }
    else
    {
        skill = SkillID::ZERO;
        lv = 0;
    }

    sc_data = battle_get_sc_data(bl);
    dex = battle_get_dex(bl);

    if (skill > SkillID::MAX_SKILL_DB /*|| skill < SkillID()*/)
        return interval_t::zero();

    castnodex = skill_get_castnodex(skill, lv);

    if (interval == interval_t::zero())
        return interval_t::zero();
    if (castnodex > 0 && bl->type == BL::PC)
        castrate = 100;
    else if (castnodex <= 0 && bl->type == BL::PC)
    {
        castrate = 100;
        interval =
            interval * castrate * (battle_config.castrate_dex_scale -
                               dex) / (battle_config.castrate_dex_scale *
                                       100);
        interval = interval * battle_config.cast_rate / 100;
    }

    return std::max(interval, interval_t::zero());
}

/*==========================================
 * ディレイ計算
 *------------------------------------------
 */
interval_t skill_delayfix(struct block_list *bl, interval_t interval)
{
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_retr(interval_t::zero(), bl);

    sc_data = battle_get_sc_data(bl);
    if (interval <= interval_t::zero())
        return interval_t::zero();

    if (bl->type == BL::PC)
    {
        if (battle_config.delay_dependon_dex)   /* dexの影響を計算する */
            interval =
                interval * (battle_config.castrate_dex_scale -
                        battle_get_dex(bl)) /
                battle_config.castrate_dex_scale;
        interval = interval * battle_config.delay_rate / 100;
    }

    return std::max(interval, interval_t::zero());
}

/*==========================================
 * スキル詠唱キャンセル
 *------------------------------------------
 */
int skill_castcancel(struct block_list *bl, int)
{
    nullpo_ret(bl);

    if (bl->type == BL::PC)
    {
        struct map_session_data *sd = (struct map_session_data *) bl;
        tick_t tick = gettick();
        sd->canact_tick = tick;
        sd->canmove_tick = tick;

        return 0;
    }
    else if (bl->type == BL::MOB)
    {
        struct mob_data *md = (struct mob_data *) bl;
        if (md->skilltimer)
        {
            md->skilltimer.cancel();
            clif_skillcastcancel(bl);
        }
        return 0;
    }
    return 1;
}

/*==========================================
 * ディボーション 有効確認
 *------------------------------------------
 */
void skill_devotion(struct map_session_data *md, int)
{
    // 総確認
    int n;

    nullpo_retv(md);

    for (n = 0; n < 5; n++)
    {
        if (md->dev.val1[n])
        {
            struct map_session_data *sd = map_id2sd(md->dev.val1[n]);
            // 相手が見つからない // 相手をディボしてるのが自分じゃない // 距離が離れてる
            if (sd == NULL
                || (md->bl.id != 0/* was something else - TODO remove this */)
                || skill_devotion3(&md->bl, md->dev.val1[n]))
            {
                skill_devotion_end(md, sd, n);
            }
        }
    }
}

int skill_devotion3(struct block_list *bl, int target)
{
    // クルセが歩いた時の距離チェック
    struct map_session_data *md;
    struct map_session_data *sd;
    int n, r = 0;

    nullpo_retr(1, bl);

    if ((md = (struct map_session_data *) bl) == NULL
        || (sd = map_id2sd(target)) == NULL)
        return 1;
    else
        r = distance(bl->x, bl->y, sd->bl.x, sd->bl.y);

    if ( + 6 < r)
    {                           // 許容範囲を超えてた
        for (n = 0; n < 5; n++)
            if (md->dev.val1[n] == target)
                md->dev.val2[n] = 0;    // 離れた時は、糸を切るだけ
        return 1;
    }
    return 0;
}

void skill_devotion_end(struct map_session_data *md,
                         struct map_session_data *, int target)
{
    // クルセと被ディボキャラのリセット
    nullpo_retv(md);

    md->dev.val1[target] = md->dev.val2[target] = 0;
}

int skill_gangsterparadise(struct map_session_data *, int)
{
    return 0;
}

/*----------------------------------------------------------------------------
 * ステータス異常
 *----------------------------------------------------------------------------
 */

/*==========================================
 * ステータス異常終了
 *------------------------------------------
 */
int skill_status_change_active(struct block_list *bl, StatusChange type)
{
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    if (bl->type != BL::PC && bl->type != BL::MOB)
    {
        if (battle_config.error_log)
            PRINTF("skill_status_change_active: neither MOB nor PC !\n");
        return 0;
    }

    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return 0;

    return bool(sc_data[type].timer);
}

void skill_status_change_end(struct block_list *bl, StatusChange type, TimerData *tid)
{
    eptr<struct status_change, StatusChange> sc_data;
    int opt_flag = 0, calc_flag = 0;
    short *sc_count;
    Option *option;
    Opt1 *opt1;
    Opt2 *opt2;
    Opt3 *opt3;

    nullpo_retv(bl);
    if (bl->type != BL::PC && bl->type != BL::MOB)
    {
        if (battle_config.error_log)
            PRINTF("skill_status_change_end: neither MOB nor PC !\n");
        return;
    }
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return;
    sc_count = battle_get_sc_count(bl);
    nullpo_retv(sc_count);
    option = battle_get_option(bl);
    nullpo_retv(option);
    opt1 = battle_get_opt1(bl);
    nullpo_retv(opt1);
    opt2 = battle_get_opt2(bl);
    nullpo_retv(opt2);
    opt3 = battle_get_opt3(bl);
    nullpo_retv(opt3);

    // status_change_end can be called 2 ways: automatically by a timer,
    // or manually to cancel it.
    if (!tid) // if this is a cancel
    {
        // and it was not active
        if (!sc_data[type].timer)
            // there's nothing to do
            return;
        // if it was active, cancel it
        sc_data[type].timer.cancel();
    }
    // whether we are the timer or a cancel no longer matters

    assert (!sc_data[type].timer);
    assert ((*sc_count) > 0);
    (*sc_count)--;

    switch (type)
    {                       /* 異常の種類ごとの処理 */
        case StatusChange::SC_SPEEDPOTION0:  /* 増速ポーション */
        case StatusChange::SC_ATKPOT:    /* attack potion [Valaris] */
        case StatusChange::SC_MATKPOT:   /* magic attack potion [Valaris] */
        case StatusChange::SC_PHYS_SHIELD:
        case StatusChange::SC_HASTE:
            calc_flag = 1;
            break;

            /* option2 */
        case StatusChange::SC_POISON:    /* 毒 */
            calc_flag = 1;
            break;
    }

    if (bl->type == BL::PC && type < StatusChange::SC_SENDMAX)
        clif_status_change(bl, type, 0);   /* アイコン消去 */

    switch (type)
    {
    case StatusChange::SC_POISON:
        *opt2 &= ~Opt2::_poison;
        opt_flag = 1;
        break;

    case StatusChange::SC_SLOWPOISON:
        if (sc_data[StatusChange::SC_POISON].timer)
            *opt2 |= Opt2::_poison;
        *opt2 &= ~Opt2::_slowpoison;
        opt_flag = 1;
        break;

    case StatusChange::SC_SPEEDPOTION0:
        *opt2 &= ~Opt2::_speedpotion0;
        opt_flag = 1;
        break;

    case StatusChange::SC_ATKPOT:
        *opt2 &= ~Opt2::_atkpot;
        opt_flag = 1;
        break;
    }

    if (opt_flag)           /* optionの変更を伝える */
        clif_changeoption(bl);

    if (bl->type == BL::PC && calc_flag)
        pc_calcstatus((struct map_session_data *) bl, 0);  /* ステータス再計算 */
}

int skill_update_heal_animation(struct map_session_data *sd)
{
    const Opt2 mask = Opt2::_heal;

    nullpo_ret(sd);
    bool was_active = bool(sd->opt2 & mask);
    bool is_active = sd->quick_regeneration_hp.amount > 0;

    if (was_active == is_active)
        return 0;               // no update

    if (is_active)
        sd->opt2 |= mask;
    else
        sd->opt2 &= ~mask;

    return clif_changeoption(&sd->bl);
}

/*==========================================
 * ステータス異常終了タイマー
 *------------------------------------------
 */
void skill_status_change_timer(TimerData *tid, tick_t tick, int id, StatusChange type)
{
    struct block_list *bl;
    struct map_session_data *sd = NULL;
    eptr<struct status_change, StatusChange> sc_data;
    //short *sc_count; //使ってない？

    if ((bl = map_id2bl(id)) == NULL)
        return;
    //該当IDがすでに消滅しているというのはいかにもありそうなのでスルーしてみる
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return;

    if (bl->type == BL::PC)
        sd = (struct map_session_data *) bl;

    //sc_count=battle_get_sc_count(bl); //使ってない？

    if (sc_data[type].spell_invocation)
    {                           // Must report termination
        spell_effect_report_termination(sc_data[type].spell_invocation,
                                         bl->id, type, 0);
        sc_data[type].spell_invocation = 0;
    }

    switch (type)
    {
        case StatusChange::SC_POISON:
            if (!sc_data[StatusChange::SC_SLOWPOISON].timer)
            {
                const int resist_poison =
                    skill_power_bl(bl, SkillID::TMW_RESIST_POISON) >> 3;
                if (resist_poison)
                    sc_data[type].val1 -= random_::in(0, resist_poison);

                if ((--sc_data[type].val1) > 0)
                {

                    int hp = battle_get_max_hp(bl);
                    if (battle_get_hp(bl) > hp >> 4)
                    {
                        if (bl->type == BL::PC)
                        {
                            hp = 3 + hp * 3 / 200;
                            pc_heal((struct map_session_data *) bl, -hp, 0);
                        }
                        else if (bl->type == BL::MOB)
                        {
                            struct mob_data *md;
                            if ((md = ((struct mob_data *) bl)) == NULL)
                                break;
                            hp = 3 + hp / 200;
                            md->hp -= hp;
                        }
                    }
                    sc_data[type].timer = Timer(tick + std::chrono::seconds(1),
                            std::bind(skill_status_change_timer, ph::_1, ph::_2,
                                bl->id, type));
                    return;
                }
            }
            else
            {
                sc_data[type].timer = Timer(tick + std::chrono::seconds(2),
                        std::bind(skill_status_change_timer, ph::_1, ph::_2,
                            bl->id, type));
                return;
            }
            break;
            // If you manually reschedule the timer, you MUST skip the
            // call to skill_status_change_end below.

            /* 時間切れ無し？？ */
        case StatusChange::SC_WEIGHT50:
        case StatusChange::SC_WEIGHT90:
        case StatusChange::SC_BROKNWEAPON:
        case StatusChange::SC_BROKNARMOR:
            sc_data[type].timer = Timer(tick + std::chrono::minutes(10),
                    std::bind(skill_status_change_timer, ph::_1, ph::_2,
                        bl->id, type));
            return;

        case StatusChange::SC_FLYING_BACKPACK:
            clif_updatestatus(sd, SP::WEIGHT);
            break;

    }

    skill_status_change_end(bl, type, tid);
}

/*==========================================
 * ステータス異常開始
 *------------------------------------------
 */
int skill_status_change_start(struct block_list *bl, StatusChange type,
        int val1,
        interval_t tick)
{
    return skill_status_effect(bl, type, val1, tick, 0);
}

int skill_status_effect(struct block_list *bl, StatusChange type,
        int val1,
        interval_t tick, int spell_invocation)
{
    struct map_session_data *sd = NULL;
    eptr<struct status_change, StatusChange> sc_data;
    short *sc_count;
    Option *option;
    Opt1 *opt1;
    Opt2 *opt2;
    Opt3 *opt3;
    int opt_flag = 0, calc_flag = 0;
    SP updateflag = SP::ZERO;
    int scdef = 0;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return 0;
    sc_count = battle_get_sc_count(bl);
    nullpo_ret(sc_count);
    option = battle_get_option(bl);
    nullpo_ret(option);
    opt1 = battle_get_opt1(bl);
    nullpo_ret(opt1);
    opt2 = battle_get_opt2(bl);
    nullpo_ret(opt2);
    opt3 = battle_get_opt3(bl);
    nullpo_ret(opt3);

    switch (type)
    {
        case StatusChange::SC_POISON:
            scdef = 3 + battle_get_vit(bl) + battle_get_luk(bl) / 3;
            break;
    }
    if (scdef >= 100)
        return 0;
    if (bl->type == BL::PC)
    {
        sd = (struct map_session_data *) bl;
    }
    else if (bl->type == BL::MOB)
    {
    }
    else
    {
        if (battle_config.error_log)
            PRINTF("skill_status_change_start: neither MOB nor PC !\n");
        return 0;
    }

    if (sc_data[type].timer)
    {                           /* すでに同じ異常になっている場合タイマ解除 */
        if (sc_data[type].val1 > val1
            && type != StatusChange::SC_SPEEDPOTION0
            && type != StatusChange::SC_ATKPOT
            && type != StatusChange::SC_MATKPOT) // added atk and matk potions [Valaris]
            return 0;
        if (type == StatusChange::SC_POISON)
            return 0;

        /* 継ぎ足しができない状態異常である時は状態異常を行わない */
        {
            (*sc_count)--;
            sc_data[type].timer.cancel();
        }
    }

    switch (type)
    {
        /* 異常の種類ごとの処理 */
        case StatusChange::SC_SLOWPOISON:
            if (!sc_data[StatusChange::SC_POISON].timer)
                return 0;
            break;

        case StatusChange::SC_SPEEDPOTION0:  /* 増速ポーション */
            *opt2 |= Opt2::_speedpotion0;
            calc_flag = 1;
//          val2 = 5*(2+type-StatusChange::SC_SPEEDPOTION0);
            break;

            /* atk & matk potions [Valaris] */
        case StatusChange::SC_ATKPOT:
            *opt2 |= Opt2::_atkpot;
            FALLTHROUGH;
        case StatusChange::SC_MATKPOT:
            calc_flag = 1;
            break;

            /* option2 */
        case StatusChange::SC_POISON:        /* 毒 */
            calc_flag = 1;
            {
                int sc_def =
                    100 - (battle_get_vit(bl) + battle_get_luk(bl) / 5);
                tick = tick * sc_def / 100;
            }

            // huh?
            tick = std::chrono::seconds(1);
            break;

        case StatusChange::SC_WEIGHT50:
        case StatusChange::SC_WEIGHT90:
        case StatusChange::SC_BROKNWEAPON:
        case StatusChange::SC_BROKNARMOR:
            tick = std::chrono::minutes(10);
            break;

        case StatusChange::SC_HASTE:
            calc_flag = 1;
            break;
        case StatusChange::SC_PHYS_SHIELD:
        case StatusChange::SC_MBARRIER:
        case StatusChange::SC_HALT_REGENERATE:
        case StatusChange::SC_HIDE:
            break;
        case StatusChange::SC_FLYING_BACKPACK:
            updateflag = SP::WEIGHT;
            break;
        default:
            if (battle_config.error_log)
                PRINTF("UnknownStatusChange [%d]\n", type);
            return 0;
    }

    if (bl->type == BL::PC && type < StatusChange::SC_SENDMAX)
        clif_status_change(bl, type, 1);   /* アイコン表示 */

    /* optionの変更 */
    switch (type)
    {
        case StatusChange::SC_POISON:
            if (!sc_data[StatusChange::SC_SLOWPOISON].timer)
            {
                *opt2 |= Opt2::_poison;
                opt_flag = 1;
            }
            break;

        case StatusChange::SC_SLOWPOISON:
            *opt2 &= ~Opt2::_poison;
            *opt2 |= Opt2::_slowpoison;
            opt_flag = 1;
            break;
    }

    if (opt_flag)               /* optionの変更 */
        clif_changeoption(bl);

    (*sc_count)++;              /* ステータス異常の数 */

    sc_data[type].val1 = val1;
    if (sc_data[type].spell_invocation) // Supplant by newer spell
        spell_effect_report_termination(sc_data[type].spell_invocation,
                                         bl->id, type, 1);

    sc_data[type].spell_invocation = spell_invocation;

    /* タイマー設定 */
    sc_data[type].timer = Timer(gettick() + tick,
            std::bind(skill_status_change_timer, ph::_1, ph::_2,
                bl->id, type));

    if (bl->type == BL::PC && calc_flag)
        pc_calcstatus(sd, 0);  /* ステータス再計算 */

    if (bl->type == BL::PC && updateflag != SP::ZERO)
        clif_updatestatus(sd, updateflag); /* ステータスをクライアントに送る */

    return 0;
}

/*==========================================
 * ステータス異常全解除
 *------------------------------------------
 */
int skill_status_change_clear(struct block_list *bl, int type)
{
    eptr<struct status_change, StatusChange> sc_data;
    short *sc_count;
    Option *option;
    Opt1 *opt1;
    Opt2 *opt2;
    Opt3 *opt3;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return 0;
    sc_count = battle_get_sc_count(bl);
    nullpo_ret(sc_count);
    option = battle_get_option(bl);
    nullpo_ret(option);
    opt1 = battle_get_opt1(bl);
    nullpo_ret(opt1);
    opt2 = battle_get_opt2(bl);
    nullpo_ret(opt2);
    opt3 = battle_get_opt3(bl);
    nullpo_ret(opt3);

    if (*sc_count == 0)
        return 0;
    for (StatusChange i : erange(StatusChange(), StatusChange::MAX_STATUSCHANGE))
    {
        if (sc_data[i].timer)
            skill_status_change_end(bl, i, nullptr);
    }
    *sc_count = 0;
    *opt1 = Opt1::ZERO;
    *opt2 = Opt2::ZERO;
    *opt3 = Opt3::ZERO;
    *option = Option::ZERO;

    if (type == 0 || type & 2)
        clif_changeoption(bl);

    return 0;
}

/*
 *----------------------------------------------------------------------------
 * スキルユニット
 *----------------------------------------------------------------------------
 */

/*==========================================
 * 演奏/ダンスをやめる
 * flag 1で合奏中なら相方にユニットを任せる
 *
 *------------------------------------------
 */
void skill_stop_dancing(struct block_list *, int)
{
    // TODO remove this
}

void skill_unit_timer_sub_ondelete(struct block_list *bl,
        struct block_list *src, unsigned int tick);

/*----------------------------------------------------------------------------
 * アイテム合成
 *----------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------
 * 初期化系
 */

static
SP scan_stat(char *statname)
{
    if (!strcasecmp(statname, "str"))
        return SP::STR;
    if (!strcasecmp(statname, "dex"))
        return SP::DEX;
    if (!strcasecmp(statname, "agi"))
        return SP::AGI;
    if (!strcasecmp(statname, "vit"))
        return SP::VIT;
    if (!strcasecmp(statname, "int"))
        return SP::INT;
    if (!strcasecmp(statname, "luk"))
        return SP::LUK;
    if (!strcasecmp(statname, "none"))
        return SP::ZERO;

    FPRINTF(stderr, "Unknown stat `%s'\n", statname);
    return SP::ZERO;
}

/*==========================================
 * スキル関係ファイル読み込み
 * skill_db.txt スキルデータ
 * skill_cast_db.txt スキルの詠唱時間とディレイデータ
 *------------------------------------------
 */
static
int skill_readdb(void)
{
    int j, k;
    FILE *fp;
    char line[1024], *p;

    /* The main skill database */
    memset(&skill_db, 0, sizeof(skill_db));
    fp = fopen_("db/skill_db.txt", "r");
    if (fp == NULL)
    {
        PRINTF("can't read db/skill_db.txt\n");
        return 1;
    }
    while (fgets(line, 1020, fp))
    {
        char *split[50], *split2[MAX_SKILL_LEVEL];
        if (line[0] == '/' && line[1] == '/')
            continue;
        for (j = 0, p = line; j < 18 && p; j++)
        {
            while (*p == '\t' || *p == ' ')
                p++;
            split[j] = p;
            p = strchr(p, ',');
            if (p)
                *p++ = 0;
        }
        if (split[17] == NULL || j < 18)
        {
            FPRINTF(stderr, "Incomplete skill db data online (%d entries)\n",
                     j);
            continue;
        }

        SkillID i = SkillID(atoi(split[0]));
        if (/*i < SkillID() ||*/ i > SkillID::MAX_SKILL_DB)
            continue;

        memset(split2, 0, sizeof(split2));
        for (j = 0, p = split[1]; j < MAX_SKILL_LEVEL && p; j++)
        {
            split2[j] = p;
            p = strchr(p, ':');
            if (p)
                *p++ = 0;
        }
        for (k = 0; k < MAX_SKILL_LEVEL; k++)
            skill_db[i].range[k] =
                (split2[k]) ? atoi(split2[k]) : atoi(split2[0]);
        skill_db[i].hit = atoi(split[2]);
        skill_db[i].inf = atoi(split[3]);
        skill_db[i].pl = atoi(split[4]);
        skill_db[i].nk = atoi(split[5]);
        skill_db[i].max_raise = atoi(split[6]);
        skill_db[i].max = atoi(split[7]);

        memset(split2, 0, sizeof(split2));
        for (j = 0, p = split[8]; j < MAX_SKILL_LEVEL && p; j++)
        {
            split2[j] = p;
            p = strchr(p, ':');
            if (p)
                *p++ = 0;
        }
        for (k = 0; k < MAX_SKILL_LEVEL; k++)
            skill_db[i].num[k] =
                (split2[k]) ? atoi(split2[k]) : atoi(split2[0]);

        if (strcasecmp(split[9], "yes") == 0)
            skill_db[i].castcancel = 1;
        else
            skill_db[i].castcancel = 0;
        skill_db[i].cast_def_rate = atoi(split[10]);
        skill_db[i].inf2 = atoi(split[11]);
        skill_db[i].maxcount = atoi(split[12]);
        // split[13] was one of: BF::WEAPON, BF::MAGIC, BF::MISC, BF::ZERO
        memset(split2, 0, sizeof(split2));
        // split[14] was colon-separated blow counts.

        if (!strcasecmp(split[15], "passive"))
        {
            skill_pool_register(i);
            skill_db[i].poolflags = SkillFlags::POOL_FLAG;
        }
        else if (!strcasecmp(split[15], "active"))
        {
            skill_pool_register(i);
            skill_db[i].poolflags = SkillFlags::POOL_FLAG | SkillFlags::POOL_ACTIVE;
        }
        else
            skill_db[i].poolflags = SkillFlags::ZERO;

        skill_db[i].stat = scan_stat(split[16]);

        char *tmp = strdup(split[17]);
        {
            // replace "_" by " "
            char *s = tmp;
            while ((s = strchr(s, '_')))
                *s = ' ';
            if ((s = strchr(tmp, '\t'))
                || (s = strchr(tmp, ' '))
                || (s = strchr(tmp, '\n')))
                *s = '\000';
        }
        skill_lookup_by_id(i).desc = tmp;
    }
    fclose_(fp);
    PRINTF("read db/skill_db.txt done\n");

    return 0;
}

void skill_reload(void)
{
    /*
     *
     * <empty skill database>
     * <?>
     *
     */

    do_init_skill();
}

/*==========================================
 * スキル関係初期化処理
 *------------------------------------------
 */
int do_init_skill(void)
{
    skill_readdb();

    return 0;
}

constexpr size_t num_names = sizeof(skill_names) / sizeof(skill_names[0]);

skill_name_db& skill_lookup_by_id(SkillID id)
{
    for (skill_name_db& ner : skill_names)
        if (ner.id == id)
            return ner;
    return skill_names[num_names - 1];
}

skill_name_db& skill_lookup_by_name(const char *name)
{
    for (skill_name_db& ner : skill_names)
        if (!strcasecmp(name, ner.name) || !strcasecmp(name, ner.desc))
            return ner;
    return skill_names[num_names - 1];
}
