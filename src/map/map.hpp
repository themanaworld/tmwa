#pragma once
//    map.hpp - Core of the map server.
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

#include "map.t.hpp"

#include "fwd.hpp"

#include <chrono>
#include <functional>
#include <list>

#include "../ints/udl.hpp"

#include "../strings/rstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/vstring.hpp"

#include "../generic/db.hpp"
#include "../generic/dumb_ptr.hpp"
#include "../generic/matrix.hpp"

#include "../net/socket.hpp"
#include "../net/timer.t.hpp"

#include "battle.t.hpp"
#include "../mmo/clif.t.hpp"
#include "mapflag.hpp"
#include "mob.t.hpp"
#include "script-buffer.hpp"
#include "script-persist.hpp"
#include "../mmo/skill.t.hpp"


namespace tmwa
{
namespace map
{
constexpr int MAX_NPC_PER_MAP = 512;
constexpr int BLOCK_SIZE = 8;
#define AREA_SIZE battle_config.area_size
constexpr std::chrono::seconds LIFETIME_FLOORITEM = 1_min;
constexpr int MAX_SKILL_LEVEL = 100;
constexpr int MAX_EVENTTIMER = 32;
constexpr interval_t NATURAL_HEAL_INTERVAL = 500_ms;
constexpr int MAX_LEVEL = 255;
constexpr int MAX_WALKPATH = 48;
constexpr int MAX_DROP_PER_MAP = 48;

constexpr std::chrono::seconds DEFAULT_AUTOSAVE_INTERVAL = 1_min;

extern map_local undefined_gat;

struct block_list
{
    dumb_ptr<block_list> bl_next, bl_prev;
    BlockId bl_id;
    Borrowed<map_local> bl_m = borrow(undefined_gat);
    short bl_x, bl_y;
    BL bl_type;

    // This deletes the copy-ctor also
    // TODO give proper ctors.
    block_list& operator = (block_list&&) = delete;
    virtual ~block_list() {}

private:
    // historically, a lot of code used this.
    // historically, a lot of code crashed.
    dumb_ptr<map_session_data> as_player();
    dumb_ptr<npc_data> as_npc();
    dumb_ptr<mob_data> as_mob();
    dumb_ptr<flooritem_data> as_item();
    dumb_ptr<magic::invocation> as_spell();
public:
    dumb_ptr<map_session_data> is_player();
    dumb_ptr<npc_data> is_npc();
    dumb_ptr<mob_data> is_mob();
    dumb_ptr<flooritem_data> is_item();
    dumb_ptr<magic::invocation> is_spell();
};

struct walkpath_data
{
    unsigned char path_len, path_pos, path_half;
    Array<DIR, MAX_WALKPATH> path;
};
struct status_change
{
    Timer timer;
    int val1;
    BlockId spell_invocation;      /* [Fate] If triggered by a spell, record here */
};

struct quick_regeneration
{                               // [Fate]
    int amount;                // Amount of HP/SP left to regenerate
    unsigned char speed;        // less is faster (number of half-second ticks to wait between updates)
    unsigned char tickdelay;    // number of ticks to next update
};

struct map_session_data : block_list, SessionData
{
    struct
    {
        unsigned auth:1;
        unsigned change_walk_target:1;
        unsigned attack_continue:1;
        unsigned menu_or_input:1;
        unsigned dead_sit:2;
        unsigned skillcastcancel:1;
        unsigned waitingdisconnect:1;
        unsigned lr_flag_is_arrow_2:1;
        unsigned connect_new:1;
        unsigned arrow_atk:1;
        BF attack_type;//:3;
        unsigned skill_flag:1;
        unsigned gangsterparadise:1;
        unsigned produce_flag:1;
        unsigned make_arrow_flag:1;
        unsigned storage_open:1;
        unsigned shroud_active:1;
        unsigned shroud_hides_name_talking:1;
        unsigned shroud_disappears_on_pickup:1;
        unsigned shroud_disappears_on_talk:1;
        unsigned seen_motd:1;
        unsigned pvpchannel;
    } state;
    struct
    {
        unsigned unbreakable_weapon:1;
        unsigned unbreakable_armor:1;
        unsigned deaf:1;
    } special_state;
    CharId char_id_;
    int login_id1, login_id2;
    SEX sex;
    unsigned char tmw_version;  // tmw client version
    CharKey status_key;
    CharData status;
    GenericArray<Option<Borrowed<struct item_data>>, InventoryIndexing<IOff0, MAX_INVENTORY>> inventory_data =
    {{
        None, None, None, None, None, None, None, None, None, None,
        None, None, None, None, None, None, None, None, None, None,
        None, None, None, None, None, None, None, None, None, None,
        None, None, None, None, None, None, None, None, None, None,
        None, None, None, None, None, None, None, None, None, None,
        None, None, None, None, None, None, None, None, None, None,
        None, None, None, None, None, None, None, None, None, None,
        None, None, None, None, None, None, None, None, None, None,
        None, None, None, None, None, None, None, None, None, None,
        None, None, None, None, None, None, None, None, None, None,
     }}; // explicit is better than implicit
    earray<IOff0, EQUIP, EQUIP::COUNT> equip_index_maybe;
    int weight, max_weight;
    MapName mapname_;
    Session *sess; // use this, you idiots!
    short to_x, to_y;
    interval_t speed;
    Opt1 opt1;
    Opt2 opt2;
    Opt3 opt3;
    DIR dir, head_dir;
    struct walkpath_data walkpath;
    Timer walktimer;
    BlockId npc_id, areanpc_id, npc_shopid;
    // this is important
    int npc_pos;
    int npc_menu;
    int npc_amount;
    // I have no idea exactly what these are doing ...
    // but one should probably be replaced with a ScriptPointer ???
    Option<Borrowed<const ScriptBuffer>> npc_script = None, npc_scriptroot = None;
    std::vector<struct script_data> npc_stackbuf;
    RString npc_str;
    struct
    {
        unsigned storage:1;
    } npc_flags;

