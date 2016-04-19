#include "skill.hpp"
//    skill.cpp - Old-style skills.
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

#include <cassert>

#include <algorithm>

#include "../compat/attr.hpp"
#include "../compat/fun.hpp"
#include "../compat/nullpo.hpp"

#include "../strings/mstring.hpp"
#include "../strings/rstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/literal.hpp"

#include "../generic/random.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/read.hpp"

#include "../net/timer.hpp"

#include "../mmo/cxxstdio_enums.hpp"
#include "../mmo/extract_enums.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "mob.hpp"
#include "pc.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
static
skill_name_db skill_names[] =
{
    {SkillID::AC_OWL, "OWL"_s, "Owl's_Eye"_s},

    {SkillID::NPC_EMOTION, "EMOTION"_s, "NPC_EMOTION"_s},
    {SkillID::NPC_POISON, "POISON"_s, "NPC_POISON"_s},
    {SkillID::NPC_SELFDESTRUCTION, "SELFDESTRUCTION"_s, "Kabooooom!"_s},
    {SkillID::NPC_SUMMONSLAVE, "SUMMONSLAVE"_s, "NPC_SUMMONSLAVE"_s},

    {SkillID::NV_EMOTE, "EMOTE"_s, "Emote_Skill"_s},
    {SkillID::NV_TRADE, "TRADE"_s, "Trade_Skill"_s},
    {SkillID::NV_PARTY, "PARTY"_s, "Party_Skill"_s},

    {SkillID::TMW_MAGIC, "MAGIC"_s, "General Magic"_s},
    {SkillID::TMW_MAGIC_LIFE, "MAGIC_LIFE"_s, "Life Magic"_s},
    {SkillID::TMW_MAGIC_WAR, "MAGIC_WAR"_s, "War Magic"_s},
    {SkillID::TMW_MAGIC_TRANSMUTE, "MAGIC_TRANSMUTE"_s, "Transmutation Magic"_s},
    {SkillID::TMW_MAGIC_NATURE, "MAGIC_NATURE"_s, "Nature Magic"_s},
    {SkillID::TMW_MAGIC_ETHER, "MAGIC_ETHER"_s, "Astral Magic"_s},
    {SkillID::TMW_MAGIC_DARK, "MAGIC_DARK"_s, "Dark Magic"_s},
    {SkillID::TMW_MAGIC_LIGHT, "MAGIC_LIGHT"_s, "Light Magic"_s},

    {SkillID::TMW_BRAWLING, "BRAWLING"_s, "Brawling"_s},
    {SkillID::TMW_LUCKY_COUNTER, "LUCKY_COUNTER"_s, "Lucky Counter"_s},
    {SkillID::TMW_SPEED, "SPEED"_s, "Speed"_s},
    {SkillID::TMW_RESIST_POISON, "RESIST_POISON"_s, "Resist Poison"_s},
    {SkillID::TMW_ASTRAL_SOUL, "ASTRAL_SOUL"_s, "Astral Soul"_s},
    {SkillID::TMW_RAGING, "RAGING"_s, "Raging"_s},

    {SkillID::ZERO, ""_s, ""_s}
};

static
int skill_attack(BF attack_type, dumb_ptr<block_list> src,
        dumb_ptr<block_list> dsrc, dumb_ptr<block_list> bl,
        SkillID skillid, int skilllv, tick_t tick, BCT flag);

static
void skill_status_change_timer(TimerData *tid, tick_t tick,
        BlockId id, StatusChange type);

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
    return (lv <= 0) ? 0 : skill_db[id].range_k;
}

int skill_get_sp(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].sp[lv - 1];
}

int skill_get_num(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].num_k;
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

/*==========================================
 * スキル追加効果
 *------------------------------------------
 */
