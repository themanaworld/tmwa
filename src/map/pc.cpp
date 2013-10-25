#include "pc.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <fstream>

#include "../strings/fstring.hpp"
#include "../strings/zstring.hpp"

#include "../common/cxxstdio.hpp"
#include "../common/io.hpp"
#include "../common/nullpo.hpp"
#include "../common/random.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "magic.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "path.hpp"
#include "script.hpp"
#include "skill.hpp"
#include "storage.hpp"
#include "trade.hpp"

#include "../poison.hpp"

// PVP順位計算の間隔
constexpr std::chrono::milliseconds PVP_CALCRANK_INTERVAL =
        std::chrono::seconds(1);

//define it here, since the ifdef only occurs in this file
#define USE_ASTRAL_SOUL_SKILL

#ifdef USE_ASTRAL_SOUL_SKILL
// [fate] At this threshold, the Astral Soul skill kicks in
constexpr int MAGIC_SKILL_THRESHOLD = 200;
#endif

#define MAP_LOG_STATS(sd, suffix)       \
        MAP_LOG_PC(sd, "STAT %d %d %d %d %d %d " suffix,    \
                sd->status.attrs[ATTR::STR],                \
                sd->status.attrs[ATTR::AGI],                \
                sd->status.attrs[ATTR::VIT],                \
                sd->status.attrs[ATTR::INT],                \
                sd->status.attrs[ATTR::DEX],                \
                sd->status.attrs[ATTR::LUK])

#define MAP_LOG_XP(sd, suffix)  \
        MAP_LOG_PC(sd, "XP %d %d JOB %d %d %d ZENY %d + %d " suffix,    \
                sd->status.base_level, sd->status.base_exp,             \
                sd->status.job_level, sd->status.job_exp, sd->status.skill_point, \
                sd->status.zeny, pc_readaccountreg(sd, stringish<VarName>("BankAccount")))

#define MAP_LOG_MAGIC(sd, suffix)       \
        MAP_LOG_PC(sd, "MAGIC %d %d %d %d %d %d EXP %d %d " suffix,     \
                   sd->status.skill[SkillID::TMW_MAGIC].lv,             \
                   sd->status.skill[SkillID::TMW_MAGIC_LIFE].lv,        \
                   sd->status.skill[SkillID::TMW_MAGIC_WAR].lv,         \
                   sd->status.skill[SkillID::TMW_MAGIC_TRANSMUTE].lv,   \
                   sd->status.skill[SkillID::TMW_MAGIC_NATURE].lv,      \
                   sd->status.skill[SkillID::TMW_MAGIC_ETHER].lv,       \
                   pc_readglobalreg(sd, stringish<VarName>("MAGIC_EXPERIENCE")) & 0xffff,   \
                   (pc_readglobalreg(sd, stringish<VarName>("MAGIC_EXPERIENCE")) >> 24) & 0xff)

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
earray<interval_t, ItemLook, ItemLook::SINGLE_HANDED_COUNT> aspd_base_0 //=
{{
std::chrono::milliseconds(650),
std::chrono::milliseconds(700),
std::chrono::milliseconds(750),
std::chrono::milliseconds(600),
std::chrono::milliseconds(2000),
std::chrono::milliseconds(2000),
std::chrono::milliseconds(800),
std::chrono::milliseconds(2000),
std::chrono::milliseconds(700),
std::chrono::milliseconds(700),
std::chrono::milliseconds(650),
std::chrono::milliseconds(900),
std::chrono::milliseconds(2000),
std::chrono::milliseconds(2000),
std::chrono::milliseconds(2000),
std::chrono::milliseconds(2000),
std::chrono::milliseconds(2000),
}};
static const
int exp_table_0[MAX_LEVEL] =
{
    // 1 .. 9
                9,          16,         25,         36,
    77,         112,        153,        200,        253,
    // 10 .. 19
    320,        385,        490,        585,        700,
    830,        970,        1120,       1260,       1420,
    // 20 .. 29
    1620,       1860,       1990,       2240,       2504,
    2950,       3426,       3934,       4474,       6889,
    // 30 .. 39
    7995,       9174,       10425,      11748,      13967,
    15775,      17678,      19677,      21773,      30543,
    // 40 .. 49
    34212,      38065,      42102,      46323,      53026,
    58419,      64041,      69892,      75973,      102468,
    // 50 .. 59
    115254,     128692,     142784,     157528,     178184,
    196300,     215198,     234879,     255341,     330188,
    // 60 .. 69
    365914,     403224,     442116,     482590,     536948,
    585191,     635278,     687211,     740988,     925400,
    // 70 .. 79
    1473746,    1594058,    1718928,    1848355,    1982340,
    2230113,    2386162,    2547417,    2713878,    3206160,
    // 80 .. 89
    3681024,    4022472,    4377024,    4744680,    5125440,
    5767272,    6204000,    6655464,    7121664,    7602600,
    // 90 .. 99
    9738720,    11649960,   13643520,   18339300,   23836800,
    35658000,   48687000,   58135000,   99999999,   0,
};
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

static
std::map<int, uint8_t> gm_accountm;

static
int pc_checkoverhp(dumb_ptr<map_session_data> sd);
static
int pc_checkoversp(dumb_ptr<map_session_data> sd);
static
int pc_nextbaseafter(dumb_ptr<map_session_data> sd);
static
int pc_nextjobafter(dumb_ptr<map_session_data> sd);
static
void pc_setdead(dumb_ptr<map_session_data> sd)
{
    sd->state.dead_sit = 1;
}

uint8_t pc_isGM(dumb_ptr<map_session_data> sd)
{
    nullpo_ret(sd);

    auto it = gm_accountm.find(sd->status.account_id);
    if (it != gm_accountm.end())
        return it->second;
    return 0;
}

int pc_iskiller(dumb_ptr<map_session_data> src,
                 dumb_ptr<map_session_data> target)
{
    nullpo_ret(src);

    if (src->bl_type != BL::PC)
        return 0;
    if (src->special_state.killer)
        return 1;

    if (target->bl_type != BL::PC)
        return 0;
    if (target->special_state.killable)
        return 1;

    return 0;
}

void pc_set_gm_level(int account_id, uint8_t level)
{
    if (level)
        gm_accountm[account_id] = level;
    else
        gm_accountm.erase(account_id);
}

static
int distance(int x0, int y0, int x1, int y1)
{
    int dx, dy;

    dx = abs(x0 - x1);
    dy = abs(y0 - y1);
    return dx > dy ? dx : dy;
}

static
void pc_invincible_timer(TimerData *, tick_t, int id)
{
    dumb_ptr<map_session_data> sd = map_id2sd(id);

    assert (sd != NULL);
    assert (sd->bl_type == BL::PC);
}

int pc_setinvincibletimer(dumb_ptr<map_session_data> sd, interval_t val)
{
    nullpo_ret(sd);

    sd->invincible_timer = Timer(gettick() + val,
            std::bind(pc_invincible_timer, ph::_1, ph::_2,
                sd->bl_id));
    return 0;
}

int pc_delinvincibletimer(dumb_ptr<map_session_data> sd)
{
    nullpo_ret(sd);

    sd->invincible_timer.cancel();
    return 0;
}

int pc_setrestartvalue(dumb_ptr<map_session_data> sd, int type)
{
    nullpo_ret(sd);

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
 *------------------------------------------
 */
static
void pc_counttargeted_sub(dumb_ptr<block_list> bl,
        int id, int *c, dumb_ptr<block_list> src, ATK target_lv)
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
 *------------------------------------------
 */
static
int pc_walktoxy_sub(dumb_ptr<map_session_data>);

/*==========================================
 * saveに必要なステータス修正を行なう
 *------------------------------------------
 */
void pc_makesavestatus(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    // 服の色は色々弊害が多いので保存対象にはしない
    if (!battle_config.save_clothcolor)
        sd->status.clothes_color = 0;

    // 死亡状態だったのでhpを1、位置をセーブ場所に変更
    if (pc_isdead(sd))
    {
        pc_setrestartvalue(sd, 0);
        sd->status.last_point = sd->status.save_point;
    }
    else
    {
        sd->status.last_point.map_ = sd->mapname_;
        sd->status.last_point.x = sd->bl_x;
        sd->status.last_point.y = sd->bl_y;
    }

    // セーブ禁止マップだったので指定位置に移動
    if (sd->bl_m->flag.nosave)
    {
        map_local *m = sd->bl_m;
        if (m->save.map_ == "SavePoint")
            sd->status.last_point = sd->status.save_point;
        else
            sd->status.last_point = m->save;
    }
}

/*==========================================
 * 接続時の初期化
 *------------------------------------------
 */
int pc_setnewpc(dumb_ptr<map_session_data> sd, int account_id, int char_id,
        int login_id1, tick_t client_tick, SEX sex)
{
    nullpo_ret(sd);

    sd->bl_id = account_id;
    sd->char_id = char_id;
    sd->login_id1 = login_id1;
    sd->login_id2 = 0;          // at this point, we can not know the value :(
    sd->client_tick = client_tick;
    sd->sex = sex;
    sd->state.auth = 0;
    sd->bl_type = BL::PC;
    sd->canact_tick = sd->canmove_tick = gettick();
    sd->canlog_tick = gettick();
    sd->state.waitingdisconnect = 0;

    return 0;
}

EPOS pc_equippoint(dumb_ptr<map_session_data> sd, int n)
{
    nullpo_retr(EPOS::ZERO, sd);

    if (!sd->inventory_data[n])
        return EPOS::ZERO;

    EPOS ep = sd->inventory_data[n]->equip;

    return ep;
}

static
int pc_setinventorydata(dumb_ptr<map_session_data> sd)
{
    int i, id;

    nullpo_ret(sd);

    for (i = 0; i < MAX_INVENTORY; i++)
    {
        id = sd->status.inventory[i].nameid;
        sd->inventory_data[i] = itemdb_search(id);
    }
    return 0;
}

static
int pc_calcweapontype(dumb_ptr<map_session_data> sd)
{
    nullpo_ret(sd);

    if (sd->weapontype1 != ItemLook::NONE
            && sd->weapontype2 == ItemLook::NONE)
        sd->status.weapon = sd->weapontype1;
    if (sd->weapontype1 == ItemLook::NONE
            && sd->weapontype2 != ItemLook::NONE)
        sd->status.weapon = sd->weapontype2;
    else if (sd->weapontype1 == ItemLook::BLADE
            && sd->weapontype2 == ItemLook::BLADE)
        sd->status.weapon = ItemLook::DUAL_BLADE;
    else if (sd->weapontype1 == ItemLook::_2
            && sd->weapontype2 == ItemLook::_2)
        sd->status.weapon = ItemLook::DUAL_2;
    else if (sd->weapontype1 == ItemLook::_6
            && sd->weapontype2 == ItemLook::_6)
        sd->status.weapon = ItemLook::DUAL_6;
    else if ((sd->weapontype1 == ItemLook::BLADE
                && sd->weapontype2 == ItemLook::_2)
            || (sd->weapontype1 == ItemLook::_2
                && sd->weapontype2 == ItemLook::BLADE))
        sd->status.weapon = ItemLook::DUAL_12;
    else if (
            (sd->weapontype1 == ItemLook::BLADE
                && sd->weapontype2 == ItemLook::_6)
            || (sd->weapontype1 == ItemLook::_6
                && sd->weapontype2 == ItemLook::BLADE))
        sd->status.weapon = ItemLook::DUAL_16;
    else if (
            (sd->weapontype1 == ItemLook::_2
                && sd->weapontype2 == ItemLook::_6)
            || (sd->weapontype1 == ItemLook::_6
                && sd->weapontype2 == ItemLook::_2))
        sd->status.weapon = ItemLook::DUAL_26;
    else
        sd->status.weapon = sd->weapontype1;

    return 0;
}

static
int pc_setequipindex(dumb_ptr<map_session_data> sd)
{
    nullpo_ret(sd);

    for (EQUIP i : EQUIPs)
        sd->equip_index[i] = -1;

    for (int i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid <= 0)
            continue;
        if (bool(sd->status.inventory[i].equip))
        {
            for (EQUIP j : EQUIPs)
                if (bool(sd->status.inventory[i].equip & equip_pos[j]))
                    sd->equip_index[j] = i;
            if (bool(sd->status.inventory[i].equip & EPOS::WEAPON))
            {
                if (sd->inventory_data[i])
                    sd->weapontype1 = sd->inventory_data[i]->look;
                else
                    sd->weapontype1 = ItemLook::NONE;
            }
            if (bool(sd->status.inventory[i].equip & EPOS::SHIELD))
            {
                if (sd->inventory_data[i])
                {
                    if (sd->inventory_data[i]->type == ItemType::WEAPON)
                    {
                        if (sd->status.inventory[i].equip == EPOS::SHIELD)
                            sd->weapontype2 = sd->inventory_data[i]->look;
                        else
                            sd->weapontype2 = ItemLook::NONE;
                    }
                    else
                        sd->weapontype2 = ItemLook::NONE;
                }
                else
                    sd->weapontype2 = ItemLook::NONE;
            }
        }
    }
    pc_calcweapontype(sd);

    return 0;
}

static
int pc_isequip(dumb_ptr<map_session_data> sd, int n)
{
    struct item_data *item;
    eptr<struct status_change, StatusChange> sc_data;
    //転生や養子の場合の元の職業を算出する

    nullpo_ret(sd);

    item = sd->inventory_data[n];
    sc_data = battle_get_sc_data(sd);

    if (battle_config.gm_all_equipment > 0
        && pc_isGM(sd) >= battle_config.gm_all_equipment)
        return 1;

    if (item == NULL)
        return 0;
    if (item->sex != SEX::NEUTRAL && sd->status.sex != item->sex)
        return 0;
    if (item->elv > 0 && sd->status.base_level < item->elv)
        return 0;

    return 1;
}

/*==========================================
 * session idに問題無し
 * char鯖から送られてきたステータスを設定
 *------------------------------------------
 */