    Timer attacktimer;
    BlockId attacktarget;
    ATK attacktarget_lv;
    tick_t attackabletime;

    // used by @hugo and @linus
    BlockId followtarget;

    tick_t cast_tick;     // [Fate] Next tick at which spellcasting is allowed
    dumb_ptr<magic::invocation> active_spells;   // [Fate] Singly-linked list of active spells linked to this PC
    BlockId attack_spell_override; // [Fate] When an attack spell is active for this player, they trigger it
    // like a weapon.  Check pc_attack_timer() for details.
    // Weapon equipment slot (slot 4) item override
    StatusChange attack_spell_icon_override;
    ItemNameId attack_spell_look_override;   // Weapon `look' (attack animation) override
    short attack_spell_charges; // [Fate] Remaining number of charges for the attack spell
    interval_t attack_spell_delay;   // [Fate] ms delay after spell attack
    short attack_spell_range;   // [Fate] spell range
    short spellpower_bonus_target, spellpower_bonus_current;    // [Fate] Spellpower boni.  _current is the active one.
    //_current slowly approximates _target, and _target is determined by equipment.

    short attackrange;

    // [Fate] Used for gradual healing; amount of enqueued regeneration
    struct quick_regeneration quick_regeneration_hp, quick_regeneration_sp;
    // [Fate] XP that can be extracted from this player by healing
    int heal_xp;               // i.e., OTHER players (healers) can partake in this player's XP

    Timer invincible_timer;
    tick_t canact_tick;
    tick_t canmove_tick;
    tick_t canlog_tick;
    interval_t hp_sub, sp_sub;
    interval_t inchealhptick, inchealsptick;