int skill_additional_effect(dumb_ptr<block_list> src, dumb_ptr<block_list> bl,
        SkillID skillid, int skilllv)
{
    dumb_ptr<map_session_data> sd = nullptr;
    dumb_ptr<mob_data> md = nullptr;

    int luk;

    int sc_def_mdef, sc_def_vit, sc_def_int, sc_def_luk;
    int sc_def_phys_shield_spell;

    nullpo_retz(src);
    nullpo_retz(bl);

    if (skilllv < 0)
        return 0;

    if (src->bl_type == BL::PC)
    {
        sd = src->is_player();
    }
    else if (src->bl_type == BL::MOB)
    {
        md = src->is_mob();
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

    if (bl->bl_type == BL::MOB)
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

int skill_attack(BF attack_type, dumb_ptr<block_list> src,
        dumb_ptr<block_list> dsrc, dumb_ptr<block_list> bl,
        SkillID skillid, int skilllv, tick_t tick, BCT flag)
{
    struct Damage dmg;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    int type, lv, damage;

    nullpo_retz(src);
    nullpo_retz(dsrc);
    nullpo_retz(bl);

    sc_data = battle_get_sc_data(bl);

//何もしない判定ここから
    if (dsrc->bl_m != bl->bl_m)       //対象が同じマップにいなければ何もしない
        return 0;
    if (src->bl_prev == nullptr || dsrc->bl_prev == nullptr || bl->bl_prev == nullptr)    //prevよくわからない※
        return 0;
    if (src->bl_type == BL::PC && pc_isdead(src->is_player()))  //術者？がPCですでに死んでいたら何もしない
        return 0;
    if (dsrc->bl_type == BL::PC && pc_isdead(dsrc->is_player()))    //術者？がPCですでに死んでいたら何もしない
        return 0;
    if (bl->bl_type == BL::PC && pc_isdead(bl->is_player()))    //対象がPCですでに死んでいたら何もしない
        return 0;

//何もしない判定ここまで

    type = -1;
    lv = flag.level;
    dmg = battle_calc_attack(attack_type, src, bl, skillid, skilllv, flag.lo); //ダメージ計算

    damage = dmg.damage;

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
    if (bl->bl_prev != nullptr)
    {
        dumb_ptr<map_session_data> sd = bl->is_player();
        if (bl->bl_type != BL::PC || !pc_isdead(sd))
        {
            if (damage > 0)
                skill_additional_effect(src, bl, skillid, skilllv);
            if (bl->bl_type == BL::MOB && src != bl)    /* スキル使用条件のMOBスキル */
            {
                dumb_ptr<mob_data> md = bl->is_mob();
                if (battle_config.mob_changetarget_byskill == 1)
                {
                    BlockId target = md->target_id;
                    if (src->bl_type == BL::PC)
                        md->target_id = src->bl_id;
                    mobskill_use(md, tick, MobSkillCondition::ANY);
                    md->target_id = target;
                }
                else
                    mobskill_use(md, tick, MobSkillCondition::ANY);
            }
        }
    }

    if (src->bl_type == BL::PC
        && bool(dmg.flag & BF::WEAPON)
        && src != bl
        && src == dsrc
        && damage > 0)
    {
        dumb_ptr<map_session_data> sd = src->is_player();
        int hp = 0, sp = 0;
        if (sd->hp_drain_rate && dmg.damage > 0
            && random_::chance({sd->hp_drain_rate, 100}))
        {
            hp += (dmg.damage * sd->hp_drain_per) / 100;
        }
        if (sd->sp_drain_rate > 0 && dmg.damage > 0
            && random_::chance({sd->sp_drain_rate, 100}))
        {
            sp += (dmg.damage * sd->sp_drain_per) / 100;
        }
        if (hp || sp)
            pc_heal(sd, hp, sp);
    }

    return (dmg.damage);  /* 与ダメを返す */
}

typedef int(*SkillFunc)(dumb_ptr<block_list>, dumb_ptr<block_list>,
        SkillID, int,
        tick_t, BCT);

static
void skill_area_sub(dumb_ptr<block_list> bl,
        dumb_ptr<block_list> src, SkillID skill_id, int skill_lv,
        tick_t tick, BCT flag, SkillFunc func)
{
    nullpo_retv(bl);

    if (bl->bl_type != BL::PC && bl->bl_type != BL::MOB)
        return;

    if (battle_check_target(src, bl, flag) > 0)
        func(src, bl, skill_id, skill_lv, tick, flag);
}


/*==========================================
 * スキル使用（詠唱完了、ID指定攻撃系）
 * （スパゲッティに向けて１歩前進！(ダメポ)）
 *------------------------------------------
 */
int skill_castend_damage_id(dumb_ptr<block_list> src, dumb_ptr<block_list> bl,
        SkillID skillid, int skilllv,
        tick_t tick, BCT flag)
{
    dumb_ptr<map_session_data> sd = nullptr;

    nullpo_retr(1, src);
    nullpo_retr(1, bl);

    if (src->bl_type == BL::PC)
        sd = src->is_player();
    if (sd && pc_isdead(sd))
        return 1;

    if (bl->bl_prev == nullptr)
        return 1;
    if (bl->bl_type == BL::PC && pc_isdead(bl->is_player()))
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
                if (src->bl_type == BL::MOB)
                {
                    dumb_ptr<mob_data> mb = src->is_mob();
                    mb->hp = skill_area_temp_hp;
                    if (bl->bl_id != skill_area_temp_id)
                        skill_attack(BF::MISC, src, src, bl,
                                      SkillID::NPC_SELFDESTRUCTION, skilllv, tick,
                                      flag);
                    mb->hp = 1;
                }
            }
            else
            {
                dumb_ptr<mob_data> md = src->is_mob();
                {
                    skill_area_temp_id = bl->bl_id;
                    skill_area_temp_hp = battle_get_hp(src);
                    map_foreachinarea(std::bind(skill_area_sub, ph::_1, src, skillid, skilllv,
                                tick, flag | BCT_ENEMY | BCT_lo_x01, skill_castend_damage_id),
                            bl->bl_m,
                            bl->bl_x - 5, bl->bl_y - 5,
                            bl->bl_x + 5, bl->bl_y + 5,
                            BL::NUL);
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
                    if (bl->bl_id != skill_area_temp_id)
                        skill_attack(BF::WEAPON, src, src, bl, skillid,
                                      skilllv, tick, BCT_mid_x05);
                }
                else
                {
                    // TODO does this happen?
                    skill_area_temp_id = bl->bl_id;
                    map_foreachinarea(std::bind(skill_area_sub, ph::_1, src, skillid, skilllv,
                                tick, flag | BCT_ENEMY | BCT_lo_x01, skill_castend_damage_id),
                            bl->bl_m,
                            bl->bl_x - 0, bl->bl_y - 0,
                            bl->bl_x + 0, bl->bl_y + 0,
                            BL::NUL);
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
int skill_castend_nodamage_id(dumb_ptr<block_list> src, dumb_ptr<block_list> bl,
        SkillID skillid, int skilllv)
{
    dumb_ptr<map_session_data> sd = nullptr;
    dumb_ptr<map_session_data> dstsd = nullptr;
    dumb_ptr<mob_data> md = nullptr;
    dumb_ptr<mob_data> dstmd = nullptr;
    int sc_def_vit, sc_def_mdef, strip_fix;

    nullpo_retr(1, src);
    nullpo_retr(1, bl);

    if (src->bl_type == BL::PC)
        sd = src->is_player();
    else if (src->bl_type == BL::MOB)
        md = src->is_mob();

    sc_def_vit = 100 - (3 + battle_get_vit(bl) + battle_get_luk(bl) / 3);
    sc_def_vit = 100 - (3 + battle_get_vit(bl) + battle_get_luk(bl) / 3);
    sc_def_mdef = 100 - (3 + battle_get_mdef(bl) + battle_get_luk(bl) / 3);
    strip_fix = battle_get_dex(src) - battle_get_dex(bl);

    if (bl->bl_type == BL::PC)
    {
        dstsd = bl->is_player();
    }
    else if (bl->bl_type == BL::MOB)
    {
        dstmd = bl->is_mob();
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

    if (bl == nullptr || bl->bl_prev == nullptr)
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
                        md->skillidx->val,
                        skilllv,
                        1);
            }
            break;

        case SkillID::NPC_EMOTION:
            if (md)
                clif_emotion(md,
                        md->skillidx->val[0]);
            break;
    }

    return 0;
}

/*==========================================
 * 詠唱時間計算
 *------------------------------------------
 */
interval_t skill_castfix(dumb_ptr<block_list> bl, interval_t interval)
{
    dumb_ptr<mob_data> md;        // [Valaris]
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    int dex;
    int castrate = 100;
    SkillID skill;
    int lv, castnodex;

    nullpo_retr(interval_t::zero(), bl);

    if (bl->bl_type == BL::MOB)
    {                           // Crash fix [Valaris]
        md = bl->is_mob();
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

    if (skill >= SkillID::MAX_SKILL_DB /*|| skill < SkillID()*/)
        return interval_t::zero();

    castnodex = skill_get_castnodex(skill, lv);

    if (interval == interval_t::zero())
        return interval_t::zero();
    if (castnodex > 0 && bl->bl_type == BL::PC)
        castrate = 100;
    else if (castnodex <= 0 && bl->bl_type == BL::PC)
    {
        castrate = 100;
        interval =
            interval * castrate * (battle_config.castrate_dex_scale -
                               dex) / (battle_config.castrate_dex_scale *
                                       100);
        interval = interval * battle_config.casting_rate / 100;
    }

    return std::max(interval, interval_t::zero());
}

/*==========================================
 * ディレイ計算
 *------------------------------------------
 */
interval_t skill_delayfix(dumb_ptr<block_list> bl, interval_t interval)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retr(interval_t::zero(), bl);

    sc_data = battle_get_sc_data(bl);
    if (interval <= interval_t::zero())
        return interval_t::zero();

    if (bl->bl_type == BL::PC)
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
int skill_castcancel(dumb_ptr<block_list> bl, int)
{
    nullpo_retz(bl);

    if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = bl->is_player();
        tick_t tick = gettick();
        sd->canact_tick = tick;
        sd->canmove_tick = tick;

        return 0;
    }
    else if (bl->bl_type == BL::MOB)
    {
        dumb_ptr<mob_data> md = bl->is_mob();
        if (md->skilltimer)
        {
            md->skilltimer.cancel();
            clif_skillcastcancel(bl);
        }
        return 0;
    }
    return 1;
}

/*----------------------------------------------------------------------------
 * ステータス異常
 *----------------------------------------------------------------------------
 */

/*==========================================
 * ステータス異常終了
 *------------------------------------------
 */
int skill_status_change_active(dumb_ptr<block_list> bl, StatusChange type)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retz(bl);
    if (bl->bl_type != BL::PC && bl->bl_type != BL::MOB)
    {
        if (battle_config.error_log)
            PRINTF("skill_status_change_active: neither MOB nor PC !\n"_fmt);
        return 0;
    }

    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return 0;

    return bool(sc_data[type].timer);
}

void skill_status_change_end(dumb_ptr<block_list> bl, StatusChange type, TimerData *tid)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    int opt_flag = 0, calc_flag = 0;
    Opt0 *option;
    Opt1 *opt1;
    Opt2 *opt2;
    Opt3 *opt3;

    nullpo_retv(bl);
    if (bl->bl_type != BL::PC && bl->bl_type != BL::MOB)
    {
        if (battle_config.error_log)
            PRINTF("skill_status_change_end: neither MOB nor PC !\n"_fmt);
        return;
    }
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return;
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

    if (bl->bl_type == BL::PC && type < StatusChange::SC_SENDMAX)
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

    if (bl->bl_type == BL::PC && calc_flag)
        pc_calcstatus(bl->is_player(), 0);  /* ステータス再計算 */
}