int pc_authok(int id, int login_id2, TimeT connect_until_time,
               short tmw_version, const struct mmo_charstatus *st)
{
    dumb_ptr<map_session_data> sd = NULL;

    struct party *p;
    tick_t tick = gettick();

    sd = map_id2sd(id);
    if (sd == NULL)
        return 1;

    sd->login_id2 = login_id2;
    sd->tmw_version = tmw_version;

    sd->status = *st;

    if (sd->status.sex != sd->sex)
    {
        clif_authfail_fd(sd->fd, 0);
        return 1;
    }

    MAP_LOG_STATS(sd, "LOGIN");
    MAP_LOG_XP(sd, "LOGIN");
    MAP_LOG_MAGIC(sd, "LOGIN");

    really_memzero_this(&sd->state);
    // 基本的な初期化
    sd->state.connect_new = 1;
    sd->bl_prev = sd->bl_next = NULL;

    sd->weapontype1 = sd->weapontype2 = ItemLook::NONE;
    sd->speed = DEFAULT_WALK_SPEED;
    sd->state.dead_sit = 0;
    sd->dir = DIR::S;
    sd->head_dir = DIR::S;
    sd->state.auth = 1;
    // sd->walktimer = nullptr;
    // sd->attacktimer = nullptr;
    // sd->invincible_timer = nullptr;

    sd->deal_locked = 0;
    sd->trade_partner = 0;

    sd->inchealhptick = interval_t::zero();
    sd->inchealsptick = interval_t::zero();
    sd->hp_sub = interval_t::zero();
    sd->sp_sub = interval_t::zero();
    sd->quick_regeneration_hp.amount = 0;
    sd->quick_regeneration_sp.amount = 0;
    sd->heal_xp = 0;
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
    sd->cast_tick = tick; // + pc_readglobalreg (sd, "MAGIC_CAST_TICK");

    // アカウント変数の送信要求
    intif_request_accountreg(sd);

    // アイテムチェック
    pc_setinventorydata(sd);
    pc_checkitem(sd);

    // ステータス異常の初期化
    for (StatusChange i : erange(StatusChange(), StatusChange::MAX_STATUSCHANGE))
    {
        // sd->sc_data[i].timer = nullptr;
        sd->sc_data[i].val1 = 0;
    }
    sd->sc_count = 0;
    {
        Option old_option = sd->status.option;
        sd->status.option = Option::ZERO;

        // This would leak information.
        // It's better to make it obvious that players can see you.
        if (false && bool(old_option & Option::INVISIBILITY))
            is_atcommand(sd->fd, sd, "@invisible", 0);

        if (bool(old_option & Option::HIDE))
            is_atcommand(sd->fd, sd, "@hide", 0);
        // atcommand_hide might already send it, but also might not
        clif_changeoption(sd);
    }

    // パーティー関係の初期化
    sd->party_sended = 0;
    sd->party_invite = 0;
    sd->party_x = -1;
    sd->party_y = -1;
    sd->party_hp = -1;

    // イベント関係の初期化
    sd->eventqueuel.clear();

    // 位置の設定
    pc_setpos(sd, sd->status.last_point.map_, sd->status.last_point.x,
               sd->status.last_point.y, BeingRemoveWhy::GONE);

    // パーティ、ギルドデータの要求
    if (sd->status.party_id > 0
        && (p = party_search(sd->status.party_id)) == NULL)
        party_request_info(sd->status.party_id);

    // pvpの設定
    sd->pvp_rank = 0;
    sd->pvp_point = 0;
    // sd->pvp_timer = nullptr;

    // 通知

    clif_authok(sd);
    map_addnickdb(sd);
    if (!map_charid2nick(sd->status.char_id).to__actual())
        map_addchariddb(sd->status.char_id, sd->status.name);

    //スパノビ用死にカウンターのスクリプト変数からの読み出しとsdへのセット
    sd->die_counter = pc_readglobalreg(sd, stringish<VarName>("PC_DIE_COUNTER"));

    // ステータス初期計算など
    pc_calcstatus(sd, 1);

    if (pc_isGM(sd))
    {
        PRINTF("Connection accepted: character '%s' (account: %d; GM level %d).\n",
             sd->status.name, sd->status.account_id, pc_isGM(sd));
        clif_updatestatus(sd, SP::GM);
    }
    else
        PRINTF("Connection accepted: Character '%s' (account: %d).\n",
                sd->status.name, sd->status.account_id);

    sd->auto_ban_info.in_progress = 0;

    // Initialize antispam vars
    sd->chat_reset_due = TimeT();
    sd->chat_lines_in = sd->chat_total_repeats = 0;
    sd->chat_repeat_reset_due = TimeT();
    sd->chat_lastmsg = FString();

    for (tick_t& t : sd->flood_rates)
        t = tick_t();
    sd->packet_flood_reset_due = TimeT();
    sd->packet_flood_in = 0;

    // message of the limited time of the account
    if (connect_until_time)
    {
        // don't display if it's unlimited or unknow value
        char tmpstr[] = WITH_TIMESTAMP("Your account time limit is: ");
        REPLACE_TIMESTAMP(tmpstr, connect_until_time);

        clif_wis_message(sd->fd, wisp_server_name, const_(tmpstr));
    }
    pc_calcstatus(sd, 1);

    return 0;
}

// TODO fix this to cache and use inotify
// this is far from the only such thing, but most of the others are logs
void pc_show_motd(dumb_ptr<map_session_data> sd)
{
    sd->state.seen_motd = true;
    std::ifstream in(motd_txt.c_str());
    if (in.is_open())
    {
        FString buf;
        while (io::getline(in, buf))
        {
            clif_displaymessage(sd->fd, buf);
        }
    }
}

/*==========================================
 * session idに問題ありなので後始末
 *------------------------------------------
 */
int pc_authfail(int id)
{
    dumb_ptr<map_session_data> sd;

    sd = map_id2sd(id);
    if (sd == NULL)
        return 1;

    clif_authfail_fd(sd->fd, 0);

    return 0;
}

static
int pc_calc_skillpoint(dumb_ptr<map_session_data> sd)
{
    int i, skill_points = 0;

    nullpo_ret(sd);

    for (i = 0; i < skill_pool_skills_size; i++) {
        int lv = sd->status.skill[skill_pool_skills[i]].lv;
        if (lv)
            skill_points += ((lv * (lv - 1)) >> 1) - 1;
    }

    return skill_points;
}

/*==========================================
 * 重量アイコンの確認
 *------------------------------------------
 */