    ItemLook weapontype1;
    earray<int, ATTR, ATTR::COUNT> paramb, paramc, parame, paramcard;
    int hit, flee, flee2;
    interval_t aspd, amotion, dmotion;
    int watk, watk2;
    int def, def2, mdef, mdef2, critical, matk1, matk2;
    int hprate, sprate, dsprate;
    int base_atk, atk_rate;
    int arrow_atk;
    int arrow_cri, arrow_hit, arrow_range;
    int nhealhp, nhealsp;
    int aspd_rate, speed_rate, hprecov_rate, sprecov_rate, critical_def,
        double_rate;
    int matk_rate;
    int perfect_hit;
    int critical_rate, hit_rate, flee_rate, flee2_rate, def_rate, def2_rate,
        mdef_rate, mdef2_rate;
    int double_add_rate, speed_add_rate, aspd_add_rate, perfect_hit_add;
    short hp_drain_rate, hp_drain_per, sp_drain_rate, sp_drain_per;

    int die_counter;

    // register keys are ints (interned)
    // Not anymore! Well, sort of.
    DMap<SIR, int> regm;
    // can't be DMap because we want predictable .c_str()s
    // TODO this can change now
    Map<SIR, RString> regstrm;

    earray<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    AccountId trade_partner;
    Array<IOff2, TRADE_MAX> deal_item_index;
    Array<int, TRADE_MAX> deal_item_amount;
    int deal_zeny;
    short deal_locked;

    int party_sended;
    PartyId party_invite;
    AccountId party_invite_account;
    int party_hp, party_x, party_y;

    PartyId partyspy;              // [Syrus22]

    int pvp_point, pvp_rank;
    Timer pvp_timer;

    std::list<NpcEvent> eventqueuel;
    Array<Timer, MAX_EVENTTIMER> eventtimer;

    struct
    {
        unsigned in_progress:1;
    } auto_ban_info;

    tick_t chat_reset_due;
    tick_t chat_repeat_reset_due;
    int chat_lines_in;
    int chat_total_repeats;
    RString chat_lastmsg;

    tick_t flood_rates[0x220];
    tick_t packet_flood_reset_due;
    int packet_flood_in;

    IP4Address get_ip()
    {
        return sess->client_ip;
    }
};

struct npc_timerevent_list
{
    interval_t timer;
    int pos;
};
struct npc_label_list
{
    ScriptLabel name;
    int pos;
};
struct npc_item_list
{
    ItemNameId nameid;
    int value;
};

struct npc_data : block_list
{
    NpcSubtype npc_subtype;
    short n;
    Species npc_class;
    DIR dir;
    interval_t speed;
    NpcName name;
    Opt1 opt1;
    Opt2 opt2;
    Opt3 opt3;
    Opt0 option;
    short flag;

    std::list<RString> eventqueuel;
    Array<Timer, MAX_EVENTTIMER> eventtimer;
    short arenaflag;

private:
    dumb_ptr<npc_data_script> as_script();
    dumb_ptr<npc_data_shop> as_shop();
    dumb_ptr<npc_data_warp> as_warp();
    dumb_ptr<npc_data_message> as_message();
public:
    dumb_ptr<npc_data_script> is_script();
    dumb_ptr<npc_data_shop> is_shop();
    dumb_ptr<npc_data_warp> is_warp();
    dumb_ptr<npc_data_message> is_message();
};

class npc_data_script : public npc_data
{
public:
    struct
    {
        // The bytecode unique to this NPC.
        std::unique_ptr<const ScriptBuffer> script;
        // Diameter.
        short xs, ys;
        bool event_needs_map;

        // Whether the timer advances if not beyond end.
        bool timer_active;
        // Tick counter through the timers.
        // It is actually updated when frobbing the thing in any way.
        // If this is timer_eventv().back().timer, it is expired
        // rather than blank. It's probably a bad idea to rely on this.
        interval_t timer;
        // Actual timer that fires the event.
        Timer timerid;
        // Event to be fired, or .end() if no timer.
        std::vector<npc_timerevent_list>::iterator next_event;
        // When the timer started. Needed to get the true diff, or to stop.
        tick_t timertick;
        // List of label events to call.
        std::vector<npc_timerevent_list> timer_eventv;