int skill_update_heal_animation(dumb_ptr<map_session_data> sd)
{
    const Opt2 mask = Opt2::_heal;

    nullpo_retz(sd);
    bool wis_active = bool(sd->opt2 & mask);
    bool is_active = sd->quick_regeneration_hp.amount > 0;

    if (wis_active == is_active)
        return 0;               // no update

    if (is_active)
        sd->opt2 |= mask;
    else
        sd->opt2 &= ~mask;

    return clif_changeoption(sd);
}

/*==========================================
 * ステータス異常終了タイマー
 *------------------------------------------
 */
void skill_status_change_timer(TimerData *tid, tick_t tick, BlockId id, StatusChange type)
{
    dumb_ptr<block_list> bl;
    dumb_ptr<map_session_data> sd = nullptr;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    if ((bl = map_id2bl(id)) == nullptr)
        return;
    //該当IDがすでに消滅しているというのはいかにもありそうなのでスルーしてみる
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return;

    if (bl->bl_type == BL::PC)
        sd = bl->is_player();

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
                        if (bl->bl_type == BL::PC)
                        {
                            // TODO boundscheck this
                            hp = 3 + hp * 3 / 200;
                            pc_heal(bl->is_player(), -hp, 0);
                        }
                        else if (bl->bl_type == BL::MOB)
                        {
                            dumb_ptr<mob_data> md = bl->is_mob();
                            hp = 3 + hp / 200;
                            md->hp -= hp;
                        }
                    }
                    sc_data[type].timer = Timer(tick + 1_s,
                            std::bind(skill_status_change_timer, ph::_1, ph::_2,
                                bl->bl_id, type));
                    return;
                }
            }
            else
            {
                sc_data[type].timer = Timer(tick + 2_s,
                        std::bind(skill_status_change_timer, ph::_1, ph::_2,
                            bl->bl_id, type));
                return;
            }
            break;
            // If you manually reschedule the timer, you MUST skip the
            // call to skill_status_change_end below.

            /* 時間切れ無し？？ */
        case StatusChange::SC_WEIGHT50:
        case StatusChange::SC_WEIGHT90:
            sc_data[type].timer = Timer(tick + 10_min,
                    std::bind(skill_status_change_timer, ph::_1, ph::_2,
                        bl->bl_id, type));
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
int skill_status_change_start(dumb_ptr<block_list> bl, StatusChange type,
        int val1,
        interval_t tick)
{
    return skill_status_effect(bl, type, val1, tick);
}