int pc_checkweighticon(dumb_ptr<map_session_data> sd)
{
    int flag = 0;

    nullpo_ret(sd);

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

static
void pc_set_weapon_look(dumb_ptr<map_session_data> sd)
{
    if (sd->attack_spell_override)
        clif_changelook(sd, LOOK::WEAPON,
                sd->attack_spell_look_override);
    else
        clif_changelook(sd, LOOK::WEAPON,
                static_cast<uint16_t>(sd->status.weapon));
}

/*==========================================
 * パラメータ計算
 * first==0の時、計算対象のパラメータが呼び出し前から
 * 変 化した場合自動でsendするが、
 * 能動的に変化させたパラメータは自前でsendするように
 *------------------------------------------
 */
int pc_calcstatus(dumb_ptr<map_session_data> sd, int first)
{
    int b_max_hp, b_max_sp, b_hp, b_sp, b_weight, b_max_weight,
        b_hit, b_flee;
    int b_watk, b_def, b_watk2, b_def2, b_flee2, b_critical,
        b_attackrange, b_matk1, b_matk2, b_mdef, b_mdef2;
    int b_base_atk;
    int bl, index;
    int aspd_rate, refinedef = 0;
    int str, dstr, dex;

    nullpo_ret(sd);

    interval_t b_speed = sd->speed;
    b_max_hp = sd->status.max_hp;
    b_max_sp = sd->status.max_sp;
    b_hp = sd->status.hp;
    b_sp = sd->status.sp;
    b_weight = sd->weight;
    b_max_weight = sd->max_weight;
    earray<int, ATTR, ATTR::COUNT> b_paramb = sd->paramb;
    earray<int, ATTR, ATTR::COUNT> b_parame = sd->paramc;
    earray<skill_value, SkillID, MAX_SKILL> b_skill = sd->status.skill;
    b_hit = sd->hit;
    b_flee = sd->flee;
    interval_t b_aspd = sd->aspd;
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

    sd->max_weight = max_weight_base_0 + sd->status.attrs[ATTR::STR] * 300;

    if (first & 1)
    {
        sd->weight = 0;
        for (int i = 0; i < MAX_INVENTORY; i++)
        {
            if (sd->status.inventory[i].nameid == 0
                || sd->inventory_data[i] == NULL)
                continue;
            sd->weight +=
                sd->inventory_data[i]->weight *
                sd->status.inventory[i].amount;
        }
        sd->cart_max_weight = battle_config.max_cart_weight;
        sd->cart_weight = 0;
        sd->cart_max_num = MAX_CART;
        sd->cart_num = 0;
        for (int i = 0; i < MAX_CART; i++)
        {
            if (sd->status.cart[i].nameid == 0)
                continue;
            sd->cart_weight +=
                itemdb_weight(sd->status.cart[i].nameid) *
                sd->status.cart[i].amount;
            sd->cart_num++;
        }
    }

    for (auto& p : sd->paramb)
        p = 0;
    for (auto& p : sd->parame)
        p = 0;

    sd->hit = 0;
    sd->flee = 0;
    sd->flee2 = 0;
    sd->critical = 0;
    sd->aspd = interval_t::zero();
    sd->watk = 0;
    sd->def = 0;
    sd->mdef = 0;
    sd->watk2 = 0;
    sd->def2 = 0;
    sd->mdef2 = 0;
    sd->status.max_hp = 0;
    sd->status.max_sp = 0;
    sd->attackrange = 0;
    sd->attackrange_ = 0;
    sd->star = 0;
    sd->overrefine = 0;
    sd->matk1 = 0;
    sd->matk2 = 0;
    sd->speed = DEFAULT_WALK_SPEED;
    sd->hprate = 100;
    sd->sprate = 100;
    sd->dsprate = 100;
    sd->base_atk = 0;
    sd->arrow_atk = 0;
    sd->arrow_hit = 0;
    sd->arrow_range = 0;
    sd->nhealhp = sd->nhealsp = sd->nshealhp = sd->nshealsp = sd->nsshealhp =
        sd->nsshealsp = 0;
    really_memzero_this(&sd->special_state);

    sd->watk_ = 0;              //二刀流用(仮)
    sd->watk_2 = 0;
    sd->star_ = 0;
    sd->overrefine_ = 0;

    sd->aspd_rate = 100;
    sd->speed_rate = 100;
    sd->hprecov_rate = 100;
    sd->sprecov_rate = 100;
    sd->critical_def = 0;
    sd->double_rate = 0;
    sd->atk_rate = sd->matk_rate = 100;
    sd->arrow_cri = 0;
    sd->perfect_hit = 0;
    sd->critical_rate = sd->hit_rate = sd->flee_rate = sd->flee2_rate = 100;
    sd->def_rate = sd->def2_rate = sd->mdef_rate = sd->mdef2_rate = 100;
    sd->speed_add_rate = sd->aspd_add_rate = 100;
    sd->double_add_rate = sd->perfect_hit_add = 0;
    sd->hp_drain_rate = sd->hp_drain_per = sd->sp_drain_rate =
        sd->sp_drain_per = 0;
    sd->hp_drain_rate_ = sd->hp_drain_per_ = sd->sp_drain_rate_ =
        sd->sp_drain_per_ = 0;

    sd->spellpower_bonus_target = 0;

    for (EQUIP i : EQUIPs_noarrow)
    {
        index = sd->equip_index[i];
        if (index < 0)
            continue;
        if (i == EQUIP::WEAPON && sd->equip_index[EQUIP::SHIELD] == index)
            continue;
        if (i == EQUIP::TORSO && sd->equip_index[EQUIP::LEGS] == index)
            continue;
        if (i == EQUIP::HAT
            && (sd->equip_index[EQUIP::TORSO] == index
                || sd->equip_index[EQUIP::LEGS] == index))
            continue;

        if (sd->inventory_data[index])
        {
            sd->spellpower_bonus_target +=
                sd->inventory_data[index]->magic_bonus;

            if (sd->inventory_data[index]->type == ItemType::WEAPON)
            {
                if (sd->status.inventory[index].card[0] != 0x00ff
                    && sd->status.inventory[index].card[0] != 0x00fe
                    && sd->status.inventory[index].card[0] != static_cast<short>(0xff00))
                {
                    int j;
                    for (j = 0; j < sd->inventory_data[index]->slot; j++)
                    {           // カード
                        int c = sd->status.inventory[index].card[j];
                        if (c > 0)
                        {
                            argrec_t arg[2] =
                            {
                                {"@slotId", static_cast<int>(i)},
                                {"@itemId", sd->inventory_data[index]->nameid},
                            };
                            if (i == EQUIP::SHIELD
                                && sd->status.inventory[index].equip == EPOS::SHIELD)
                                sd->state.lr_flag = 1;
                            run_script_l(ScriptPointer(itemdb_equipscript(c), 0), sd->bl_id, 0,
                                    2, arg);
                            sd->state.lr_flag = 0;
                        }
                    }
                }
            }
            else if (sd->inventory_data[index]->type == ItemType::ARMOR)
            {                   // 防具
                if (sd->status.inventory[index].card[0] != 0x00ff
                    && sd->status.inventory[index].card[0] != 0x00fe
                    && sd->status.inventory[index].card[0] != static_cast<short>(0xff00))
                {
                    int j;
                    for (j = 0; j < sd->inventory_data[index]->slot; j++)
                    {           // カード
                        int c = sd->status.inventory[index].card[j];
                        if (c > 0) {
                            argrec_t arg[2] =
                            {
                                {"@slotId", static_cast<int>(i)},
                                {"@itemId", sd->inventory_data[index]->nameid}
                            };
                            run_script_l(ScriptPointer(itemdb_equipscript(c), 0), sd->bl_id, 0,
                                    2, arg);
                        }
                    }
                }
            }
        }
    }

#ifdef USE_ASTRAL_SOUL_SKILL
    if (sd->spellpower_bonus_target < 0)
        sd->spellpower_bonus_target =
            (sd->spellpower_bonus_target * 256) /
            (min(128 + skill_power(sd, SkillID::TMW_ASTRAL_SOUL), 256));
#endif

    if (sd->spellpower_bonus_target < sd->spellpower_bonus_current)
        sd->spellpower_bonus_current = sd->spellpower_bonus_target;

    sd->paramcard = sd->parame;

    // 装備品によるステータス変化はここで実行
    for (EQUIP i : EQUIPs_noarrow)
    {
        index = sd->equip_index[i];
        if (index < 0)
            continue;
        if (i == EQUIP::WEAPON && sd->equip_index[EQUIP::SHIELD] == index)
            continue;
        if (i == EQUIP::TORSO && sd->equip_index[EQUIP::LEGS] == index)
            continue;
        if (i == EQUIP::HAT
            && (sd->equip_index[EQUIP::TORSO] == index
                || sd->equip_index[EQUIP::LEGS] == index))
            continue;
        if (sd->inventory_data[index])
        {
            sd->def += sd->inventory_data[index]->def;
            if (sd->inventory_data[index]->type == ItemType::WEAPON)
            {
                int r;
                if (i == EQUIP::SHIELD
                    && sd->status.inventory[index].equip == EPOS::SHIELD)
                {
                    //二刀流用データ入力
                    sd->watk_ += sd->inventory_data[index]->atk;
                    sd->watk_2 = (r = sd->status.inventory[index].refine) * // 精錬攻撃力
                        0;
                    if ((r -= 10) > 0) // 過剰精錬ボーナス
                        sd->overrefine_ = r * 0;

                    if (sd->status.inventory[index].card[0] == 0x00ff)
                    {           // 製造武器
                        sd->star_ = (sd->status.inventory[index].card[1] >> 8); // 星のかけら
                    }
                    sd->attackrange_ += sd->inventory_data[index]->range;
                    sd->state.lr_flag = 1;
                    {
                        argrec_t arg[2] =
                        {
                            {"@slotId", static_cast<int>(i)},
                            {"@itemId", sd->inventory_data[index]->nameid},
                        };
                        run_script_l(ScriptPointer(sd->inventory_data[index]->equip_script.get(), 0),
                                sd->bl_id, 0,
                                2, arg);
                    }
                    sd->state.lr_flag = 0;
                }
                else
                {
                    //二刀流武器以外
                    argrec_t arg[2] =
                    {
                        {"@slotId", static_cast<int>(i)},
                        {"@itemId", sd->inventory_data[index]->nameid},
                    };
                    sd->watk += sd->inventory_data[index]->atk;
                    sd->watk2 += (r = sd->status.inventory[index].refine) * // 精錬攻撃力
                        0;
                    if ((r -= 10) > 0) // 過剰精錬ボーナス
                        sd->overrefine += r * 0;

                    if (sd->status.inventory[index].card[0] == 0x00ff)
                    {           // 製造武器
                        sd->star += (sd->status.inventory[index].card[1] >> 8); // 星のかけら
                    }
                    sd->attackrange += sd->inventory_data[index]->range;
                    run_script_l(ScriptPointer(sd->inventory_data[index]->equip_script.get(), 0),
                            sd->bl_id, 0,
                            2, arg);
                }
            }
            else if (sd->inventory_data[index]->type == ItemType::ARMOR)
            {
                argrec_t arg[2] =
                {
                    {"@slotId", static_cast<int>(i)},
                    {"@itemId", sd->inventory_data[index]->nameid},
                };
                sd->watk += sd->inventory_data[index]->atk;
                refinedef +=
                    sd->status.inventory[index].refine * 0;
                run_script_l(ScriptPointer(sd->inventory_data[index]->equip_script.get(), 0),
                        sd->bl_id, 0,
                        2, arg);
            }
        }
    }

    if (battle_is_unarmed(sd))
    {
        sd->watk += skill_power(sd, SkillID::TMW_BRAWLING) / 3; // +66 for 200
        sd->watk2 += skill_power(sd, SkillID::TMW_BRAWLING) >> 3;   // +25 for 200
        sd->watk_ += skill_power(sd, SkillID::TMW_BRAWLING) / 3;    // +66 for 200
        sd->watk_2 += skill_power(sd, SkillID::TMW_BRAWLING) >> 3;  // +25 for 200
    }

    if (sd->equip_index[EQUIP::ARROW] >= 0)
    {                           // 矢
        index = sd->equip_index[EQUIP::ARROW];
        if (sd->inventory_data[index])
        {                       //まだ属性が入っていない
            argrec_t arg[2] =
            {
                {"@slotId", static_cast<int>(EQUIP::ARROW)},
                {"@itemId", sd->inventory_data[index]->nameid},
            };
            sd->state.lr_flag = 2;
            run_script_l(ScriptPointer(sd->inventory_data[index]->equip_script.get(), 0),
                    sd->bl_id, 0,
                    2, arg);
            sd->state.lr_flag = 0;
            sd->arrow_atk += sd->inventory_data[index]->atk;
        }
    }
    sd->def += (refinedef + 50) / 100;

    if (sd->attackrange < 1)
        sd->attackrange = 1;
    if (sd->attackrange_ < 1)
        sd->attackrange_ = 1;
    if (sd->attackrange < sd->attackrange_)
        sd->attackrange = sd->attackrange_;
    if (sd->status.weapon == ItemLook::BOW)
        sd->attackrange += sd->arrow_range;
    sd->double_rate += sd->double_add_rate;
    sd->perfect_hit += sd->perfect_hit_add;
    if (sd->speed_add_rate != 100)
        sd->speed_rate += sd->speed_add_rate - 100;
    if (sd->aspd_add_rate != 100)
        sd->aspd_rate += sd->aspd_add_rate - 100;

    sd->speed -= std::chrono::milliseconds(skill_power(sd, SkillID::TMW_SPEED) / 8);
    sd->aspd_rate -= skill_power(sd, SkillID::TMW_SPEED) / 10;
    if (sd->aspd_rate < 20)
        sd->aspd_rate = 20;

    for (ATTR attr : ATTRs)
        sd->paramc[attr] = max(0, sd->status.attrs[attr] + sd->paramb[attr] + sd->parame[attr]);

    if (sd->status.weapon == ItemLook::BOW
            || sd->status.weapon == ItemLook::_13
            || sd->status.weapon == ItemLook::_14)
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
//FPRINTF(stderr, "baseatk = %d = x + %d + %d + %d + %d\n", sd->base_atk, str, dstr*dstr, dex/5, sd->paramc[ATTR::LUK]/5);
    sd->matk1 += sd->paramc[ATTR::INT] + (sd->paramc[ATTR::INT] / 5) * (sd->paramc[ATTR::INT] / 5);
    sd->matk2 += sd->paramc[ATTR::INT] + (sd->paramc[ATTR::INT] / 7) * (sd->paramc[ATTR::INT] / 7);
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

    // 二刀流 ASPD 修正
    if (sd->status.weapon < ItemLook::SINGLE_HANDED_COUNT)
        sd->aspd += aspd_base_0[sd->status.weapon]
            - (sd->paramc[ATTR::AGI] * 4 + sd->paramc[ATTR::DEX])
            * aspd_base_0[sd->status.weapon] / 1000;
    else
        sd->aspd += (
                (aspd_base_0[sd->weapontype1]
                    - (sd->paramc[ATTR::AGI] * 4 + sd->paramc[ATTR::DEX])
                    * aspd_base_0[sd->weapontype1] / 1000)
                + (aspd_base_0[sd->weapontype2]
                    - (sd->paramc[ATTR::AGI] * 4 + sd->paramc[ATTR::DEX])
                    * aspd_base_0[sd->weapontype2] / 1000)
                )
            * 140 / 200;

    aspd_rate = sd->aspd_rate;

    //攻撃速度増加

    if (sd->attackrange > 2)
    {
        // [fate] ranged weapon?
        sd->attackrange += min(skill_power(sd, SkillID::AC_OWL) / 60, 3);
        sd->hit += skill_power(sd, SkillID::AC_OWL) / 10;   // 20 for 200
    }

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

    // 最大SP計算
    sd->status.max_sp += ((sp_coefficient_0 * bl) + 1000)
        / 100 * (100 + sd->paramc[ATTR::INT]) / 100
        + (sd->parame[ATTR::INT] - sd->paramcard[ATTR::INT]);
    if (sd->sprate != 100)
        sd->status.max_sp = sd->status.max_sp * sd->sprate / 100;

    if (sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
        sd->status.max_sp = battle_config.max_sp;

    //自然回復HP
    sd->nhealhp = 1 + (sd->paramc[ATTR::VIT] / 5) + (sd->status.max_hp / 200);
    //自然回復SP
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

    // スキルやステータス異常による残りのパラメータ補正
    if (sd->sc_count)
    {
        // ATK/DEF変化形
        if (sd->sc_data[StatusChange::SC_POISON].timer) // 毒状態
            sd->def2 = sd->def2 * 75 / 100;

        if (sd->sc_data[StatusChange::SC_ATKPOT].timer)
            sd->watk += sd->sc_data[StatusChange::SC_ATKPOT].val1;
        if (sd->sc_data[StatusChange::SC_MATKPOT].timer)
        {
            sd->matk1 += sd->sc_data[StatusChange::SC_MATKPOT].val1;
            sd->matk2 += sd->sc_data[StatusChange::SC_MATKPOT].val1;
        }

        if (sd->sc_data[StatusChange::SC_SPEEDPOTION0].timer)
            aspd_rate -= sd->sc_data[StatusChange::SC_SPEEDPOTION0].val1;

        if (sd->sc_data[StatusChange::SC_HASTE].timer)
            aspd_rate -= sd->sc_data[StatusChange::SC_HASTE].val1;

        /* Slow down if protected */

        if (sd->sc_data[StatusChange::SC_PHYS_SHIELD].timer)
            aspd_rate += sd->sc_data[StatusChange::SC_PHYS_SHIELD].val1;
    }

    if (sd->speed_rate != 100)
        sd->speed = sd->speed * sd->speed_rate / 100;
    sd->speed = std::max(sd->speed, std::chrono::milliseconds(1));
    if (aspd_rate != 100)
        sd->aspd = sd->aspd * aspd_rate / 100;

    if (sd->attack_spell_override)
        sd->aspd = sd->attack_spell_delay;

    sd->aspd = std::max(sd->aspd, static_cast<interval_t>(battle_config.max_aspd));
    sd->amotion = sd->aspd;
    sd->dmotion = std::chrono::milliseconds(800 - sd->paramc[ATTR::AGI] * 4);
    sd->dmotion = std::max(sd->dmotion, std::chrono::milliseconds(400));

    if (sd->status.hp > sd->status.max_hp)
        sd->status.hp = sd->status.max_hp;
    if (sd->status.sp > sd->status.max_sp)
        sd->status.sp = sd->status.max_sp;

    if (first & 4)
        return 0;
    if (first & 3)
    {
        clif_updatestatus(sd, SP::SPEED);
        clif_updatestatus(sd, SP::MAXHP);
        clif_updatestatus(sd, SP::MAXSP);
        if (first & 1)
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

    return 0;
}

/*==========================================
 * 装 備品による能力等のボーナス設定
 *------------------------------------------
 */
int pc_bonus(dumb_ptr<map_session_data> sd, SP type, int val)
{
    nullpo_ret(sd);

    switch (type)
    {
        case SP::STR:
        case SP::AGI:
        case SP::VIT:
        case SP::INT:
        case SP::DEX:
        case SP::LUK:
            if (sd->state.lr_flag != 2)
                sd->parame[sp_to_attr(type)] += val;
            break;
#if 0
        case SP::ATK1:
            if (!sd->state.lr_flag)
                sd->watk += val;
            else if (sd->state.lr_flag == 1)
                sd->watk_ += val;
            break;
#endif
#if 0
        case SP::ATK2:
            if (!sd->state.lr_flag)
                sd->watk2 += val;
            else if (sd->state.lr_flag == 1)
                sd->watk_2 += val;
            break;
#endif
#if 0
        case SP::BASE_ATK:
            if (sd->state.lr_flag != 2)
                sd->base_atk += val;
            break;
#endif
#if 0
        case SP::MATK1:
            if (sd->state.lr_flag != 2)
                sd->matk1 += val;
            break;
#endif
#if 0
        case SP::MATK2:
            if (sd->state.lr_flag != 2)
                sd->matk2 += val;
            break;
#endif
#if 0
        case SP::DEF1:
            if (sd->state.lr_flag != 2)
                sd->def += val;
            break;
#endif
        case SP::MDEF1:
            if (sd->state.lr_flag != 2)
                sd->mdef += val;
            break;
#if 0
        case SP::MDEF2:
            if (sd->state.lr_flag != 2)
                sd->mdef += val;
            break;
#endif
        case SP::HIT:
            if (sd->state.lr_flag != 2)
                sd->hit += val;
            else
                sd->arrow_hit += val;
            break;
        case SP::FLEE1:
            if (sd->state.lr_flag != 2)
                sd->flee += val;
            break;
#if 0
        case SP::FLEE2:
            if (sd->state.lr_flag != 2)
                sd->flee2 += val * 10;
            break;
#endif
        case SP::CRITICAL:
            if (sd->state.lr_flag != 2)
                sd->critical += val * 10;
            else
                sd->arrow_cri += val * 10;
            break;
        case SP::MAXHP:
            if (sd->state.lr_flag != 2)
                sd->status.max_hp += val;
            break;
        case SP::MAXSP:
            if (sd->state.lr_flag != 2)
                sd->status.max_sp += val;
            break;
        case SP::MAXHPRATE:
            if (sd->state.lr_flag != 2)
                sd->hprate += val;
            break;
#if 0
        case SP::MAXSPRATE:
            if (sd->state.lr_flag != 2)
                sd->sprate += val;
            break;
#endif
#if 0
        case SP::SPRATE:
            if (sd->state.lr_flag != 2)
                sd->dsprate += val;
            break;
#endif
        case SP::ATTACKRANGE:
            if (!sd->state.lr_flag)
                sd->attackrange += val;
            else if (sd->state.lr_flag == 1)
                sd->attackrange_ += val;
            else if (sd->state.lr_flag == 2)
                sd->arrow_range += val;
            break;
#if 0
        case SP::ADD_SPEED:
            if (sd->state.lr_flag != 2)
                sd->speed -= val;
            break;
#endif
#if 0
        case SP::SPEED_RATE:
            if (sd->state.lr_flag != 2)
            {
                if (sd->speed_rate > 100 - val)
                    sd->speed_rate = 100 - val;
            }
            break;
#endif
        case SP::SPEED_ADDRATE:
            if (sd->state.lr_flag != 2)
                sd->speed_add_rate = sd->speed_add_rate * (100 - val) / 100;
            break;
#if 0
        case SP::ASPD:
            if (sd->state.lr_flag != 2)
                sd->aspd -= val * 10;
            break;
#endif
        case SP::ASPD_RATE:
            if (sd->state.lr_flag != 2)
            {
                if (sd->aspd_rate > 100 - val)
                    sd->aspd_rate = 100 - val;
            }
            break;
#if 0
        case SP::ASPD_ADDRATE:
            if (sd->state.lr_flag != 2)
                sd->aspd_add_rate = sd->aspd_add_rate * (100 - val) / 100;
            break;
#endif
        case SP::HP_RECOV_RATE:
            if (sd->state.lr_flag != 2)
                sd->hprecov_rate += val;
            break;
#if 0
        case SP::SP_RECOV_RATE:
            if (sd->state.lr_flag != 2)
                sd->sprecov_rate += val;
            break;
#endif
        case SP::CRITICAL_DEF:
            if (sd->state.lr_flag != 2)
                sd->critical_def += val;
            break;
#if 0
        case SP::DOUBLE_RATE:
            if (sd->state.lr_flag == 0 && sd->double_rate < val)
                sd->double_rate = val;
            break;
#endif
        case SP::DOUBLE_ADD_RATE:
            if (sd->state.lr_flag == 0)
                sd->double_add_rate += val;
            break;
#if 0
        case SP::MATK_RATE:
            if (sd->state.lr_flag != 2)
                sd->matk_rate += val;
            break;
#endif
#if 0
        case SP::ATK_RATE:
            if (sd->state.lr_flag != 2)
                sd->atk_rate += val;
            break;
#endif
#if 0
        case SP::PERFECT_HIT_RATE:
            if (sd->state.lr_flag != 2 && sd->perfect_hit < val)
                sd->perfect_hit = val;
            break;
#endif
#if 0
        case SP::PERFECT_HIT_ADD_RATE:
            if (sd->state.lr_flag != 2)
                sd->perfect_hit_add += val;
            break;
#endif
#if 0
        case SP::CRITICAL_RATE:
            if (sd->state.lr_flag != 2)
                sd->critical_rate += val;
            break;
#endif
#if 0
        case SP::HIT_RATE:
            if (sd->state.lr_flag != 2)
                sd->hit_rate += val;
            break;
#endif
#if 0
        case SP::FLEE_RATE:
            if (sd->state.lr_flag != 2)
                sd->flee_rate += val;
            break;
#endif
#if 0
        case SP::FLEE2_RATE:
            if (sd->state.lr_flag != 2)
                sd->flee2_rate += val;
            break;
#endif
        case SP::DEF_RATE:
            if (sd->state.lr_flag != 2)
                sd->def_rate += val;
            break;
        case SP::DEF2_RATE:
            if (sd->state.lr_flag != 2)
                sd->def2_rate += val;
            break;
#if 0
        case SP::MDEF_RATE:
            if (sd->state.lr_flag != 2)
                sd->mdef_rate += val;
            break;
#endif
#if 0
        case SP::MDEF2_RATE:
            if (sd->state.lr_flag != 2)
                sd->mdef2_rate += val;
            break;
#endif
        case SP::DEAF:
            sd->special_state.deaf = 1;
            break;
        default:
            if (battle_config.error_log)
                PRINTF("pc_bonus: unknown type %d %d !\n",
                        type, val);
            break;
    }
    return 0;
}

/*==========================================
 * ｿｽｿｽ ｿｽｿｽｿｽiｿｽﾉゑｿｽｿｽｿｽｿｽ\ｿｽﾍ難ｿｽｿｽﾌボｿｽ[ｿｽiｿｽXｿｽﾝ抵ｿｽ
 *------------------------------------------
 */
int pc_bonus2(dumb_ptr<map_session_data> sd, SP type, int type2, int val)
{
    nullpo_ret(sd);

    switch (type)
    {
        case SP::HP_DRAIN_RATE:
            if (!sd->state.lr_flag)
            {
                sd->hp_drain_rate += type2;
                sd->hp_drain_per += val;
            }
            else if (sd->state.lr_flag == 1)
            {
                sd->hp_drain_rate_ += type2;
                sd->hp_drain_per_ += val;
            }
            break;
#if 0
        case SP::SP_DRAIN_RATE:
            if (!sd->state.lr_flag)
            {
                sd->sp_drain_rate += type2;
                sd->sp_drain_per += val;
            }
            else if (sd->state.lr_flag == 1)
            {
                sd->sp_drain_rate_ += type2;
                sd->sp_drain_per_ += val;
            }
            break;
#endif
        default:
            if (battle_config.error_log)
                PRINTF("pc_bonus2: unknown type %d %d %d!\n",
                        type, type2, val);
            break;
    }
    return 0;
}

/*==========================================
 * スクリプトによるスキル所得
 *------------------------------------------
 */
int pc_skill(dumb_ptr<map_session_data> sd, SkillID id, int level, int flag)
{
    nullpo_ret(sd);

    if (level > MAX_SKILL_LEVEL)
    {
        if (battle_config.error_log)
            PRINTF("support card skill only!\n");
        return 0;
    }
    if (!flag && (sd->status.skill[id].lv || level == 0))
    {
        sd->status.skill[id].lv = level;
        pc_calcstatus(sd, 0);
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
 *------------------------------------------
 */
ADDITEM pc_checkadditem(dumb_ptr<map_session_data> sd, int nameid, int amount)
{
    int i;

    nullpo_retr(ADDITEM::ZERO, sd);

    if (itemdb_isequip(nameid))
        return ADDITEM::NEW;

    for (i = 0; i < MAX_INVENTORY; i++)
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
 *------------------------------------------
 */
int pc_inventoryblank(dumb_ptr<map_session_data> sd)
{
    int i, b;

    nullpo_ret(sd);

    for (i = 0, b = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid == 0)
            b++;
    }

    return b;
}

/*==========================================
 * お金を払う
 *------------------------------------------
 */
int pc_payzeny(dumb_ptr<map_session_data> sd, int zeny)
{
    nullpo_ret(sd);

    double z = sd->status.zeny;
    if (sd->status.zeny < zeny || z - zeny > MAX_ZENY)
        return 1;
    sd->status.zeny -= zeny;
    clif_updatestatus(sd, SP::ZENY);

    return 0;
}

/*==========================================
 * お金を得る
 *------------------------------------------
 */
int pc_getzeny(dumb_ptr<map_session_data> sd, int zeny)
{
    nullpo_ret(sd);

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
 *------------------------------------------
 */
int pc_search_inventory(dumb_ptr<map_session_data> sd, int item_id)
{
    int i;

    nullpo_retr(-1, sd);

    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid == item_id &&
            (sd->status.inventory[i].amount > 0 || item_id == 0))
            return i;
    }

    return -1;
}

int pc_count_all_items(dumb_ptr<map_session_data> player, int item_id)
{
    int i;
    int count = 0;

    nullpo_ret(player);

    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (player->status.inventory[i].nameid == item_id)
            count += player->status.inventory[i].amount;
    }

    return count;
}

int pc_remove_items(dumb_ptr<map_session_data> player, int item_id, int count)
{
    int i;

    nullpo_ret(player);

    for (i = 0; i < MAX_INVENTORY && count; i++)
    {
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
 *------------------------------------------
 */
PickupFail pc_additem(dumb_ptr<map_session_data> sd, struct item *item_data,
                int amount)
{
    struct item_data *data;
    int i, w;

    MAP_LOG_PC(sd, "PICKUP %d %d", item_data->nameid, amount);

    nullpo_retr(PickupFail::BAD_ITEM, sd);
    nullpo_retr(PickupFail::BAD_ITEM, item_data);

    if (item_data->nameid <= 0 || amount <= 0)
        return PickupFail::BAD_ITEM;
    data = itemdb_search(item_data->nameid);
    if ((w = data->weight * amount) + sd->weight > sd->max_weight)
        return PickupFail::TOO_HEAVY;

    i = MAX_INVENTORY;

    if (!itemdb_isequip2(data))
    {
        // 装 備品ではないので、既所有品なら個数のみ変化させる
        for (i = 0; i < MAX_INVENTORY; i++)
            if (sd->status.inventory[i].nameid == item_data->nameid &&
                sd->status.inventory[i].card[0] == item_data->card[0]
                && sd->status.inventory[i].card[1] == item_data->card[1]
                && sd->status.inventory[i].card[2] == item_data->card[2]
                && sd->status.inventory[i].card[3] == item_data->card[3])
            {
                if (sd->status.inventory[i].amount + amount > MAX_AMOUNT)
                    return PickupFail::STACK_FULL;
                sd->status.inventory[i].amount += amount;
                clif_additem(sd, i, amount, PickupFail::OKAY);
                break;
            }
    }
    if (i >= MAX_INVENTORY)
    {
        // 装 備品か未所有品だったので空き欄へ追加
        i = pc_search_inventory(sd, 0);
        if (i >= 0)
        {
            sd->status.inventory[i] = *item_data;

            if (bool(item_data->equip))
                sd->status.inventory[i].equip = EPOS::ZERO;

            sd->status.inventory[i].amount = amount;
            sd->inventory_data[i] = data;
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
 *------------------------------------------
 */
int pc_delitem(dumb_ptr<map_session_data> sd, int n, int amount, int type)
{
    nullpo_retr(1, sd);

    if (sd->trade_partner != 0)
        trade_tradecancel(sd);

    if (sd->status.inventory[n].nameid == 0 || amount <= 0
        || sd->status.inventory[n].amount < amount
        || sd->inventory_data[n] == NULL)
        return 1;

    sd->status.inventory[n].amount -= amount;
    sd->weight -= sd->inventory_data[n]->weight * amount;
    if (sd->status.inventory[n].amount <= 0)
    {
        if (bool(sd->status.inventory[n].equip))
            pc_unequipitem(sd, n, CalcStatus::NOW);
        sd->status.inventory[n] = item{};
        sd->inventory_data[n] = NULL;
    }
    if (!(type & 1))
        clif_delitem(sd, n, amount);
    if (!(type & 2))
        clif_updatestatus(sd, SP::WEIGHT);

    return 0;
}

/*==========================================
 * アイテムを落す
 *------------------------------------------
 */
int pc_dropitem(dumb_ptr<map_session_data> sd, int n, int amount)
{
    nullpo_retr(1, sd);

    if (sd->trade_partner != 0 || sd->npc_id != 0 || sd->state.storage_open)
        return 0;               // no dropping while trading/npc/storage

    if (n < 0 || n >= MAX_INVENTORY)
        return 0;

    if (amount <= 0)
        return 0;

    pc_unequipinvyitem(sd, n, CalcStatus::NOW);

    if (sd->status.inventory[n].nameid <= 0 ||
        sd->status.inventory[n].amount < amount ||
        sd->trade_partner != 0 || sd->status.inventory[n].amount <= 0)
        return 1;
    map_addflooritem(&sd->status.inventory[n], amount,
            sd->bl_m, sd->bl_x, sd->bl_y,
            NULL, NULL, NULL);
    pc_delitem(sd, n, amount, 0);

    return 0;
}

/*==========================================
 * アイテムを拾う
 *------------------------------------------
 */

static
int can_pick_item_up_from(dumb_ptr<map_session_data> self, int other_id)
{
    struct party *p = party_search(self->status.party_id);

    /* From ourselves or from no-one? */
    if (!self || self->bl_id == other_id || !other_id)
        return 1;

    dumb_ptr<map_session_data> other = map_id2sd(other_id);

    /* Other no longer exists? */
    if (!other)
        return 1;

    /* From our partner? */
    if (self->status.partner_id == other->status.char_id)
        return 1;

    /* From a party member? */
    if (self->status.party_id
        && self->status.party_id == other->status.party_id
        && p && p->item != 0)
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

int pc_takeitem(dumb_ptr<map_session_data> sd, dumb_ptr<flooritem_data> fitem)
{
    tick_t tick = gettick();
    int can_take;

    nullpo_ret(sd);
    nullpo_ret(fitem);

    /* Sometimes the owners reported to us are buggy: */

    if (fitem->first_get_id == fitem->third_get_id
        || fitem->second_get_id == fitem->third_get_id)
        fitem->third_get_id = 0;

    if (fitem->first_get_id == fitem->second_get_id)
    {
        fitem->second_get_id = fitem->third_get_id;
        fitem->third_get_id = 0;
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
            // 重量overで取得失敗
            clif_additem(sd, 0, 0, flag);
        else
        {
            // 取得成功
            if (sd->attacktimer)
                pc_stopattack(sd);
            clif_takeitem(sd, fitem);
            map_clearflooritem(fitem->bl_id);
        }
        return 0;
    }

    /* Otherwise, we can't pick up */
    clif_additem(sd, 0, 0, PickupFail::DROP_STEAL);
    return 0;
}

static
int pc_isUseitem(dumb_ptr<map_session_data> sd, int n)
{
    struct item_data *item;
    int nameid;

    nullpo_ret(sd);

    item = sd->inventory_data[n];
    nameid = sd->status.inventory[n].nameid;

    if (item == NULL)
        return 0;
    if (itemdb_type(nameid) != ItemType::USE)
        return 0;

    if (item->sex != SEX::NEUTRAL && sd->status.sex != item->sex)
        return 0;
    if (item->elv > 0 && sd->status.base_level < item->elv)
        return 0;

    return 1;
}

/*==========================================
 * アイテムを使う
 *------------------------------------------
 */
int pc_useitem(dumb_ptr<map_session_data> sd, int n)
{
    int amount;

    nullpo_retr(1, sd);

    if (n >= 0 && n < MAX_INVENTORY && sd->inventory_data[n])
    {
        amount = sd->status.inventory[n].amount;
        if (sd->status.inventory[n].nameid <= 0
            || sd->status.inventory[n].amount <= 0
            || !pc_isUseitem(sd, n))
        {
            clif_useitemack(sd, n, 0, 0);
            return 1;
        }

        const ScriptBuffer *script = sd->inventory_data[n]->use_script.get();
        clif_useitemack(sd, n, amount - 1, 1);
        pc_delitem(sd, n, 1, 1);

        run_script(ScriptPointer(script, 0), sd->bl_id, 0);
    }

    return 0;
}

//
//
//
/*==========================================
 * PCの位置設定
 *------------------------------------------
 */
int pc_setpos(dumb_ptr<map_session_data> sd,
        MapName mapname_org, int x, int y, BeingRemoveWhy clrtype)
{
    MapName mapname_;

    nullpo_ret(sd);

    if (sd->trade_partner)      // 取引を中断する
        trade_tradecancel(sd);
    if (sd->state.storage_open)
        storage_storage_quit(sd);  // 倉庫を開いてるなら保存する

    if (sd->party_invite > 0)   // パーティ勧誘を拒否する
        party_reply_invite(sd, sd->party_invite_account, 0);

    skill_castcancel(sd, 0);  // 詠唱中断
    pc_stop_walking(sd, 0);    // 歩行中断
    pc_stopattack(sd);         // 攻撃中断

    if (pc_issit(sd))
    {
//        pc_setstand (sd); // [fate] Nothing wrong with warping while sitting
    }

    mapname_ = mapname_org;

    map_local *m = map_mapname2mapid(mapname_);
    if (!m)
    {
        if (sd->mapname_)
        {
            IP4Address ip;
            int port;
            if (map_mapname2ipport(mapname_, &ip, &port) == 0)
            {
                skill_stop_dancing(sd, 1);
                clif_clearchar(sd, clrtype);
                map_delblock(sd);
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

    if (x < 0 || x >= m->xs || y < 0 || y >= m->ys)
        x = y = 0;
    if ((x == 0 && y == 0)
        || bool(read_gatp(m, x, y) & MapCell::UNWALKABLE))
    {
        if (x || y)
        {
            if (battle_config.error_log)
                PRINTF("stacked (%d,%d)\n", x, y);
        }
        do
        {
            x = random_::in(1, m->xs - 2);
            y = random_::in(1, m->ys - 2);
        }
        while (bool(read_gatp(m, x, y) & MapCell::UNWALKABLE));
    }

    if (sd->mapname_ && sd->bl_prev != NULL)
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

//  map_addblock(sd);  // ブロック登録とspawnは
//  clif_spawnpc(sd);

    return 0;
}

/*==========================================
 * PCのランダムワープ
 *------------------------------------------
 */
int pc_randomwarp(dumb_ptr<map_session_data> sd, BeingRemoveWhy type)
{
    int x, y, i = 0;

    nullpo_ret(sd);

    map_local *m = sd->bl_m;

    if (sd->bl_m->flag.noteleport)  // テレポート禁止
        return 0;

    do
    {
        x = random_::in(1, m->xs - 2);
        y = random_::in(1, m->ys - 2);
    }
    while (bool(read_gatp(m, x, y) & MapCell::UNWALKABLE)
        && (i++) < 1000);

    if (i < 1000)
        pc_setpos(sd, m->name_, x, y, type);

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

    nullpo_ret(sd);

    if (sd->bl_x == x && sd->bl_y == y) // 同じマス
        return 1;

    // 障害物判定
    wpd.path_len = 0;
    wpd.path_pos = 0;
    wpd.path_half = 0;
    return (path_search(&wpd, sd->bl_m, sd->bl_x, sd->bl_y, x, y, 0) != -1);
}

//
// 歩 行物
//
/*==========================================
 * 次の1歩にかかる時間を計算
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
 *------------------------------------------
 */
static
void pc_walk(TimerData *, tick_t tick, int id, unsigned char data)
{
    dumb_ptr<map_session_data> sd;
    int moveblock;
    int x, y, dx, dy;

    sd = map_id2sd(id);
    if (sd == NULL)
        return;

    if (sd->walkpath.path_pos >= sd->walkpath.path_len
        || sd->walkpath.path_pos != data)
        return;

    sd->walkpath.path_half ^= 1;
    if (sd->walkpath.path_half == 0)
    {                           // マス目中心へ到着
        sd->walkpath.path_pos++;
        if (sd->state.change_walk_target)
        {
            pc_walktoxy_sub(sd);
            return;
        }
    }
    else
    {                           // マス目境界へ到着
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

        if (sd->status.party_id > 0)
        {                       // パーティのＨＰ情報通知検査
            struct party *p = party_search(sd->status.party_id);
            if (p != NULL)
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

        if (bool(map_getcell(sd->bl_m, x, y) & MapCell::NPC_NEAR))
            npc_touch_areanpc(sd, sd->bl_m, x, y);
        else
            sd->areanpc_id = 0;
    }
    interval_t i = calc_next_walk_step(sd);
    if (i > interval_t::zero())
    {
        i = i / 2;
        if (sd->walkpath.path_half == 0)
            i = std::max(i, std::chrono::milliseconds(1));

        sd->walktimer = Timer(tick + i,
                std::bind(pc_walk, ph::_1, ph::_2,
                    id, sd->walkpath.path_pos));
    }
}

/*==========================================
 * 移動可能か確認して、可能なら歩行開始
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

    return 0;
}

/*==========================================
 * pc歩 行要求
 *------------------------------------------
 */
int pc_walktoxy(dumb_ptr<map_session_data> sd, int x, int y)
{

    nullpo_ret(sd);

    sd->to_x = x;
    sd->to_y = y;

    if (pc_issit(sd))
        pc_setstand(sd);

    if (sd->walktimer && sd->state.change_walk_target == 0)
    {
        // 現在歩いている最中の目的地変更なのでマス目の中心に来た時に
        // timer関数からpc_walktoxy_subを呼ぶようにする
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
 *------------------------------------------
 */
int pc_stop_walking(dumb_ptr<map_session_data> sd, int type)
{
    nullpo_ret(sd);

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

void pc_touch_all_relevant_npcs(dumb_ptr<map_session_data> sd)
{
    if (bool(map_getcell(sd->bl_m, sd->bl_x, sd->bl_y) & MapCell::NPC_NEAR))
        npc_touch_areanpc(sd, sd->bl_m, sd->bl_x, sd->bl_y);
    else
        sd->areanpc_id = 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_movepos(dumb_ptr<map_session_data> sd, int dst_x, int dst_y)
{
    int moveblock;
    int dx, dy;

    struct walkpath_data wpd;

    nullpo_ret(sd);

    if (path_search(&wpd, sd->bl_m, sd->bl_x, sd->bl_y, dst_x, dst_y, 0))
        return 1;

    sd->dir = sd->head_dir = map_calc_dir(sd, dst_x, dst_y);

    dx = dst_x - sd->bl_x;
    dy = dst_y - sd->bl_y;

    moveblock = (sd->bl_x / BLOCK_SIZE != dst_x / BLOCK_SIZE
                 || sd->bl_y / BLOCK_SIZE != dst_y / BLOCK_SIZE);

    map_foreachinmovearea(std::bind(clif_pcoutsight, ph::_1, sd),
            sd->bl_m,
            sd->bl_x - AREA_SIZE, sd->bl_y - AREA_SIZE,
            sd->bl_x + AREA_SIZE, sd->bl_y + AREA_SIZE,
            dx, dy,
            BL::NUL);

    if (moveblock)
        map_delblock(sd);
    sd->bl_x = dst_x;
    sd->bl_y = dst_y;
    if (moveblock)
        map_addblock(sd);

    map_foreachinmovearea(std::bind(clif_pcinsight, ph::_1, sd),
            sd->bl_m,
            sd->bl_x - AREA_SIZE, sd->bl_y - AREA_SIZE,
            sd->bl_x + AREA_SIZE, sd->bl_y + AREA_SIZE,
            -dx, -dy,
            BL::NUL);

    if (sd->status.party_id > 0)
    {                           // パーティのＨＰ情報通知検査
        struct party *p = party_search(sd->status.party_id);
        if (p != NULL)
        {
            int flag = 0;
            map_foreachinmovearea(std::bind(party_send_hp_check, ph::_1, sd->status.party_id, &flag),
                    sd->bl_m,
                    sd->bl_x - AREA_SIZE, sd->bl_y - AREA_SIZE,
                    sd->bl_x + AREA_SIZE, sd->bl_y + AREA_SIZE,
                    -dx, -dy,
                    BL::PC);
            if (flag)
                sd->party_hp = -1;
        }
    }

    pc_touch_all_relevant_npcs(sd);
    return 0;
}

//
// 武器戦闘
//
/*==========================================
 * スキルの検索 所有していた場合Lvが返る
 *------------------------------------------
 */
int pc_checkskill(dumb_ptr<map_session_data> sd, SkillID skill_id)
{
    if (sd == NULL)
        return 0;

    return sd->status.skill[skill_id].lv;
}

/*==========================================
 * 装 備品のチェック
 *------------------------------------------
 */
int pc_checkequip(dumb_ptr<map_session_data> sd, EPOS pos)
{
    nullpo_retr(-1, sd);

    for (EQUIP i : EQUIPs)
    {
        if (bool(pos & equip_pos[i]))
            return sd->equip_index[i];
    }

    return -1;
}

/*==========================================
 * PCの攻撃 (timer関数)
 *------------------------------------------
 */
static
void pc_attack_timer(TimerData *, tick_t tick, int id)
{
    dumb_ptr<map_session_data> sd;
    dumb_ptr<block_list> bl;
    eptr<struct status_change, StatusChange> sc_data;
    int dist, range;

    sd = map_id2sd(id);
    if (sd == NULL)
        return;

    if (sd->bl_prev == NULL)
        return;

    bl = map_id2bl(sd->attacktarget);
    if (bl == NULL || bl->bl_prev == NULL)
        return;

    if (bl->bl_type == BL::PC && pc_isdead(bl->is_player()))
        return;

    // 同じmapでないなら攻撃しない
    // PCが死んでても攻撃しない
    if (sd->bl_m != bl->bl_m || pc_isdead(sd))
        return;

    // 異常などで攻撃できない
    if (sd->opt1 != Opt1::ZERO)
        return;

    Option *opt = battle_get_option(bl);
    if (opt != NULL && bool(*opt & Option::REAL_ANY_HIDE))
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

    interval_t attack_spell_delay = sd->attack_spell_delay;
    if (sd->attack_spell_override   // [Fate] If we have an active attack spell, use that
        && spell_attack(id, sd->attacktarget))
    {
        // Return if the spell succeeded.  If the spell had disspiated, spell_attack() may fail.
        sd->attackabletime = tick + attack_spell_delay;

    }
    else
    {
        dist = distance(sd->bl_x, sd->bl_y, bl->bl_x, bl->bl_y);
        range = sd->attackrange;
        if (sd->status.weapon != ItemLook::BOW)
            range++;
        if (dist > range)
        {                       // 届 かないので移動
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
                sd->dir = sd->head_dir = map_calc_dir(sd, bl->bl_x, bl->bl_y);  // 向き設定

            if (sd->walktimer)
                pc_stop_walking(sd, 1);

            {
                MapBlockLock lock;
                pc_stop_walking(sd, 0);
                sd->attacktarget_lv = battle_weapon_attack(sd, bl, tick);
                sd->attackabletime = tick + (sd->aspd * 2);
            }
            if (sd->attackabletime <= tick)
                sd->attackabletime = tick + static_cast<interval_t>(battle_config.max_aspd) * 2;
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
 *------------------------------------------
 */
int pc_attack(dumb_ptr<map_session_data> sd, int target_id, int type)
{
    dumb_ptr<block_list> bl;

    nullpo_ret(sd);

    bl = map_id2bl(target_id);
    if (bl == NULL)
        return 1;

    if (bl->bl_type == BL::NPC)
    {                           // monster npcs [Valaris]
        npc_click(sd, RFIFOL(sd->fd, 2));
        return 0;
    }

    if (!battle_check_target(sd, bl, BCT_ENEMY))
        return 1;
    if (sd->attacktimer)
        pc_stopattack(sd);
    sd->attacktarget = target_id;
    sd->state.attack_continue = type;

    interval_t d = sd->attackabletime - gettick();
    if (d > interval_t::zero() && d < std::chrono::seconds(2))
    {                           // 攻撃delay中
        sd->attacktimer = Timer(sd->attackabletime,
                std::bind(pc_attack_timer, ph::_1, ph::_2,
                    sd->bl_id));
    }
    else
    {
        // 本来timer関数なので引数を合わせる
        pc_attack_timer(nullptr, gettick(), sd->bl_id);
    }

    return 0;
}

/*==========================================
 * 継続攻撃停止
 *------------------------------------------
 */
int pc_stopattack(dumb_ptr<map_session_data> sd)
{
    nullpo_ret(sd);

    sd->attacktimer.cancel();

    sd->attacktarget = 0;
    sd->state.attack_continue = 0;

    return 0;
}

static
int pc_checkbaselevelup(dumb_ptr<map_session_data> sd)
{
    int next = pc_nextbaseexp(sd);

    nullpo_ret(sd);

    if (sd->status.base_exp >= next && next > 0)
    {
        // base側レベルアップ処理
        sd->status.base_exp -= next;

        sd->status.base_level++;
        sd->status.status_point += (sd->status.base_level + 14) / 4;
        clif_updatestatus(sd, SP::STATUSPOINT);
        clif_updatestatus(sd, SP::BASELEVEL);
        clif_updatestatus(sd, SP::NEXTBASEEXP);
        pc_calcstatus(sd, 0);
        pc_heal(sd, sd->status.max_hp, sd->status.max_sp);

        clif_misceffect(sd, 0);
        //レベルアップしたのでパーティー情報を更新する
        //(公平範囲チェック)
        party_send_movemap(sd);
        MAP_LOG_XP(sd, "LEVELUP");
        return 1;
    }

    return 0;
}

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

    for (SkillID skill_id = SkillID(); skill_id < MAX_SKILL;
            skill_id = SkillID(uint16_t(skill_id) + 1))
        if (sd->status.skill[skill_id].lv
            && sd->status.skill[skill_id].lv < skill_db[skill_id].max_raise)
            potential += RAISE_COST(skill_db[skill_id].max_raise)
                - RAISE_COST(sd->status.skill[skill_id].lv);

    return potential;
}

static
int pc_checkjoblevelup(dumb_ptr<map_session_data> sd)
{
    int next = pc_nextjobexp(sd);

    nullpo_ret(sd);

    if (sd->status.job_exp >= next && next > 0)
    {
        if (pc_skillpt_potential(sd) < sd->status.skill_point)
        {                       // [Fate] Bah, this is is painful.
            // But the alternative is quite error-prone, and eAthena has far worse performance issues...
            sd->status.job_exp = next - 1;
            pc_calcstatus(sd,0);
            return 0;
        }

        // job側レベルアップ処理
        sd->status.job_exp -= next;
        clif_updatestatus(sd, SP::NEXTJOBEXP);
        sd->status.skill_point++;
        clif_updatestatus(sd, SP::SKILLPOINT);
        pc_calcstatus(sd, 0);

        MAP_LOG_PC(sd, "SKILLPOINTS-UP %d", sd->status.skill_point);

        if (sd->status.job_level < 250
            && sd->status.job_level < sd->status.base_level * 2)
            sd->status.job_level++; // Make levelling up a little harder

        clif_misceffect(sd, 1);
        return 1;
    }

    return 0;
}

int pc_gainexp_reason(dumb_ptr<map_session_data> sd, int base_exp, int job_exp,
        PC_GAINEXP_REASON reason)
{
    nullpo_ret(sd);

    if (sd->bl_prev == NULL || pc_isdead(sd))
        return 0;

    earray<const char *, PC_GAINEXP_REASON, PC_GAINEXP_REASON::COUNT> reasons //=
    {{
        "KILLXP",
        "HEALXP",
        "SCRIPTXP",
        "SHAREXP",
        /* Insert new types here */
        "UNKNOWNXP"
    }};
    MAP_LOG_PC(sd, "GAINXP %d %d %s", base_exp, job_exp, reasons[reason]);

    if (!battle_config.multi_level_up && pc_nextbaseafter(sd))
    {
        while (sd->status.base_exp + base_exp >= pc_nextbaseafter(sd)
               && sd->status.base_exp <= pc_nextbaseexp(sd)
               && pc_nextbaseafter(sd) > 0)
        {
            base_exp *= .90;
        }
    }

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

    while (pc_checkbaselevelup(sd));

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

    sd->status.job_exp += job_exp;
    if (sd->status.job_exp < 0)
        sd->status.job_exp = 0;

    while (pc_checkjoblevelup(sd));

    clif_updatestatus(sd, SP::JOBEXP);

    if (battle_config.disp_experience)
    {
        FString output = STRPRINTF(
                "Experienced Gained Base:%d Job:%d",
                base_exp, job_exp);
        clif_displaymessage(sd->fd, output);
    }

    return 0;
}

int pc_extract_healer_exp(dumb_ptr<map_session_data> sd, int max)
{
    int amount;
    nullpo_ret(sd);

    amount = sd->heal_xp;
    if (max < amount)
        amount = max;

    sd->heal_xp -= amount;
    return amount;
}

/*==========================================
 * base level側必要経験値計算
 *------------------------------------------
 */
int pc_nextbaseexp(dumb_ptr<map_session_data> sd)
{
    nullpo_ret(sd);

    if (sd->status.base_level >= MAX_LEVEL || sd->status.base_level <= 0)
        return 0;

    return exp_table_0[sd->status.base_level - 1];
}

/*==========================================
 * job level側必要経験値計算
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
    nullpo_ret(sd);

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
    nullpo_ret(sd);

    if (sd->status.job_level >= MAX_LEVEL || sd->status.job_level <= 0)
        return 0;

    return exp_table_7[sd->status.job_level];
}

/*==========================================
 * 必要ステータスポイント計算
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
 *------------------------------------------
 */
int pc_statusup(dumb_ptr<map_session_data> sd, SP type)
{
    int need, val = 0;

    nullpo_ret(sd);

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
    pc_calcstatus(sd, 0);
    clif_statusupack(sd, type, 1, val);

    MAP_LOG_STATS(sd, "STATUP");

    return 0;
}

/*==========================================
 * 能力値成長
 *------------------------------------------
 */
int pc_statusup2(dumb_ptr<map_session_data> sd, SP type, int val)
{
    nullpo_ret(sd);

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
    pc_calcstatus(sd, 0);
    clif_statusupack(sd, type, 1, val);
    MAP_LOG_STATS(sd, "STATUP2");

    return 0;
}

/*==========================================
 * スキルポイント割り振り
 *------------------------------------------
 */
int pc_skillup(dumb_ptr<map_session_data> sd, SkillID skill_num)
{
    nullpo_ret(sd);

    if (sd->status.skill[skill_num].lv
        && sd->status.skill_point >= sd->status.skill[skill_num].lv
        && sd->status.skill[skill_num].lv < skill_db[skill_num].max_raise)
    {
        sd->status.skill_point -= sd->status.skill[skill_num].lv;
        sd->status.skill[skill_num].lv++;

        pc_calcstatus(sd, 0);
        clif_skillup(sd, skill_num);
        clif_updatestatus(sd, SP::SKILLPOINT);
        clif_skillinfoblock(sd);
        MAP_LOG_PC(sd, "SKILLUP %d %d %d",
                skill_num, sd->status.skill[skill_num].lv,
                skill_power(sd, skill_num));
    }

    return 0;
}

/*==========================================
 * /resetlvl
 *------------------------------------------
 */
int pc_resetlvl(dumb_ptr<map_session_data> sd, int type)
{
    nullpo_ret(sd);

    for (SkillID i : erange(SkillID(1), MAX_SKILL))
    {
        sd->status.skill[i].lv = 0;
    }

    if (type == 1)
    {
        sd->status.skill_point = 0;
        sd->status.base_level = 1;
        sd->status.job_level = 1;
        sd->status.base_exp = 0;
        sd->status.job_exp = 0;
        sd->status.option = Option::ZERO;

        for (ATTR attr : ATTRs)
            sd->status.attrs[attr] = 1;
    }

    if (type == 2)
    {
        sd->status.skill_point = 0;
        sd->status.base_level = 1;
        sd->status.job_level = 1;
        sd->status.base_exp = 0;
        sd->status.job_exp = 0;
    }
    if (type == 3)
    {
        sd->status.base_level = 1;
        sd->status.base_exp = 0;
    }
    if (type == 4)
    {
        sd->status.job_level = 1;
        sd->status.job_exp = 0;
    }

    clif_updatestatus(sd, SP::STATUSPOINT);
    clif_updatestatus(sd, SP::STR);
    clif_updatestatus(sd, SP::AGI);
    clif_updatestatus(sd, SP::VIT);
    clif_updatestatus(sd, SP::INT);
    clif_updatestatus(sd, SP::DEX);
    clif_updatestatus(sd, SP::LUK);
    clif_updatestatus(sd, SP::BASELEVEL);
    clif_updatestatus(sd, SP::JOBLEVEL);
    clif_updatestatus(sd, SP::STATUSPOINT);
    clif_updatestatus(sd, SP::NEXTBASEEXP);
    clif_updatestatus(sd, SP::NEXTJOBEXP);
    clif_updatestatus(sd, SP::SKILLPOINT);

    clif_updatestatus(sd, SP::USTR);    // Updates needed stat points - Valaris
    clif_updatestatus(sd, SP::UAGI);
    clif_updatestatus(sd, SP::UVIT);
    clif_updatestatus(sd, SP::UINT);
    clif_updatestatus(sd, SP::UDEX);
    clif_updatestatus(sd, SP::ULUK);    // End Addition

    for (EQUIP i : EQUIPs)
    {
        // unequip items that can't be equipped by base 1 [Valaris]
        if (sd->equip_index[i] >= 0)
            if (!pc_isequip(sd, sd->equip_index[i]))
            {
                pc_unequipitem(sd, sd->equip_index[i], CalcStatus::LATER);
                sd->equip_index[i] = -1;
            }
    }

    clif_skillinfoblock(sd);
    pc_calcstatus(sd, 0);

    MAP_LOG_STATS(sd, "STATRESET");

    return 0;
}

/*==========================================
 * /resetstate
 *------------------------------------------
 */
int pc_resetstate(dumb_ptr<map_session_data> sd)
{

    nullpo_ret(sd);

    sd->status.status_point = stat_p[sd->status.base_level - 1];

    clif_updatestatus(sd, SP::STATUSPOINT);

    for (ATTR attr : ATTRs)
        sd->status.attrs[attr] = 1;
    for (ATTR attr : ATTRs)
        clif_updatestatus(sd, attr_to_sp(attr));
    for (ATTR attr : ATTRs)
        clif_updatestatus(sd, attr_to_usp(attr));

    pc_calcstatus(sd, 0);

    return 0;
}

/*==========================================
 * /resetskill
 *------------------------------------------
 */
int pc_resetskill(dumb_ptr<map_session_data> sd)
{
    int skill;

    nullpo_ret(sd);

    sd->status.skill_point += pc_calc_skillpoint(sd);

    for (SkillID i : erange(SkillID(1), MAX_SKILL))
        if ((skill = pc_checkskill(sd, i)) > 0)
        {
            sd->status.skill[i].lv = 0;
            sd->status.skill[i].flags = SkillFlags::ZERO;
        }

    clif_updatestatus(sd, SP::SKILLPOINT);
    clif_skillinfoblock(sd);
    pc_calcstatus(sd, 0);

    return 0;
}

/*==========================================
 * pcにダメージを与える
 *------------------------------------------
 */
int pc_damage(dumb_ptr<block_list> src, dumb_ptr<map_session_data> sd,
               int damage)
{
    nullpo_ret(sd);

    // 既に死んでいたら無効
    if (pc_isdead(sd))
        return 0;
    // 座ってたら立ち上がる
    if (pc_issit(sd))
    {
        pc_setstand(sd);
    }

    if (src)
    {
        if (src->bl_type == BL::PC)
        {
            MAP_LOG_PC(sd, "INJURED-BY PC%d FOR %d",
                        src->is_player()->status.char_id,
                        damage);
        }
        else
        {
            MAP_LOG_PC(sd, "INJURED-BY MOB%d FOR %d", src->bl_id, damage);
        }
    }
    else
        MAP_LOG_PC(sd, "INJURED-BY null FOR %d", damage);

    pc_stop_walking(sd, 3);
    // 演奏/ダンスの中断
    if (damage > sd->status.max_hp >> 2)
        skill_stop_dancing(sd, 0);

    sd->status.hp -= damage;

    if (sd->status.hp > 0)
    {
        // まだ生きているならHP更新
        clif_updatestatus(sd, SP::HP);

        sd->canlog_tick = gettick();

        if (sd->status.party_id > 0)
        {                       // on-the-fly party hp updates [Valaris]
            struct party *p = party_search(sd->status.party_id);
            if (p != NULL)
                clif_party_hp(p, sd);
        }                       // end addition [Valaris]

        return 0;
    }

    MAP_LOG_PC(sd, "DEAD%s", "");

    // Character is dead!

    sd->status.hp = 0;
    // [Fate] Stop quickregen
    sd->quick_regeneration_hp.amount = 0;
    sd->quick_regeneration_sp.amount = 0;
    skill_update_heal_animation(sd);

    pc_setdead(sd);

    pc_stop_walking(sd, 0);
    skill_castcancel(sd, 0);  // 詠唱の中止
    clif_clearchar(sd, BeingRemoveWhy::DEAD);
    pc_setglobalreg(sd, stringish<VarName>("PC_DIE_COUNTER"), ++sd->die_counter);  //死にカウンター書き込み
    skill_status_change_clear(sd, 0); // ステータス異常を解除する
    clif_updatestatus(sd, SP::HP);
    pc_calcstatus(sd, 0);
    // [Fate] Reset magic
    sd->cast_tick = gettick();
    magic_stop_completely(sd);

    if (battle_config.death_penalty_type > 0 && sd->status.base_level >= 20)
    {
        // changed penalty options, added death by player if pk_mode [Valaris]
        if (!sd->bl_m->flag.nopenalty)
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
    if (sd->bl_m->flag.pvp && !battle_config.pk_mode)
    {                           // disable certain pvp functions on pk_mode [Valaris]
        //ランキング計算
        if (!sd->bl_m->flag.pvp_nocalcrank)
        {
            sd->pvp_point -= 5;
            if (src && src->bl_type == BL::PC)
                src->is_player()->pvp_point++;
            pc_setdead(sd);
        }
        // 強制送還
        if (sd->pvp_point < 0)
        {
            sd->pvp_point = 0;
            pc_setstand(sd);
            pc_setrestartvalue(sd, 3);
            pc_setpos(sd, sd->status.save_point.map_, sd->status.save_point.x,
                       sd->status.save_point.y, BeingRemoveWhy::GONE);
        }
    }

    if (src && src->bl_type == BL::PC)
    {
        // [Fate] PK death, trigger scripts
        argrec_t arg[3] =
        {
            {"@killerrid", src->bl_id},
            {"@victimrid", sd->bl_id},
            {"@victimlvl", sd->status.base_level},
        };
        npc_event_doall_l(stringish<ScriptLabel>("OnPCKilledEvent"), sd->bl_id, 3, arg);
        npc_event_doall_l(stringish<ScriptLabel>("OnPCKillEvent"), src->bl_id, 3, arg);
    }
    npc_event_doall_l(stringish<ScriptLabel>("OnPCDieEvent"), sd->bl_id, 0, NULL);

    return 0;
}

//
// script関 連
//
/*==========================================
 * script用PCステータス読み出し
 *------------------------------------------
 */
int pc_readparam(dumb_ptr<map_session_data> sd, SP type)
{
    int val = 0;

    nullpo_ret(sd);

    switch (type)
    {
        case SP::SKILLPOINT:
            val = sd->status.skill_point;
            break;
        case SP::STATUSPOINT:
            val = sd->status.status_point;
            break;
        case SP::ZENY:
            val = sd->status.zeny;
            break;
        case SP::BASELEVEL:
            val = sd->status.base_level;
            break;
        case SP::JOBLEVEL:
            val = sd->status.job_level;
            break;
        case SP::CLASS:
            val = sd->status.species;
            break;
        case SP::SEX:
            val = static_cast<uint8_t>(sd->sex);
            break;
        case SP::WEIGHT:
            val = sd->weight;
            break;
        case SP::MAXWEIGHT:
            val = sd->max_weight;
            break;
        case SP::BASEEXP:
            val = sd->status.base_exp;
            break;
        case SP::JOBEXP:
            val = sd->status.job_exp;
            break;
        case SP::NEXTBASEEXP:
            val = pc_nextbaseexp(sd);
            break;
        case SP::NEXTJOBEXP:
            val = pc_nextjobexp(sd);
            break;
        case SP::HP:
            val = sd->status.hp;
            break;
        case SP::MAXHP:
            val = sd->status.max_hp;
            break;
        case SP::SP:
            val = sd->status.sp;
            break;
        case SP::MAXSP:
            val = sd->status.max_sp;
            break;
        case SP::STR:
        case SP::AGI:
        case SP::VIT:
        case SP::INT:
        case SP::DEX:
        case SP::LUK:
            val = sd->status.attrs[sp_to_attr(type)];
            break;
    }

    return val;
}

/*==========================================
 * script用PCステータス設定
 *------------------------------------------
 */
int pc_setparam(dumb_ptr<map_session_data> sd, SP type, int val)
{
    int i = 0, up_level = 50;

    nullpo_ret(sd);

    switch (type)
    {
        case SP::BASELEVEL:
            if (val > sd->status.base_level)
            {
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
            pc_calcstatus(sd, 0);
            pc_heal(sd, sd->status.max_hp, sd->status.max_sp);
            break;
        case SP::JOBLEVEL:
            up_level -= 40;
            if (val >= sd->status.job_level)
            {
                if (val > up_level)
                    val = up_level;
                sd->status.skill_point += (val - sd->status.job_level);
                sd->status.job_level = val;
                sd->status.job_exp = 0;
                clif_updatestatus(sd, SP::JOBLEVEL);
                clif_updatestatus(sd, SP::NEXTJOBEXP);
                clif_updatestatus(sd, SP::JOBEXP);
                clif_updatestatus(sd, SP::SKILLPOINT);
                pc_calcstatus(sd, 0);
                clif_misceffect(sd, 1);
            }
            else
            {
                sd->status.job_level = val;
                sd->status.job_exp = 0;
                clif_updatestatus(sd, SP::JOBLEVEL);
                clif_updatestatus(sd, SP::NEXTJOBEXP);
                clif_updatestatus(sd, SP::JOBEXP);
                pc_calcstatus(sd, 0);
            }
            clif_updatestatus(sd, type);
            break;
        case SP::SKILLPOINT:
            sd->status.skill_point = val;
            break;
        case SP::STATUSPOINT:
            sd->status.status_point = val;
            break;
        case SP::ZENY:
            sd->status.zeny = val;
            break;
        case SP::BASEEXP:
            if (pc_nextbaseexp(sd) > 0)
            {
                sd->status.base_exp = val;
                if (sd->status.base_exp < 0)
                    sd->status.base_exp = 0;
                pc_checkbaselevelup(sd);
            }
            break;
        case SP::JOBEXP:
            if (pc_nextjobexp(sd) > 0)
            {
                sd->status.job_exp = val;
                if (sd->status.job_exp < 0)
                    sd->status.job_exp = 0;
                pc_checkjoblevelup(sd);
            }
            break;
        case SP::SEX:
            // this is a really bad idea
            sd->sex = static_cast<SEX>(val);
            break;
        case SP::WEIGHT:
            sd->weight = val;
            break;
        case SP::MAXWEIGHT:
            sd->max_weight = val;
            break;
        case SP::HP:
            sd->status.hp = val;
            break;
        case SP::MAXHP:
            sd->status.max_hp = val;
            break;
        case SP::SP:
            sd->status.sp = val;
            break;
        case SP::MAXSP:
            sd->status.max_sp = val;
            break;
        case SP::STR:
        case SP::AGI:
        case SP::VIT:
        case SP::INT:
        case SP::DEX:
        case SP::LUK:
            sd->status.attrs[sp_to_attr(type)] = val;
            break;
    }
    clif_updatestatus(sd, type);

    return 0;
}

/*==========================================
 * HP/SP回復
 *------------------------------------------
 */
int pc_heal(dumb_ptr<map_session_data> sd, int hp, int sp)
{
//  if(battle_config.battle_log)
//      PRINTF("heal %d %d\n",hp,sp);

    nullpo_ret(sd);

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
        pc_damage(NULL, sd, 1);
        hp = 0;
    }
    sd->status.sp += sp;
    if (sd->status.sp <= 0)
        sd->status.sp = 0;
    if (hp)
        clif_updatestatus(sd, SP::HP);
    if (sp)
        clif_updatestatus(sd, SP::SP);

    if (sd->status.party_id > 0)
    {                           // on-the-fly party hp updates [Valaris]
        struct party *p = party_search(sd->status.party_id);
        if (p != NULL)
            clif_party_hp(p, sd);
    }                           // end addition [Valaris]

    return hp + sp;
}

/*==========================================
 * HP/SP回復
 *------------------------------------------
 */
static
int pc_itemheal_effect(dumb_ptr<map_session_data> sd, int hp, int sp);

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

static
void pc_heal_quick_accumulate(int new_amount,
                          struct quick_regeneration *quick_regen, int max)
{
    int current_amount = quick_regen->amount;
    int current_speed = quick_regen->speed;
    int new_speed = pc_heal_quick_speed(new_amount);

    int average_speed = ((new_speed * new_amount) + (current_speed * current_amount)) / (current_amount + new_amount); // new_amount > 0, current_amount >= 0

    quick_regen->speed = average_speed;
    quick_regen->amount = min(current_amount + new_amount, max);

    quick_regen->tickdelay = min(quick_regen->speed, quick_regen->tickdelay);
}

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

/* pc_itemheal_effect is invoked once every 0.5s whenever the pc
 * has health recovery queued up (cf. pc_natural_heal_sub).
 */
static
int pc_itemheal_effect(dumb_ptr<map_session_data> sd, int hp, int sp)
{
    nullpo_ret(sd);

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
        pc_damage(NULL, sd, 1);
        hp = 0;
    }
    sd->status.sp += sp;
    if (sd->status.sp <= 0)
        sd->status.sp = 0;
    if (hp)
        clif_updatestatus(sd, SP::HP);
    if (sp)
        clif_updatestatus(sd, SP::SP);

    return 0;
}

/*==========================================
 * HP/SP回復
 *------------------------------------------
 */
int pc_percentheal(dumb_ptr<map_session_data> sd, int hp, int sp)
{
    nullpo_ret(sd);

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
    if (hp)
    {
        if (hp >= 100)
        {
            sd->status.hp = sd->status.max_hp;
        }
        else if (hp <= -100)
        {
            sd->status.hp = 0;
            pc_damage(NULL, sd, 1);
        }
        else
        {
            sd->status.hp += sd->status.max_hp * hp / 100;
            if (sd->status.hp > sd->status.max_hp)
                sd->status.hp = sd->status.max_hp;
            if (sd->status.hp <= 0)
            {
                sd->status.hp = 0;
                pc_damage(NULL, sd, 1);
                hp = 0;
            }
        }
    }
    if (sp)
    {
        if (sp >= 100)
        {
            sd->status.sp = sd->status.max_sp;
        }
        else if (sp <= -100)
        {
            sd->status.sp = 0;
        }
        else
        {
            sd->status.sp += sd->status.max_sp * sp / 100;
            if (sd->status.sp > sd->status.max_sp)
                sd->status.sp = sd->status.max_sp;
            if (sd->status.sp < 0)
                sd->status.sp = 0;
        }
    }
    if (hp)
        clif_updatestatus(sd, SP::HP);
    if (sp)
        clif_updatestatus(sd, SP::SP);

    return 0;
}

/*==========================================
 * 見た目変更
 *------------------------------------------
 */
int pc_changelook(dumb_ptr<map_session_data> sd, LOOK type, int val)
{
    nullpo_ret(sd);

    switch (type)
    {
        case LOOK::HAIR:
            sd->status.hair = val;
            break;
        case LOOK::WEAPON:
            sd->status.weapon = static_cast<ItemLook>(static_cast<uint16_t>(val));
            break;
        case LOOK::HEAD_BOTTOM:
            sd->status.head_bottom = val;
            break;
        case LOOK::HEAD_TOP:
            sd->status.head_top = val;
            break;
        case LOOK::HEAD_MID:
            sd->status.head_mid = val;
            break;
        case LOOK::HAIR_COLOR:
            sd->status.hair_color = val;
            break;
        case LOOK::CLOTHES_COLOR:
            sd->status.clothes_color = val;
            break;
        case LOOK::SHIELD:
            sd->status.shield = val;
            break;
        case LOOK::SHOES:
            break;
    }
    clif_changelook(sd, type, val);

    return 0;
}

/*==========================================
 * 付属品(鷹,ペコ,カート)設定
 *------------------------------------------
 */
int pc_setoption(dumb_ptr<map_session_data> sd, Option type)
{
    nullpo_ret(sd);

    sd->status.option = type;
    clif_changeoption(sd);
    pc_calcstatus(sd, 0);

    return 0;
}

/*==========================================
 * script用変数の値を読む
 *------------------------------------------
 */
int pc_readreg(dumb_ptr<map_session_data> sd, SIR reg)
{
    nullpo_ret(sd);

    return sd->regm.get(reg);
}

/*==========================================
 * script用変数の値を設定
 *------------------------------------------
 */
void pc_setreg(dumb_ptr<map_session_data> sd, SIR reg, int val)
{
    nullpo_retv(sd);

    sd->regm.put(reg, val);
}

/*==========================================
 * script用文字列変数の値を読む
 *------------------------------------------
 */
ZString pc_readregstr(dumb_ptr<map_session_data> sd, SIR reg)
{
    nullpo_retr(ZString(), sd);

    FString *s = sd->regstrm.search(reg);
    if (s)
        return *s;

    return ZString();
}

/*==========================================
 * script用文字列変数の値を設定
 *------------------------------------------
 */
void pc_setregstr(dumb_ptr<map_session_data> sd, SIR reg, FString str)
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
 *------------------------------------------
 */
int pc_readglobalreg(dumb_ptr<map_session_data> sd, VarName reg)
{
    int i;

    nullpo_ret(sd);

    for (i = 0; i < sd->status.global_reg_num; i++)
    {
        if (sd->status.global_reg[i].str == reg)
            return sd->status.global_reg[i].value;
    }

    return 0;
}

/*==========================================
 * script用グローバル変数の値を設定
 *------------------------------------------
 */
int pc_setglobalreg(dumb_ptr<map_session_data> sd, VarName reg, int val)
{
    int i;

    nullpo_ret(sd);

    //PC_DIE_COUNTERがスクリプトなどで変更された時の処理
    if (reg == stringish<VarName>("PC_DIE_COUNTER") && sd->die_counter != val)
    {
        sd->die_counter = val;
        pc_calcstatus(sd, 0);
    }
    if (val == 0)
    {
        for (i = 0; i < sd->status.global_reg_num; i++)
        {
            if (sd->status.global_reg[i].str == reg)
            {
                sd->status.global_reg[i] =
                    sd->status.global_reg[sd->status.global_reg_num - 1];
                sd->status.global_reg_num--;
                break;
            }
        }
        return 0;
    }
    for (i = 0; i < sd->status.global_reg_num; i++)
    {
        if (sd->status.global_reg[i].str == reg)
        {
            sd->status.global_reg[i].value = val;
            return 0;
        }
    }
    if (sd->status.global_reg_num < GLOBAL_REG_NUM)
    {
        sd->status.global_reg[i].str = reg;
        sd->status.global_reg[i].value = val;
        sd->status.global_reg_num++;
        return 0;
    }
    if (battle_config.error_log)
        PRINTF("pc_setglobalreg : couldn't set %s (GLOBAL_REG_NUM = %d)\n",
                reg, GLOBAL_REG_NUM);

    return 1;
}

/*==========================================
 * script用アカウント変数の値を読む
 *------------------------------------------
 */
int pc_readaccountreg(dumb_ptr<map_session_data> sd, VarName reg)
{
    int i;

    nullpo_ret(sd);

    for (i = 0; i < sd->status.account_reg_num; i++)
    {
        if (sd->status.account_reg[i].str == reg)
            return sd->status.account_reg[i].value;
    }

    return 0;
}

/*==========================================
 * script用アカウント変数の値を設定
 *------------------------------------------
 */
int pc_setaccountreg(dumb_ptr<map_session_data> sd, VarName reg, int val)
{
    int i;

    nullpo_ret(sd);

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
        PRINTF("pc_setaccountreg : couldn't set %s (ACCOUNT_REG_NUM = %d)\n",
                reg, ACCOUNT_REG_NUM);

    return 1;
}

/*==========================================
 * script用アカウント変数2の値を読む
 *------------------------------------------
 */
int pc_readaccountreg2(dumb_ptr<map_session_data> sd, VarName reg)
{
    int i;

    nullpo_ret(sd);

    for (i = 0; i < sd->status.account_reg2_num; i++)
    {
        if (sd->status.account_reg2[i].str == reg)
            return sd->status.account_reg2[i].value;
    }

    return 0;
}

/*==========================================
 * script用アカウント変数2の値を設定
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
        PRINTF("pc_setaccountreg2 : couldn't set %s (ACCOUNT_REG2_NUM = %d)\n",
             reg, ACCOUNT_REG2_NUM);

    return 1;
}

/*==========================================
 * イベントタイマー処理
 *------------------------------------------
 */
static
void pc_eventtimer(TimerData *, tick_t, int id, NpcEvent data)
{
    dumb_ptr<map_session_data> sd = map_id2sd(id);
    assert (sd != NULL);

    npc_event(sd, data, 0);
}

/*==========================================
 * イベントタイマー追加
 *------------------------------------------
 */
int pc_addeventtimer(dumb_ptr<map_session_data> sd, interval_t tick, NpcEvent name)
{
    int i;

    nullpo_ret(sd);

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
 *------------------------------------------
 */
int pc_cleareventtimer(dumb_ptr<map_session_data> sd)
{
    nullpo_ret(sd);

    for (int i = 0; i < MAX_EVENTTIMER; i++)
        sd->eventtimer[i].cancel();

    return 0;
}

//
// 装 備物
//
/*==========================================
 * アイテムを装備する
 *------------------------------------------
 */
static
int pc_signal_advanced_equipment_change(dumb_ptr<map_session_data> sd, int n)
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

int pc_equipitem(dumb_ptr<map_session_data> sd, int n, EPOS)
{
    int nameid;
    struct item_data *id;
    //ｿｽ]ｿｽｿｽｿｽｿｽｿｽ{ｿｽqｿｽﾌ場合ｿｽﾌ鯉ｿｽｿｽﾌ職ｿｽﾆゑｿｽｿｽZｿｽoｿｽｿｽｿｽｿｽ

    nullpo_ret(sd);

    if (n < 0 || n >= MAX_INVENTORY)
    {
        clif_equipitemack(sd, 0, EPOS::ZERO, 0);
        return 0;
    }

    nameid = sd->status.inventory[n].nameid;
    id = sd->inventory_data[n];
    if (!id) // can't actually happen - the only caller checks this.
        return 0;
    EPOS pos = pc_equippoint(sd, n);

    if (battle_config.battle_log)
        PRINTF("equip %d (%d) %x:%x\n",
                nameid, n, id->equip, pos);
    if (!pc_isequip(sd, n) || pos == EPOS::ZERO || sd->status.inventory[n].broken == 1)
    {                           // [Valaris]
        clif_equipitemack(sd, n, EPOS::ZERO, 0);    // fail
        return 0;
    }

// -- moonsoul (if player is berserk then cannot equip)
//
    if (pos == (EPOS::MISC2 | EPOS::CAPE))
    {
        // アクセサリ用例外処理
        EPOS epor = EPOS::ZERO;
        if (sd->equip_index[EQUIP::MISC2] >= 0)
            epor |= sd->status.inventory[sd->equip_index[EQUIP::MISC2]].equip;
        if (sd->equip_index[EQUIP::CAPE] >= 0)
            epor |= sd->status.inventory[sd->equip_index[EQUIP::CAPE]].equip;
        epor &= (EPOS::MISC2 | EPOS::CAPE);
        pos = (epor == EPOS::CAPE ? EPOS::MISC2 : EPOS::CAPE);
    }

    for (EQUIP i : EQUIPs)
    {
        if (bool(pos & equip_pos[i]))
        {
            if (sd->equip_index[i] >= 0)    //Slot taken, remove item from there.
                pc_unequipitem(sd, sd->equip_index[i], CalcStatus::LATER);
            sd->equip_index[i] = n;
        }
    }
    // 弓矢装備
    if (pos == EPOS::ARROW)
    {
        clif_arrowequip(sd, n);
        clif_arrow_fail(sd, 3);    // 3=矢が装備できました
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
            sd->equip_index[i] = n;
    }
    sd->status.inventory[n].equip = pos;

    int view_i = 0;
    ItemLook view_l = ItemLook::NONE;
    // TODO: This is ugly.
    if (sd->inventory_data[n])
    {
        bool look_not_weapon = sd->inventory_data[n]->look == ItemLook::NONE;
        bool equip_is_weapon = bool(sd->status.inventory[n].equip & EPOS::WEAPON);
        assert (look_not_weapon != equip_is_weapon);

        if (look_not_weapon)
            view_i = sd->inventory_data[n]->nameid;
        else
            view_l = sd->inventory_data[n]->look;
    }

    if (bool(sd->status.inventory[n].equip & EPOS::WEAPON))
    {
        sd->weapontype1 = view_l;
        pc_calcweapontype(sd);
        pc_set_weapon_look(sd);
    }
    if (bool(sd->status.inventory[n].equip & EPOS::SHIELD))
    {
        if (sd->inventory_data[n])
        {
            if (sd->inventory_data[n]->type == ItemType::WEAPON)
            {
                sd->status.shield = 0;
                if (sd->status.inventory[n].equip == EPOS::SHIELD)
                    sd->weapontype2 = view_l;
            }
            else if (sd->inventory_data[n]->type == ItemType::ARMOR)
            {
                sd->status.shield = view_i;
                sd->weapontype2 = ItemLook::NONE;
            }
        }
        else
        {
            sd->status.shield = 0;
            sd->weapontype2 = ItemLook::NONE;
        }
        pc_calcweapontype(sd);
        clif_changelook(sd, LOOK::SHIELD, sd->status.shield);
    }
    if (bool(sd->status.inventory[n].equip & EPOS::LEGS))
    {
        sd->status.head_bottom = view_i;
        clif_changelook(sd, LOOK::HEAD_BOTTOM, sd->status.head_bottom);
    }
    if (bool(sd->status.inventory[n].equip & EPOS::HAT))
    {
        sd->status.head_top = view_i;
        clif_changelook(sd, LOOK::HEAD_TOP, sd->status.head_top);
    }
    if (bool(sd->status.inventory[n].equip & EPOS::TORSO))
    {
        sd->status.head_mid = view_i;
        clif_changelook(sd, LOOK::HEAD_MID, sd->status.head_mid);
    }
    pc_signal_advanced_equipment_change(sd, n);

    pc_calcstatus(sd, 0);

    return 0;
}

/*==========================================
 * 装 備した物を外す
 *------------------------------------------
 */
int pc_unequipitem(dumb_ptr<map_session_data> sd, int n, CalcStatus type)
{
    nullpo_ret(sd);

// -- moonsoul  (if player is berserk then cannot unequip)
//
    if (battle_config.battle_log)
        PRINTF("unequip %d %x:%x\n",
                n, pc_equippoint(sd, n),
                sd->status.inventory[n].equip);
    if (bool(sd->status.inventory[n].equip))
    {
        for (EQUIP i : EQUIPs)
        {
            if (bool(sd->status.inventory[n].equip & equip_pos[i]))
                sd->equip_index[i] = -1;
        }
        if (bool(sd->status.inventory[n].equip & EPOS::WEAPON))
        {
            sd->weapontype1 = ItemLook::NONE;
            sd->status.weapon = sd->weapontype2;
            pc_calcweapontype(sd);
            pc_set_weapon_look(sd);
        }
        if (bool(sd->status.inventory[n].equip & EPOS::SHIELD))
        {
            sd->status.shield = 0;
            sd->weapontype2 = ItemLook::NONE;
            pc_calcweapontype(sd);
            clif_changelook(sd, LOOK::SHIELD, sd->status.shield);
        }
        if (bool(sd->status.inventory[n].equip & EPOS::LEGS))
        {
            sd->status.head_bottom = 0;
            clif_changelook(sd, LOOK::HEAD_BOTTOM,
                             sd->status.head_bottom);
        }
        if (bool(sd->status.inventory[n].equip & EPOS::HAT))
        {
            sd->status.head_top = 0;
            clif_changelook(sd, LOOK::HEAD_TOP, sd->status.head_top);
        }
        if (bool(sd->status.inventory[n].equip & EPOS::TORSO))
        {
            sd->status.head_mid = 0;
            clif_changelook(sd, LOOK::HEAD_MID, sd->status.head_mid);
        }
        pc_signal_advanced_equipment_change(sd, n);

        if (sd->sc_data[StatusChange::SC_BROKNWEAPON].timer
            && bool(sd->status.inventory[n].equip & EPOS::WEAPON)
            && sd->status.inventory[n].broken == 1)
            skill_status_change_end(sd, StatusChange::SC_BROKNWEAPON, nullptr);

        clif_unequipitemack(sd, n, sd->status.inventory[n].equip, 1);
        sd->status.inventory[n].equip = EPOS::ZERO;
    }
    else
    {
        clif_unequipitemack(sd, n, EPOS::ZERO, 0);
    }
    if (type == CalcStatus::NOW)
    {
        pc_calcstatus(sd, 0);
    }

    return 0;
}

int pc_unequipinvyitem(dumb_ptr<map_session_data> sd, int n, CalcStatus type)
{
    nullpo_retr(1, sd);

    for (EQUIP i : EQUIPs)
    {
        if (equip_pos[i] != EPOS::ZERO
            && !bool(equip_pos[i] & EPOS::ARROW) // probably a bug
            && sd->equip_index[i] == n)
        {
            //Slot taken, remove item from there.
            pc_unequipitem(sd, sd->equip_index[i], type);
            sd->equip_index[i] = -1;
        }
    }

    return 0;
}

/*==========================================
 * アイテムのindex番号を詰めたり
 * 装 備品の装備可能チェックを行なう
 *------------------------------------------
 */
int pc_checkitem(dumb_ptr<map_session_data> sd)
{
    int i, j, k, id, calc_flag = 0;

    nullpo_ret(sd);

    // 所持品空き詰め
    for (i = j = 0; i < MAX_INVENTORY; i++)
    {
        if ((id = sd->status.inventory[i].nameid) == 0)
            continue;
        if (i > j)
        {
            sd->status.inventory[j] = sd->status.inventory[i];
            sd->inventory_data[j] = sd->inventory_data[i];
        }
        j++;
    }
    for (k = j; k < MAX_INVENTORY; ++k)
        sd->status.inventory[k] = item{};
    for (k = j; k < MAX_INVENTORY; k++)
        sd->inventory_data[k] = NULL;

    // カート内空き詰め
    for (i = j = 0; i < MAX_CART; i++)
    {
        if ((id = sd->status.cart[i].nameid) == 0)
            continue;
        if (i > j)
        {
            sd->status.cart[j] = sd->status.cart[i];
        }
        j++;
    }
    while (j < MAX_CART)
        sd->status.cart[j++] = item{};

    // 装 備位置チェック

    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid == 0)
            continue;
        if (bool(sd->status.inventory[i].equip & ~pc_equippoint(sd, i)))
        {
            sd->status.inventory[i].equip = EPOS::ZERO;
            calc_flag = 1;
        }
    }

    pc_setequipindex(sd);
    if (calc_flag)
        pc_calcstatus(sd, 2);

    return 0;
}

int pc_checkoverhp(dumb_ptr<map_session_data> sd)
{
    nullpo_ret(sd);

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

int pc_checkoversp(dumb_ptr<map_session_data> sd)
{
    nullpo_ret(sd);

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
 *------------------------------------------
 */
int pc_calc_pvprank(dumb_ptr<map_session_data> sd)
{
    nullpo_ret(sd);
    map_local *m = sd->bl_m;
    nullpo_ret(m);

    if (!(m->flag.pvp))
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
 *------------------------------------------
 */
void pc_calc_pvprank_timer(TimerData *, tick_t, int id)
{
    dumb_ptr<map_session_data> sd = NULL;
    if (battle_config.pk_mode)  // disable pvp ranking if pk_mode on [Valaris]
        return;

    sd = map_id2sd(id);
    if (sd == NULL)
        return;
    sd->pvp_timer.cancel();
    if (pc_calc_pvprank(sd) > 0)
        sd->pvp_timer = Timer(gettick() + PVP_CALCRANK_INTERVAL,
                std::bind(pc_calc_pvprank_timer, ph::_1, ph::_2,
                    id));
}

/*==========================================
 * sdは結婚しているか(既婚の場合は相方のchar_idを返す)
 *------------------------------------------
 */
static
int pc_ismarried(dumb_ptr<map_session_data> sd)
{
    if (sd == NULL)
        return -1;
    if (sd->status.partner_id > 0)
        return sd->status.partner_id;
    else
        return 0;
}

/*==========================================
 * sdがdstsdと結婚(dstsd→sdの結婚処理も同時に行う)
 *------------------------------------------
 */
int pc_marriage(dumb_ptr<map_session_data> sd, dumb_ptr<map_session_data> dstsd)
{
    if (sd == NULL || dstsd == NULL || sd->status.partner_id > 0
        || dstsd->status.partner_id > 0)
        return -1;
    sd->status.partner_id = dstsd->status.char_id;
    dstsd->status.partner_id = sd->status.char_id;
    return 0;
}

/*==========================================
 * sdが離婚(相手はsd->status.partner_idに依る)(相手も同時に離婚・結婚指輪自動剥奪)
 *------------------------------------------
 */
int pc_divorce(dumb_ptr<map_session_data> sd)
{
    dumb_ptr<map_session_data> p_sd = NULL;
    if (sd == NULL || !pc_ismarried(sd))
        return -1;

    // If both are on map server we don't need to bother the char server
    if ((p_sd =
         map_nick2sd(map_charid2nick(sd->status.partner_id))) != NULL)
    {
        if (p_sd->status.partner_id != sd->status.char_id
            || sd->status.partner_id != p_sd->status.char_id)
        {
            PRINTF("pc_divorce: Illegal partner_id sd=%d p_sd=%d\n",
                    sd->status.partner_id, p_sd->status.partner_id);
            return -1;
        }
        p_sd->status.partner_id = 0;
        sd->status.partner_id = 0;

        if (sd->npc_flags.divorce)
        {
            sd->npc_flags.divorce = 0;
            map_scriptcont(sd, sd->npc_id);
        }
    }
    else
        chrif_send_divorce(sd->status.char_id);

    return 0;
}

/*==========================================
 * sdの相方のmap_session_dataを返す
 *------------------------------------------
 */
dumb_ptr<map_session_data> pc_get_partner(dumb_ptr<map_session_data> sd)
{
    dumb_ptr<map_session_data> p_sd = NULL;
    if (sd == NULL || !pc_ismarried(sd))
        return NULL;

    CharName nick = map_charid2nick(sd->status.partner_id);

    if (!nick.to__actual())
        return NULL;

    if ((p_sd = map_nick2sd(nick)) == NULL)
        return NULL;

    return p_sd;
}

//
// 自然回復物
//
/*==========================================
 * SP回復量計算
 *------------------------------------------
 */
static
tick_t natural_heal_tick, natural_heal_prev_tick;
static
interval_t natural_heal_diff_tick;

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

static
int pc_natural_heal_hp(dumb_ptr<map_session_data> sd)
{
    int bhp;
    int bonus;

    nullpo_ret(sd);

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

    if (sd->hp_sub >= static_cast<interval_t>(battle_config.natural_healhp_interval))
    {
        bonus = sd->nhealhp;
        while (sd->hp_sub >= static_cast<interval_t>(battle_config.natural_healhp_interval))
        {
            sd->hp_sub -= static_cast<interval_t>(battle_config.natural_healhp_interval);
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
        clif_updatestatus(sd, SP::HP);

    if (sd->nshealhp > 0)
    {
        if (sd->inchealhptick >= static_cast<interval_t>(battle_config.natural_heal_skill_interval)
            && sd->status.hp < sd->status.max_hp)
        {
            bonus = sd->nshealhp;
            while (sd->inchealhptick >= static_cast<interval_t>(battle_config.natural_heal_skill_interval))
            {
                sd->inchealhptick -= static_cast<interval_t>(battle_config.natural_heal_skill_interval);
                if (sd->status.hp + bonus <= sd->status.max_hp)
                    sd->status.hp += bonus;
                else
                {
                    bonus = sd->status.max_hp - sd->status.hp;
                    sd->status.hp = sd->status.max_hp;
                    sd->hp_sub = sd->inchealhptick = interval_t::zero();
                }
            }
        }
    }
    else
        sd->inchealhptick = interval_t::zero();

    return 0;
}

static
int pc_natural_heal_sp(dumb_ptr<map_session_data> sd)
{
    int bsp;
    int bonus;

    nullpo_ret(sd);

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

    if (sd->sp_sub >= static_cast<interval_t>(battle_config.natural_healsp_interval))
    {
        bonus = sd->nhealsp;
        while (sd->sp_sub >= static_cast<interval_t>(battle_config.natural_healsp_interval))
        {
            sd->sp_sub -= static_cast<interval_t>(battle_config.natural_healsp_interval);
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

    if (sd->nshealsp > 0)
    {
        if (sd->inchealsptick >= static_cast<interval_t>(battle_config.natural_heal_skill_interval)
            && sd->status.sp < sd->status.max_sp)
        {
            bonus = sd->nshealsp;
            while (sd->inchealsptick >= static_cast<interval_t>(battle_config.natural_heal_skill_interval))
            {
                sd->inchealsptick -= static_cast<interval_t>(battle_config.natural_heal_skill_interval);
                if (sd->status.sp + bonus <= sd->status.max_sp)
                    sd->status.sp += bonus;
                else
                {
                    bonus = sd->status.max_sp - sd->status.sp;
                    sd->status.sp = sd->status.max_sp;
                    sd->sp_sub = sd->inchealsptick = interval_t::zero();
                }
            }
        }
    }
    else
        sd->inchealsptick = interval_t::zero();
    return 0;
}

/*==========================================
 * HP/SP 自然回復 各クライアント
 *------------------------------------------
 */

static
int pc_quickregenerate_effect(struct quick_regeneration *quick_regen,
                           int heal_speed)
{
    if (!(quick_regen->tickdelay--))
    {
        int bonus =
            min(heal_speed * battle_config.itemheal_regeneration_factor,
                 quick_regen->amount);

        quick_regen->amount -= bonus;

        quick_regen->tickdelay = quick_regen->speed;

        return bonus;
    }

    return 0;
}

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
        pc_calcstatus(sd, 0);
    }
    else if (sd->spellpower_bonus_target > sd->spellpower_bonus_current)
    {
        sd->spellpower_bonus_current +=
            1 +
            ((sd->spellpower_bonus_target -
              sd->spellpower_bonus_current) >> 5);
        pc_calcstatus(sd, 0);
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
 *------------------------------------------
 */
static
int last_save_fd, save_flag;
static
void pc_autosave_sub(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    if (save_flag == 0 && sd->fd > last_save_fd)
    {
        pc_makesavestatus(sd);
        chrif_save(sd);

        save_flag = 1;
        last_save_fd = sd->fd;
    }
}

/*==========================================
 * 自動セーブ (timer関数)
 *------------------------------------------
 */
static
void pc_autosave(TimerData *, tick_t)
{
    save_flag = 0;
    clif_foreachclient(pc_autosave_sub);
    if (save_flag == 0)
        last_save_fd = 0;

    interval_t interval = autosave_time / (clif_countusers() + 1);
    if (interval <= interval_t::zero())
        interval = std::chrono::milliseconds(1);
    Timer(gettick() + interval,
            pc_autosave
    ).detach();
}

int pc_read_gm_account(int fd)
{
    gm_accountm.clear();

    // (RFIFOW(fd, 2) - 4) / 5
    for (int i = 4; i < RFIFOW(fd, 2); i += 5)
    {
        int account_id = RFIFOL(fd, i);
        uint8_t level = RFIFOB(fd, i + 4);
        gm_accountm[account_id] = level;
    }
    return gm_accountm.size();
}

void pc_setstand(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    sd->state.dead_sit = 0;
}

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
 * pc関 係初期化
 *------------------------------------------
 */
int do_init_pc(void)
{
    pc_calc_sigma();
    natural_heal_prev_tick = gettick() + NATURAL_HEAL_INTERVAL;
    Timer(natural_heal_prev_tick,
            pc_natural_heal,
            NATURAL_HEAL_INTERVAL
    ).detach();
    Timer(gettick() + autosave_time,
            pc_autosave
    ).detach();

    return 0;
}

void pc_cleanup(dumb_ptr<map_session_data> sd)
{
    magic_stop_completely(sd);
}

void pc_invisibility(dumb_ptr<map_session_data> sd, int enabled)
{
    if (enabled && !bool(sd->status.option & Option::INVISIBILITY))
    {
        clif_clearchar(sd, BeingRemoveWhy::WARPED);
        sd->status.option |= Option::INVISIBILITY;
        clif_status_change(sd, StatusChange::CLIF_OPTION_SC_INVISIBILITY, 1);
    }
    else if (!enabled)
    {
        sd->status.option &= ~Option::INVISIBILITY;
        clif_status_change(sd, StatusChange::CLIF_OPTION_SC_INVISIBILITY, 0);
        pc_setpos(sd, sd->bl_m->name_, sd->bl_x, sd->bl_y, BeingRemoveWhy::WARPED);
    }
}

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
        if (pc_setglobalreg(sd, "MAGIC_CAST_TICK", sd->cast_tick - tick))
            sd->status.sp = 1;
    }
    else
#endif
        pc_setglobalreg(sd, stringish<VarName>("MAGIC_CAST_TICK"), 0);

    MAP_LOG_STATS(sd, "LOGOUT");
    return 0;
}