        // List of (name, offset) label locations in the bytecode
        std::vector<npc_label_list> label_listv;
    } scr;
};

class npc_data_shop : public npc_data
{
public:
    std::vector<npc_item_list> shop_items;
};

class npc_data_warp : public npc_data
{
public:
    struct
    {
        short xs, ys;
        short x, y;
        MapName name;
    } warp;
};

class npc_data_message : public npc_data
{
public:
    RString message;
};

constexpr int MOB_XP_BONUS_BASE = 1024;
constexpr int MOB_XP_BONUS_SHIFT = 10;

struct mob_data : block_list
{
    short n;
    Species mob_class;
    DIR dir;
    MobMode mode;
    struct
    {
        Borrowed<map_local> m = borrow(undefined_gat);
        short x0, y0, xs, ys;
        interval_t delay1, delay2;
    } spawn;
    MobName name;
    struct
    {
        MS state;
        MobSkillState skillstate;
        unsigned attackable:1;
        unsigned steal_flag:1;
        unsigned steal_coin_flag:1;
        unsigned skillcastcancel:1;
        unsigned master_check:1;
        unsigned change_walk_target:1;
        unsigned walk_easy:1;
        unsigned special_mob_ai:3;
    } state;
    Timer timer;
    short to_x, to_y;
    int hp;
    BlockId target_id, attacked_id;
    ATK target_lv;
    struct walkpath_data walkpath;
    tick_t next_walktime;
    tick_t attackabletime;
    tick_t last_deadtime, last_spawntime, last_thinktime;
    tick_t canmove_tick;
    short move_fail_count;
    struct DmgLogEntry
    {
        BlockId id;
        int dmg;
    };
    // logically a map ...
    std::vector<DmgLogEntry> dmglogv;
    std::vector<Item> lootitemv;

    earray<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;
    Opt1 opt1;
    Opt2 opt2;
    Opt3 opt3;
    Opt0 option;
    short min_chase;
    Timer deletetimer;

    Timer skilltimer;
    BlockId skilltarget;
    short skillx, skilly;
    SkillID skillid;
    short skilllv;
    struct mob_skill *skillidx;
    std::unique_ptr<tick_t[]> skilldelayup; // [MAX_MOBSKILL];
    LevelElement def_ele;
    BlockId master_id;
    int master_dist;
    int exclusion_src, exclusion_party;
    NpcEvent npc_event;
    // [Fate] mob-specific stats
    earray<unsigned short, mob_stat, mob_stat::LAST> stats;
    short size;
};

struct BlockLists
{
    dumb_ptr<block_list> normal, mobs_only;
};

struct map_abstract
{
    MapName name_;
    // gat is nullptr for map_remote and non-nullptr for map_local
    std::unique_ptr<MapCell[]> gat;

    map_abstract() = default;
    map_abstract(map_abstract&&) = default;
    virtual ~map_abstract() {}
};

struct map_local : map_abstract
{
    Matrix<BlockLists> blocks;
    short xs, ys;
    int npc_num;
    int users;
    MapFlags flag;
    Point save;
    Point resave;
    Array<dumb_ptr<npc_data>, MAX_NPC_PER_MAP> npc;
};

struct map_remote : map_abstract
{
    IP4Address ip;
    uint16_t port;
};

inline
MapCell read_gatp(Borrowed<map_local> m, int x, int y)
{
    assert (0 <= x && x < m->xs);
    assert (0 <= y && y < m->ys);
    return m->gat[x + y * m->xs];
}

struct flooritem_data : block_list
{
    short subx, suby;
    Timer cleartimer;
    BlockId first_get_id, second_get_id, third_get_id;
    tick_t first_get_tick, second_get_tick, third_get_tick;
    Item item_data;
};

extern const CharName WISP_SERVER_NAME;

// 鯖全体情報
void map_setusers(int);
int map_getusers(void);

class MapBlockLock
{
    MapBlockLock(const MapBlockLock&) = delete;
    MapBlockLock& operator = (const MapBlockLock&) = delete;
public:
    MapBlockLock();
    ~MapBlockLock();

