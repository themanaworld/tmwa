#include "pc.hpp"
//    pc.cpp - Player state changes.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011 Jessica Tölke
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2012-2013 Freeyorp
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

#include "../compat/fun.hpp"
#include "../compat/nullpo.hpp"

#include "../strings/rstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/literal.hpp"

#include "../generic/random.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/read.hpp"

#include "../mmo/cxxstdio_enums.hpp"

#include "../net/timer.hpp"
#include "../net/timestamp-utils.hpp"

#include "../proto2/char-map.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "battle_conf.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "map_conf.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "path.hpp"
#include "script-call.hpp"
#include "skill.hpp"
#include "storage.hpp"
#include "trade.hpp"
#include "quest.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
// PVP順位計算の間隔 | Interval for PVP ranking calculation
constexpr std::chrono::milliseconds PVP_CALCRANK_INTERVAL =
        1_s;

//define it here, since the ifdef only occurs in this file
#define USE_ASTRAL_SOUL_SKILL

#ifdef USE_ASTRAL_SOUL_SKILL
// [fate] At this threshold, the Astral Soul skill kicks in
constexpr int MAGIC_SKILL_THRESHOLD = 200;
#endif

#define MAP_LOG_STATS(sd, suffix)                           \
        MAP_LOG_PC(sd, "STAT %d %d %d %d %d %d " suffix,    \
                sd->status.attrs[ATTR::STR],                \
                sd->status.attrs[ATTR::AGI],                \
                sd->status.attrs[ATTR::VIT],                \
                sd->status.attrs[ATTR::INT],                \
                sd->status.attrs[ATTR::DEX],                \
                sd->status.attrs[ATTR::LUK])

#define MAP_LOG_XP(sd, suffix)                                                      \
        MAP_LOG_PC(sd, "XP %d %d JOB %d %d %d ZENY %d + %d " suffix,                \
                sd->status.base_level, sd->status.base_exp,                         \
                sd->status.job_level, sd->status.job_exp, sd->status.skill_point,   \
                sd->status.zeny, pc_readaccountreg(sd, stringish<VarName>("BankAccount"_s)))

#define MAP_LOG_MAGIC(sd, suffix)                                                           \
        MAP_LOG_PC(sd, "MAGIC %d %d %d %d %d %d EXP %d %d " suffix,                         \
                   sd->status.skill[SkillID::TMW_MAGIC].lv,                                 \
                   sd->status.skill[SkillID::TMW_MAGIC_LIFE].lv,                            \
                   sd->status.skill[SkillID::TMW_MAGIC_WAR].lv,                             \
                   sd->status.skill[SkillID::TMW_MAGIC_TRANSMUTE].lv,                       \
                   sd->status.skill[SkillID::TMW_MAGIC_NATURE].lv,                          \
                   sd->status.skill[SkillID::TMW_MAGIC_ETHER].lv,                           \
                   pc_readglobalreg(sd, stringish<VarName>("MAGIC_EXPERIENCE"_s)) & 0xffff, \
                   (pc_readglobalreg(sd, stringish<VarName>("MAGIC_EXPERIENCE"_s)) >> 24) & 0xff)

static //const
int max_weight_base_0 = 20000;
static //const
int hp_coefficient_0 = 0;
static //const
int hp_coefficient2_0 = 500;
// TODO see if this can be turned into an "as-needed" formula
static
int hp_sigma_val_0[MAX_LEVEL];
static //const
int sp_coefficient_0 = 100;

// coefficients for each weapon type
// (not all used)
static //const
earray<interval_t, ItemLook, ItemLook::COUNT> aspd_base_0 //=
{{
    550_ms,  //  0 { "Fist", W_FIST },
    600_ms,  //  1 { "Dagger", W_DAGGER },
    650_ms,  //  2 { "Sword", W_1HSWORD },
    700_ms,  //  3 { "TwoHandSword", W_2HSWORD },
    650_ms,  //  4 { "Spear", W_1HSPEAR },
    700_ms,  //  5 { "TwoHandSpear", W_2HSPEAR },
    700_ms,  //  6 { "Axe", W_1HAXE },
    750_ms,  //  7 { "TwoHandAxe", W_2HAXE },
    700_ms,  //  8 { "Mace", W_MACE },
    750_ms,  //  9 { "TwoHandMace", W_2HMACE },
    700_ms,  // 10 { "Rod", W_STAFF },
    800_ms,  // 11 { "Bow", W_BOW },
    575_ms,  // 12 { "Knuckle", W_KNUCKLE },
    800_ms,  // 13 { "Instrument", W_MUSICAL },
    675_ms,  // 14 { "Whip", W_WHIP },
    700_ms,  // 15 { "Book", W_BOOK },
    600_ms,  // 16 { "Katar", W_KATAR },
    600_ms,  // 17 { "Revolver", W_REVOLVER },
    800_ms,  // 18 { "Rifle", W_RIFLE },
    500_ms,  // 19 { "GatlingGun", W_GATLING },
    1000_ms, // 20 { "Shotgun", W_SHOTGUN },
    2000_ms, // 21 { "GrenadeLauncher", W_GRENADE },
    650_ms,  // 22 { "FuumaShuriken", W_HUUMA },
    750_ms,  // 23 { "TwoHandRod", W_2HSTAFF },
}};

static const
int exp_table_0[MAX_LEVEL] =
{
    // 1 .. 10
                9,          16,         25,         36,
    77,         112,        153,        200,        253,
    // 11 .. 20
    320,        385,        490,        585,        700,
    830,        970,        1120,       1260,       1420,
    // 21 .. 30
    1620,       1860,       1990,       2240,       2504,
    2950,       3426,       3934,       4474,       6889,
    // 31 .. 40
    7995,       9174,       10425,      11748,      13967,
    15775,      17678,      19677,      21773,      30543,
    // 41 .. 50
    34212,      38065,      42102,      46323,      53026,
    58419,      64041,      69892,      75973,      102468,
    // 51 .. 60
    115254,     128692,     142784,     157528,     178184,
    196300,     215198,     234879,     255341,     330188,
    // 61 .. 70
    365914,     403224,     442116,     482590,     536948,
    585191,     635278,     687211,     740988,     925400,
    // 71 .. 80
    1473746,    1594058,    1718928,    1848355,    1982340,
    2230113,    2386162,    2547417,    2713878,    3206160,
    // 81 .. 90
    3681024,    4022472,    4377024,    4744680,    5125440,
    5767272,    6204000,    6655464,    7121664,    7602600,
    // 91 .. 100
    9738720,    11649960,   13643520,   18339300,   23836800,
    35658000,   48687000,   58135000,   99999999,   103000000,
    // 101 .. 110
    107000000,  112000000,  116000000,  121000000,  125000000,
    130000000,  134000000,  139000000,  145000000,  152200000,
    // 111 .. 120
    160840000,  171200000,  191930000,  202290000,  214720000,
    229640000,  247550000,  283370000,  301280000,  322770000,
    // 121 .. 130
    348560000,  379500000,  417450000,  459195000,  505114500,
    555625950,  622301064,  696977191,  780614454,  880533104,
    // 131 .. 140
    993241342,  1120376234, 1263784392, 1425548794, 1588019039,
    1680000000, 1780000000, 1890000000, 2010000000, 2140000000,
    // 141
    0
};
// The old table contained a bug, actually max level was 136 and not 135 but to reach level 136 you must hit exactly the value 2147483647
// else and overflow occurs and you lose all the progress of that level and start with level 135 at 0% again.
// I made a buffer that is great enough now I guess its 7483647 while looking through all gm logs I saw the highest values ever where 300%.
// A Xakelbael killed on 300% exp one time tabbed gives around 6 mil so I have a little buffer on top of that as well, it won't make it for
// 400% exp but since this will most likely never happen and the chance that one will make his lvl up with Xakelbael one time tabbed
// is extremely rare as well.
    // 130 .. 135
    //993241342,  1120376234, 1263784392, 1425548794, 1608019039,
    //2147483647, 0


// is this *actually* used anywhere?
static const
int exp_table_7[MAX_LEVEL] =
{
    // 1 .. 9
        10, 18, 28, 40, 91, 151, 205, 268, 340
};

// TODO generate this table instead
static int stat_p[MAX_LEVEL] =
{
    // 1..9
        48, 52, 56, 60,         64, 69, 74, 79, 84,
    // 10..19
    90, 96, 102,108,115,        122,129,136,144,152,
    // 20..29
    160,168,177,186,195,        204,214,224,234,244,
    // 30..39
    255,266,277,288,300,        312,324,336,349,362,
    // 40..49
    375,388,402,416,430,        444,459,474,489,504,
    // 50..59
    520,536,552,568,585,        602,619,636,654,672,
    // 60..69
    690,708,727,746,765,        784,804,824,844,864,
    // 70..79
    885,906,927,948,970,        992,1014,1036,1059,1082,
    // 80..89
    1105,1128,1152,1176,1200,   1224,1249,1274,1299,1324,
    // 90..99
    1350,1376,1402,1428,1455,   1482,1509,1536,1564,1592,
    // 100..109
    1620,1648,1677,1706,1735,   1764,1794,1824,1854,1884,
    // 110..119
    1915,1946,1977,2008,2040,   2072,2104,2136,2169,2202,
    // 120..129
    2235,2268,2302,2336,2370,   2404,2439,2474,2509,2544,
    // 130..139
    2580,2616,2652,2688,2725,   2762,2799,2836,2874,2912,
    // 140..149
    2950,2988,3027,3066,3105,   3144,3184,3224,3264,3304,
    // 150..159
    3345,3386,3427,3468,3510,   3552,3594,3636,3679,3722,
    // 160..169
    3765,3808,3852,3896,3940,   3984,4029,4074,4119,4164,
    // 170..179
    4210,4256,4302,4348,4395,   4442,4489,4536,4584,4632,
    // 180..189
    4680,4728,4777,4826,4875,   4924,4974,5024,5074,5124,
    // 190..199
    5175,5226,5277,5328,5380,   5432,5484,5536,5589,5642,
    // 200..209
    5695,5748,5802,5856,5910,   5964,6019,6074,6129,6184,
    // 210..219
    6240,6296,6352,6408,6465,   6522,6579,6636,6694,6752,
    // 220..229
    6810,6868,6927,6986,7045,   7104,7164,7224,7284,7344,
    // 230..239
    7405,7466,7527,7588,7650,   7712,7774,7836,7899,7962,
    // 240..249
    8025,8088,8152,8216,8280,   8344,8409,8474,8539,8604,
    // 250..255
    8670,8736,8802,8868,8935,   9002,
};

static
earray<EPOS, EQUIP, EQUIP::COUNT> equip_pos //=
{{
    EPOS::MISC2,
    EPOS::CAPE,
    EPOS::SHOES,
    EPOS::GLOVES,
    EPOS::LEGS,
    EPOS::TORSO,
    EPOS::HAT,
    EPOS::MISC1,
    EPOS::SHIELD,
    EPOS::WEAPON,
    EPOS::ARROW,
}};

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_checkoverhp(dumb_ptr<map_session_data> sd);

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_checkoversp(dumb_ptr<map_session_data> sd);

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_nextbaseafter(dumb_ptr<map_session_data> sd);

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_nextjobafter(dumb_ptr<map_session_data> sd);

/*==========================================
 * 
 *------------------------------------------
 */
static
void pc_setdead(dumb_ptr<map_session_data> sd)
{
    sd->state.dead_sit = 1;
}

/*==========================================
 * 
 *------------------------------------------
 */