int skill_status_effect(dumb_ptr<block_list> bl, StatusChange type,
        int val1,
        interval_t tick)
{
    dumb_ptr<map_session_data> sd = nullptr;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    Opt0 *option;
    Opt1 *opt1;
    Opt2 *opt2;
    Opt3 *opt3;
    int opt_flag = 0, calc_flag = 0;
    SP updateflag = SP::ZERO;
    int scdef = 0;

    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return 0;
    option = battle_get_option(bl);
    nullpo_retz(option);
    opt1 = battle_get_opt1(bl);
    nullpo_retz(opt1);
    opt2 = battle_get_opt2(bl);
    nullpo_retz(opt2);
    opt3 = battle_get_opt3(bl);
    nullpo_retz(opt3);

    switch (type)
    {
        case StatusChange::SC_POISON:
            scdef = 3 + battle_get_vit(bl) + battle_get_luk(bl) / 3;
            break;
    }
    if (scdef >= 100)
        return 0;
    if (bl->bl_type == BL::PC)
    {
        sd = bl->is_player();
    }
    else if (bl->bl_type == BL::MOB)
    {
    }
    else
    {
        if (battle_config.error_log)
            PRINTF("skill_status_change_start: neither MOB nor PC !\n"_fmt);
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
            tick = 1_s;
            break;

        case StatusChange::SC_WEIGHT50:
        case StatusChange::SC_WEIGHT90:
            tick = 10_min;
            break;

        case StatusChange::SC_HASTE:
        case StatusChange::SC_PHYS_SHIELD:
        case StatusChange::SC_MBARRIER:
            calc_flag = 1;
            break;
        case StatusChange::SC_HALT_REGENERATE:
        case StatusChange::SC_HIDE:
            break;
        case StatusChange::SC_FLYING_BACKPACK:
            updateflag = SP::WEIGHT;
            break;
        default:
            if (battle_config.error_log)
                PRINTF("UnknownStatusChange [%d]\n"_fmt, type);
            return 0;
    }

    if (bl->bl_type == BL::PC && type < StatusChange::SC_SENDMAX)
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

    sc_data[type].val1 = val1;

    /* タイマー設定 */
    sc_data[type].timer = Timer(gettick() + tick,
            std::bind(skill_status_change_timer, ph::_1, ph::_2,
                bl->bl_id, type));

    if (bl->bl_type == BL::PC && calc_flag)
        pc_calcstatus(sd, 0);  /* ステータス再計算 */

    if (bl->bl_type == BL::PC && updateflag != SP::ZERO)
        clif_updatestatus(sd, updateflag); /* ステータスをクライアントに送る */

    return 0;
}