    static
    void freeblock(dumb_ptr<block_list>);
};

int map_addblock(dumb_ptr<block_list>);
int map_delblock(dumb_ptr<block_list>);
void map_foreachinarea(std::function<void(dumb_ptr<block_list>)>,
        Borrowed<map_local>,
        int, int, int, int,
        BL);
// -- moonsoul (added map_foreachincell)
void map_foreachincell(std::function<void(dumb_ptr<block_list>)>,
        Borrowed<map_local>,
        int, int,
        BL);
void map_foreachinmovearea(std::function<void(dumb_ptr<block_list>)>,
        Borrowed<map_local>,
        int, int, int, int,
        int, int,
        BL);
//block関連に追加
int map_count_oncell(Borrowed<map_local> m, int x, int y);
// 一時的object関連
BlockId map_addobject(dumb_ptr<block_list>);
void map_delobject(BlockId, BL type);
void map_delobjectnofree(BlockId id, BL type);
void map_foreachobject(std::function<void(dumb_ptr<block_list>)>,
        BL);
//
void map_quit(dumb_ptr<map_session_data>);
// npc
int map_addnpc(Borrowed<map_local>, dumb_ptr<npc_data>);

void map_log(XString line);
#define MAP_LOG(format, ...)    \
    map_log(STRPRINTF(format, ## __VA_ARGS__))

#define MAP_LOG_PC(sd, fmt, ...)    \
    MAP_LOG("PC%d %s:%d,%d " fmt,   \
            sd->status_key.char_id, (sd->bl_m->name_), sd->bl_x, sd->bl_y, ## __VA_ARGS__)

// 床アイテム関連
void map_clearflooritem_timer(TimerData *, tick_t, BlockId);
inline
void map_clearflooritem(BlockId id)
{
    map_clearflooritem_timer(nullptr, tick_t(), id);
}
BlockId map_addflooritem_any(Item *, int amount,
        Borrowed<map_local> m, int x, int y,
        dumb_ptr<map_session_data> *owners, interval_t *owner_protection,
        interval_t lifetime, int dispersal);
BlockId map_addflooritem(Item *, int,
        Borrowed<map_local>, int, int,
        dumb_ptr<map_session_data>, dumb_ptr<map_session_data>,
        dumb_ptr<map_session_data>);

// キャラid＝＞キャラ名 変換関連
void map_addchariddb(CharId charid, CharName name);
CharName map_charid2nick(CharId);

dumb_ptr<map_session_data> map_id2sd(BlockId);
dumb_ptr<block_list> map_id2bl(BlockId);

inline
dumb_ptr<map_session_data> map_id_is_player(BlockId id)
{
    dumb_ptr<block_list> bl = map_id2bl(id);
    return bl ? bl->is_player() : nullptr;
}
inline
dumb_ptr<npc_data> map_id_is_npc(BlockId id)
{
    dumb_ptr<block_list> bl = map_id2bl(id);
    return bl ? bl->is_npc() : nullptr;
}
inline
dumb_ptr<mob_data> map_id_is_mob(BlockId id)
{
    dumb_ptr<block_list> bl = map_id2bl(id);
    return bl ? bl->is_mob() : nullptr;
}
inline
dumb_ptr<flooritem_data> map_id_is_item(BlockId id)
{
    dumb_ptr<block_list> bl = map_id2bl(id);
    return bl ? bl->is_item() : nullptr;
}
inline
dumb_ptr<magic::invocation> map_id_is_spell(BlockId id)
{
    dumb_ptr<block_list> bl = map_id2bl(id);
    return bl ? bl->is_spell() : nullptr;
}


Option<Borrowed<map_local>> map_mapname2mapid(MapName);
int map_mapname2ipport(MapName, Borrowed<IP4Address>, Borrowed<int>);
int map_setipport(MapName name, IP4Address ip, int port);
void map_addiddb(dumb_ptr<block_list>);
void map_deliddb(dumb_ptr<block_list> bl);
void map_addnickdb(dumb_ptr<map_session_data>);
int map_scriptcont(dumb_ptr<map_session_data> sd, BlockId id);  /* Continues a script either on a spell or on an NPC */
dumb_ptr<map_session_data> map_nick2sd(CharName);
int compare_item(Item *a, Item *b);

dumb_ptr<map_session_data> map_get_first_session(void);
dumb_ptr<map_session_data> map_get_last_session(void);
dumb_ptr<map_session_data> map_get_next_session(
        dumb_ptr<map_session_data> current);
dumb_ptr<map_session_data> map_get_prev_session(
        dumb_ptr<map_session_data> current);

// gat関連
MapCell map_getcell(Borrowed<map_local>, int, int);
void map_setcell(Borrowed<map_local>, int, int, MapCell);

// その他
bool map_check_dir(DIR s_dir, DIR t_dir);
DIR map_calc_dir(dumb_ptr<block_list> src, int x, int y);

std::pair<uint16_t, uint16_t> map_randfreecell(Borrowed<map_local> m,
        uint16_t x, uint16_t y, uint16_t w, uint16_t h);

inline dumb_ptr<map_session_data> block_list::as_player() { return dumb_ptr<map_session_data>(static_cast<map_session_data *>(this)) ; }
inline dumb_ptr<npc_data> block_list::as_npc() { return dumb_ptr<npc_data>(static_cast<npc_data *>(this)) ; }
inline dumb_ptr<mob_data> block_list::as_mob() { return dumb_ptr<mob_data>(static_cast<mob_data *>(this)) ; }
inline dumb_ptr<flooritem_data> block_list::as_item() { return dumb_ptr<flooritem_data>(static_cast<flooritem_data *>(this)) ; }
//inline dumb_ptr<invocation> block_list::as_spell() { return dumb_ptr<invocation>(static_cast<invocation *>(this)) ; }

inline dumb_ptr<map_session_data> block_list::is_player() { return bl_type == BL::PC ? as_player() : nullptr; }
inline dumb_ptr<npc_data> block_list::is_npc() { return bl_type == BL::NPC ? as_npc() : nullptr; }
inline dumb_ptr<mob_data> block_list::is_mob() { return bl_type == BL::MOB ? as_mob() : nullptr; }
inline dumb_ptr<flooritem_data> block_list::is_item() { return bl_type == BL::ITEM ? as_item() : nullptr; }
//inline dumb_ptr<invocation> block_list::is_spell() { return bl_type == BL::SPELL ? as_spell() : nullptr; }

// struct invocation is defined in another header

inline dumb_ptr<npc_data_script> npc_data::as_script() { return dumb_ptr<npc_data_script>(static_cast<npc_data_script *>(this)) ; }
inline dumb_ptr<npc_data_shop> npc_data::as_shop() { return dumb_ptr<npc_data_shop>(static_cast<npc_data_shop *>(this)) ; }
inline dumb_ptr<npc_data_warp> npc_data::as_warp() { return dumb_ptr<npc_data_warp>(static_cast<npc_data_warp *>(this)) ; }
inline dumb_ptr<npc_data_message> npc_data::as_message() { return dumb_ptr<npc_data_message>(static_cast<npc_data_message *>(this)) ; }

inline dumb_ptr<npc_data_script> npc_data::is_script() { return npc_subtype == NpcSubtype::SCRIPT ? as_script() : nullptr ; }
inline dumb_ptr<npc_data_shop> npc_data::is_shop() { return npc_subtype == NpcSubtype::SHOP ? as_shop() : nullptr ; }
inline dumb_ptr<npc_data_warp> npc_data::is_warp() { return npc_subtype == NpcSubtype::WARP ? as_warp() : nullptr ; }
inline dumb_ptr<npc_data_message> npc_data::is_message() { return npc_subtype == NpcSubtype::MESSAGE ? as_message() : nullptr ; }

void map_addmap(MapName mapname);
void map_delmap(MapName mapname);

struct charid2nick
{
    CharName nick;
    int req_id;
};
} // namespace map
} // namespace tmwa