GmLevel pc_isGM(dumb_ptr<map_session_data> sd)
{
    nullpo_retr(GmLevel(), sd);

    auto it = gm_accountm.find(sd->status_key.account_id);
    if (it != gm_accountm.end())
        return it->second;
    return GmLevel();
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_iskiller(dumb_ptr<map_session_data> src,
                 dumb_ptr<map_session_data> target)
{
    nullpo_retz(src);

    if (src->bl_type != BL::PC || target->bl_type != BL::PC)
        return 0;
    if ((src->state.pvpchannel == 1) && (target->state.pvpchannel == 1) && !src->bl_m->flag.get(MapFlag::NOPVP))
        return 1;
    if ((src->state.pvpchannel > 1) && (target->state.pvpchannel == src->state.pvpchannel)) // this one does not respect NOPVP
        return 1;
    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
void pc_set_gm_level(AccountId account_id, GmLevel level)
{
    if (level)
        gm_accountm[account_id] = level;
    else
        gm_accountm.erase(account_id);
}

/*==========================================
 * 
 *------------------------------------------
 */
static
int distance(int x0, int y0, int x1, int y1)
{
    int dx, dy;

    dx = abs(x0 - x1);
    dy = abs(y0 - y1);
    return dx > dy ? dx : dy;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
void pc_pvp_timer(TimerData *, tick_t, BlockId id)
{
    dumb_ptr<map_session_data> sd = map_id2sd(id);

    assert (sd != nullptr);
    assert (sd->bl_type == BL::PC);
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_setpvptimer(dumb_ptr<map_session_data> sd, interval_t val)
{
    nullpo_retz(sd);

    sd->pvp_timer = Timer(gettick() + val,
            std::bind(pc_pvp_timer, ph::_1, ph::_2,
                sd->bl_id));
    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_delpvptimer(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    sd->pvp_timer.cancel();
    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
void pc_invincible_timer(TimerData *, tick_t, BlockId id)
{
    dumb_ptr<map_session_data> sd = map_id2sd(id);

    assert (sd != nullptr);
    assert (sd->bl_type == BL::PC);
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_setinvincibletimer(dumb_ptr<map_session_data> sd, interval_t val)
{
    nullpo_retz(sd);

    sd->invincible_timer = Timer(gettick() + val,
            std::bind(pc_invincible_timer, ph::_1, ph::_2,
                sd->bl_id));
    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_delinvincibletimer(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    sd->invincible_timer.cancel();
    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_setrestartvalue(dumb_ptr<map_session_data> sd, int type)
{
    nullpo_retz(sd);

    {
        if (battle_config.restart_hp_rate < 50)
            sd->status.hp = (sd->status.max_hp) / 2;
        else
        {
            if (battle_config.restart_hp_rate <= 0)
                sd->status.hp = 1;
            else
            {
                sd->status.hp =
                    sd->status.max_hp * battle_config.restart_hp_rate / 100;
                if (sd->status.hp <= 0)
                    sd->status.hp = 1;
            }
        }
        if (battle_config.restart_sp_rate > 0)
        {
            int sp = sd->status.max_sp * battle_config.restart_sp_rate / 100;
            if (sd->status.sp < sp)
                sd->status.sp = sp;
        }
    }
    if (type & 1)
        clif_updatestatus(sd, SP::HP);
    if (type & 1)
        clif_updatestatus(sd, SP::SP);

    sd->heal_xp = 0;            // [Fate] Set gainable xp for healing this player to 0
    return 0;
}

/*==========================================
 * 自分をロックしているMOBの数を数える(foreachclient)
 * Count the number of MOBs that lock you (foreachclient)
 *------------------------------------------
 */
static
void pc_counttargeted_sub(dumb_ptr<block_list> bl,
        BlockId id, int *c, dumb_ptr<block_list> src, ATK target_lv)
{
    nullpo_retv(bl);

    if (id == bl->bl_id || (src && id == src->bl_id))
        return;
    if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = bl->is_player();
        if (sd->attacktarget == id && sd->attacktimer
            && sd->attacktarget_lv >= target_lv)
            (*c)++;
    }
    else if (bl->bl_type == BL::MOB)
    {
        dumb_ptr<mob_data> md = bl->is_mob();
        if (md->target_id == id && md->timer
            && md->state.state == MS::ATTACK && md->target_lv >= target_lv)

            (*c)++;
    }
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_counttargeted(dumb_ptr<map_session_data> sd, dumb_ptr<block_list> src,
        ATK target_lv)
{
    int c = 0;
    map_foreachinarea(std::bind(pc_counttargeted_sub, ph::_1, sd->bl_id, &c, src, target_lv),
            sd->bl_m,
            sd->bl_x - AREA_SIZE, sd->bl_y - AREA_SIZE,
            sd->bl_x + AREA_SIZE, sd->bl_y + AREA_SIZE,
            BL::NUL);
    return c;
}

/*==========================================
 * ローカルプロトタイプ宣言 (必要な物のみ)
 * Local prototype declarations (only what is needed)
 *------------------------------------------
 */
static
int pc_walktoxy_sub(dumb_ptr<map_session_data>);

/*==========================================
 * saveに必要なステータス修正を行なう
 * Make the necessary status corrections to save
 *------------------------------------------
 */
void pc_makesavestatus(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    // 服の色は色々弊害が多いので保存対象にはしない | The color of clothes has many adverse effects, so do not store them
    if (!battle_config.save_clothcolor)
        sd->status.clothes_color = 0;

    // 死亡状態だったのでhpを1、位置をセーブ場所に変更 | Since he was dead, HP 1, changed position to save location
    if (pc_isdead(sd))
    {
        pc_setrestartvalue(sd, 0);
        if (sd->bl_m->flag.get(MapFlag::RESAVE))
        {
            sd->status.last_point = sd->bl_m->resave;
        }
        else
        {
            sd->status.last_point = sd->status.save_point;
        }
    }
    else
    {
        sd->status.last_point.map_ = sd->mapname_;
        sd->status.last_point.x = sd->bl_x;
        sd->status.last_point.y = sd->bl_y;
    }

    // セーブ禁止マップだったので指定位置に移動 | Since it was a save prohibited map, moved to the specified position
    if (sd->bl_m->flag.get(MapFlag::NOSAVE))
    {
        P<map_local> m = sd->bl_m;
        if (m->save.map_ == "SavePoint"_s)
            sd->status.last_point = sd->status.save_point;
        else
            sd->status.last_point = m->save;
    }
}

/*==========================================
 * 接続時の初期化
 * Initialization on connection
 *------------------------------------------
 */
int pc_setnewpc(dumb_ptr<map_session_data> sd, AccountId account_id, CharId char_id,
        int login_id1, uint32_t client_tick, SEX sex)
{
    nullpo_retz(sd);

    // TODO this is the primary surface
    sd->bl_id = account_to_block(account_id);
    sd->char_id_ = char_id;
    // TODO figure out wtf is going on here.
    // shouldn't these things be in the .status_key.char_id ?
    // My guess is that this stuff happens even for non-auth'ed connections
    // Possible fix: char send auth before client is allowed to know my IP?
    sd->login_id1 = login_id1;
    sd->login_id2 = 0;          // at this point, we can not know the value :(
    (void)client_tick;
    sd->sex = sex;
    sd->state.auth = 0;
    sd->state.connect_new = 0;
    sd->bl_type = BL::PC;
    sd->canact_tick = sd->canmove_tick = gettick();
    sd->canlog_tick = gettick();
    sd->state.waitingdisconnect = 0;

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
EPOS pc_equippoint(dumb_ptr<map_session_data> sd, IOff0 n)
{
    nullpo_retr(EPOS::ZERO, sd);

    return sd->inventory_data[n].pmd_pget(&item_data::equip).copy_or(EPOS::ZERO);
}

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_setinventorydata(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    for (IOff0 i : IOff0::iter())
    {
        ItemNameId id = sd->status.inventory[i].nameid;
        // If you think you understand this line, you're wrong.
        // It does not do what you think it does. Rather, you need to
        // understand it in the context in which it is used. Despite this,
        // it is quite common for elements to be None.
        sd->inventory_data[i] = Some(itemdb_search(id));
    }
    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_calcweapontype(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    // TODO now that there is no calculation here, store only once
    sd->status.weapon = sd->weapontype1;

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_setequipindex(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    for (EQUIP i : EQUIPs)
        sd->equip_index_maybe[i] = IOff0::from(-1);

    for (IOff0 i : IOff0::iter())
    {
        if (!sd->status.inventory[i].nameid)
            continue;
        if (bool(sd->status.inventory[i].equip))
        {
            for (EQUIP j : EQUIPs)
                if (bool(sd->status.inventory[i].equip & equip_pos[j]))
                    sd->equip_index_maybe[j] = i;
            if (bool(sd->status.inventory[i].equip & EPOS::WEAPON))
            {
                OMATCH_BEGIN (sd->inventory_data[i])
                {
                    OMATCH_CASE_SOME (sdidi)
                    {
                        sd->weapontype1 = sdidi->look;
                    }
                    OMATCH_CASE_NONE ()
                    {
                        sd->weapontype1 = ItemLook::W_FIST;
                    }
                }
                OMATCH_END ();
            }
            if (bool(sd->status.inventory[i].equip & EPOS::SHIELD))
            {
                OMATCH_BEGIN_SOME (sdidi, sd->inventory_data[i])
                {
                    if (sdidi->type == ItemType::WEAPON)
                    {
                        if (sd->status.inventory[i].equip == EPOS::SHIELD)
                            assert(0 && "unreachable - offhand weapons are not supported");
                    }
                }
                OMATCH_END ();
            }
        }
    }
    pc_calcweapontype(sd);

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_isequip(dumb_ptr<map_session_data> sd, IOff0 n)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    //転生や養子の場合の元の職業を算出する | Calculate the original occupation in case of reincarnation or adoption

    nullpo_retz(sd);

    sc_data = battle_get_sc_data(sd);

    GmLevel gm_all_equipment = battle_config.gm_all_equipment;
    if (gm_all_equipment && pc_isGM(sd).satisfies(gm_all_equipment))
        return 1;

    P<struct item_data> item = TRY_UNWRAP(sd->inventory_data[n], return 0);
    if (item->sex != SEX::UNSPECIFIED && sd->status.sex != item->sex)
        return 0;
    if (item->elv > 0 && sd->status.base_level < item->elv)
        return 0;

    return 1;
}

/*==========================================
 * 
 *------------------------------------------
 */
void pc_set_weapon_icon(dumb_ptr<map_session_data> sd, int count,
        StatusChange icon, ItemNameId look)
{
    const StatusChange old_icon = sd->attack_spell_icon_override;

    sd->attack_spell_icon_override = icon;
    sd->attack_spell_look_override = look;

    if (old_icon != StatusChange::ZERO && old_icon != icon)
        clif_status_change(sd, old_icon, 0);

    clif_fixpcpos(sd);
    if (count)
    {
        clif_changelook(sd, LOOK::WEAPON, unwrap<ItemNameId>(look));
        if (icon != StatusChange::ZERO)
            clif_status_change(sd, icon, 1);
    }
    else
    {
        /* Set it to `normal' */
        clif_changelook(sd, LOOK::WEAPON,
                static_cast<uint16_t>(sd->status.weapon));
    }
}

/*==========================================
 * 
 *------------------------------------------
 */
void pc_set_attack_info(dumb_ptr<map_session_data> sd, interval_t speed, int range)
{
    sd->attack_spell_delay = speed;
    sd->attack_spell_range = range;

    if (speed == interval_t::zero())
    {
        pc_calcstatus(sd, (int)CalcStatusKind::INITIAL_CALC);
        clif_updatestatus(sd, SP::ASPD);
        clif_updatestatus(sd, SP::ATTACKRANGE);
    }
    else
    {
        pc_calcstatus(sd, ((int)CalcStatusKind::INITIAL_CALC + (int)CalcStatusKind::MAGIC_OVERRIDE));
        clif_updatestatus(sd, SP::ASPD);
        clif_updatestatus(sd, SP::ATTACKRANGE);
    }
}

/*==========================================
 * session idに問題無し
 * char鯖から送られてきたステータスを設定
 * No problem with session id
 * Set the status sent from char server
 *------------------------------------------
 */
int pc_authok(AccountId id, int login_id2, ClientVersion client_version,
    const CharKey *st_key, const CharData *st_data)
{
    dumb_ptr<map_session_data> sd = nullptr;

    tick_t tick = gettick();

    sd = map_id2sd(account_to_block(id));
    if (sd == nullptr)
        return 1;

    sd->login_id2 = login_id2;
    sd->client_version = client_version;

    sd->status_key = *st_key;
    sd->status = *st_data;

    MAP_LOG_STATS(sd, "LOGIN"_fmt);
    MAP_LOG_XP(sd, "LOGIN"_fmt);
    MAP_LOG_MAGIC(sd, "LOGIN"_fmt);

    really_memzero_this(&sd->state);
    // 基本的な初期化 | Basic initialization
    sd->state.connect_new = 1;
    sd->bl_prev = sd->bl_next = nullptr;

    sd->weapontype1 = ItemLook::W_FIST;
    sd->speed = DEFAULT_WALK_SPEED;
    sd->state.dead_sit = 0;
    sd->dir = DIR::S;
    sd->head_dir = DIR::S;
    sd->state.auth = 1;
    // sd->walktimer = nullptr;
    // sd->attacktimer = nullptr;
    // sd->invincible_timer = nullptr;

    sd->deal_locked = 0;
    sd->trade_partner = AccountId();

    sd->inchealhptick = interval_t::zero();
    sd->inchealsptick = interval_t::zero();
    sd->hp_sub = interval_t::zero();
    sd->sp_sub = interval_t::zero();
    sd->quick_regeneration_hp.amount = 0;
    sd->quick_regeneration_sp.amount = 0;
    sd->heal_xp = 0;
    sd->max_weight_override = 0;
    sd->activity.kills = 0;
    sd->activity.casts = 0;
    sd->activity.items_used = 0;
    sd->activity.tiles_walked = 0;
    sd->activity.attacks = 0;
    sd->canact_tick = tick;
    sd->canmove_tick = tick;
    sd->attackabletime = tick;
    /* We don't want players bypassing spell restrictions. [remoitnane] */
    // Removed because it was buggy with the ~50 day wraparound,
    // and there's already a limit on how fast you can log in and log out.
    // -o11c
    //
    // The above is no longer accurate now that we use <chrono>, but
    // I'm still not reverting this.
    // -o11c
    //sd->cast_tick = tick; // + pc_readglobalreg (sd, "MAGIC_CAST_TICK"_s);

    // アカウント変数の送信要求 | Request to send account variables
    intif_request_accountreg(sd);

    // アイテムチェック | Item Check
    pc_setinventorydata(sd);
    pc_checkitem(sd);

    // ステータス異常の初期化 | Initializing Status Anomalies
    for (StatusChange i : erange(StatusChange(), StatusChange::MAX_STATUSCHANGE))
    {
        // sd->sc_data[i].timer = nullptr;
        sd->sc_data[i].val1 = 0;
    }

    // パーティー関係の初期化 | Initializing Party Relationships
    sd->party_sended = 0;
    sd->party_invite = PartyId();
    sd->party_x = -1;
    sd->party_y = -1;
    sd->party_hp = -1;

    // イベント関係の初期化 | Initializing Event Relationships
    sd->eventqueuel.clear();

    {
        Opt0 old_option = sd->status.option;
        sd->status.option = Opt0::ZERO;

        if (bool(old_option & Opt0::INVISIBILITY) && can_use_atcommand(sd, "@invisible"_s)) {
            // prevent leaking by first applying hide before status_change
            sd->status.option |= Opt0::HIDE;
            sd->status.option |= Opt0::INVISIBILITY;
            clif_status_change(sd, StatusChange::CLIF_OPTION_SC_INVISIBILITY, 1);
        } else if (bool(old_option & Opt0::HIDE) && can_use_atcommand(sd, "@hide"_s)) {
            sd->status.option |= Opt0::HIDE;
        }

        clif_changeoption(sd);
    }

    // 位置の設定 | Setting the Position
    pc_setpos(sd, sd->status.last_point.map_, sd->status.last_point.x,
               sd->status.last_point.y, BeingRemoveWhy::GONE);

    // パーティ、ギルドデータの要求 | Request for Party, Guild Data
    if (sd->status.party_id
        && party_search(sd->status.party_id).is_none())
        party_request_info(sd->status.party_id);

    // pvpの設定 | Configuring PVP
    sd->pvp_rank = 0;
    sd->pvp_point = 0;
    // sd->pvp_timer = nullptr;

    // 通知 | notice

    clif_authok(sd);
    map_addnickdb(sd);
    if (!map_charid2nick(sd->status_key.char_id).to__actual())
        map_addchariddb(sd->status_key.char_id, sd->status_key.name);

    //スパノビ用死にカウンターのスクリプト変数からの読み出しとsdへのセット | Reading from Counter Script Variable to Death for Spa Novi and Set to SD
    sd->die_counter = pc_readglobalreg(sd, stringish<VarName>("PC_DIE_COUNTER"_s));

    // ステータス初期計算など | Status initial calculation, etc.
    pc_calcstatus(sd, (int)CalcStatusKind::INITIAL_CALC);

    if (pc_isGM(sd))
    {
        PRINTF("Connection accepted: character '%s' (account: %d; GM level %d).\n"_fmt,
                sd->status_key.name, sd->status_key.account_id, pc_isGM(sd));
        clif_updatestatus(sd, SP::GM);
    }
    else
        PRINTF("Connection accepted: Character '%s' (account: %d).\n"_fmt,
                sd->status_key.name, sd->status_key.account_id);

    sd->auto_ban_info.in_progress = 0;

    // Initialize antispam vars
    sd->chat_reset_due = tick_t();
    sd->chat_lines_in = sd->chat_total_repeats = 0;
    sd->chat_repeat_reset_due = tick_t();
    sd->chat_lastmsg = RString();

    // Initialize mute vars
    sd->mute.global = 0;
    sd->mute.party = 0;
    sd->mute.whisper = 0;
    sd->mute.guild = 0;

    sd->automod = AutoMod::off;

    for (tick_t& t : sd->flood_rates)
        t = tick_t();
    sd->packet_flood_reset_due = tick_t();
    sd->packet_flood_in = 0;

    pc_calcstatus(sd, (int)CalcStatusKind::INITIAL_CALC);

    if(sd->bl_m->mask > 0)
        clif_send_mask(sd, sd->bl_m->mask);

    // Init Quest Log
    clif_sendallquest(sd);
    return 0;
}

void pc_show_motd(dumb_ptr<map_session_data> sd)
{
    // Attention all forks: DO NOT REMOVE THIS NOTICE.
    // It exists to meet legal requirements.
    //
    // If you remove the sending of this message,
    // the license does not permit you to publicly use this software.

    clif_displaymessage(sd->sess, "Server : ##7This server is Free Software, for details type @source in chat."_s);
}

/*==========================================
 * session idに問題ありなので後始末
 * There is a problem with the session id, so clean up after it
 *------------------------------------------
 */
int pc_authfail(AccountId id)
{
    dumb_ptr<map_session_data> sd;

    sd = map_id2sd(account_to_block(id));
    if (sd == nullptr)
        return 1;

    clif_authfail_fd(sd->sess, 0);

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_calc_skillpoint(dumb_ptr<map_session_data> sd)
{
    int i, skill_points = 0;

    nullpo_retz(sd);

    for (i = 0; i < skill_pool_skills.size(); i++)
    {
        int lv = sd->status.skill[skill_pool_skills[i]].lv;
        if (lv)
            skill_points += ((lv * (lv - 1)) >> 1) - 1;
    }

    return skill_points;
}

/*==========================================
 * 重量アイコンの確認
 * Checking the weight icon
 *------------------------------------------
 */
int pc_checkweighticon(dumb_ptr<map_session_data> sd)
{
    int flag = 0;

    nullpo_retz(sd);

    if (sd->weight * 2 >= sd->max_weight
        && !sd->sc_data[StatusChange::SC_FLYING_BACKPACK].timer)
        flag = 1;
    if (sd->weight * 10 >= sd->max_weight * 9)
        flag = 2;

    // this is horribly hackish and may have caused crashes
    if (flag == 1)
    {
        if (!sd->sc_data[StatusChange::SC_WEIGHT50].timer)
            skill_status_change_start(sd, StatusChange::SC_WEIGHT50, 0, interval_t::zero());
    }
    else
    {
        skill_status_change_end(sd, StatusChange::SC_WEIGHT50, nullptr);
    }
    if (flag == 2)
    {
        if (!sd->sc_data[StatusChange::SC_WEIGHT90].timer)
            skill_status_change_start(sd, StatusChange::SC_WEIGHT90, 0, interval_t::zero());
    }
    else
    {
        skill_status_change_end(sd, StatusChange::SC_WEIGHT90, nullptr);
    }
    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
void pc_set_weapon_look(dumb_ptr<map_session_data> sd)
{
    if (sd->attack_spell_override)
        clif_changelook(sd, LOOK::WEAPON,
                unwrap<ItemNameId>(sd->attack_spell_look_override));
    else
        clif_changelook(sd, LOOK::WEAPON,
                static_cast<uint16_t>(sd->status.weapon));
}

/*==========================================
 * パラメータ計算
 * first==0の時、計算対象のパラメータが呼び出し前から
 * 変 化した場合自動でsendするが、
 * 能動的に変化させたパラメータは自前でsendするように
 * Parameter Calculation
 * When first==0, the parameter to be calculated is from before the call
 * If it changes, it will automatically send it,
 * Actively changed parameters should be send on their own
 *
 * First is a bitmask
 * &0 = Status Recalculation | ステータス再計算
 * &1 = Status initial calculation, etc.| ステータス初期計算など
 * &2 = Recalculate item bonus (used in pc_checkitem but not handled in this function atm, so if function is called with first = 2 its basically first = 0 only)
 * &4 = Status Recalculation but don't use any clif_updatestatus to send status to client (used by map_quit)
 * &8 = magic override (used in pc_set_attack_info)
 *------------------------------------------
 */
int pc_calcstatus(dumb_ptr<map_session_data> sd, int first)
{
    int b_max_hp, b_max_sp, b_hp, b_sp, b_weight, b_max_weight,
        b_hit, b_flee;
    int b_watk, b_def, b_watk2, b_def2, b_flee2, b_critical,
        b_attackrange, b_matk1, b_matk2, b_mdef, b_mdef2;
    int b_base_atk;
    int bl;
    int aspd_rate, speed_rate, refinedef = 0;
    int str, dstr, dex;
    int b_pvpchannel = 0;

    nullpo_retz(sd);

    interval_t b_speed = sd->speed;
    b_max_hp = sd->status.max_hp;
    b_max_sp = sd->status.max_sp;
    b_hp = sd->status.hp;
    b_sp = sd->status.sp;
    b_weight = sd->weight;
    b_max_weight = sd->max_weight;
    earray<int, ATTR, ATTR::COUNT> b_paramb = sd->paramb;
    earray<int, ATTR, ATTR::COUNT> b_parame = sd->parame;
    earray<SkillValue, SkillID, MAX_SKILL> b_skill = sd->status.skill;
    b_hit = sd->hit;
    b_flee = sd->flee;
    interval_t b_aspd = sd->aspd, b_base_weapon_delay_adjust = interval_t::zero();
    b_watk = sd->watk;
    b_def = sd->def;
    b_watk2 = sd->watk2;
    b_def2 = sd->def2;
    b_flee2 = sd->flee2;
    b_critical = sd->critical;
    b_attackrange = sd->attackrange;
    b_matk1 = sd->matk1;
    b_matk2 = sd->matk2;
    b_mdef = sd->mdef;
    b_mdef2 = sd->mdef2;
    b_base_atk = sd->base_atk;
    if (sd->state.pvpchannel == 1)
        b_pvpchannel = sd->state.pvpchannel;

    sd->max_weight = max_weight_base_0 + sd->status.attrs[ATTR::STR] * 300;

    if (first & (int)CalcStatusKind::INITIAL_CALC)
    {
        sd->weight = 0;
        for (IOff0 i : IOff0::iter())
        {
            if (!sd->status.inventory[i].nameid)
                continue;
            P<struct item_data> sdidi = TRY_UNWRAP(sd->inventory_data[i], continue);
            sd->weight +=
                sdidi->weight *
                sd->status.inventory[i].amount;
        }
        // used to fill cart
    }

    for (auto& p : sd->paramb)
        p = 0;
    for (auto& p : sd->parame)
        p = 0;

    sd->hit = 0;
    sd->flee = 0;
    sd->flee2 = 0;
    sd->critical = 0;
    sd->aspd = sd->base_weapon_delay_adjust = interval_t::zero();
    sd->watk = 0;
    sd->def = 0;
    sd->mdef = 0;
    sd->watk2 = 0;
    sd->def2 = 0;
    sd->mdef2 = 0;
    sd->status.max_hp = 0;
    sd->status.max_sp = 0;
    sd->attackrange = 0;
    sd->matk1 = 0;
    sd->matk2 = 0;
    sd->speed = DEFAULT_WALK_SPEED;
    sd->speed_cap = interval_t::zero();
    sd->hprate = 100;
    sd->sprate = 100;
    sd->dsprate = 100;
    sd->base_atk = 0;
    sd->arrow_atk = 0;
    sd->arrow_hit = 0;
    sd->arrow_range = 0;
    sd->nhealhp = sd->nhealsp = 0;
    really_memzero_this(&sd->special_state);

    sd->aspd_rate = 100;
    sd->speed_rate = 100;
    sd->hprecov_rate = 100;
    sd->sprecov_rate = 100;
    sd->critical_def = 0;
    sd->double_rate = 0;
    sd->atk_rate = sd->matk_rate = 100;
    sd->arrow_cri = 0;
    sd->perfect_hit = sd->deadly_strike = 0;
    sd->critical_rate = sd->hit_rate = sd->flee_rate = sd->flee2_rate = 100;
    sd->def_rate = sd->def2_rate = sd->mdef_rate = sd->mdef2_rate = 100;
    sd->speed_add_rate = sd->aspd_add_rate = 100;
    sd->double_add_rate = sd->perfect_hit_add_rate = sd->deadly_strike_add_rate = 0;
    sd->hp_drain_rate = sd->hp_drain_per = sd->sp_drain_rate =
        sd->sp_drain_per = 0;

    sd->spellpower_bonus_target = 0;

    for (EQUIP i : EQUIPs_noarrow)
    {
        IOff0 index = sd->equip_index_maybe[i];
        if (!index.ok())
            continue;
        if (i == EQUIP::WEAPON && sd->equip_index_maybe[EQUIP::SHIELD] == index)
            continue;
        if (i == EQUIP::TORSO && sd->equip_index_maybe[EQUIP::LEGS] == index)
            continue;
        if (i == EQUIP::HAT
            && (sd->equip_index_maybe[EQUIP::TORSO] == index
                || sd->equip_index_maybe[EQUIP::LEGS] == index))
            continue;

        OMATCH_BEGIN_SOME (sdidi, sd->inventory_data[index])
        {
            sd->spellpower_bonus_target +=
                sdidi->magic_bonus;

            // used to apply cards
        }
        OMATCH_END ();
    }

#ifdef USE_ASTRAL_SOUL_SKILL
    if (sd->spellpower_bonus_target < 0)
        sd->spellpower_bonus_target =
            (sd->spellpower_bonus_target * 256) /
            (std::min(128 + skill_power(sd, SkillID::TMW_ASTRAL_SOUL), 256));
#endif

    if (sd->spellpower_bonus_target < sd->spellpower_bonus_current)
        sd->spellpower_bonus_current = sd->spellpower_bonus_target;

    sd->paramcard = sd->parame;

    // 装備品によるステータス変化はここで実行 | Status changes by equipment are performed here
    for (EQUIP i : EQUIPs_noarrow)
    {
        IOff0 index = sd->equip_index_maybe[i];
        if (!index.ok())
            continue;
        if (i == EQUIP::WEAPON && sd->equip_index_maybe[EQUIP::SHIELD] == index)
            continue;
        if (i == EQUIP::TORSO && sd->equip_index_maybe[EQUIP::LEGS] == index)
            continue;
        if (i == EQUIP::HAT
            && (sd->equip_index_maybe[EQUIP::TORSO] == index
                || sd->equip_index_maybe[EQUIP::LEGS] == index))
            continue;
        OMATCH_BEGIN_SOME (sdidi, sd->inventory_data[index])
        {
            sd->def += sdidi->def;
            if (sdidi->type == ItemType::WEAPON)
            {
                if (i == EQUIP::SHIELD
                    && sd->status.inventory[index].equip == EPOS::SHIELD)
                {
                    assert(0 && "unreachable - offhand weapons are not supported");
                }
                else
                {
                    //二刀流武器以外 | Other than two-pronged weapons
                    argrec_t arg[2] =
                    {
                        {"@slotId"_s, static_cast<int>(i)},
                        {"@itemId"_s, unwrap<ItemNameId>(sdidi->nameid)},
                    };
                    sd->watk += sdidi->atk;

                    sd->attackrange += sdidi->range;
                    run_script_l(ScriptPointer(borrow(*sdidi->equip_script), 0),
                            sd->bl_id, BlockId(),
                            arg);
                }
            }
            else if (sdidi->type == ItemType::ARMOR)
            {
                argrec_t arg[2] =
                {
                    {"@slotId"_s, static_cast<int>(i)},
                    {"@itemId"_s, unwrap<ItemNameId>(sdidi->nameid)},
                };
                sd->watk += sdidi->atk;
                run_script_l(ScriptPointer(borrow(*sdidi->equip_script), 0),
                        sd->bl_id, BlockId(),
                        arg);
            }
        }
        OMATCH_END ();
    }

    if (battle_is_unarmed(sd))
    {
        sd->watk += skill_power(sd, SkillID::TMW_BRAWLING) / 3; // +66 for 200
        sd->watk2 += skill_power(sd, SkillID::TMW_BRAWLING) >> 3;   // +25 for 200
    }

    IOff0 aidx = sd->equip_index_maybe[EQUIP::ARROW];
    if (aidx.ok())
    {
        IOff0 index = aidx;
        OMATCH_BEGIN_SOME (sdidi, sd->inventory_data[index])
        {                       //まだ属性が入っていない | Attributes not yet included
            argrec_t arg[2] =
            {
                {"@slotId"_s, static_cast<int>(EQUIP::ARROW)},
                {"@itemId"_s, unwrap<ItemNameId>(sdidi->nameid)},
            };
            sd->state.lr_flag_is_arrow_2 = 1;
            run_script_l(ScriptPointer(borrow(*sdidi->equip_script), 0),
                    sd->bl_id, BlockId(),
                    arg);
            sd->state.lr_flag_is_arrow_2 = 0;
            sd->arrow_atk += sdidi->atk;
        }
        OMATCH_END ();
    }
    sd->def += (refinedef + 50) / 100;

    if (sd->attackrange < 1)
        sd->attackrange = 1;
    if (sd->status.weapon == ItemLook::W_BOW)
        sd->attackrange += sd->arrow_range;
    sd->double_rate += sd->double_add_rate;
    sd->perfect_hit += sd->perfect_hit_add_rate;
    sd->deadly_strike += sd->deadly_strike_add_rate;
    if (sd->speed_add_rate != 100)
        sd->speed_rate += sd->speed_add_rate - 100;
    if (sd->aspd_add_rate != 100)
        sd->aspd_rate += sd->aspd_add_rate - 100;

    sd->speed -= std::chrono::milliseconds(skill_power(sd, SkillID::TMW_SPEED) / 8);
    sd->aspd_rate -= skill_power(sd, SkillID::TMW_SPEED) / 10;
    if (sd->aspd_rate < 20)
        sd->aspd_rate = 20;

    for (ATTR attr : ATTRs)
        sd->paramc[attr] = std::max(0, sd->status.attrs[attr] + sd->paramb[attr] + sd->parame[attr]);

    if (sd->status.weapon == ItemLook::W_BOW)
    {
        str = sd->paramc[ATTR::DEX];
        dex = sd->paramc[ATTR::STR];
    }
    else
    {
        str = sd->paramc[ATTR::STR];
        dex = sd->paramc[ATTR::DEX];
        sd->critical += ((dex * 3) >> 1);
    }
    dstr = str / 10;
    sd->base_atk += str + dstr * dstr + dex / 5 + sd->paramc[ATTR::LUK] / 5;
    sd->matk1 += sd->paramc[ATTR::INT] + (sd->paramc[ATTR::INT] / 5) * (sd->paramc[ATTR::INT] / 5);
    sd->matk2 += sd->paramc[ATTR::INT] + (sd->paramc[ATTR::INT] / 7) * (sd->paramc[ATTR::INT] / 7);

    if (sd->sc_data[StatusChange::SC_MATKPOT].timer)
    {
        sd->matk1 += sd->sc_data[StatusChange::SC_MATKPOT].val1;
        sd->matk2 += sd->sc_data[StatusChange::SC_MATKPOT].val1;
    }

    if (sd->matk1 < sd->matk2)
    {
        int temp = sd->matk2;
        sd->matk2 = sd->matk1;
        sd->matk1 = temp;
    }
    // [Fate] New tmw magic system
    sd->matk1 += sd->status.base_level + sd->spellpower_bonus_current;
#ifdef USE_ASTRAL_SOUL_SKILL
    if (sd->matk1 > MAGIC_SKILL_THRESHOLD)
    {
        int bonus = sd->matk1 - MAGIC_SKILL_THRESHOLD;
        // Ok if you are above a certain threshold, you get only (1/8) of that matk1
        // if you have Astral soul skill you can get the whole power again (and additionally the 1/8 added)
        sd->matk1 = MAGIC_SKILL_THRESHOLD + (bonus>>3) + ((3*bonus*skill_power(sd, SkillID::TMW_ASTRAL_SOUL))>>9);
    }
#endif
    sd->matk2 = 0;
    if (sd->matk1 < 0)
        sd->matk1 = 0;
    else
        if (sd->matk_rate != 100)
        {
            sd->matk1 = (sd->matk1 * sd->matk_rate) / 100;
        }

    sd->hit += sd->paramc[ATTR::DEX] + sd->status.base_level;
    sd->flee += sd->paramc[ATTR::AGI] + sd->status.base_level;
    sd->def2 += sd->paramc[ATTR::VIT];
    sd->mdef2 += sd->paramc[ATTR::INT];
    sd->flee2 += sd->paramc[ATTR::LUK] + 10;
    sd->critical += (sd->paramc[ATTR::LUK] * 3) + 10;

    // 200 is the maximum of the skill
    // def2 is the defence gained by vit, whereas "def", which is gained by armor, stays as is
    int spbsk = skill_power(sd, SkillID::TMW_RAGING);
    if (spbsk != 0 && sd->attackrange <= 2)
    {
        sd->critical += sd->critical * spbsk / 100;
        sd->def2 = (sd->def2 * 256) / (256 + spbsk);
    }

    if (sd->base_atk < 1)
        sd->base_atk = 1;
    if (sd->critical_rate != 100)
        sd->critical = (sd->critical * sd->critical_rate) / 100;
    if (sd->critical < 10)
        sd->critical = 10;
    if (sd->hit_rate != 100)
        sd->hit = (sd->hit * sd->hit_rate) / 100;
    if (sd->hit < 1)
        sd->hit = 1;
    if (sd->flee_rate != 100)
        sd->flee = (sd->flee * sd->flee_rate) / 100;
    if (sd->flee < 1)
        sd->flee = 1;
    if (sd->flee2_rate != 100)
        sd->flee2 = (sd->flee2 * sd->flee2_rate) / 100;
    if (sd->flee2 < 10)
        sd->flee2 = 10;
    if (sd->def_rate != 100)
        sd->def = (sd->def * sd->def_rate) / 100;
    if (sd->def < 0)
        sd->def = 0;
    if (sd->def2_rate != 100)
        sd->def2 = (sd->def2 * sd->def2_rate) / 100;
    if (sd->def2 < 1)
        sd->def2 = 1;
    if (sd->mdef_rate != 100)
        sd->mdef = (sd->mdef * sd->mdef_rate) / 100;
    if (sd->mdef < 0)
        sd->mdef = 0;
    if (sd->mdef2_rate != 100)
        sd->mdef2 = (sd->mdef2 * sd->mdef2_rate) / 100;
    if (sd->mdef2 < 1)
        sd->mdef2 = 1;

    //二刀流 ASPD 修正 | Two-cut ASPD correction
    {
        b_base_weapon_delay_adjust = aspd_base_0[sd->status.weapon] + sd->base_weapon_delay_adjust;
        if (b_base_weapon_delay_adjust.count() < 0)
            b_base_weapon_delay_adjust = interval_t::zero();
        sd->aspd += b_base_weapon_delay_adjust
            - (sd->paramc[ATTR::AGI] * 4 + sd->paramc[ATTR::DEX])
            * b_base_weapon_delay_adjust / 1000;
    }

    aspd_rate = sd->aspd_rate;
    speed_rate = sd->speed_rate;

    //攻撃速度増加 | Increased attack speed

    if (sd->attackrange > 2)
    {
        // [fate] ranged weapon?
        sd->attackrange += std::min(skill_power(sd, SkillID::AC_OWL) / 60, 3);
        sd->hit += skill_power(sd, SkillID::AC_OWL) / 10;   // 20 for 200
    }

    if (sd->max_weight_override)
        sd->max_weight = sd->max_weight_override;

    sd->max_weight += 1000;

    bl = sd->status.base_level;

    sd->status.max_hp += (
            3500
            + bl * hp_coefficient2_0
            + hp_sigma_val_0[(bl > 0) ? bl - 1 : 0]
            ) / 100 * (100 + sd->paramc[ATTR::VIT]) / 100
        + (sd->parame[ATTR::VIT] - sd->paramcard[ATTR::VIT]);
    if (sd->hprate != 100)
        sd->status.max_hp = sd->status.max_hp * sd->hprate / 100;

    if (sd->status.max_hp > battle_config.max_hp)   // removed negative max hp bug by Valaris
        sd->status.max_hp = battle_config.max_hp;
    if (sd->status.max_hp <= 0)
        sd->status.max_hp = 1;  // end

    // 最大SP計算 | Maximum SP calculation
    sd->status.max_sp += ((sp_coefficient_0 * bl) + 1000)
        / 100 * (100 + sd->paramc[ATTR::INT]) / 100
        + (sd->parame[ATTR::INT] - sd->paramcard[ATTR::INT]);
    if (sd->sprate != 100)
        sd->status.max_sp = sd->status.max_sp * sd->sprate / 100;

    if (sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
        sd->status.max_sp = battle_config.max_sp;

    //自然回復HP | Natural Recovery HP
    sd->nhealhp = 1 + (sd->paramc[ATTR::VIT] / 5) + (sd->status.max_hp / 200);
    //自然回復SP | Natural Recovery SP
    sd->nhealsp = 1 + (sd->paramc[ATTR::INT] / 6) + (sd->status.max_sp / 100);
    if (sd->paramc[ATTR::INT] >= 120)
        sd->nhealsp += ((sd->paramc[ATTR::INT] - 120) >> 1) + 4;

    if (sd->hprecov_rate != 100)
    {
        sd->nhealhp = sd->nhealhp * sd->hprecov_rate / 100;
        if (sd->nhealhp < 1)
            sd->nhealhp = 1;
    }
    if (sd->sprecov_rate != 100)
    {
        sd->nhealsp = sd->nhealsp * sd->sprecov_rate / 100;
        if (sd->nhealsp < 1)
            sd->nhealsp = 1;
    }

    //スキルやステータス異常による残りのパラメータ補正 | Correction of the remaining parameters due to skill or status anomalies
    {
        // ATK/DEF変化形 | ATK/DEF variants
        if (sd->sc_data[StatusChange::SC_POISON].timer) // 毒状態 | Poisonous state
            sd->def2 = sd->def2 * 75 / 100;

        if (sd->sc_data[StatusChange::SC_ATKPOT].timer)
            sd->watk += sd->sc_data[StatusChange::SC_ATKPOT].val1;

        if (sd->sc_data[StatusChange::SC_SPEEDPOTION0].timer)
            aspd_rate -= sd->sc_data[StatusChange::SC_SPEEDPOTION0].val1;

        if (sd->sc_data[StatusChange::SC_HASTE].timer)
            aspd_rate -= sd->sc_data[StatusChange::SC_HASTE].val1;

        /* Slow down if protected */

        if (sd->sc_data[StatusChange::SC_PHYS_SHIELD].timer || sd->sc_data[StatusChange::SC_PHYS_SHIELD_ITEM].timer)
            aspd_rate += std::max(sd->sc_data[StatusChange::SC_PHYS_SHIELD].val1, sd->sc_data[StatusChange::SC_PHYS_SHIELD_ITEM].val1); // highest value is taken here but serverdata should make sure only one of those is active

        if (sd->sc_data[StatusChange::SC_SLOWMOVE].timer)
            speed_rate += sd->sc_data[StatusChange::SC_SLOWMOVE].val1;
    }

    if (speed_rate != 100)
        sd->speed = sd->speed * speed_rate / 100;
    sd->speed = std::max(sd->speed, 1_ms);
    if (sd->speed_cap < interval_t::zero())
        sd->speed_cap = interval_t::zero();
    if (sd->speed < sd->speed_cap)
        sd->speed = sd->speed_cap;
    if (aspd_rate != 100)
        sd->aspd = sd->aspd * aspd_rate / 100;

    /* Magic speed */
    if (sd->attack_spell_override || first & (int)CalcStatusKind::MAGIC_OVERRIDE)
        sd->aspd = sd->attack_spell_delay;

    /* Red Threshold Calculation */
    if (sd->aspd < 300_ms) {
        sd->aspd = 300_ms + ((sd->aspd - 300_ms) * 19 / 20);
    }

    sd->aspd = std::max(sd->aspd, battle_config.max_aspd);
    sd->amotion = sd->aspd;
    sd->dmotion = std::chrono::milliseconds(800 - sd->paramc[ATTR::AGI] * 4);
    sd->dmotion = std::max(sd->dmotion, 400_ms);

    if (sd->status.hp > sd->status.max_hp)
        sd->status.hp = sd->status.max_hp;
    if (sd->status.sp > sd->status.max_sp)
        sd->status.sp = sd->status.max_sp;

    if (first & (int)CalcStatusKind::NORMAL_RECALC_NO_CLIENT_UPDATE)
        return 0;
    if (first & ((int)CalcStatusKind::INITIAL_CALC + (int)CalcStatusKind::ITEM_BONUS_RECALC)) // never executed atm
    {
        clif_updatestatus(sd, SP::SPEED);
        clif_updatestatus(sd, SP::MAXHP);
        clif_updatestatus(sd, SP::MAXSP);
        if (first & (int)CalcStatusKind::INITIAL_CALC) // its always 1 here if first is 3 so this if is not needed normally
        {
            clif_updatestatus(sd, SP::HP);
            clif_updatestatus(sd, SP::SP);
        }
        return 0;
    }

    if (b_skill != sd->status.skill
        || b_attackrange != sd->attackrange)
        clif_skillinfoblock(sd);

    if (b_speed != sd->speed)
        clif_updatestatus(sd, SP::SPEED);
    if (b_weight != sd->weight)
        clif_updatestatus(sd, SP::WEIGHT);
    if (b_max_weight != sd->max_weight)
    {
        clif_updatestatus(sd, SP::MAXWEIGHT);
        pc_checkweighticon(sd);
    }
    for (ATTR i : ATTRs)
        if (b_paramb[i] + b_parame[i] != sd->paramb[i] + sd->parame[i])
            clif_updatestatus(sd, attr_to_sp(i));
    if (b_hit != sd->hit)
        clif_updatestatus(sd, SP::HIT);
    if (b_flee != sd->flee)
        clif_updatestatus(sd, SP::FLEE1);
    if (b_aspd != sd->aspd)
        clif_updatestatus(sd, SP::ASPD);
    if (b_watk != sd->watk || b_base_atk != sd->base_atk)
        clif_updatestatus(sd, SP::ATK1);
    if (b_def != sd->def)
        clif_updatestatus(sd, SP::DEF1);
    if (b_watk2 != sd->watk2)
        clif_updatestatus(sd, SP::ATK2);
    if (b_def2 != sd->def2)
        clif_updatestatus(sd, SP::DEF2);
    if (b_flee2 != sd->flee2)
        clif_updatestatus(sd, SP::FLEE2);
    if (b_critical != sd->critical)
        clif_updatestatus(sd, SP::CRITICAL);
    if (b_matk1 != sd->matk1)
        clif_updatestatus(sd, SP::MATK1);
    if (b_matk2 != sd->matk2)
        clif_updatestatus(sd, SP::MATK2);
    if (b_mdef != sd->mdef)
        clif_updatestatus(sd, SP::MDEF1);
    if (b_mdef2 != sd->mdef2)
        clif_updatestatus(sd, SP::MDEF2);
    if (b_attackrange != sd->attackrange)
        clif_updatestatus(sd, SP::ATTACKRANGE);
    if (b_max_hp != sd->status.max_hp)
        clif_updatestatus(sd, SP::MAXHP);
    if (b_max_sp != sd->status.max_sp)
        clif_updatestatus(sd, SP::MAXSP);
    if (b_hp != sd->status.hp)
        clif_updatestatus(sd, SP::HP);
    if (b_sp != sd->status.sp)
        clif_updatestatus(sd, SP::SP);
    if (b_pvpchannel != sd->state.pvpchannel)
        sd->state.pvpchannel = b_pvpchannel;

    return 0;
}

/*==========================================
 * 装 備品による能力等のボーナス設定
 * Bonus settings such as abilities by equipment
 *------------------------------------------
 */
int pc_bonus(dumb_ptr<map_session_data> sd, SP type, int val)
{
    nullpo_retz(sd);

    switch (type)
    {
        case SP::STR:
        case SP::AGI:
        case SP::VIT:
        case SP::INT:
        case SP::DEX:
        case SP::LUK:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->parame[sp_to_attr(type)] += val;
            break;
        case SP::ATK1:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->watk += val;
            break;
        case SP::ATK2:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->watk2 += val;
            break;
        case SP::BASE_ATK:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->base_atk += val;
            break;
        case SP::MATK1:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->matk1 += val;
            break;
        case SP::MATK2:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->matk2 += val;
            break;
        case SP::DEF1:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->def += val;
            break;
        case SP::DEF2:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->def2 += val;
            break;
        case SP::MDEF1:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->mdef += val;
            break;
        case SP::MDEF2:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->mdef2 += val;
            break;
        case SP::HIT:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->hit += val;
            else
                sd->arrow_hit += val;
            break;
        case SP::FLEE1:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->flee += val;
            break;
        case SP::FLEE2:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->flee2 += val * 10;
            break;
        case SP::CRITICAL:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->critical += val * 10;
            else
                sd->arrow_cri += val * 10;
            break;
        case SP::MAXHP:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->status.max_hp += val;
            break;
        case SP::MAXSP:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->status.max_sp += val;
            break;
        case SP::MAXHPRATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->hprate += val;
            break;
        case SP::MAXSPRATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->sprate += val;
            break;
#if 0
        case SP::SPRATE: // mana is subtracted at serverdata this can only become useable if spells are handled by servercode
            if (!sd->state.lr_flag_is_arrow_2)
                sd->dsprate += val;
            break;
#endif
        case SP::ATTACKRANGE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->attackrange += val;
            else
                sd->arrow_range += val;
            break;
        case SP::ADD_SPEED:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->speed -= interval_t(val);
            break;
        case SP::SPEED_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
            {
                if (sd->speed_rate > 100 - val)
                    sd->speed_rate = 100 - val;
            }
            break;
        case SP::SPEED_ADDRATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->speed_add_rate = sd->speed_add_rate * (100 - val) / 100;
            break;
        case SP::ASPD:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->aspd -= interval_t(val);
            break;
        case SP::ASPD_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
            {
                if (sd->aspd_rate > 100 - val)
                    sd->aspd_rate = 100 - val;
            }
            break;
        case SP::ASPD_ADDRATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->aspd_add_rate = sd->aspd_add_rate * (100 - val) / 100;
            break;
        case SP::HP_RECOV_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->hprecov_rate += val;
            break;
        case SP::SP_RECOV_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->sprecov_rate += val;
            break;
        case SP::CRITICAL_DEF:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->critical_def += val;
            break;
        case SP::DOUBLE_RATE:
            if (!sd->state.lr_flag_is_arrow_2 && sd->double_rate < val)
                sd->double_rate = val;
            break;
        case SP::DOUBLE_ADD_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->double_add_rate += val;
            break;
        case SP::MATK_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->matk_rate += val;
            break;
        case SP::ATK_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->atk_rate += val;
            break;
        case SP::PERFECT_HIT_RATE:
            if (!sd->state.lr_flag_is_arrow_2 && sd->perfect_hit < val)
                sd->perfect_hit = val;
            break;
        case SP::PERFECT_HIT_ADD_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->perfect_hit_add_rate += val;
            break;
        case SP::CRITICAL_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->critical_rate += val;
            break;
        case SP::HIT_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->hit_rate += val;
            break;
        case SP::FLEE_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->flee_rate += val;
            break;
        case SP::FLEE2_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->flee2_rate += val;
            break;
        case SP::DEF_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->def_rate += val;
            break;
        case SP::DEF2_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->def2_rate += val;
            break;
        case SP::MDEF_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->mdef_rate += val;
            break;
        case SP::MDEF2_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->mdef2_rate += val;
            break;
        case SP::DEAF:
            sd->special_state.deaf = 1;
            break;
        case SP::SPEED_CAP:
            if (!sd->state.lr_flag_is_arrow_2)
                // highest value (slowest speed) is taken others are ignored
                if (sd->speed_cap < interval_t(val))
                    sd->speed_cap = interval_t(val);
            break;
        case SP::ALL_STATS:
            sd->parame[ATTR::STR] += val;
            sd->parame[ATTR::AGI] += val;
            sd->parame[ATTR::VIT] += val;
            sd->parame[ATTR::INT] += val;
            sd->parame[ATTR::DEX] += val;
            sd->parame[ATTR::LUK] += val;
            break;
        case SP::AGI_VIT:
            sd->parame[ATTR::AGI] += val;
            sd->parame[ATTR::VIT] += val;
            break;
        case SP::AGI_DEX_STR:
            sd->parame[ATTR::AGI] += val;
            sd->parame[ATTR::DEX] += val;
            sd->parame[ATTR::STR] += val;
            break;
        case SP::DEADLY_STRIKE_RATE:
            if (!sd->state.lr_flag_is_arrow_2 && sd->deadly_strike < val)
                sd->deadly_strike = val;
            break;
        case SP::DEADLY_STRIKE_ADD_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->deadly_strike_add_rate += val;
            break;
        case SP::BASE_WEAPON_DELAY_ADJUST:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->base_weapon_delay_adjust += interval_t(val);
            break;
        case SP::MAXWEIGHT:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->max_weight = val;
            break;
        case SP::MAXWEIGHT_ADD:
            if (!sd->state.lr_flag_is_arrow_2)
                sd->max_weight += val;
            break;
        default:
            if (battle_config.error_log)
                PRINTF("pc_bonus: unknown type %d %d !\n"_fmt,
                        type, val);
            break;
    }
    return 0;
}

/*==========================================
 * ｿｽｿｽ ｿｽｿｽｿｽiｿｽﾉゑｿｽｿｽｿｽｿｽ\ｿｽﾍ難ｿｽｿｽﾌボｿｽ[ｿｽiｿｽXｿｽﾝ抵ｿｽ
 * sos sos sos sos sos i sos no sos sos sos sos\
 *------------------------------------------
 */
int pc_bonus2(dumb_ptr<map_session_data> sd, SP type, int type2, int val)
{
    nullpo_retz(sd);

    switch (type)
    {
        case SP::HP_DRAIN_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
            {
                sd->hp_drain_rate += type2;
                sd->hp_drain_per += val;
            }
            break;
        case SP::SP_DRAIN_RATE:
            if (!sd->state.lr_flag_is_arrow_2)
            {
                sd->sp_drain_rate += type2;
                sd->sp_drain_per += val;
            }
            break;
        default:
            if (battle_config.error_log)
                PRINTF("pc_bonus2: unknown type %d %d %d!\n"_fmt,
                        type, type2, val);
            break;
    }
    return 0;
}

/*==========================================
 * スクリプトによるスキル所得
 * Skill Income by Script
 *------------------------------------------
 */
int pc_skill(dumb_ptr<map_session_data> sd, SkillID id, int level, int flag)
{
    nullpo_retz(sd);

    if (level > MAX_SKILL_LEVEL)
    {
        if (battle_config.error_log)
            PRINTF("support card skill only!\n"_fmt);
        return 0;
    }
    if (!flag && (sd->status.skill[id].lv || level == 0))
    {
        sd->status.skill[id].lv = level;
        pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
        clif_skillinfoblock(sd);
    }
    else if (sd->status.skill[id].lv < level)
    {
        sd->status.skill[id].lv = level;
    }

    return 0;
}


/*==========================================
 * アイテムを買った時に、新しいアイテム欄を使うか、
 * 3万個制限にかかるか確認
 * When you buy an item, use the new item field, or
 * Check if you hit the 30,000 limit
 *------------------------------------------
 */
ADDITEM pc_checkadditem(dumb_ptr<map_session_data> sd, ItemNameId nameid, int amount)
{
    nullpo_retr(ADDITEM::ZERO, sd);

    if (itemdb_isequip(nameid))
        return ADDITEM::NEW;

    for (IOff0 i : IOff0::iter())
    {
        if (sd->status.inventory[i].nameid == nameid)
        {
            if (sd->status.inventory[i].amount + amount > MAX_AMOUNT)
                return ADDITEM::OVERAMOUNT;
            return ADDITEM::EXIST;
        }
    }

    if (amount > MAX_AMOUNT)
        return ADDITEM::OVERAMOUNT;
    return ADDITEM::NEW;
}

/*==========================================
 * 空きアイテム欄の個数
 * Number of free item fields
 *------------------------------------------
 */
int pc_inventoryblank(dumb_ptr<map_session_data> sd)
{
    int b = 0;

    nullpo_retz(sd);

    for (IOff0 i : IOff0::iter())
    {
        if (!sd->status.inventory[i].nameid)
            b++;
    }

    return b;
}

/*==========================================
 * お金を払う
 * Pay Money
 *------------------------------------------
 */
int pc_payzeny(dumb_ptr<map_session_data> sd, int zeny)
{
    nullpo_retz(sd);

    double z = sd->status.zeny;
    if (sd->status.zeny < zeny || z - zeny > MAX_ZENY)
        return 1;
    sd->status.zeny -= zeny;
    clif_updatestatus(sd, SP::ZENY);

    return 0;
}

/*==========================================
 * お金を得る
 * Get Money
 *------------------------------------------
 */
int pc_getzeny(dumb_ptr<map_session_data> sd, int zeny)
{
    nullpo_retz(sd);

    double z = sd->status.zeny;
    if (z + zeny > MAX_ZENY)
    {
        zeny = 0;
        sd->status.zeny = MAX_ZENY;
    }
    sd->status.zeny += zeny;
    clif_updatestatus(sd, SP::ZENY);

    return 0;
}

/*==========================================
 * アイテムを探して、インデックスを返す
 * Find an item and return an index
 *------------------------------------------
 */
IOff0 pc_search_inventory(dumb_ptr<map_session_data> sd, ItemNameId item_id)
{
    nullpo_retr(IOff0::from(-1), sd);

    for (IOff0 i : IOff0::iter())
    {
        if (sd->status.inventory[i].nameid == item_id &&
            (sd->status.inventory[i].amount > 0 || !item_id))
            return i;
    }

    return IOff0::from(-1);
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_count_all_items(dumb_ptr<map_session_data> player, ItemNameId item_id)
{
    int count = 0;

    nullpo_retz(player);

    for (IOff0 i : IOff0::iter())
    {
        if (player->status.inventory[i].nameid == item_id)
            count += player->status.inventory[i].amount;
    }

    return count;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_remove_items(dumb_ptr<map_session_data> player, ItemNameId item_id, int count)
{
    nullpo_retz(player);

    for (IOff0 i : IOff0::iter())
    {
        if (!count)
            break;
        if (player->status.inventory[i].nameid == item_id)
        {
            int to_delete = count;
            /* only delete as much as we have */
            if (to_delete > player->status.inventory[i].amount)
                to_delete = player->status.inventory[i].amount;

            count -= to_delete;

            pc_delitem(player, i, to_delete,
                        0 /* means `really delete and update status' */ );

            if (!count)
                return 0;
        }
    }
    return 0;
}

/*==========================================
 * アイテム追加。個数のみitem構造体の数字を無視
 * Add Item. Ignore the number of items only in the structure
 *------------------------------------------
 */
PickupFail pc_additem(dumb_ptr<map_session_data> sd, Item *item_data,
                int amount)
{
    int w;

    MAP_LOG_PC(sd, "PICKUP %d %d"_fmt, item_data->nameid, amount);

    nullpo_retr(PickupFail::BAD_ITEM, sd);
    nullpo_retr(PickupFail::BAD_ITEM, item_data);

    if (!item_data->nameid || amount <= 0)
        return PickupFail::BAD_ITEM;
    P<struct item_data> data = itemdb_search(item_data->nameid);
    if ((w = data->weight * amount) + sd->weight > sd->max_weight)
        return PickupFail::TOO_HEAVY;

    IOff0 i = IOff0::from(MAX_INVENTORY);

    if (!itemdb_isequip2(data))
    {
        // TODO see if there's any nicer way to preserve the foreach var
        for (i = IOff0::from(0); i != IOff0::from(MAX_INVENTORY); ++i)
        {
            if (sd->status.inventory[i].nameid == item_data->nameid)
            {
                if (sd->status.inventory[i].amount + amount > MAX_AMOUNT)
                    return PickupFail::STACK_FULL;
                sd->status.inventory[i].amount += amount;
                clif_additem(sd, i, amount, PickupFail::OKAY);
                break;
            }
        }
    }
    if (!i.ok())
    {
        i = pc_search_inventory(sd, ItemNameId());
        if (i.ok())
        {
            sd->status.inventory[i] = *item_data;

            if (bool(item_data->equip))
                sd->status.inventory[i].equip = EPOS::ZERO;

            sd->status.inventory[i].amount = amount;
            sd->inventory_data[i] = Some(data);
            clif_additem(sd, i, amount, PickupFail::OKAY);
        }
        else
            return PickupFail::INV_FULL;
    }
    sd->weight += w;
    clif_updatestatus(sd, SP::WEIGHT);

    return PickupFail::OKAY;
}

/*==========================================
 * アイテムを減らす
 * Reduce items
 *------------------------------------------
 */
int pc_delitem(dumb_ptr<map_session_data> sd, IOff0 n, int amount, int type)
{
    nullpo_retr(1, sd);

    if (sd->trade_partner)
        trade_tradecancel(sd);

    if (!sd->status.inventory[n].nameid || amount <= 0
        || sd->status.inventory[n].amount < amount)
        return 1;
    P<struct item_data> sdidn = TRY_UNWRAP(sd->inventory_data[n], return 1);

    sd->status.inventory[n].amount -= amount;
    sd->weight -= sdidn->weight * amount;
    if (sd->status.inventory[n].amount <= 0)
    {
        if (bool(sd->status.inventory[n].equip))
            pc_unequipitem(sd, n, CalcStatus::NOW);
        sd->status.inventory[n] = Item{};
        sd->inventory_data[n] = None;
    }
    if (!(type & 1))
        clif_delitem(sd, n, amount);
    if (!(type & 2))
        clif_updatestatus(sd, SP::WEIGHT);

    return 0;
}

/*==========================================
 * アイテムを落す
 * Drop an item
 *------------------------------------------
 */
int pc_dropitem(dumb_ptr<map_session_data> sd, IOff0 n, int amount)
{
    nullpo_retr(1, sd);

    if (sd->trade_partner || sd->npc_id || sd->state.storage_open)
        return 0;               // no dropping while trading/npc/storage

    if (!n.ok())
        return 0;

    if (amount <= 0)
        return 0;

    pc_unequipinvyitem(sd, n, CalcStatus::NOW);

    if (!sd->status.inventory[n].nameid ||
        sd->status.inventory[n].amount < amount ||
        sd->trade_partner || sd->status.inventory[n].amount <= 0)
        return 1;
    map_addflooritem(&sd->status.inventory[n], amount,
            sd->bl_m, sd->bl_x, sd->bl_y,
            nullptr, nullptr, nullptr);
    pc_delitem(sd, n, amount, 0);

    return 0;
}

/*==========================================
 * アイテムを拾う
 * Pick up items
 *------------------------------------------
 */
static
int can_pick_item_up_from(dumb_ptr<map_session_data> self, BlockId other_id)
{
    /* From ourselves or from no-one? */
    if (!self || self->bl_id == other_id || !other_id)
        return 1;

    dumb_ptr<map_session_data> other = map_id2sd(other_id);

    /* Other no longer exists? */
    if (!other)
        return 1;

    /* From our partner? */
    if (self->status.partner_id == other->status_key.char_id)
        return 1;

    /* From a party member? */
    Option<PartyPair> p = party_search(self->status.party_id);
    if (self->status.party_id
        && self->status.party_id == other->status.party_id
        && p.pmd_pget(&PartyMost::item).copy_or(0) != 0)
        return 1;

    /* From someone who is far away? */
    /* On another map? */
    if (other->bl_m != self->bl_m)
        return 1;
    else
    {
        int distance_x = abs(other->bl_x - self->bl_x);
        int distance_y = abs(other->bl_y - self->bl_y);
        int distance = (distance_x > distance_y) ? distance_x : distance_y;

        return distance > battle_config.drop_pickup_safety_zone;
    }
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_takeitem(dumb_ptr<map_session_data> sd, dumb_ptr<flooritem_data> fitem)
{
    tick_t tick = gettick();
    int can_take;

    nullpo_retz(sd);
    nullpo_retz(fitem);

    /* Sometimes the owners reported to us are buggy: */

    if (fitem->first_get_id == fitem->third_get_id
        || fitem->second_get_id == fitem->third_get_id)
        fitem->third_get_id = BlockId();

    if (fitem->first_get_id == fitem->second_get_id)
    {
        fitem->second_get_id = fitem->third_get_id;
        fitem->third_get_id = BlockId();
    }

    can_take = can_pick_item_up_from(sd, fitem->first_get_id);
    if (!can_take)
        can_take = fitem->first_get_tick <= tick
            && can_pick_item_up_from(sd, fitem->second_get_id);

    if (!can_take)
        can_take = fitem->second_get_tick <= tick
            && can_pick_item_up_from(sd, fitem->third_get_id);

    if (!can_take)
        can_take = fitem->third_get_tick <= tick;

    if (can_take)
    {
        /* Can pick up */

        PickupFail flag = pc_additem(sd, &fitem->item_data, fitem->item_data.amount);
        if (flag != PickupFail::OKAY)
            //重量overで取得失敗 | Failure to get over weight
            clif_additem(sd, IOff0::from(0), 0, flag);
        else
        {
            //取得成功 | Succeed
            if (sd->attacktimer)
                pc_stopattack(sd);
            clif_takeitem(sd, fitem);
            map_clearflooritem(fitem->bl_id);
        }
        return 0;
    }

    /* Otherwise, we can't pick up */
    clif_additem(sd, IOff0::from(0), 0, PickupFail::DROP_STEAL);
    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_isUseitem(dumb_ptr<map_session_data> sd, IOff0 n)
{
    ItemNameId nameid;

    nullpo_retz(sd);

    P<struct item_data> item = TRY_UNWRAP(sd->inventory_data[n], return 0);
    nameid = sd->status.inventory[n].nameid;

    if (itemdb_type(nameid) != ItemType::USE)
        return 0;

    if (item->sex != SEX::UNSPECIFIED && sd->status.sex != item->sex)
        return 0;
    if (item->elv > 0 && sd->status.base_level < item->elv)
        return 0;

    return 1;
}

/*==========================================
 * アイテムを使う
 * Working with Items
 *------------------------------------------
 */
int pc_useitem(dumb_ptr<map_session_data> sd, IOff0 n)
{
    int amount;

    nullpo_retr(1, sd);

    if (!n.ok())
        return 0;
    OMATCH_BEGIN_SOME (sdidn, sd->inventory_data[n])
    {
        amount = sd->status.inventory[n].amount;
        if (!sd->status.inventory[n].nameid
            || sd->status.inventory[n].amount <= 0
            || !pc_isUseitem(sd, n))
        {
            clif_useitemack(sd, n, 0, 0);
            return 1;
        }

        P<const ScriptBuffer> script = borrow(*sdidn->use_script);

        if (!bool(sdidn->mode & ItemMode::KEEP_AFTER_USE))
        {
            clif_useitemack(sd, n, amount - 1, 1);
            pc_delitem(sd, n, 1, 1);
        }

        // activity
        if (sd)
            if (sd->activity.items_used == 2147483647)
                sd->activity.items_used = 1;
            else
                sd->activity.items_used++;

        run_script(ScriptPointer(script, 0), sd->bl_id, BlockId());
    }
    OMATCH_END ();

    return 0;
}

/*==========================================
 * PCの位置設定
 * PC location setting
 *------------------------------------------
 */
int pc_setpos(dumb_ptr<map_session_data> sd,
        MapName mapname_org, int x, int y, BeingRemoveWhy clrtype)
{
    MapName mapname_;

    nullpo_retz(sd);

    if (sd->trade_partner)      //取引を中断する | Suspend a transaction
        trade_tradecancel(sd);
    if (sd->state.storage_open)
        storage_storage_quit(sd);  //倉庫を開いてるなら保存する | If you have a warehouse open, save it

    if (sd->party_invite)   //パーティ勧誘を拒否する | Refuse to solicit a party
        party_reply_invite(sd, sd->party_invite_account, 0);

    skill_castcancel(sd, 0);  //詠唱中断 | The singing is interrupted
    pc_stop_walking(sd, 0);    //歩行中断 | Walking interruption
    pc_stopattack(sd);         //攻撃中断 | Attack Interruption

    if (pc_issit(sd))
    {
//        pc_setstand (sd); // [fate] Nothing wrong with warping while sitting
    }

    mapname_ = mapname_org;

    Option<P<map_local>> m_ = map_mapname2mapid(mapname_);
    if (m_.is_none())
    {
        if (sd->mapname_)
        {
            IP4Address ip;
            int port;
            if (map_mapname2ipport(mapname_, borrow(ip), borrow(port)) == 0)
            {
                skill_stop_dancing(sd, 1);
                clif_clearchar(sd, clrtype);
                map_delblock(sd);
                // *cringe*
                sd->mapname_ = mapname_;
                sd->bl_x = x;
                sd->bl_y = y;
                sd->state.waitingdisconnect = 1;
                pc_makesavestatus(sd);
                //The storage close routines save the char data. [Skotlex]
                if (!sd->state.storage_open)
                    chrif_save(sd);
                else if (sd->state.storage_open)
                    storage_storage_quit(sd);

                chrif_changemapserver(sd, mapname_, x, y, ip, port);
                return 0;
            }
        }
#if 0
        clif_authfail_fd(sd->fd, 0);   // cancel
        clif_setwaitclose(sd->fd);
#endif
        return 1;
    }
    P<map_local> m = TRY_UNWRAP(m_, abort());

    if (x < 0 || x >= m->xs || y < 0 || y >= m->ys)
        x = y = 0;
    if ((x == 0 && y == 0)
        || bool(read_gatp(m, x, y) & MapCell::UNWALKABLE))
    {
        if (x || y)
        {
            if (battle_config.error_log)
                PRINTF("stacked (%d,%d)\n"_fmt, x, y);
        }
        do
        {
            x = random_::in(1, m->xs - 2);
            y = random_::in(1, m->ys - 2);
        }
        while (bool(read_gatp(m, x, y) & MapCell::UNWALKABLE));
    }

    if (sd->mapname_ && sd->bl_prev != nullptr)
    {
        clif_clearchar(sd, clrtype);
        map_delblock(sd);
        clif_changemap(sd, m->name_, x, y); // [MouseJstr]
    }

    sd->mapname_ = mapname_;
    sd->bl_m = m;
    sd->to_x = x;
    sd->to_y = y;

    // moved and changed dance effect stopping

    sd->bl_x = x;
    sd->bl_y = y;

//  map_addblock(sd);  //ブロック登録とspawnは | Block registration and spawn
//  clif_spawnpc(sd);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int pc_can_reach(dumb_ptr<map_session_data> sd, int x, int y)
{
    struct walkpath_data wpd;

    nullpo_retz(sd);

    if (sd->bl_x == x && sd->bl_y == y) // 同じマス
        return 1;

    //障害物判定 | Obstacle determination
    wpd.path_len = 0;
    wpd.path_pos = 0;
    wpd.path_half = 0;
    return (path_search(&wpd, sd->bl_m, sd->bl_x, sd->bl_y, x, y, 0) != -1);
}

//
// 歩 行物 | Walks
//
/*==========================================
 * 次の1歩にかかる時間を計算
 * Calculate how long it will take to take the next step
 *------------------------------------------
 */
static
interval_t calc_next_walk_step(dumb_ptr<map_session_data> sd)
{
    nullpo_retr(interval_t::zero(), sd);

    if (sd->walkpath.path_pos >= sd->walkpath.path_len)
        return static_cast<interval_t>(-1);
    if (dir_is_diagonal(sd->walkpath.path[sd->walkpath.path_pos]))
        return sd->speed * 14 / 10;

    return sd->speed;
}

/*==========================================
 * 半歩進む(timer関数)
 * Half a step forward (timer function)
 *------------------------------------------
 */
static
void pc_walk(TimerData *, tick_t tick, BlockId id, unsigned char data)
{
    dumb_ptr<map_session_data> sd;
    int moveblock;
    int x, y, dx, dy;

    sd = map_id2sd(id);
    if (sd == nullptr)
        return;

    if (sd->walkpath.path_pos >= sd->walkpath.path_len
        || sd->walkpath.path_pos != data)
        return;

    sd->walkpath.path_half ^= 1;
    if (sd->walkpath.path_half == 0)
    {                           //マス目中心へ到着 | Arrive at the center of the grid
        sd->walkpath.path_pos++;
        if (sd->state.change_walk_target)
        {
            pc_walktoxy_sub(sd);
            return;
        }
    }
    else
    {                           //マス目境界へ到着 | Arriving at the grid boundary
        if (sd->walkpath.path[sd->walkpath.path_pos] >= DIR::COUNT)
            return;

        x = sd->bl_x;
        y = sd->bl_y;
        if (bool(map_getcell(sd->bl_m, x, y) & MapCell::UNWALKABLE))
        {
            pc_stop_walking(sd, 1);
            return;
        }
        sd->dir = sd->head_dir = sd->walkpath.path[sd->walkpath.path_pos];
        dx = dirx[sd->dir];
        dy = diry[sd->dir];
        if (bool(map_getcell(sd->bl_m, x + dx, y + dy)
                & MapCell::UNWALKABLE))
        {
            pc_walktoxy_sub(sd);
            return;
        }

        // Prsm-sitting countermeasures
        dumb_ptr<block_list> d_bl = sd->bl_m->blocks.ref((x + dx) / BLOCK_SIZE, (y + dy) / BLOCK_SIZE).normal;
        for (; d_bl; d_bl = d_bl->bl_next)
        {
            if (d_bl->bl_type == BL::PC && d_bl->bl_x == (x + dx) && d_bl->bl_y == (y + dy))
            {
                dumb_ptr<map_session_data> d_sd = d_bl->is_player();

                if (pc_issit(d_sd) && d_sd != sd)
                {
                    switch (d_sd->automod)
                    {
                    case AutoMod::autoblock:
                        clif_update_collision(sd, x + dx, y + dy, x + dx, y + dy, sd->bl_m->name_, 5); // BlockType::PLAYERWALL
                        pc_stop_walking(sd, 1);
                        // clif_changemap(sd, sd->mapname_, x, y); <= FIXME: crashes manaplus!
                        pc_setpos(sd, sd->mapname_, x, y, BeingRemoveWhy::WARPED);
                        return;
                    }
                }
            }
        }

        moveblock = (x / BLOCK_SIZE != (x + dx) / BLOCK_SIZE
                     || y / BLOCK_SIZE != (y + dy) / BLOCK_SIZE);

        // sd->walktimer = dummy value that is not nullptr;
        map_foreachinmovearea(std::bind(clif_pcoutsight, ph::_1, sd),
                sd->bl_m,
                x - AREA_SIZE, y - AREA_SIZE,
                x + AREA_SIZE, y + AREA_SIZE,
                dx, dy,
                BL::NUL);

        x += dx;
        y += dy;

        if (moveblock)
            map_delblock(sd);
        sd->bl_x = x;
        sd->bl_y = y;
        if (moveblock)
            map_addblock(sd);

        map_foreachinmovearea(std::bind(clif_pcinsight, ph::_1, sd),
                sd->bl_m,
                x - AREA_SIZE, y - AREA_SIZE,
                x + AREA_SIZE, y + AREA_SIZE,
                -dx, -dy,
                BL::NUL);
        // sd->walktimer = nullptr;

        if (sd->status.party_id)
        {                       //パーティのＨＰ情報通知検査 | Inspection of the party's HP information notification
            Option<PartyPair> p = party_search(sd->status.party_id);
            if (p.is_some())
            {
                int p_flag = 0;
                map_foreachinmovearea(std::bind(party_send_hp_check, ph::_1, sd->status.party_id, &p_flag),
                        sd->bl_m,
                        x - AREA_SIZE, y - AREA_SIZE,
                        x + AREA_SIZE, y + AREA_SIZE,
                        -dx, -dy,
                        BL::PC);
                if (p_flag)
                    sd->party_hp = -1;
            }
        }

        if (npc_touch_areanpc(sd, sd->bl_m, x, y) != 2)
            sd->areanpc_id = BlockId();
    }
    interval_t i = calc_next_walk_step(sd);
    if (i > interval_t::zero())
    {
        i = i / 2;
        if (sd->walkpath.path_half == 0)
            i = std::max(i, 1_ms);

        sd->walktimer = Timer(tick + i,
                std::bind(pc_walk, ph::_1, ph::_2,
                    id, sd->walkpath.path_pos));
    }
}

/*==========================================
 * 移動可能か確認して、可能なら歩行開始
 * Check if you can move and start walking if possible
 *------------------------------------------
 */
static
int pc_walktoxy_sub(dumb_ptr<map_session_data> sd)
{
    struct walkpath_data wpd;

    nullpo_retr(1, sd);

    if (path_search(&wpd, sd->bl_m, sd->bl_x, sd->bl_y, sd->to_x, sd->to_y, 0))
        return 1;
    sd->walkpath = wpd;

    clif_walkok(sd);
    sd->state.change_walk_target = 0;

    interval_t i = calc_next_walk_step(sd);
    if (i > interval_t::zero())
    {
        i = i / 4;
        sd->walktimer = Timer(gettick() + i,
                std::bind(pc_walk, ph::_1, ph::_2,
                    sd->bl_id, 0));
    }
    clif_movechar(sd);

    // activity
    if (sd)
        if (sd->activity.tiles_walked == 2147483647)
            sd->activity.tiles_walked = 1;
        else
            sd->activity.tiles_walked++;

    return 0;
}

/*==========================================
 * pc歩 行要求
 * pc step request
 *------------------------------------------
 */
int pc_walktoxy(dumb_ptr<map_session_data> sd, int x, int y)
{

    nullpo_retz(sd);

    sd->to_x = x;
    sd->to_y = y;

    if (pc_issit(sd))
        pc_setstand(sd);

    if (sd->walktimer && sd->state.change_walk_target == 0)
    {
        //現在歩いている最中の目的地変更なのでマス目の中心に来た時に | Since it is a destination change while currently walking, when you come to the center of the square
        // timer関数からpc_walktoxy_subを呼ぶようにする | Make sure to call pc_walktoxy_sub from timer function
        sd->state.change_walk_target = 1;
    }
    else
    {
        pc_walktoxy_sub(sd);
    }

    return 0;
}

/*==========================================
 * 歩 行停止
 * Walk stop
 *------------------------------------------
 */
int pc_stop_walking(dumb_ptr<map_session_data> sd, int type)
{
    nullpo_retz(sd);

    sd->walktimer.cancel();

    sd->walkpath.path_len = 0;
    sd->to_x = sd->bl_x;
    sd->to_y = sd->bl_y;
    if (type & 0x01)
        clif_fixpos(sd);
    if (type & 0x02 && battle_config.player_damage_delay)
    {
        tick_t tick = gettick();
        interval_t delay = battle_get_dmotion(sd);
        if (sd->canmove_tick < tick)
            sd->canmove_tick = tick + delay;
    }

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
void pc_touch_all_relevant_npcs(dumb_ptr<map_session_data> sd)
{
    if (npc_touch_areanpc(sd, sd->bl_m, sd->bl_x, sd->bl_y) != 2)
        sd->areanpc_id = BlockId();
}

//
// 武器戦闘 | Weapons Combat
//
/*==========================================
 * スキルの検索 所有していた場合Lvが返る
 * Search for a skill If you owned it, it will return Lv
 *------------------------------------------
 */
int pc_checkskill(dumb_ptr<map_session_data> sd, SkillID skill_id)
{
    if (sd == nullptr)
        return 0;

    return sd->status.skill[skill_id].lv;
}

/*==========================================
 * 装 備品のチェック
 * Checking Equipment
 *------------------------------------------
 */
IOff0 pc_checkequip(dumb_ptr<map_session_data> sd, EPOS pos)
{
    nullpo_retr(IOff0::from(-1), sd);

    for (EQUIP i : EQUIPs)
    {
        if (bool(pos & equip_pos[i]))
            return sd->equip_index_maybe[i];
    }

    return IOff0::from(-1);
}

/*==========================================
 * PCの攻撃 (timer関数)
 * PC Attack (Timer Function)
 *------------------------------------------
 */
static
void pc_attack_timer(TimerData *, tick_t tick, BlockId id)
{
    dumb_ptr<map_session_data> sd;
    dumb_ptr<block_list> bl;
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    int dist, range;

    sd = map_id2sd(id);
    if (sd == nullptr)
        return;

    if (sd->bl_prev == nullptr)
        return;

    bl = map_id2bl(sd->attacktarget);
    if (bl == nullptr || bl->bl_prev == nullptr)
        return;

    if (bl->bl_type == BL::PC && pc_isdead(bl->is_player()))
        return;

    //同じmapでないなら攻撃しない | If it's not the same map, don't attack
    //PCが死んでても攻撃しない | Does not attack even if the PC is dead
    if (sd->bl_m != bl->bl_m || pc_isdead(sd))
        return;

    //異常などで攻撃できない | Unable to attack due to abnormalities, etc.
    if (sd->opt1 != Opt1::ZERO)
        return;

    Opt0 *opt = battle_get_option(bl);
    if (opt != nullptr && bool(*opt & Opt0::REAL_ANY_HIDE))
        return;

    if (!battle_config.skill_delay_attack_enable)
    {
        if (tick < sd->canact_tick)
        {
            clif_skill_fail(sd, SkillID::ONE, 4, 0);
            return;
        }
    }

    if (sd->attackabletime > tick)
        return;               // cannot attack yet

    if (sd->attack_spell_override)   // [Fate] If we have an active attack spell, use that
    {
        if (battle_config.player_attack_direction_change)
            sd->dir = sd->head_dir = map_calc_dir(sd, bl->bl_x, bl->bl_y);

        if (sd->walktimer)
            pc_stop_walking(sd, 1);

        // call_spell_event_script
        argrec_t arg[1] =
        {
            {"@target_id"_s, static_cast<int32_t>(unwrap<BlockId>(bl->bl_id))},
        };
        npc_event_do_l(sd->magic_attack, sd->bl_id, arg);
        sd->attackabletime = tick + sd->aspd; // sd->attack_spell_delay
        sd->attack_spell_charges--;
        if (!sd->attack_spell_charges)
        {
            sd->attack_spell_override = BlockId();
            pc_set_weapon_icon(sd, 0, StatusChange::ZERO, ItemNameId());
            pc_set_attack_info(sd, interval_t::zero(), 0);
            pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
        }
    }
    else
    {
        dist = distance(sd->bl_x, sd->bl_y, bl->bl_x, bl->bl_y);
        range = sd->attackrange;
        if (sd->status.weapon != ItemLook::W_BOW)
            range++;
        if (dist > range)
        {                       //届 かないので移動 | Move because it does not arrive
            //if(pc_can_reach(sd,bl->bl_x,bl->bl_y))
            //clif_movetoattack(sd,bl);
            return;
        }

        if (dist <= range && !battle_check_range(sd, bl, range))
        {
            if (pc_can_reach(sd, bl->bl_x, bl->bl_y) && sd->canmove_tick < tick)
                // TMW client doesn't support this
                //pc_walktoxy(sd,bl->bl_x,bl->bl_y);
                clif_movetoattack(sd, bl);
            sd->attackabletime = tick + (sd->aspd * 2);
        }
        else
        {
            if (battle_config.player_attack_direction_change)
                sd->dir = sd->head_dir = map_calc_dir(sd, bl->bl_x, bl->bl_y);  //向き設定 | Orientation setting

            if (sd->walktimer)
                pc_stop_walking(sd, 1);

            {
                MapBlockLock lock;
                pc_stop_walking(sd, 0);
                sd->attacktarget_lv = battle_weapon_attack(sd, bl, tick);
                sd->attackabletime = tick + (sd->aspd * 2);
            }
            if (sd->attackabletime <= tick)
                sd->attackabletime = tick + battle_config.max_aspd * 2;
        }
    }

    if (sd->state.attack_continue)
    {
        sd->attacktimer = Timer(sd->attackabletime,
                std::bind(pc_attack_timer, ph::_1, ph::_2,
                    sd->bl_id));
    }
}

/*==========================================
 * 攻撃要求
 * typeが1なら継続攻撃
 * Attack Requests
 * If type is 1, continue attack
 *------------------------------------------
 */
int pc_attack(dumb_ptr<map_session_data> sd, BlockId target_id, int type)
{
    dumb_ptr<block_list> bl;

    nullpo_retz(sd);

    bl = map_id2bl(target_id);
    if (bl == nullptr)
        return 1;

    if (bl->bl_type == BL::NPC)
    {                           // monster npcs [Valaris]
        if (battle_get_class(bl) != INVISIBLE_CLASS && !pc_isdead(sd))
            npc_click(sd, target_id);
        return 0;
    }

    if (!battle_check_target(sd, bl, BCT_ENEMY))
        return 1;
    if (sd->attacktimer)
        pc_stopattack(sd);
    sd->attacktarget = target_id;
    sd->state.attack_continue = type;

    interval_t d = sd->attackabletime - gettick();
    if (d > interval_t::zero() && d < 2_s)
    {                           //攻撃delay中 | Attack delaying
        sd->attacktimer = Timer(sd->attackabletime,
                std::bind(pc_attack_timer, ph::_1, ph::_2,
                    sd->bl_id));
    }
    else
    {
        //本来timer関数なので引数を合わせる | Since it is originally a timer function, match the arguments
        pc_attack_timer(nullptr, gettick(), sd->bl_id);
    }

    return 0;
}

/*==========================================
 * 継続攻撃停止
 * Continuous Attack Halt
 *------------------------------------------
 */
int pc_stopattack(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    sd->attacktimer.cancel();

    sd->attacktarget = BlockId();
    sd->state.attack_continue = 0;

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_checkbaselevelup(dumb_ptr<map_session_data> sd)
{
    int next = pc_nextbaseexp(sd);

    nullpo_retz(sd);

    if (sd->status.base_exp >= next && next > 0)
    {
        //base側レベルアップ処理 | Base side level-up processing
        sd->status.base_exp -= next;

        sd->status.base_level++;
        sd->status.status_point += (sd->status.base_level + 14) / 4;
        clif_updatestatus(sd, SP::STATUSPOINT);
        clif_updatestatus(sd, SP::BASELEVEL);
        clif_updatestatus(sd, SP::NEXTBASEEXP);
        pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
        pc_heal(sd, sd->status.max_hp, sd->status.max_sp, true);

        clif_misceffect(sd, 0);
        //レベルアップしたのでパーティー情報を更新する | Now that you've leveled up, update your party information
        //(公平範囲チェック) | (Fair Scope Check)
        party_send_movemap(sd);
        MAP_LOG_XP(sd, "LEVELUP"_fmt);
        return 1;
    }

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
inline
int RAISE_COST(int x)
{
    return (x * (x - 1)) / 2;
}

/*========================================
 * Compute the maximum for sd->skill_point, i.e., the max. number of skill points that can still be filled in
 *----------------------------------------
 */
static
int pc_skillpt_potential(dumb_ptr<map_session_data> sd)
{
    int potential = 0;

    for (SkillID skill_id : erange(SkillID(), MAX_SKILL))
        if (sd->status.skill[skill_id].lv
            && sd->status.skill[skill_id].lv < skill_db[skill_id].max_raise)
            potential += RAISE_COST(skill_db[skill_id].max_raise)
                - RAISE_COST(sd->status.skill[skill_id].lv);

    return potential;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_checkjoblevelup(dumb_ptr<map_session_data> sd)
{
    int next = pc_nextjobexp(sd);

    nullpo_retz(sd);

    if (sd->status.job_exp >= next && next > 0)
    {
        if (pc_skillpt_potential(sd) < sd->status.skill_point)
        {                       // [Fate] Bah, this is is painful.
            // But the alternative is quite error-prone, and eAthena has far worse performance issues...
            sd->status.job_exp = next - 1;
            pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
            return 0;
        }

        //job側レベルアップ処理 | Job side level-up processing
        sd->status.job_exp -= next;
        clif_updatestatus(sd, SP::NEXTJOBEXP);
        sd->status.skill_point++;
        clif_updatestatus(sd, SP::SKILLPOINT);
        pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);

        MAP_LOG_PC(sd, "SKILLPOINTS-UP %d"_fmt, sd->status.skill_point);

        if (sd->status.job_level < 250
            && sd->status.job_level < sd->status.base_level * 2)
            sd->status.job_level++; // Make levelling up a little harder

        clif_misceffect(sd, 1);
        return 1;
    }

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_gainexp_reason(dumb_ptr<map_session_data> sd, int base_exp, int job_exp,
        PC_GAINEXP_REASON reason)
{
    nullpo_retz(sd);

    if (sd->bl_prev == nullptr || pc_isdead(sd))
        return 0;

    earray<LString, PC_GAINEXP_REASON, PC_GAINEXP_REASON::COUNT> reasons //=
    {{
        "KILLXP"_s,
        "HEALXP"_s,
        "SCRIPTXP"_s,
        "SHAREXP"_s,
        /* Insert new types here */
        "UNKNOWNXP"_s
    }};
    MAP_LOG_PC(sd, "GAINXP %d %d %s"_fmt, base_exp, job_exp, reasons[reason]);

    if (!battle_config.multi_level_up && pc_nextbaseafter(sd))
    {
        while (sd->status.base_exp + base_exp >= pc_nextbaseafter(sd)
               && sd->status.base_exp <= pc_nextbaseexp(sd)
               && pc_nextbaseafter(sd) > 0)
        {
            base_exp *= .90;
        }
    }

    // Double Xp Weekends
    if (reason != PC_GAINEXP_REASON::SCRIPT)
        base_exp = (base_exp * static_cast<double>(battle_config.base_exp_rate) / 100.);

    // Sanitization
    if (base_exp <= 0)
        base_exp = 0;
    else if (base_exp > 1000000000)
        base_exp = 1000000000;
    sd->status.base_exp += base_exp;

    // [Fate] Adjust experience points that healers can extract from this character
    if (reason != PC_GAINEXP_REASON::HEALING)
    {
        const int max_heal_xp =
            20 + (sd->status.base_level * sd->status.base_level);
        sd->heal_xp += base_exp;
        if (sd->heal_xp > max_heal_xp)
            sd->heal_xp = max_heal_xp;
    }

    if (sd->status.base_exp < 0)
        sd->status.base_exp = 0;

    while (pc_checkbaselevelup(sd))
    {}

    clif_updatestatus(sd, SP::BASEEXP);
    if (!battle_config.multi_level_up && pc_nextjobafter(sd))
    {
        while (sd->status.job_exp + job_exp >= pc_nextjobafter(sd)
               && sd->status.job_exp <= pc_nextjobexp(sd)
               && pc_nextjobafter(sd) > 0)
        {
            job_exp *= .90;
        }
    }

    // Double Xp Weekends
    job_exp = (job_exp * static_cast<double>(battle_config.job_exp_rate) / 100.);
    if (job_exp <= 0)
        job_exp = 0;
    else if (job_exp > 1000000000)
        job_exp = 1000000000;
    sd->status.job_exp += job_exp;
    if (sd->status.job_exp < 0)
        sd->status.job_exp = 0;

    while (pc_checkjoblevelup(sd))
    {}

    clif_updatestatus(sd, SP::JOBEXP);

    if (battle_config.disp_experience)
    {
        AString output = STRPRINTF(
                "Experienced Gained Base:%d Job:%d"_fmt,
                base_exp, job_exp);
        clif_displaymessage(sd->sess, output);
    }

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_extract_healer_exp(dumb_ptr<map_session_data> sd, int max)
{
    int amount;
    nullpo_retz(sd);

    amount = sd->heal_xp;
    if (max < amount)
        amount = max;

    sd->heal_xp -= amount;
    return amount;
}

/*==========================================
 * base level側必要経験値計算
 * Base Level side required experience calculation
 *------------------------------------------
 */
int pc_nextbaseexp(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    if (sd->status.base_level >= MAX_LEVEL || sd->status.base_level <= 0)
        return 0;

    return exp_table_0[sd->status.base_level - 1];
}

/*==========================================
 * job level側必要経験値計算
 * Job level side required experience calculation
 *------------------------------------------
 */
int pc_nextjobexp(dumb_ptr<map_session_data> sd)
{
    // [fate]  For normal levels, this ranges from 20k to 50k, depending on job level.
    // Job level is at most twice the player's experience level (base_level).  Levelling
    // from 2 to 9 is 44 points, i.e., 880k to 2.2M job experience points (this is per
    // skill, obviously.)

    return 20000 + sd->status.job_level * 150;
}

/*==========================================
 * base level after next [Valaris]
 *------------------------------------------
 */
int pc_nextbaseafter(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    if (sd->status.base_level >= MAX_LEVEL || sd->status.base_level <= 0)
        return 0;

    return exp_table_0[sd->status.base_level];
}

/*==========================================
 * job level after next [Valaris]
 *------------------------------------------
 */
int pc_nextjobafter(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    if (sd->status.job_level >= MAX_LEVEL || sd->status.job_level <= 0)
        return 0;

    return exp_table_7[sd->status.job_level];
}

/*==========================================
 * 必要ステータスポイント計算
 * Calculating Required Status Points
 *------------------------------------------
 */
// TODO: replace SP by ATTR
int pc_need_status_point(dumb_ptr<map_session_data> sd, SP type)
{
    int val;

    nullpo_retr(-1, sd);

    if (type < SP::STR || type > SP::LUK)
        return -1;
    val = sd->status.attrs[sp_to_attr(type)];

    return (val + 9) / 10 + 1;
}

/*==========================================
 * 能力値成長
 * Ability Growth
 *------------------------------------------
 */
int pc_statusup(dumb_ptr<map_session_data> sd, SP type)
{
    int need, val = 0;

    nullpo_retz(sd);

    if (SP::STR <= type && type <= SP::LUK)
        val = sd->status.attrs[sp_to_attr(type)];

    need = pc_need_status_point(sd, type);
    if (type < SP::STR || type > SP::LUK || need < 0
        || need > sd->status.status_point
        || val >= battle_config.max_parameter)
    {
        clif_statusupack(sd, type, 0, val);
        clif_updatestatus(sd, SP::STATUSPOINT);
        return 1;
    }
    val = ++sd->status.attrs[sp_to_attr(type)];
    sd->status.status_point -= need;
    if (need != pc_need_status_point(sd, type))
    {
        clif_updatestatus(sd, sp_to_usp(type));
    }
    clif_updatestatus(sd, SP::STATUSPOINT);
    clif_updatestatus(sd, type);
    pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
    clif_statusupack(sd, type, 1, val);

    MAP_LOG_STATS(sd, "STATUP"_fmt);

    return 0;
}

/*==========================================
 * 能力値成長
 * Ability Growth
 *------------------------------------------
 */
int pc_statusup2(dumb_ptr<map_session_data> sd, SP type, int val)
{
    nullpo_retz(sd);

    if (type < SP::STR || type > SP::LUK)
    {
        clif_statusupack(sd, type, 0, 0);
        return 1;
    }
    ATTR attr = sp_to_attr(type);
    val = sd->status.attrs[attr] + val;
    val = std::min(val, battle_config.max_parameter);
    val = std::max(val, 1);
    sd->status.attrs[attr] = val;
    clif_updatestatus(sd, sp_to_usp(type));
    clif_updatestatus(sd, type);
    pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
    clif_statusupack(sd, type, 1, val);
    MAP_LOG_STATS(sd, "STATUP2"_fmt);

    return 0;
}

/*==========================================
 * スキルポイント割り振り
 * Skill Point Allocation
 *------------------------------------------
 */
int pc_skillup(dumb_ptr<map_session_data> sd, SkillID skill_num)
{
    nullpo_retz(sd);

    if (sd->status.skill[skill_num].lv
        && sd->status.skill_point >= sd->status.skill[skill_num].lv
        && sd->status.skill[skill_num].lv < skill_db[skill_num].max_raise)
    {
        sd->status.skill_point -= sd->status.skill[skill_num].lv;
        sd->status.skill[skill_num].lv++;

        pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
        clif_skillup(sd, skill_num);
        clif_updatestatus(sd, SP::SKILLPOINT);
        clif_skillinfoblock(sd);
        MAP_LOG_PC(sd, "SKILLUP %d %d %d"_fmt,
                skill_num, sd->status.skill[skill_num].lv,
                skill_power(sd, skill_num));
    }

    return 0;
}

/*==========================================
 * /resetstate
 *------------------------------------------
 */
int pc_resetstate(dumb_ptr<map_session_data> sd)
{

    nullpo_retz(sd);

    sd->status.status_point = stat_p[sd->status.base_level - 1];

    clif_updatestatus(sd, SP::STATUSPOINT);

    for (ATTR attr : ATTRs)
        sd->status.attrs[attr] = 1;
    for (ATTR attr : ATTRs)
        clif_updatestatus(sd, attr_to_sp(attr));
    for (ATTR attr : ATTRs)
        clif_updatestatus(sd, attr_to_usp(attr));

    pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);

    return 0;
}

/*==========================================
 * /resetskill
 *------------------------------------------
 */
int pc_resetskill(dumb_ptr<map_session_data> sd)
{
    int skill;

    nullpo_retz(sd);

    sd->status.skill_point += pc_calc_skillpoint(sd);

    for (SkillID i : erange(SkillID(1), MAX_SKILL))
        if ((skill = pc_checkskill(sd, i)) > 0)
        {
            sd->status.skill[i].lv = 0;
            sd->status.skill[i].flags = SkillFlags::ZERO;
        }

    clif_updatestatus(sd, SP::SKILLPOINT);
    clif_skillinfoblock(sd);
    pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);

    return 0;
}

/*==========================================
 * pcにダメージを与える
 * Damage your PC
 *------------------------------------------
 */
int pc_damage(dumb_ptr<block_list> src, dumb_ptr<map_session_data> sd,
               int damage)
{
    nullpo_retz(sd);

    //既に死んでいたら無効 | Invalid if already dead
    if (pc_isdead(sd))
        return 0;
    //座ってたら立ち上がる | Sit down and stand up
    if (pc_issit(sd))
    {
        pc_setstand(sd);
    }

    if (src)
    {
        if (src->bl_type == BL::PC)
        {
            MAP_LOG_PC(sd, "INJURED-BY PC%d FOR %d"_fmt,
                    src->is_player()->status_key.char_id,
                    damage);
        }
        else
        {
            MAP_LOG_PC(sd, "INJURED-BY MOB%d FOR %d"_fmt, src->bl_id, damage);
        }
    }
    else
        MAP_LOG_PC(sd, "INJURED-BY null FOR %d"_fmt, damage);

    pc_stop_walking(sd, 3);
    //演奏/ダンスの中断 | Performance/Dance Interruptions
    if (damage > sd->status.max_hp >> 2)
        skill_stop_dancing(sd, 0);

    sd->status.hp -= damage;

    if (sd->status.hp > 0)
    {
        //まだ生きているならHP更新 | HP update if you're still alive
        clif_updatestatus(sd, SP::HP);

        sd->canlog_tick = gettick();

        if (sd->status.party_id)
        {                       // on-the-fly party hp updates [Valaris]
            Option<PartyPair> p_ = party_search(sd->status.party_id);
            OMATCH_BEGIN_SOME (p, p_)
            {
                clif_party_hp(p, sd);
            }
            OMATCH_END ();
        }                       // end addition [Valaris]

        return 0;
    }

    MAP_LOG_PC(sd, "DEAD%s"_fmt, ""_s);

    // Character is dead!

    sd->status.hp = 0;
    // [Fate] Stop quickregen
    sd->quick_regeneration_hp.amount = 0;
    sd->quick_regeneration_sp.amount = 0;
    skill_update_heal_animation(sd);

    pc_setdead(sd);

    pc_stop_walking(sd, 0);
    skill_castcancel(sd, 0);  //詠唱の中止 | Cessation of chanting
    clif_clearchar(sd, BeingRemoveWhy::DEAD);
    pc_setglobalreg(sd, stringish<VarName>("PC_DIE_COUNTER"_s), ++sd->die_counter);  //死にカウンター書き込み | Counter writing to death
    skill_status_change_clear(sd, 0); //ステータス異常を解除する | Release status anomalies
    clif_updatestatus(sd, SP::HP);
    // [Fate] Reset magic
    // FIXME: make spells manage their own charge counter, and reset on death
    if (sd->attack_spell_override)
    {
        sd->attack_spell_override = BlockId();
        pc_set_weapon_icon(sd, 0, StatusChange::ZERO, ItemNameId());
        pc_set_attack_info(sd, interval_t::zero(), 0);
    }
    pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);

    if (battle_config.death_penalty_type > 0 && sd->status.base_level >= 20)
    {
        // changed penalty options, added death by player if pk_mode [Valaris]
        if (!sd->bl_m->flag.get(MapFlag::NOPENALTY))
        {
            if (battle_config.death_penalty_type == 1
                && battle_config.death_penalty_base > 0)
                sd->status.base_exp -=
                    static_cast<double>(pc_nextbaseexp(sd)) *
                    static_cast<double>(battle_config.death_penalty_base) / 10000;
            if (battle_config.pk_mode && src && src->bl_type == BL::PC)
                sd->status.base_exp -=
                    static_cast<double>(pc_nextbaseexp(sd)) *
                    static_cast<double>(battle_config.death_penalty_base) / 10000;
            else if (battle_config.death_penalty_type == 2
                     && battle_config.death_penalty_base > 0)
            {
                if (pc_nextbaseexp(sd) > 0)
                    sd->status.base_exp -=
                        static_cast<double>(sd->status.base_exp) *
                        static_cast<double>(battle_config.death_penalty_base) / 10000;
                if (battle_config.pk_mode && src && src->bl_type == BL::PC)
                    sd->status.base_exp -=
                        static_cast<double>(sd->status.base_exp) *
                        static_cast<double>(battle_config.death_penalty_base) / 10000;
            }
            if (sd->status.base_exp < 0)
                sd->status.base_exp = 0;
            clif_updatestatus(sd, SP::BASEEXP);

            if (battle_config.death_penalty_type == 1
                && battle_config.death_penalty_job > 0)
                sd->status.job_exp -=
                    static_cast<double>(pc_nextjobexp(sd)) *
                    static_cast<double>(battle_config.death_penalty_job) / 10000;
            if (battle_config.pk_mode && src && src->bl_type == BL::PC)
                sd->status.job_exp -=
                    static_cast<double>(pc_nextjobexp(sd)) *
                    static_cast<double>(battle_config.death_penalty_job) / 10000;
            else if (battle_config.death_penalty_type == 2
                     && battle_config.death_penalty_job > 0)
            {
                if (pc_nextjobexp(sd) > 0)
                    sd->status.job_exp -=
                        static_cast<double>(sd->status.job_exp) *
                        static_cast<double>(battle_config.death_penalty_job) / 10000;
                if (battle_config.pk_mode && src && src->bl_type == BL::PC)
                    sd->status.job_exp -=
                        static_cast<double>(sd->status.job_exp) *
                        static_cast<double>(battle_config.death_penalty_job) / 10000;
            }
            if (sd->status.job_exp < 0)
                sd->status.job_exp = 0;
            clif_updatestatus(sd, SP::JOBEXP);
        }
    }

    // pvp
    if (sd->bl_m->flag.get(MapFlag::PVP) && !battle_config.pk_mode)
    {                           // disable certain pvp functions on pk_mode [Valaris]
        //ランキング計算 | Ranking Calculation
        if (!sd->bl_m->flag.get(MapFlag::PVP_NOCALCRANK))
        {
            sd->pvp_point -= 5;
            if (src && src->bl_type == BL::PC)
                src->is_player()->pvp_point++;
        }
        //強制送還 | Deportation
        if (sd->pvp_point < 0)
        {
            sd->pvp_point = 0;
        }
    }

    if (src && src->bl_type == BL::PC)
    {
        // [Fate] PK death, trigger scripts
        argrec_t arg[1] =
        {
            {"@victimrid"_s, static_cast<int32_t>(unwrap<BlockId>(sd->bl_id))},
        };
        npc_event_doall_l(stringish<ScriptLabel>("OnPCKillEvent"_s), src->bl_id, arg);

        sd->state.pvp_rank = 0;
        src->is_player()->state.pvp_rank++;
        clif_pvpstatus(sd);
        clif_pvpstatus(src->is_player());
    }
    npc_event_doall_l(stringish<ScriptLabel>("OnPCDieEvent"_s), sd->bl_id, nullptr);

    return 0;
}

//
// script関 連 | Script-related
//
/*==========================================
 * script用PCステータス読み出し
 * Read PC status for script
 *------------------------------------------
 */
int pc_readparam(dumb_ptr<block_list> bl, SP type)
{
    nullpo_retz(bl);
    dumb_ptr<map_session_data> sd;
    dumb_ptr<npc_data> nd;
    dumb_ptr<mob_data> md;
    int val = 0;

    if (bl->bl_type == BL::PC)
        sd = bl->is_player();
    else if (bl->bl_type == BL::MOB)
        md = bl->is_mob();
    else if (bl->bl_type == BL::NPC)
        nd = bl->is_npc();
    else
        return val;

    switch (type)
    {
        case SP::SKILLPOINT:
            val = sd ? sd->status.skill_point : 0;
            break;
        case SP::STATUSPOINT:
            val = sd ? sd->status.status_point : 0;
            break;
        case SP::ZENY:
            val = sd ? sd->status.zeny : 0;
            break;
        case SP::BASELEVEL:
            val = battle_get_lv(bl);
            break;
        case SP::JOBLEVEL:
            val = sd ? sd->status.job_level : 0;
            break;
        case SP::CLASS:
            val = unwrap<Species>(battle_get_class(bl));
            break;
        case SP::SEX:
            if (sd)
                val = static_cast<uint8_t>(sd->status.sex);
            else if (nd)
                val = static_cast<uint8_t>(nd->sex);
            break;
        case SP::WEIGHT:
            val = sd ? sd->weight : 0;
            break;
        case SP::MAXWEIGHT:
            val = sd ? sd->max_weight : 0;
            break;
        case SP::MAXWEIGHT_OVERRIDE:
            val = sd ? sd->max_weight_override : 0;
            break;
        case SP::BASEEXP:
            val = sd ? sd->status.base_exp : 0;
            break;
        case SP::JOBEXP:
            val = sd ? sd->status.job_exp : 0;
            break;
        case SP::NEXTBASEEXP:
            val = sd ? pc_nextbaseexp(sd) : 0;
            break;
        case SP::NEXTJOBEXP:
            val = sd ? pc_nextjobexp(sd) : 0;
            break;
        case SP::HP:
            if (sd && pc_isdead(sd))
                break; // val = 0
            val = battle_get_hp(bl);
            break;
        case SP::MAXHP:
            val = battle_get_max_hp(bl);
            break;
        case SP::HEALXP:
            val = sd ? sd->heal_xp : 0;
            break;
        case SP::SP:
            val = sd ? sd->status.sp : 0;
            break;
        case SP::MAXSP:
            val = sd ? sd->status.max_sp : 0;
            break;
        case SP::BASE_ATK:
            val = battle_get_baseatk(bl);
            break;
        case SP::ATK1:
            val = battle_get_atk(bl);
            break;
        case SP::ATK2:
            val = battle_get_atk2(bl);
            break;
        case SP::DEF1:
            val = battle_get_def(bl);
            break;
        case SP::DEF2:
            val = battle_get_def2(bl);
            break;
        case SP::MATK1:
            val = battle_get_matk1(bl);
            break;
        case SP::MATK2:
            val = battle_get_matk2(bl);
            break;
        case SP::MDEF1:
            val = battle_get_mdef(bl);
            break;
        case SP::MDEF2:
            val = battle_get_mdef2(bl);
            break;
        case SP::STR:
        case SP::AGI:
        case SP::VIT:
        case SP::INT:
        case SP::DEX:
        case SP::LUK:
            if (sd)
                val = sd->status.attrs[sp_to_attr(type)];
            else
                val = battle_get_stat(type, bl);
            break;
        case SP::STR2:
        case SP::AGI2:
        case SP::VIT2:
        case SP::INT2:
        case SP::DEX2:
        case SP::LUK2:
            if (sd)
                val = sd->paramb[sp_to_attr(SP(static_cast<int>(type)-(static_cast<int>(SP::STR2)-static_cast<int>(SP::STR))))] + sd->parame[sp_to_attr(SP(static_cast<int>(type)-(static_cast<int>(SP::STR2)-static_cast<int>(SP::STR))))];
            else
                val = battle_get_stat(SP(static_cast<int>(type)-(static_cast<int>(SP::STR2)-static_cast<int>(SP::STR))), bl); // not sure if mobs can have equipment with stats so i only return thier normal stat
            break;
        case SP::SPEED:
            val = battle_get_speed(bl).count();
            break;
        case SP::HIT:
            val = battle_get_hit(bl);
            break;
        case SP::FLEE1:
            val = battle_get_flee(bl);
            break;
        case SP::FLEE2:
            val = battle_get_flee2(bl);
            break;
        case SP::CRITICAL:
            val = battle_get_critical(bl);
            break;
        case SP::GM:
            val = sd ? pc_isGM(sd).get_all_bits() : 0;
            break;
        case SP::ATTACKRANGE:
            val = battle_get_range(bl);
            break;
        case SP::POS_X:
            val = bl->bl_x;
            break;
        case SP::POS_Y:
            val = bl->bl_y;
            break;
        case SP::PVP_CHANNEL:
            val = sd ? sd->state.pvpchannel : 0;
            break;
        case SP::BL_ID:
            val = unwrap<BlockId>(bl->bl_id);
            break;
        case SP::BL_TYPE:
            val = static_cast<uint8_t>(bl->bl_type);
            break;
        case SP::PARTNER:
            val = sd ? unwrap<CharId>(sd->status.partner_id) : 0;
            break;
        case SP::CHAR_ID:
            val = sd ? unwrap<CharId>(sd->status_key.char_id) : 0;
            break;
        case SP::ELTLVL:
            val = static_cast<int>(battle_get_element(bl).level);
            break;
        case SP::ELTTYPE:
            val = static_cast<int>(battle_get_element(bl).element);
            break;
        case SP::INVISIBLE:
            if (sd)
                val = bool(sd->status.option & Opt0::INVISIBILITY);
            if (nd)
                val = bool(nd->flag & 1);
            break;
        case SP::HIDDEN:
            if (sd)
                val = bool(sd->status.option & Opt0::HIDE);
            break;
        case SP::MUTE_GLOBAL:
            val = sd ? sd->mute.global : 0;
            break;
        case SP::MUTE_PARTY:
            val = sd ? sd->mute.party : 0;
            break;
        case SP::MUTE_WHISPER:
            val = sd ? sd->mute.whisper : 0;
            break;
        case SP::MUTE_GUILD:
            val = sd ? sd->mute.guild : 0;
            break;
        case SP::KILLS:
            val = sd ? sd->activity.kills : 0;
            break;
        case SP::CASTS:
            val = sd ? sd->activity.casts : 0;
            break;
        case SP::ITEMS_USED:
            val = sd ? sd->activity.items_used : 0;
            break;
        case SP::TILES_WALKED:
            val = sd ? sd->activity.tiles_walked : 0;
            break;
        case SP::ATTACKS:
            val = sd ? sd->activity.attacks : 0;
            break;
        case SP::AUTOMOD:
            val = sd ? static_cast<int>(sd->automod) : 0;
            break;
    }

    return val;
}

/*==========================================
 * script用PCステータス設定
 * PC status setting for script
 *------------------------------------------
 */
int pc_setparam(dumb_ptr<block_list> bl, SP type, int val)
{
    nullpo_retz(bl);
    dumb_ptr<map_session_data> sd;
    dumb_ptr<npc_data> nd;
    dumb_ptr<mob_data> md;

    if (bl->bl_type == BL::PC)
        sd = bl->is_player();
    else if (bl->bl_type == BL::MOB)
        md = bl->is_mob();
    else if (bl->bl_type == BL::NPC)
        nd = bl->is_npc();
    else
        return 0;

    int i = 0;

    switch (type)
    {
        case SP::BASELEVEL:
            nullpo_retz(sd);
            // TODO: mob mutation
            if (val > sd->status.base_level)
            {
                if (val > MAX_LEVEL)
                    val = MAX_LEVEL;
                for (i = 1; i <= (val - sd->status.base_level); i++)
                    sd->status.status_point +=
                        (sd->status.base_level + i + 14) / 4;
            }
            sd->status.base_level = val;
            sd->status.base_exp = 0;
            clif_updatestatus(sd, SP::BASELEVEL);
            clif_updatestatus(sd, SP::NEXTBASEEXP);
            clif_updatestatus(sd, SP::STATUSPOINT);
            clif_updatestatus(sd, SP::BASEEXP);
            pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
            pc_heal(sd, sd->status.max_hp, sd->status.max_sp, true);
            break;
        case SP::JOBLEVEL:
            nullpo_retz(sd);
            if (val > sd->status.job_level)
            {
                if (val > MAX_LEVEL)
                    val = MAX_LEVEL;
                sd->status.skill_point += (val - sd->status.job_level);
                clif_updatestatus(sd, SP::SKILLPOINT);
                clif_misceffect(sd, 1);
            }
            sd->status.job_level = val;
            sd->status.job_exp = 0;
            clif_updatestatus(sd, SP::JOBLEVEL);
            clif_updatestatus(sd, SP::NEXTJOBEXP);
            clif_updatestatus(sd, SP::JOBEXP);
            pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
            break;
        case SP::CLASS:
            // TODO: mob class change
            if (sd)
            {
                sd->status.species = wrap<Species>(val);
                clif_changelook(sd, LOOK::BASE, val);
            }
            else if (nd)
            {
                if (unwrap<Species>(nd->npc_class) != val)
                {
                    nd->npc_class = wrap<Species>(val);
                    npc_enable(nd->name, 0);
                    npc_enable(nd->name, 1);
                }
            }
            return 0;
        case SP::SKILLPOINT:
            nullpo_retz(sd);
            sd->status.skill_point = val;
            clif_updatestatus(sd, type);
            break;
        case SP::STATUSPOINT:
            nullpo_retz(sd);
            sd->status.status_point = val;
            clif_updatestatus(sd, type);
            break;
        case SP::ZENY:
            nullpo_retz(sd);
            sd->status.zeny = val;
            clif_updatestatus(sd, type);
            break;
        case SP::BASEEXP:
            nullpo_retz(sd);
            if (pc_nextbaseexp(sd) > 0)
            {
                sd->status.base_exp = val;
                if (sd->status.base_exp < 0)
                    sd->status.base_exp = 0;
                pc_checkbaselevelup(sd);
            }
            clif_updatestatus(sd, type);
            break;
        case SP::JOBEXP:
            nullpo_retz(sd);
            if (pc_nextjobexp(sd) > 0)
            {
                sd->status.job_exp = val;
                if (sd->status.job_exp < 0)
                    sd->status.job_exp = 0;
                pc_checkjoblevelup(sd);
            }
            clif_updatestatus(sd, type);
            break;
        case SP::SEX:
            {
                SEX sex = SEX::NEUTRAL;
                if (val == 0)
                    sex = SEX::FEMALE;
                if (val == 1)
                    sex = SEX::MALE;

                if (nd)
                {
                    nd->sex = sex;
                    npc_enable(nd->name, 0);
                    npc_enable(nd->name, 1);
                }
                else if (sd)
                {
                    sd->sex = sd->status.sex = sex;
                    for (IOff0 j : IOff0::iter())
                    {
                        if (sd->status.inventory[j].nameid
                            && bool(sd->status.inventory[j].equip)
                            && !pc_isequip(sd, j))
                            pc_unequipitem(sd, j, CalcStatus::LATER);
                    }
                    pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
                    chrif_save(sd);
                    clif_fixpcpos(sd);
                }
            }
            break;
        case SP::WEIGHT:
            nullpo_retz(sd);
            sd->weight = val;
            clif_updatestatus(sd, type);
            break;
        case SP::MAXWEIGHT:
            nullpo_retz(sd);
            sd->max_weight = val;
            clif_updatestatus(sd, type);
            break;
        case SP::MAXWEIGHT_OVERRIDE:
            nullpo_retz(sd);
            sd->max_weight_override = val;
            pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
            break;
        case SP::HP:
            nullpo_retz(sd);
            // TODO: mob mutation
            pc_heal(sd, (val - sd->status.hp), 0);
            break;
        case SP::MAXHP:
            nullpo_retz(sd);
            // TODO: mob mutation
            sd->status.max_hp = val;
            clif_updatestatus(sd, type);
            break;
        case SP::SP:
            nullpo_retz(sd);
            sd->status.sp = val;
            clif_updatestatus(sd, type);
            break;
        case SP::MAXSP:
            nullpo_retz(sd);
            sd->status.max_sp = val;
            clif_updatestatus(sd, type);
            break;
        case SP::STR:
        case SP::AGI:
        case SP::VIT:
        case SP::INT:
        case SP::DEX:
        case SP::LUK:
            nullpo_retz(sd);
            // TODO: mob mutation
            pc_statusup2(sd, type, (val - sd->status.attrs[sp_to_attr(type)]));
            break;
        case SP::PARTNER:
            if (sd)
            {
                dumb_ptr<block_list> p_bl;
                if (val < 2000000 && val >= 150000)
                {
                    dumb_ptr<map_session_data> p_sd = nullptr;
                    if ((p_sd = map_nick2sd(map_charid2nick(wrap<CharId>(val)))) != nullptr)
                        p_bl = map_id2bl(p_sd->bl_id);
                }
                else
                    p_bl = map_id2bl(wrap<BlockId>(val));
                if (val < 1)
                    pc_divorce(sd);
                else
                    p_bl ? pc_marriage(sd, p_bl->is_player()) : 0;
            }
            break;
        case SP::INVISIBLE:
            if (sd)
                pc_invisibility(sd, (val > 0) ? 1 : 0);
            else if (nd)
                npc_enable(nd->name, (val > 0) ? false : true);
            break;
        case SP::GM:
            nullpo_retz(sd);
            pc_set_gm_level(sd->status_key.account_id, GmLevel::from(static_cast<uint32_t>(val)));
            clif_updatestatus(sd, SP::GM); // propagate to self
            clif_fixpcpos(sd); // propagate to others
            break;
        case SP::HIDDEN:
            nullpo_retz(sd);
            if (val == 1)
                sd->status.option |= Opt0::HIDE;
            else
                sd->status.option &= ~Opt0::HIDE;
            clif_changeoption(sd);
            break;
        case SP::MUTE_GLOBAL:
            nullpo_retz(sd);
            sd->mute.global = (val == 1);
            break;
        case SP::MUTE_PARTY:
            nullpo_retz(sd);
            sd->mute.party = (val == 1);
            break;
        case SP::MUTE_WHISPER:
            nullpo_retz(sd);
            sd->mute.whisper = (val == 1);
            break;
        case SP::MUTE_GUILD:
            nullpo_retz(sd);
            sd->mute.guild = (val == 1);
            break;
        // atm only setting of casts is needed since magic is handled in serverdata but I let the others here as well for whatever reason
        case SP::KILLS:
            nullpo_retz(sd);
            sd->activity.kills = val;
            break;
        case SP::CASTS:
            nullpo_retz(sd);
            sd->activity.casts = val;
            break;
        case SP::ITEMS_USED:
            nullpo_retz(sd);
            sd->activity.items_used = val;
            break;
        case SP::TILES_WALKED:
            nullpo_retz(sd);
            sd->activity.tiles_walked = val;
            break;
        case SP::ATTACKS:
            nullpo_retz(sd);
            sd->activity.attacks = val;
            break;
        case SP::AUTOMOD:
            nullpo_retz(sd);
            sd->automod = static_cast<AutoMod>(val);
            break;
    }

    return 0;
}

/*==========================================
 * HP/SP回復
 * HP/SP Recovery
 *------------------------------------------
 */
int pc_heal(dumb_ptr<map_session_data> sd, int hp, int sp, bool levelup)
{
    nullpo_retz(sd);

    if (pc_checkoverhp(sd))
    {
        if (hp > 0)
            hp = 0;
    }
    if (pc_checkoversp(sd))
    {
        if (sp > 0)
            sp = 0;
    }

    if (hp + sd->status.hp > sd->status.max_hp)
        hp = sd->status.max_hp - sd->status.hp;
    if (sp + sd->status.sp > sd->status.max_sp)
        sp = sd->status.max_sp - sd->status.sp;
    sd->status.hp += hp;
    if (sd->status.hp <= 0)
    {
        sd->status.hp = 0;
        pc_damage(nullptr, sd, 1);
        hp = 0;
    }
    sd->status.sp += sp;
    if (sd->status.sp <= 0)
        sd->status.sp = 0;
    if (hp)
    {
        clif_updatestatus(sd, SP::HP);

        if (hp > 0 && levelup) {
            clif_send_hp_full(sd);
        } else if (hp > 0) {
            clif_send_hp_partial(sd);
        }
    }

    if (sp)
        clif_updatestatus(sd, SP::SP);

    if (sd->status.party_id)
    {                           // on-the-fly party hp updates [Valaris]
        Option<PartyPair> p_ = party_search(sd->status.party_id);
        OMATCH_BEGIN_SOME (p, p_)
        {
            clif_party_hp(p, sd);
        }
        OMATCH_END ();
    }                           // end addition [Valaris]

    return hp + sp;
}
int pc_heal(dumb_ptr<map_session_data> sd, int hp, int sp)
{
    return pc_heal(sd, hp, sp, false);
}

/*==========================================
 * HP/SP回復
 * HP/SP Recovery
 *------------------------------------------
 */
static
int pc_itemheal_effect(dumb_ptr<map_session_data> sd, int hp, int sp);

/*==========================================
 * 
 *------------------------------------------
 */
static
int                     // Compute how quickly we regenerate (less is faster) for that amount
pc_heal_quick_speed(int amount)
{
    if (amount >= 100)
    {
        if (amount >= 500)
            return 0;
        if (amount >= 250)
            return 1;
        return 2;
    }
    else
    {                           // < 100
        if (amount >= 50)
            return 3;
        if (amount >= 20)
            return 4;
        return 5;
    }
}

/*==========================================
 * 
 *------------------------------------------
 */
static
void pc_heal_quick_accumulate(int new_amount,
                          struct quick_regeneration *quick_regen, int max)
{
    int current_amount = quick_regen->amount;
    int current_speed = quick_regen->speed;
    int new_speed = pc_heal_quick_speed(new_amount);

    int average_speed = ((new_speed * new_amount) + (current_speed * current_amount)) / (current_amount + new_amount); // new_amount > 0, current_amount >= 0

    quick_regen->speed = average_speed;
    quick_regen->amount = std::min(current_amount + new_amount, max);

    quick_regen->tickdelay = std::min(quick_regen->speed, quick_regen->tickdelay);
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_itemheal(dumb_ptr<map_session_data> sd, int hp, int sp)
{
    /* defer healing */
    if (hp > 0)
    {
        pc_heal_quick_accumulate(hp,
                                  &sd->quick_regeneration_hp,
                                  sd->status.max_hp - sd->status.hp);
        hp = 0;
    }
    if (sp > 0)
    {
        pc_heal_quick_accumulate(sp,
                                  &sd->quick_regeneration_sp,
                                  sd->status.max_sp - sd->status.sp);

        sp = 0;
    }

    /* Hurt right away, if necessary */
    if (hp < 0 || sp < 0)
        pc_itemheal_effect(sd, hp, sp);

    return 0;
}

/*==========================================
 * pc_itemheal_effect is invoked once every 0.5s whenever the pc
 * has health recovery queued up (cf. pc_natural_heal_sub).
 *------------------------------------------
 */
static
int pc_itemheal_effect(dumb_ptr<map_session_data> sd, int hp, int sp)
{
    nullpo_retz(sd);

    if (pc_checkoverhp(sd))
    {
        if (hp > 0)
            hp = 0;
    }
    if (pc_checkoversp(sd))
    {
        if (sp > 0)
            sp = 0;
    }
    if (hp > 0)
    {
        int bonus = (sd->paramc[ATTR::VIT] << 1) + 100;
        hp = hp * bonus / 100;
    }
    if (sp > 0)
    {
        int bonus = (sd->paramc[ATTR::INT] << 1) + 100;
        sp = sp * bonus / 100;
    }
    if (hp + sd->status.hp > sd->status.max_hp)
        hp = sd->status.max_hp - sd->status.hp;
    if (sp + sd->status.sp > sd->status.max_sp)
        sp = sd->status.max_sp - sd->status.sp;
    sd->status.hp += hp;
    if (sd->status.hp <= 0)
    {
        sd->status.hp = 0;
        pc_damage(nullptr, sd, 1);
        hp = 0;
    }
    sd->status.sp += sp;
    if (sd->status.sp <= 0)
        sd->status.sp = 0;
    if (hp)
    {
        clif_updatestatus(sd, SP::HP);

        if (hp > 0) {
            clif_send_hp_partial(sd);
        }
    }

    if (sp)
        clif_updatestatus(sd, SP::SP);

    return 0;
}

/*==========================================
 * 見た目変更
 * Change the look
 *------------------------------------------
 */
int pc_changelook(dumb_ptr<map_session_data> sd, LOOK type, int val)
{
    nullpo_retz(sd);

    switch (type)
    {
        case LOOK::HAIR:
            sd->status.hair = val;
            break;
        case LOOK::WEAPON:
            sd->status.weapon = static_cast<ItemLook>(static_cast<uint16_t>(val));
            break;
        case LOOK::HEAD_BOTTOM:
            sd->status.head_bottom = wrap<ItemNameId>(val);
            break;
        case LOOK::HEAD_TOP:
            sd->status.head_top = wrap<ItemNameId>(val);
            break;
        case LOOK::HEAD_MID:
            sd->status.head_mid = wrap<ItemNameId>(val);
            break;
        case LOOK::HAIR_COLOR:
            sd->status.hair_color = val;
            break;
        case LOOK::CLOTHES_COLOR:
            sd->status.clothes_color = val;
            break;
        case LOOK::SHIELD:
            sd->status.shield = wrap<ItemNameId>(val);
            break;
        case LOOK::SHOES:
            break;
    }
    clif_changelook(sd, type, val);

    return 0;
}

/*==========================================
 * script用変数の値を読む
 * Read the value of a variable for script
 *------------------------------------------
 */
int pc_readreg(dumb_ptr<block_list> sd, SIR reg)
{
    nullpo_retz(sd);

    return sd->regm.get(reg);
}

/*==========================================
 * script用変数の値を設定
 * Set the value of a variable for script
 *------------------------------------------
 */
void pc_setreg(dumb_ptr<block_list> sd, SIR reg, int val)
{
    nullpo_retv(sd);

    sd->regm.put(reg, val);
}

/*==========================================
 * script用文字列変数の値を読む
 * Reading the value of a string variable for script
 *------------------------------------------
 */
ZString pc_readregstr(dumb_ptr<block_list> sd, SIR reg)
{
    nullpo_retr(ZString(), sd);

    Option<P<RString>> s = sd->regstrm.search(reg);
    return s.map([](P<RString> s_) -> ZString { return *s_; }).copy_or(""_s);
}

/*==========================================
 * script用文字列変数の値を設定
 * Set the value of a string variable for script
 *------------------------------------------
 */
void pc_setregstr(dumb_ptr<block_list> sd, SIR reg, RString str)
{
    nullpo_retv(sd);

    if (!str)
    {
        sd->regstrm.erase(reg);
        return;
    }

    sd->regstrm.insert(reg, str);
}

/*==========================================
 * script用グローバル変数の値を読む
 * Read the value of a global variable for script
 *------------------------------------------
 */
int pc_readglobalreg(dumb_ptr<map_session_data> sd, VarName reg)
{
    int i;
    int quest_shift = 0;
    int quest_mask = 0;
    nullpo_retz(sd);
    QuestId questid;
    XString var = reg;
    VarName vr;

    assert (sd->status.global_reg_num <= GLOBAL_REG_NUM);
    Option<P<struct quest_data>> quest_data_ = questdb_searchname(var);
    OMATCH_BEGIN_SOME(quest_data, quest_data_)
    {
        questid = quest_data->questid;
        reg = quest_data->quest_vr;
        vr = quest_data->quest_var;
        quest_shift = quest_data->quest_shift;
        quest_mask = quest_data->quest_mask;
    }
    OMATCH_END ();
    for (i = 0; i < sd->status.global_reg_num; i++)
    {
        if (sd->status.global_reg[i].str == reg)
        {
            if (questid)
            {
                return ((sd->status.global_reg[i].value & (((1 << quest_mask) - 1) << (quest_shift * quest_mask))) >> (quest_shift * quest_mask));
            }
            else
            {
                return sd->status.global_reg[i].value;
            }
        }
    }

    return 0;
}

/*==========================================
 * script用グローバル変数の値を設定
 * Set the value of a global variable for script
 *------------------------------------------
 */
int pc_setglobalreg(dumb_ptr<map_session_data> sd, VarName reg, int val)
{
    int i;
    int quest_shift = 0;
    int quest_mask = 0;
    int bitval = val;
    nullpo_retz(sd);
    QuestId questid;
    XString var = reg;
    VarName vr;

    //PC_DIE_COUNTERがスクリプトなどで変更された時の処理 | What to do when the PC_DIE_COUNTER is changed by script, etc.
    if (reg == stringish<VarName>("PC_DIE_COUNTER"_s) && sd->die_counter != val)
    {
        sd->die_counter = val;
        pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
    }
    Option<P<struct quest_data>> quest_data_ = questdb_searchname(var);
    OMATCH_BEGIN_SOME(quest_data, quest_data_)
    {
        questid = quest_data->questid;
        reg = quest_data->quest_vr;
        vr = quest_data->quest_var;
        quest_shift = quest_data->quest_shift;
        quest_mask = quest_data->quest_mask;
        assert (((1 << quest_mask) - 1) >= val);
    }
    OMATCH_END ();
    assert (sd->status.global_reg_num <= GLOBAL_REG_NUM);
    if (val == 0)
    {
        for (i = 0; i < sd->status.global_reg_num; i++)
        {
            if (sd->status.global_reg[i].str == reg)
            {
                if (questid)
                {
                    bitval = ((sd->status.global_reg[i].value & ~(((1 << quest_mask) - 1) << (quest_shift * quest_mask))) | (val << (quest_shift * quest_mask)));
                    clif_sendquest(sd, questid, val);
                }
                sd->status.global_reg[i].value = bitval;
                if (sd->status.global_reg[i].value == 0)
                {
                    sd->status.global_reg[i] =
                        sd->status.global_reg[sd->status.global_reg_num - 1];
                    sd->status.global_reg_num--;
                }
                break;
            }
        }
        return 0;
    }
    for (i = 0; i < sd->status.global_reg_num; i++)
    {
        if (sd->status.global_reg[i].str == reg)
        {
            if (questid)
            {
                bitval = ((sd->status.global_reg[i].value & ~(((1 << quest_mask) - 1) << (quest_shift * quest_mask))) | (val << (quest_shift * quest_mask)));
                clif_sendquest(sd, questid, val);
            }
            sd->status.global_reg[i].value = bitval;
            return 0;
        }
    }
    if (sd->status.global_reg_num < GLOBAL_REG_NUM)
    {
        sd->status.global_reg[i].str = reg;
        if (questid)
        {
            bitval = ((sd->status.global_reg[i].value & ~(((1 << quest_mask) - 1) << (quest_shift * quest_mask))) | (val << (quest_shift * quest_mask)));
            clif_sendquest(sd, questid, val);
        }
        sd->status.global_reg[i].value = bitval;
        sd->status.global_reg_num++;
        return 0;
    }
    if (battle_config.error_log)
        PRINTF("pc_setglobalreg : couldn't set %s (GLOBAL_REG_NUM = %d)\n"_fmt,
                reg, GLOBAL_REG_NUM);

    return 1;
}

/*==========================================
 * script用アカウント変数の値を読む
 * Read the value of the account variable for script
 *------------------------------------------
 */
int pc_readaccountreg(dumb_ptr<map_session_data> sd, VarName reg)
{
    int i;

    nullpo_retz(sd);

    assert (sd->status.account_reg_num <= ACCOUNT_REG_NUM);
    for (i = 0; i < sd->status.account_reg_num; i++)
    {
        if (sd->status.account_reg[i].str == reg)
            return sd->status.account_reg[i].value;
    }

    return 0;
}

/*==========================================
 * script用アカウント変数の値を設定
 * Set the value of the account variable for script
 *------------------------------------------
 */
int pc_setaccountreg(dumb_ptr<map_session_data> sd, VarName reg, int val)
{
    int i;

    nullpo_retz(sd);

    if (val == 0)
    {
        for (i = 0; i < sd->status.account_reg_num; i++)
        {
            if (sd->status.account_reg[i].str == reg)
            {
                sd->status.account_reg[i] =
                    sd->status.account_reg[sd->status.account_reg_num - 1];
                sd->status.account_reg_num--;
                break;
            }
        }
        intif_saveaccountreg(sd);
        return 0;
    }
    for (i = 0; i < sd->status.account_reg_num; i++)
    {
        if (sd->status.account_reg[i].str == reg)
        {
            sd->status.account_reg[i].value = val;
            intif_saveaccountreg(sd);
            return 0;
        }
    }
    if (sd->status.account_reg_num < ACCOUNT_REG_NUM)
    {
        sd->status.account_reg[i].str = reg;
        sd->status.account_reg[i].value = val;
        sd->status.account_reg_num++;
        intif_saveaccountreg(sd);
        return 0;
    }
    if (battle_config.error_log)
        PRINTF("pc_setaccountreg : couldn't set %s (ACCOUNT_REG_NUM = %zu)\n"_fmt,
                reg, ACCOUNT_REG_NUM);

    return 1;
}

/*==========================================
 * script用アカウント変数2の値を読む
 * Read the value of account variable 2 for script
 *------------------------------------------
 */
int pc_readaccountreg2(dumb_ptr<map_session_data> sd, VarName reg)
{
    int i;

    nullpo_retz(sd);

    for (i = 0; i < sd->status.account_reg2_num; i++)
    {
        if (sd->status.account_reg2[i].str == reg)
            return sd->status.account_reg2[i].value;
    }

    return 0;
}

/*==========================================
 * script用アカウント変数2の値を設定
 * Set the value of account variable 2 for script
 *------------------------------------------
 */
int pc_setaccountreg2(dumb_ptr<map_session_data> sd, VarName reg, int val)
{
    int i;

    nullpo_retr(1, sd);

    if (val == 0)
    {
        for (i = 0; i < sd->status.account_reg2_num; i++)
        {
            if (sd->status.account_reg2[i].str == reg)
            {
                sd->status.account_reg2[i] =
                    sd->status.account_reg2[sd->status.account_reg2_num - 1];
                sd->status.account_reg2_num--;
                break;
            }
        }
        chrif_saveaccountreg2(sd);
        return 0;
    }
    for (i = 0; i < sd->status.account_reg2_num; i++)
    {
        if (sd->status.account_reg2[i].str == reg)
        {
            sd->status.account_reg2[i].value = val;
            chrif_saveaccountreg2(sd);
            return 0;
        }
    }
    if (sd->status.account_reg2_num < ACCOUNT_REG2_NUM)
    {
        sd->status.account_reg2[i].str = reg;
        sd->status.account_reg2[i].value = val;
        sd->status.account_reg2_num++;
        chrif_saveaccountreg2(sd);
        return 0;
    }
    if (battle_config.error_log)
        PRINTF("pc_setaccountreg2 : couldn't set %s (ACCOUNT_REG2_NUM = %zu)\n"_fmt,
                reg, ACCOUNT_REG2_NUM);

    return 1;
}

/*==========================================
 * イベントタイマー処理
 * Event Timer Processing
 *------------------------------------------
 */
static
void pc_eventtimer(TimerData *, tick_t, BlockId id, NpcEvent data)
{
    dumb_ptr<map_session_data> sd = map_id2sd(id);
    assert (sd != nullptr);

    npc_event(sd, data, 0);
}

/*==========================================
 * イベントタイマー追加
 * Add Event Timer
 *------------------------------------------
 */
int pc_addeventtimer(dumb_ptr<map_session_data> sd, interval_t tick, NpcEvent name)
{
    int i;

    nullpo_retz(sd);

    for (i = 0; i < MAX_EVENTTIMER; i++)
        if (!sd->eventtimer[i])
            break;

    if (i < MAX_EVENTTIMER)
    {
        sd->eventtimer[i] = Timer(gettick() + tick,
                std::bind(pc_eventtimer, ph::_1, ph::_2,
                    sd->bl_id, name));
        return 1;
    }

    return 0;
}

/*==========================================
 * イベントタイマー全削除
 * Delete all event timers
 *------------------------------------------
 */
int pc_cleareventtimer(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    for (int i = 0; i < MAX_EVENTTIMER; i++)
        sd->eventtimer[i].cancel();

    return 0;
}

//
// 装 備物 | Equipment
//
/*==========================================
 * アイテムを装備する
 * Equip items
 *------------------------------------------
 */
static
int pc_signal_advanced_equipment_change(dumb_ptr<map_session_data> sd, IOff0 n)
{
    if (bool(sd->status.inventory[n].equip & EPOS::SHOES))
        clif_changelook(sd, LOOK::SHOES, 0);
    if (bool(sd->status.inventory[n].equip & EPOS::GLOVES))
        clif_changelook(sd, LOOK::GLOVES, 0);
    if (bool(sd->status.inventory[n].equip & EPOS::CAPE))
        clif_changelook(sd, LOOK::CAPE, 0);
    if (bool(sd->status.inventory[n].equip & EPOS::MISC1))
        clif_changelook(sd, LOOK::MISC1, 0);
    if (bool(sd->status.inventory[n].equip & EPOS::MISC2))
        clif_changelook(sd, LOOK::MISC2, 0);
    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_equipitem(dumb_ptr<map_session_data> sd, IOff0 n, EPOS)
{
    ItemNameId nameid;
    //ｿｽ]ｿｽｿｽｿｽｿｽｿｽ{ｿｽqｿｽﾌ場合ｿｽﾌ鯉ｿｽｿｽﾌ職ｿｽﾆゑｿｽｿｽZｿｽoｿｽｿｽｿｽｿｽ | Sos] sos sos sos sos sos { sos q sos if sos f (???)

    nullpo_retz(sd);

    if (!n.ok())
    {
        clif_equipitemack(sd, IOff0::from(0), EPOS::ZERO, 0);
        return 0;
    }

    nameid = sd->status.inventory[n].nameid;
    // can't actually happen - the only caller checks this.
    P<struct item_data> id = TRY_UNWRAP(sd->inventory_data[n], return 0);
    EPOS pos = pc_equippoint(sd, n);

    if (battle_config.battle_log)
        PRINTF("equip %d (%d) %x:%x\n"_fmt,
                nameid, n, id->equip, pos);
    if (!pc_isequip(sd, n) || pos == EPOS::ZERO)
    {
        clif_equipitemack(sd, n, EPOS::ZERO, 0);    // fail
        return 0;
    }

// -- moonsoul (if player is berserk then cannot equip)
//
    if (pos == (EPOS::MISC2 | EPOS::CAPE))
    {
        //アクセサリ用例外処理 | Accessory exception handling
        EPOS epor = EPOS::ZERO;
        IOff0 midx = sd->equip_index_maybe[EQUIP::MISC2];
        IOff0 cidx = sd->equip_index_maybe[EQUIP::CAPE];
        if (midx.ok())
            epor |= sd->status.inventory[midx].equip;
        if (cidx.ok())
            epor |= sd->status.inventory[cidx].equip;
        epor &= (EPOS::MISC2 | EPOS::CAPE);
        pos = (epor == EPOS::CAPE ? EPOS::MISC2 : EPOS::CAPE);
    }

    for (EQUIP i : EQUIPs)
    {
        if (bool(pos & equip_pos[i]))
        {
            IOff0 *idx = &sd->equip_index_maybe[i];
            if ((*idx).ok())    //Slot taken, remove item from there.
                pc_unequipitem(sd, *idx, CalcStatus::LATER);
            *idx = n;
        }
    }
    //弓矢装備 | Bow and arrow equipment
    if (pos == EPOS::ARROW)
    {
        clif_arrowequip(sd, n);
        clif_arrow_fail(sd, 3);    //3=矢が装備できました | 3=Arrows can be equipped.
    }
    else
    {
        /* Don't update re-equipping if we're using a spell */
        if (!(pos == EPOS::GLOVES && sd->attack_spell_override))
            clif_equipitemack(sd, n, pos, 1);
    }

    for (EQUIP i : EQUIPs)
    {
        if (bool(pos & equip_pos[i]))
            sd->equip_index_maybe[i] = n;
    }
    sd->status.inventory[n].equip = pos;

    ItemNameId view_i;
    ItemLook view_l = ItemLook::W_FIST;
    // TODO: This is ugly.
    OMATCH_BEGIN_SOME (sdidn, sd->inventory_data[n])
    {
        bool look_not_weapon = sdidn->look == ItemLook::W_FIST;
        bool equip_is_weapon = bool(sd->status.inventory[n].equip & EPOS::WEAPON);
        assert (look_not_weapon != equip_is_weapon);

        if (look_not_weapon)
            view_i = sdidn->nameid;
        else
            view_l = sdidn->look;
    }
    OMATCH_END ();

    if (bool(sd->status.inventory[n].equip & EPOS::WEAPON))
    {
        sd->weapontype1 = view_l;
        pc_calcweapontype(sd);
        pc_set_weapon_look(sd);
    }
    if (bool(sd->status.inventory[n].equip & EPOS::SHIELD))
    {
        OMATCH_BEGIN (sd->inventory_data[n])
        {
            OMATCH_CASE_SOME (sdidn)
            {
                if (sdidn->type == ItemType::WEAPON)
                {
                    sd->status.shield = ItemNameId();
                    if (sd->status.inventory[n].equip == EPOS::SHIELD)
                        assert(0 && "unreachable - offhand weapons are not supported");
                }
                else if (sdidn->type == ItemType::ARMOR)
                {
                    sd->status.shield = view_i;
                }
            }
            OMATCH_CASE_NONE ()
            {
                sd->status.shield = ItemNameId();
            }
        }
        OMATCH_END ();
        pc_calcweapontype(sd);
        clif_changelook(sd, LOOK::SHIELD, unwrap<ItemNameId>(sd->status.shield));
    }
    if (bool(sd->status.inventory[n].equip & EPOS::LEGS))
    {
        sd->status.head_bottom = view_i;
        clif_changelook(sd, LOOK::HEAD_BOTTOM, unwrap<ItemNameId>(sd->status.head_bottom));
    }
    if (bool(sd->status.inventory[n].equip & EPOS::HAT))
    {
        sd->status.head_top = view_i;
        clif_changelook(sd, LOOK::HEAD_TOP, unwrap<ItemNameId>(sd->status.head_top));
    }
    if (bool(sd->status.inventory[n].equip & EPOS::TORSO))
    {
        sd->status.head_mid = view_i;
        clif_changelook(sd, LOOK::HEAD_MID, unwrap<ItemNameId>(sd->status.head_mid));
    }
    pc_signal_advanced_equipment_change(sd, n);

    pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);

    return 0;
}

/*==========================================
 * 装 備した物を外す
 * Remove the attached object
 *------------------------------------------
 */
int pc_unequipitem(dumb_ptr<map_session_data> sd, IOff0 n, CalcStatus type)
{
    nullpo_retz(sd);

// -- moonsoul  (if player is berserk then cannot unequip)
//
    if (battle_config.battle_log)
        PRINTF("unequip %d %x:%x\n"_fmt,
                n, pc_equippoint(sd, n),
                sd->status.inventory[n].equip);
    if (bool(sd->status.inventory[n].equip))
    {
        for (EQUIP i : EQUIPs)
        {
            if (bool(sd->status.inventory[n].equip & equip_pos[i]))
                sd->equip_index_maybe[i] = IOff0::from(-1);
        }
        if (bool(sd->status.inventory[n].equip & EPOS::WEAPON))
        {
            sd->weapontype1 = ItemLook::W_FIST;
            // when reading the diff, think twice about this
            sd->status.weapon = ItemLook::W_FIST;
            pc_calcweapontype(sd);
            pc_set_weapon_look(sd);
        }
        if (bool(sd->status.inventory[n].equip & EPOS::SHIELD))
        {
            sd->status.shield = ItemNameId();
            pc_calcweapontype(sd);
            clif_changelook(sd, LOOK::SHIELD, unwrap<ItemNameId>(sd->status.shield));
        }
        if (bool(sd->status.inventory[n].equip & EPOS::LEGS))
        {
            sd->status.head_bottom = ItemNameId();
            clif_changelook(sd, LOOK::HEAD_BOTTOM, unwrap<ItemNameId>(sd->status.head_bottom));
        }
        if (bool(sd->status.inventory[n].equip & EPOS::HAT))
        {
            sd->status.head_top = ItemNameId();
            clif_changelook(sd, LOOK::HEAD_TOP, unwrap<ItemNameId>(sd->status.head_top));
        }
        if (bool(sd->status.inventory[n].equip & EPOS::TORSO))
        {
            sd->status.head_mid = ItemNameId();
            clif_changelook(sd, LOOK::HEAD_MID, unwrap<ItemNameId>(sd->status.head_mid));
        }
        pc_signal_advanced_equipment_change(sd, n);

        clif_unequipitemack(sd, n, sd->status.inventory[n].equip, 1);
        sd->status.inventory[n].equip = EPOS::ZERO;
    }
    else
    {
        clif_unequipitemack(sd, n, EPOS::ZERO, 0);
    }
    if (type == CalcStatus::NOW)
    {
        pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
    }

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_unequipinvyitem(dumb_ptr<map_session_data> sd, IOff0 n, CalcStatus type)
{
    nullpo_retr(1, sd);

    for (EQUIP i : EQUIPs)
    {
        if (equip_pos[i] != EPOS::ZERO
            && !bool(equip_pos[i] & EPOS::ARROW) // probably a bug
            && sd->equip_index_maybe[i] == n)
        {
            //Slot taken, remove item from there.
            pc_unequipitem(sd, sd->equip_index_maybe[i], type);
            sd->equip_index_maybe[i] = IOff0::from(-1);
        }
    }

    return 0;
}

/*==========================================
 * アイテムのindex番号を詰めたり
 * 装 備品の装備可能チェックを行なう
 * Stuffing the index number of the item or
 * Check the equipping of equipment
 *------------------------------------------
 */
int pc_checkitem(dumb_ptr<map_session_data> sd)
{
    int calc_flag = 0;

    nullpo_retz(sd);

    IOff0 j = IOff0::from(0);
    for (IOff0 i : IOff0::iter())
    {
        if (!(sd->status.inventory[i].nameid))
            continue;
        if (i != j)
        {
            sd->status.inventory[j] = sd->status.inventory[i];
            sd->inventory_data[j] = sd->inventory_data[i];
        }
        ++j;
    }
    for (IOff0 k = j; k != IOff0::from(MAX_INVENTORY); ++k)
    {
        sd->status.inventory[k] = Item{};
        sd->inventory_data[k] = nullptr;
    }

    for (IOff0 i : IOff0::iter())
    {
        if (!sd->status.inventory[i].nameid)
            continue;
        if (bool(sd->status.inventory[i].equip & ~pc_equippoint(sd, i)))
        {
            sd->status.inventory[i].equip = EPOS::ZERO;
            calc_flag = 1;
        }
    }

    pc_setequipindex(sd);
    if (calc_flag)
        pc_calcstatus(sd, (int)CalcStatusKind::ITEM_BONUS_RECALC);

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_checkoverhp(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    if (sd->status.hp == sd->status.max_hp)
        return 1;
    if (sd->status.hp > sd->status.max_hp)
    {
        sd->status.hp = sd->status.max_hp;
        clif_updatestatus(sd, SP::HP);
        return 2;
    }

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_checkoversp(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    if (sd->status.sp == sd->status.max_sp)
        return 1;
    if (sd->status.sp > sd->status.max_sp)
    {
        sd->status.sp = sd->status.max_sp;
        clif_updatestatus(sd, SP::SP);
        return 2;
    }

    return 0;
}

/*==========================================
 * PVP順位計算用(foreachinarea)
 * For PVP ranking calculation (foreachinarea)
 *------------------------------------------
 */
static
void pc_calc_pvprank_sub(dumb_ptr<block_list> bl, dumb_ptr<map_session_data> sd2)
{
    dumb_ptr<map_session_data> sd1;

    nullpo_retv(bl);
    sd1 = bl->is_player();
    nullpo_retv(sd2);

    if (sd1->pvp_point > sd2->pvp_point)
        sd2->pvp_rank++;
}

/*==========================================
 * PVP順位計算
 * PVP ranking calculation
 *------------------------------------------
 */
int pc_calc_pvprank(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);
    P<map_local> m = sd->bl_m;

    if (!(m->flag.get(MapFlag::PVP)))
        return 0;
    sd->pvp_rank = 1;
    map_foreachinarea(std::bind(pc_calc_pvprank_sub, ph::_1, sd),
            sd->bl_m,
            0, 0,
            m->xs, m->ys,
            BL::PC);
    return sd->pvp_rank;
}

/*==========================================
 * PVP順位計算(timer)
 * PVP ranking calculation (timer)
 *------------------------------------------
 */
void pc_calc_pvprank_timer(TimerData *, tick_t, BlockId id)
{
    dumb_ptr<map_session_data> sd = nullptr;
    if (battle_config.pk_mode)  // disable pvp ranking if pk_mode on [Valaris]
        return;

    sd = map_id2sd(id);
    if (sd == nullptr)
        return;
    sd->pvp_timer.cancel();
    if (pc_calc_pvprank(sd) > 0)
    {
        sd->pvp_timer = Timer(gettick() + PVP_CALCRANK_INTERVAL,
                std::bind(pc_calc_pvprank_timer, ph::_1, ph::_2,
                    id));
    }
}

/*==========================================
 * sdは結婚しているか(既婚の場合は相方のchar_idを返す)
 * Is sd married (returns partner's char_id if married)
 *------------------------------------------
 */
static
CharId pc_ismarried(dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return CharId();
    if (sd->status.partner_id)
        return sd->status.partner_id;
    else
        return CharId();
}

/*==========================================
 * sdがdstsdと結婚(dstsd→sdの結婚処理も同時に行う)
 * sd marries dstsd (dstsd → sd marriage processing is also performed at the same time)
 *------------------------------------------
 */
int pc_marriage(dumb_ptr<map_session_data> sd, dumb_ptr<map_session_data> dstsd)
{
    if (sd == nullptr || dstsd == nullptr || sd->status.partner_id
        || dstsd->status.partner_id)
        return -1;
    sd->status.partner_id = dstsd->status_key.char_id;
    dstsd->status.partner_id = sd->status_key.char_id;
    return 0;
}

/*==========================================
 * sdが離婚(相手はsd->status.partner_idに依る)(相手も同時に離婚・結婚指輪自動剥奪)
 * sd is divorced (partner depends on sd->status.partner_id) (partner is also divorced at the same time and wedding ring is automatically stripped)
 *------------------------------------------
 */
int pc_divorce(dumb_ptr<map_session_data> sd)
{
    dumb_ptr<map_session_data> p_sd = nullptr;
    if (sd == nullptr || !pc_ismarried(sd))
        return -1;

    // If both are on map server we don't need to bother the char server
    if ((p_sd =
         map_nick2sd(map_charid2nick(sd->status.partner_id))) != nullptr)
    {
        if (p_sd->status.partner_id != sd->status_key.char_id
            || sd->status.partner_id != p_sd->status_key.char_id)
        {
            PRINTF("pc_divorce: Illegal partner_id sd=%d p_sd=%d\n"_fmt,
                    sd->status.partner_id, p_sd->status.partner_id);
            return -1;
        }
        p_sd->status.partner_id = CharId();
        sd->status.partner_id = CharId();
    }
    else
    {
        sd->status.partner_id = CharId();
        chrif_send_divorce(sd->status_key.char_id);
    }

    return 0;
}

/*==========================================
 * sdの相方のmap_session_dataを返す
 * return map_session_data of sd's counterpart
 *------------------------------------------
 */
dumb_ptr<map_session_data> pc_get_partner(dumb_ptr<map_session_data> sd)
{
    dumb_ptr<map_session_data> p_sd = nullptr;
    if (sd == nullptr || !pc_ismarried(sd))
        return nullptr;

    CharName nick = map_charid2nick(sd->status.partner_id);

    if (!nick.to__actual())
        return nullptr;

    if ((p_sd = map_nick2sd(nick)) == nullptr)
        return nullptr;

    return p_sd;
}

//
// 自然回復物 | Natural Restoration
//
/*==========================================
 * SP回復量計算
 * SP Recovery Calculator
 *------------------------------------------
 */
static
interval_t pc_spheal(dumb_ptr<map_session_data> sd)
{
    nullpo_retr(interval_t::zero(), sd);

    interval_t a = natural_heal_diff_tick;
    if (pc_issit(sd))
        a += a;

    return a;
}

/*==========================================
 * HP回復量計算
 * HP Recovery Calculator
 *------------------------------------------
 */
static
interval_t pc_hpheal(dumb_ptr<map_session_data> sd)
{
    nullpo_retr(interval_t::zero(), sd);

    interval_t a = natural_heal_diff_tick;
    if (pc_issit(sd))
        a += a;

    return a;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_natural_heal_hp(dumb_ptr<map_session_data> sd)
{
    int bhp;
    int bonus;

    nullpo_retz(sd);

    if (pc_checkoverhp(sd))
    {
        sd->hp_sub = sd->inchealhptick = interval_t::zero();
        return 0;
    }

    bhp = sd->status.hp;

    if (!sd->walktimer)
    {
        interval_t inc_num = pc_hpheal(sd);
        sd->hp_sub += inc_num;
        sd->inchealhptick += natural_heal_diff_tick;
    }
    else
    {
        sd->hp_sub = sd->inchealhptick = interval_t::zero();
        return 0;
    }

    if (sd->hp_sub >= battle_config.natural_healhp_interval)
    {
        bonus = sd->nhealhp;
        while (sd->hp_sub >= battle_config.natural_healhp_interval)
        {
            sd->hp_sub -= battle_config.natural_healhp_interval;
            if (sd->status.hp + bonus <= sd->status.max_hp)
                sd->status.hp += bonus;
            else
            {
                sd->status.hp = sd->status.max_hp;
                sd->hp_sub = sd->inchealhptick = interval_t::zero();
            }
        }
    }
    if (bhp != sd->status.hp)
    {
        clif_updatestatus(sd, SP::HP);
        clif_send_hp_partial(sd);
    }

    sd->inchealhptick = interval_t::zero();

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
int pc_natural_heal_sp(dumb_ptr<map_session_data> sd)
{
    int bsp;
    int bonus;

    nullpo_retz(sd);

    if (pc_checkoversp(sd))
    {
        sd->sp_sub = sd->inchealsptick = interval_t::zero();
        return 0;
    }

    bsp = sd->status.sp;

    interval_t inc_num = pc_spheal(sd);
    sd->sp_sub += inc_num;
    if (!sd->walktimer)
        sd->inchealsptick += natural_heal_diff_tick;
    else
        sd->inchealsptick = interval_t::zero();

    if (sd->sp_sub >= battle_config.natural_healsp_interval)
    {
        bonus = sd->nhealsp;
        while (sd->sp_sub >= battle_config.natural_healsp_interval)
        {
            sd->sp_sub -= battle_config.natural_healsp_interval;
            if (sd->status.sp + bonus <= sd->status.max_sp)
                sd->status.sp += bonus;
            else
            {
                sd->status.sp = sd->status.max_sp;
                sd->sp_sub = sd->inchealsptick = interval_t::zero();
            }
        }
    }

    if (bsp != sd->status.sp)
        clif_updatestatus(sd, SP::SP);

    sd->inchealsptick = interval_t::zero();
    return 0;
}

/*==========================================
 * HP/SP 自然回復 各クライアント
 * HP/SP Natural Recovery Each client
 *------------------------------------------
 */
static
int pc_quickregenerate_effect(struct quick_regeneration *quick_regen,
                           int heal_speed)
{
    if (!(quick_regen->tickdelay--))
    {
        int bonus =
            std::min(heal_speed * battle_config.itemheal_regeneration_factor,
                 quick_regen->amount);

        quick_regen->amount -= bonus;

        quick_regen->tickdelay = quick_regen->speed;

        return bonus;
    }

    return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
static
void pc_natural_heal_sub(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    if (sd->heal_xp > 0)
    {
        if (sd->heal_xp < 64)
            --sd->heal_xp;      // [Fate] Slowly reduce XP that healers can get for healing this char
        else
            sd->heal_xp -= (sd->heal_xp >> 6);
    }

    // Hijack this callback:  Adjust spellpower bonus
    if (sd->spellpower_bonus_target < sd->spellpower_bonus_current)
    {
        sd->spellpower_bonus_current = sd->spellpower_bonus_target;
        pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
    }
    else if (sd->spellpower_bonus_target > sd->spellpower_bonus_current)
    {
        sd->spellpower_bonus_current +=
            1 +
            ((sd->spellpower_bonus_target -
              sd->spellpower_bonus_current) >> 5);
        pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
    }

    if (sd->sc_data[StatusChange::SC_HALT_REGENERATE].timer)
        return;

    if (sd->quick_regeneration_hp.amount || sd->quick_regeneration_sp.amount)
    {
        int hp_bonus = pc_quickregenerate_effect(&sd->quick_regeneration_hp,
                (!sd->sc_data[StatusChange::SC_POISON].timer
                    || sd->sc_data[StatusChange::SC_SLOWPOISON].timer)
                ? sd->nhealhp
                : 1);   // [fate] slow down when poisoned
        int sp_bonus = pc_quickregenerate_effect(&sd->quick_regeneration_sp,
                                                   sd->nhealsp);

        pc_itemheal_effect(sd, hp_bonus, sp_bonus);
    }
    skill_update_heal_animation(sd);   // if needed.

    if ((sd->sc_data[StatusChange::SC_FLYING_BACKPACK].timer
            || battle_config.natural_heal_weight_rate > 100
            || sd->weight * 100 / sd->max_weight < battle_config.natural_heal_weight_rate)
        && !pc_isdead(sd)
        && !sd->sc_data[StatusChange::SC_POISON].timer)
    {
        pc_natural_heal_hp(sd);
        pc_natural_heal_sp(sd);
    }
    else
    {
        sd->hp_sub = sd->inchealhptick = interval_t::zero();
        sd->sp_sub = sd->inchealsptick = interval_t::zero();
    }
}

/*==========================================
 * HP/SP自然回復 (interval timer関数)
 * HP/SP natural recovery (interval timer function)
 *------------------------------------------
 */
static
void pc_natural_heal(TimerData *, tick_t tick)
{
    natural_heal_tick = tick;
    natural_heal_diff_tick = natural_heal_tick - natural_heal_prev_tick;
    clif_foreachclient(pc_natural_heal_sub);

    natural_heal_prev_tick = tick;
}

/*==========================================
 * セーブポイントの保存
 * Saving savepoints
 *------------------------------------------
 */
void pc_setsavepoint(dumb_ptr<map_session_data> sd, MapName mapname, int x, int y)
{
    nullpo_retv(sd);

    sd->status.save_point.map_ = mapname;
    sd->status.save_point.x = x;
    sd->status.save_point.y = y;
}

/*==========================================
 * 自動セーブ 各クライアント
 * Autosave each client
 *------------------------------------------
 */
static
void pc_autosave_sub(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    if (save_flag == 0 && sd->sess->fd.uncast_dammit() > last_save_fd)
    {
        pc_makesavestatus(sd);
        chrif_save(sd);

        save_flag = 1;
        last_save_fd = sd->sess->fd.uncast_dammit();
    }
}

/*==========================================
 * 自動セーブ (timer関数)
 * Autosave (timer function)
 *------------------------------------------
 */
static
void pc_autosave(TimerData *, tick_t)
{
    save_flag = 0;
    clif_foreachclient(pc_autosave_sub);
    if (save_flag == 0)
        last_save_fd = -1;

    interval_t interval = map_conf.autosave_time / (clif_countusers() + 1);
    if (interval <= interval_t::zero())
        interval = 1_ms;
    Timer(gettick() + interval,
            pc_autosave
    ).detach();
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_read_gm_account(Session *, const std::vector<Packet_Repeat<0x2b15>>& repeat)
{
    gm_accountm.clear();

    for (const auto& i : repeat)
    {
        AccountId account_id = i.account_id;
        GmLevel level = i.gm_level;
        gm_accountm[account_id] = level;
    }
    return gm_accountm.size();
}

/*==========================================
 * 
 *------------------------------------------
 */
void pc_setstand(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    sd->state.dead_sit = 0;

    if (sd->automod == AutoMod::autoblock)
        clif_gm_collision(sd, 0);
}

/*==========================================
 * 
 *------------------------------------------
 */
static
void pc_calc_sigma(void)
{
    int j, k;

    {
        for (int& it : hp_sigma_val_0)
            it = 0;
        for (k = 0, j = 2; j <= MAX_LEVEL; j++)
        {
            k += hp_coefficient_0 * j + 50;
            k -= k % 100;
            hp_sigma_val_0[j - 1] = k;
        }
    }
}

/*==========================================
 * 
 *------------------------------------------
 */
void do_init_pc(void)
{
    pc_calc_sigma();
    natural_heal_prev_tick = gettick() + NATURAL_HEAL_INTERVAL;
    Timer(natural_heal_prev_tick,
            pc_natural_heal,
            NATURAL_HEAL_INTERVAL
    ).detach();
    Timer(gettick() + map_conf.autosave_time,
            pc_autosave
    ).detach();
}

/*==========================================
 * 
 *------------------------------------------
 */
void pc_invisibility(dumb_ptr<map_session_data> sd, int enabled)
{
    if (enabled && !bool(sd->status.option & Opt0::INVISIBILITY))
    {
        clif_clearchar(sd, BeingRemoveWhy::WARPED);
        sd->status.option |= Opt0::INVISIBILITY;
        clif_status_change(sd, StatusChange::CLIF_OPTION_SC_INVISIBILITY, 1);
    }
    else if (!enabled)
    {
        sd->status.option &= ~Opt0::INVISIBILITY;
        clif_status_change(sd, StatusChange::CLIF_OPTION_SC_INVISIBILITY, 0);
        pc_setpos(sd, sd->bl_m->name_, sd->bl_x, sd->bl_y, BeingRemoveWhy::WARPED);
    }
}

/*==========================================
 * 
 *------------------------------------------
 */
int pc_logout(dumb_ptr<map_session_data> sd) // [fate] Player logs out
{
    if (!sd)
        return 0;

    if (sd->sc_data[StatusChange::SC_POISON].timer)
        sd->status.hp = 1;      // Logging out while poisoned -> bad

    /*
     * Trying to rapidly sign out/in or switch characters to avoid a spell's
     * cast time is also bad. [remoitnane]
     */
#if 0
    // Removed because it's buggy, see above.
    if (sd->cast_tick > tick)
    {
        if (pc_setglobalreg(sd, "MAGIC_CAST_TICK"_s, sd->cast_tick - tick))
            sd->status.sp = 1;
    }
    else
#endif
        pc_setglobalreg(sd, stringish<VarName>("MAGIC_CAST_TICK"_s), 0);

    npc_event_doall_l(stringish<ScriptLabel>("OnPCLogoutEvent"_s), sd->bl_id, nullptr);

    MAP_LOG_STATS(sd, "LOGOUT"_fmt);
    return 0;
}
} // namespace map
} // namespace tmwa