/*==========================================
 * ステータス異常全解除
 *------------------------------------------
 */
int skill_status_change_clear(dumb_ptr<block_list> bl, int type)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    Opt0 *option;
    Opt1 *opt1;
    Opt2 *opt2;
    Opt3 *opt3;

    nullpo_retz(bl);
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return 0;
    option = battle_get_option(bl);
    nullpo_retz(option);
    opt1 = battle_get_opt1(bl);
    nullpo_retz(opt1);
    opt2 = battle_get_opt2(bl);
    nullpo_retz(opt2);
    opt3 = battle_get_opt3(bl);
    nullpo_retz(opt3);

    for (StatusChange i : erange(StatusChange(), StatusChange::MAX_STATUSCHANGE))
    {
        if (sc_data[i].timer)
            skill_status_change_end(bl, i, nullptr);
    }
    *opt1 = Opt1::ZERO;
    *opt2 = Opt2::ZERO;
    *opt3 = Opt3::ZERO;
    *option = Opt0::ZERO;

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
void skill_stop_dancing(dumb_ptr<block_list>, int)
{
    // TODO remove this
}

void skill_unit_timer_sub_ondelete(dumb_ptr<block_list> bl,
        dumb_ptr<block_list> src, unsigned int tick);

/*----------------------------------------------------------------------------
 * アイテム合成
 *----------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------
 * 初期化系
 */

static
SP scan_stat(XString statname)
{
    if (statname == "str"_s)
        return SP::STR;
    if (statname == "dex"_s)
        return SP::DEX;
    if (statname == "agi"_s)
        return SP::AGI;
    if (statname == "vit"_s)
        return SP::VIT;
    if (statname == "int"_s)
        return SP::INT;
    if (statname == "luk"_s)
        return SP::LUK;
    if (statname == "none"_s)
        return SP::ZERO;

    FPRINTF(stderr, "Unknown stat `%s'\n"_fmt, AString(statname));
    return SP::ZERO;
}

bool skill_readdb(ZString filename)
{
    io::ReadFile in(filename);
    if (!in.is_open())
    {
        PRINTF("can't read %s\n"_fmt, filename);
        return false;
    }

    bool rv = true;
    AString line_;
    while (in.getline(line_))
    {
        // is_comment only works for whole-line comments
        // that could change once the Z dependency is dropped ...
        LString comment = "//"_s;
        XString line = line_.xislice_h(std::search(line_.begin(), line_.end(), comment.begin(), comment.end())).rstrip();
        if (!line)
            continue;

        struct skill_db_ skdb {};

        SkillID i;
        XString castcancel, ignore, flags, stat, desc;
        if (!extract(line,
                    record<','>(
                        &i,
                        lstripping(&skdb.range_k),
                        lstripping(&skdb.hit),
                        lstripping(&skdb.inf),
                        lstripping(&skdb.pl),
                        lstripping(&skdb.nk),
                        lstripping(&skdb.max_raise),
                        lstripping(&skdb.max),
                        lstripping(&skdb.num_k),
                        lstripping(&castcancel),
                        lstripping(&skdb.cast_def_rate),
                        lstripping(&skdb.inf2),
                        lstripping(&skdb.maxcount),
                        lstripping(&ignore), // weapon/magic/misc/none
                        lstripping(&ignore), // blow count
                        lstripping(&flags),
                        lstripping(&stat),
                        lstripping(&desc)
                    )
                )
        )
        {
            rv = false;
            continue;
        }
        if (/*i < SkillID() ||*/ i >= SkillID::MAX_SKILL_DB)
        {
            rv = false;
            continue;
        }

        if (castcancel == "yes"_s)
            skdb.castcancel = true;
        else if (castcancel == "no"_s)
            skdb.castcancel = false;
        else
        {
            rv = false;
            continue;
        }

        if (flags == "passive"_s)
        {
            skill_pool_register(i);
            skdb.poolflags = SkillFlags::POOL_FLAG;
        }
        else if (flags == "active"_s)
        {
            skill_pool_register(i);
            skdb.poolflags = SkillFlags::POOL_FLAG | SkillFlags::POOL_ACTIVE;
        }
        else if (flags == "no"_s)
            skdb.poolflags = SkillFlags::ZERO;
        else
        {
            rv = false;
            continue;
        }

        skdb.stat = scan_stat(stat);

        MString tmp;
        tmp += desc;
        for (char& c : tmp)
            if (c == '_')
                c = ' ';

        skill_db[i] = skdb;
        skill_lookup_by_id(i).desc = RString(tmp);
    }
    PRINTF("read %s done\n"_fmt, filename);

    return rv;
}

constexpr size_t num_names = sizeof(skill_names) / sizeof(skill_names[0]);

skill_name_db& skill_lookup_by_id(SkillID id)
{
    for (skill_name_db& ner : skill_names)
        if (ner.id == id)
            return ner;
    return skill_names[num_names - 1];
}

skill_name_db& skill_lookup_by_name(XString name)
{
    for (skill_name_db& ner : skill_names)
        if (name == ner.name || name == ner.desc)
            return ner;
    return skill_names[num_names - 1];
}
} // namespace map
} // namespace tmwa
