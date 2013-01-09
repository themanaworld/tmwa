#ifndef MAP_HPP
#define MAP_HPP

#include "map.t.hpp"

#include <netinet/in.h>
#include <sys/time.h>

#include <cstdio>
#include <ctime>

#include "../common/cxxstdio.hpp"
#include "../common/db.hpp"
#include "../common/mmo.hpp"
#include "../common/timer.hpp"

#include "battle.t.hpp"
#include "mob.t.hpp"
#include "script.hpp"   // change to script.t.hpp
#include "skill.t.hpp"

#define MAX_PC_CLASS (1+6+6+1+6+1+1+1+1+4023)
#define PC_CLASS_BASE 0
#define PC_CLASS_BASE2 (PC_CLASS_BASE + 4001)
#define PC_CLASS_BASE3 (PC_CLASS_BASE2 + 22)
#define MAX_NPC_PER_MAP 512
#define BLOCK_SIZE 8
#define AREA_SIZE battle_config.area_size
#define LOCAL_REG_NUM 16
#define LIFETIME_FLOORITEM 60
#define DAMAGELOG_SIZE 30
#define LOOTITEM_SIZE 10
#define MAX_SKILL_LEVEL 100
#define MAX_SKILLUNITGROUP 32
#define MAX_MOBSKILLUNITGROUP 8
#define MAX_SKILLUNITGROUPTICKSET 128
#define MAX_SKILLTIMERSKILL 32
#define MAX_MOBSKILLTIMERSKILL 10
#define MAX_MOBSKILL 32
#define MAX_EVENTQUEUE 2
#define MAX_EVENTTIMER 32
#define NATURAL_HEAL_INTERVAL 500
#define MAX_FLOORITEM 500000
#define MAX_LEVEL 255
#define MAX_WALKPATH 48
#define MAX_DROP_PER_MAP 48

#define DEFAULT_AUTOSAVE_INTERVAL 60*1000

struct block_list
{
    struct block_list *next, *prev;
    int id;
    short m, x, y;
    BL type;
    NpcSubtype subtype;
};

struct walkpath_data
{
    unsigned char path_len, path_pos, path_half;
    unsigned char path[MAX_WALKPATH];
};
struct script_reg
{
    int index;
    int data;
};
struct script_regstr
{
    int index;
    char data[256];
};
struct status_change
{
    int timer;
    int val1, val2, val3, val4;
    SkillID val1_sk() { return SkillID(val1); }
    BCT& val1_bct() { return reinterpret_cast<BCT&>(val1); }
    int spell_invocation;      /* [Fate] If triggered by a spell, record here */
};

struct invocation;

struct skill_unit_group;
struct skill_unit
{
    struct block_list bl;

    struct skill_unit_group *group;

    int limit;
    int val1, val2;
    short alive, range;
};
struct skill_unit_group
{
    int src_id;
    int party_id;
    int map, range;
    BCT target_flag;
    unsigned int tick;
    int limit, interval;

    SkillID skill_id;
    int skill_lv;
    int val1, val2;
    char *valstr;
    int unit_id;
    int group_id;
    int unit_count, alive_count;
    struct skill_unit *unit;
};
struct skill_unit_group_tickset
{
    unsigned int tick;
    int group_id;
};
struct skill_timerskill
{
    int timer;
    int src_id;
    int target_id;
    int map;
    short x, y;
    SkillID skill_id;
    short skill_lv;
    union sktst
    {
        int32_t n;
        struct { uint16_t x, y; } xy;
        BF bf;

        static sktst from_n(int32_t n)
        {
            sktst r;
            r.n = n;
            return r;
        }
        static sktst from_xy(uint16_t x, uint16_t y)
        {
            sktst r;
            r.xy.x = x;
            r.xy.y = y;
            return r;
        }
        static sktst from_bf(BF bf)
        {
            sktst r;
            r.bf = bf;
            return r;
        }
    } type;
    BCT flag;
};

struct npc_data;
struct item_data;
struct square;

struct quick_regeneration
{                               // [Fate]
    int amount;                // Amount of HP/SP left to regenerate
    unsigned char speed;        // less is faster (number of half-second ticks to wait between updates)
    unsigned char tickdelay;    // number of ticks to next update
};

#define VERSION_2_SKILLINFO  0x02   // client supports full skillinfo blocks

struct map_session_data
{
    struct block_list bl;
    struct
    {
        unsigned auth:1;
        unsigned change_walk_target:1;
        unsigned attack_continue:1;
        unsigned menu_or_input:1;
        unsigned dead_sit:2;
        unsigned skillcastcancel:1;
        unsigned waitingdisconnect:1;
        unsigned lr_flag:2;
        unsigned connect_new:1;
        unsigned arrow_atk:1;
        BF attack_type;//:3;
        unsigned skill_flag:1;
        unsigned gangsterparadise:1;
        unsigned produce_flag:1;
        unsigned make_arrow_flag:1;
        unsigned potionpitcher_flag:1;
        unsigned storage_open:1;
        unsigned shroud_active:1;
        unsigned shroud_hides_name_talking:1;
        unsigned shroud_disappears_on_pickup:1;
        unsigned shroud_disappears_on_talk:1;
    } state;
    struct
    {
        unsigned killer:1;
        unsigned killable:1;
        unsigned restart_full_recover:1;
        unsigned no_castcancel:1;
        unsigned no_castcancel2:1;
        unsigned no_sizefix:1;
        unsigned no_magic_damage:1;
        unsigned no_weapon_damage:1;
        unsigned no_gemstone:1;
        unsigned infinite_endure:1;
        unsigned unbreakable_weapon:1;
        unsigned unbreakable_armor:1;
        unsigned infinite_autospell:1;
        unsigned deaf:1;
    } special_state;
    int char_id, login_id1, login_id2, sex;
    unsigned char tmw_version;  // tmw client version
    struct mmo_charstatus status;
    struct item_data *inventory_data[MAX_INVENTORY];
    earray<short, EQUIP, EQUIP::COUNT> equip_index;
    int weight, max_weight;
    int cart_weight, cart_max_weight, cart_num, cart_max_num;
    char mapname[24];
    int fd, new_fd;
    short to_x, to_y;
    short speed, prev_speed;
    Opt1 opt1;
    Opt2 opt2;
    Opt3 opt3;
    char dir, head_dir;
    unsigned int client_tick, server_tick;
    struct walkpath_data walkpath;
    int walktimer;
    int npc_id, areanpc_id, npc_shopid;
    int npc_pos;
    int npc_menu;
    int npc_amount;
    int npc_stack, npc_stackmax;
    const ScriptCode *npc_script, *npc_scriptroot;
    char *npc_stackbuf;
    char npc_str[256];
    struct
    {
        unsigned storage:1;
        unsigned divorce:1;
    } npc_flags;
    unsigned int chatID;

    int attacktimer;
    int attacktarget;
    ATK attacktarget_lv;
    unsigned int attackabletime;

    int followtimer;           // [MouseJstr]
    int followtarget;

    unsigned int cast_tick;     // [Fate] Next tick at which spellcasting is allowed
    struct invocation *active_spells;   // [Fate] Singly-linked list of active spells linked to this PC
    int attack_spell_override; // [Fate] When an attack spell is active for this player, they trigger it
    // like a weapon.  Check pc_attack_timer() for details.
    // Weapon equipment slot (slot 4) item override
    StatusChange attack_spell_icon_override;
    short attack_spell_look_override;   // Weapon `look' (attack animation) override
    short attack_spell_charges; // [Fate] Remaining number of charges for the attack spell
    short attack_spell_delay;   // [Fate] ms delay after spell attack
    short attack_spell_range;   // [Fate] spell range
    short spellpower_bonus_target, spellpower_bonus_current;    // [Fate] Spellpower boni.  _current is the active one.
    //_current slowly approximates _target, and _target is determined by equipment.

    short attackrange, attackrange_;
    int skilltimer;
    int skilltarget;
    short skillx, skilly;
    SkillID skillid;
    short skilllv;
    SkillID skillitem;
    short skillitemlv;
    SkillID skillid_old;
    short skilllv_old;
    SkillID skillid_dance;
    short skilllv_dance;
    struct skill_unit_group skillunit[MAX_SKILLUNITGROUP];
    struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
    struct skill_timerskill skilltimerskill[MAX_SKILLTIMERSKILL];
    int cloneskill_id, cloneskill_lv;
    int potion_hp, potion_sp, potion_per_hp, potion_per_sp;

    // [Fate] Used for gradual healing; amount of enqueued regeneration
    struct quick_regeneration quick_regeneration_hp, quick_regeneration_sp;
    // [Fate] XP that can be extracted from this player by healing
    int heal_xp;               // i.e., OTHER players (healers) can partake in this player's XP

    int invincible_timer;
    unsigned int canact_tick;
    unsigned int canmove_tick;
    unsigned int canlog_tick;
    int hp_sub, sp_sub;
    int inchealhptick, inchealsptick, inchealspirithptick,
        inchealspiritsptick;
// -- moonsoul (new tick for berserk self-damage)
    int berserkdamagetick;
    int fame;

    short view_class;
    short weapontype1, weapontype2;
    short disguiseflag, disguise;   // [Valaris]
    earray<int, ATTR, ATTR::COUNT> paramb, paramc, parame, paramcard;
    int hit, flee, flee2, aspd, amotion, dmotion;
    int watk, watk2, atkmods[3];
    int def, def2, mdef, mdef2, critical, matk1, matk2;
    int atk_ele, def_ele, star, overrefine;
    int castrate, hprate, sprate, dsprate;
    int addele[10], addrace[12], addsize[3], subele[10], subrace[12];
    earray<int, BadSC, BadSC::COUNT> addeff, addeff2, reseff;
    int watk_, watk_2, atkmods_[3], addele_[10], addrace_[12], addsize_[3];    //二刀流のために追加
    int atk_ele_, star_, overrefine_;  //二刀流のために追加
    int base_atk, atk_rate;
    int arrow_atk, arrow_ele, arrow_cri, arrow_hit, arrow_range;
    int arrow_addele[10], arrow_addrace[12], arrow_addsize[3];
    earray<int, BadSC, BadSC::COUNT> arrow_addeff, arrow_addeff2;
    int nhealhp, nhealsp, nshealhp, nshealsp, nsshealhp, nsshealsp;
    int aspd_rate, speed_rate, hprecov_rate, sprecov_rate, critical_def,
        double_rate;
    int near_attack_def_rate, long_attack_def_rate, magic_def_rate,
        misc_def_rate;
    int matk_rate, ignore_def_ele, ignore_def_race, ignore_def_ele_,
        ignore_def_race_;
    int ignore_mdef_ele, ignore_mdef_race;
    int magic_addele[10], magic_addrace[12], magic_subrace[12];
    int perfect_hit, get_zeny_num;
    int critical_rate, hit_rate, flee_rate, flee2_rate, def_rate, def2_rate,
        mdef_rate, mdef2_rate;
    int def_ratio_atk_ele, def_ratio_atk_ele_, def_ratio_atk_race,
        def_ratio_atk_race_;
    int add_damage_class_count, add_damage_class_count_,
        add_magic_damage_class_count;
    short add_damage_classid[10], add_damage_classid_[10],
        add_magic_damage_classid[10];
    int add_damage_classrate[10], add_damage_classrate_[10],
        add_magic_damage_classrate[10];
    short add_def_class_count, add_mdef_class_count;
    short add_def_classid[10], add_mdef_classid[10];
    int add_def_classrate[10], add_mdef_classrate[10];
    short monster_drop_item_count;
    short monster_drop_itemid[10];
    int monster_drop_race[10], monster_drop_itemrate[10];
    int double_add_rate, speed_add_rate, aspd_add_rate, perfect_hit_add,
        get_zeny_add_num;
    short splash_range, splash_add_range;
    SkillID autospell_id;
    short autospell_lv, autospell_rate;
    short hp_drain_rate, hp_drain_per, sp_drain_rate, sp_drain_per;
    short hp_drain_rate_, hp_drain_per_, sp_drain_rate_, sp_drain_per_;
    int short_weapon_damage_return, long_weapon_damage_return;
    int weapon_coma_ele[10], weapon_coma_race[12];
    short break_weapon_rate, break_armor_rate;
    short add_steal_rate;

    short spiritball, spiritball_old;
    int spirit_timer[MAX_SKILL_LEVEL];
    int magic_damage_return;   // AppleGirl Was Here
    int random_attack_increase_add, random_attack_increase_per;    // [Valaris]
    int perfect_hiding;        // [Valaris]
    int unbreakable;

    int die_counter;
    short doridori_counter;

    int reg_num;
    struct script_reg *reg;
    int regstr_num;
    struct script_regstr *regstr;

    earray<struct status_change, StatusChange, MAX_STATUSCHANGE> sc_data;
    short sc_count;
    struct square dev;

    int trade_partner;
    int deal_item_index[10];
    int deal_item_amount[10];
    int deal_zeny;
    short deal_locked;

    int party_sended, party_invite, party_invite_account;
    int party_hp, party_x, party_y;

    int partyspy;              // [Syrus22]

    char message[80];

    int catch_target_class;

    int pvp_point, pvp_rank, pvp_timer, pvp_lastusers;

    char eventqueue[MAX_EVENTQUEUE][50];
    int eventtimer[MAX_EVENTTIMER];

    SkillID last_skillid;
    int last_skilllv;

    struct
    {
        char name[24];
    } ignore[80];
    int ignoreAll;
    short sg_count;

    struct
    {
        unsigned in_progress:1;
    } auto_ban_info;

    time_t chat_reset_due;
    time_t chat_repeat_reset_due;
    int chat_lines_in;
    int chat_total_repeats;
    char chat_lastmsg[513];

    unsigned int flood_rates[0x220];
    time_t packet_flood_reset_due;
    int packet_flood_in;

    struct in_addr ip;
};

struct npc_timerevent_list
{
    int timer, pos;
};
struct npc_label_list
{
    char name[24];
    int pos;
};
struct npc_item_list
{
    int nameid, value;
};
struct npc_data
{
    struct block_list bl;
    short n;
    short npc_class, dir;
    short speed;
    char name[24];
    char exname[24];
    int chat_id;
    Opt1 opt1;
    Opt2 opt2;
    Opt3 opt3;
    Option option;
    short flag;
    union
    {
        struct
        {
            const ScriptCode *script;
            short xs, ys;
            int timer, timerid, timeramount, nexttimer;
            unsigned int timertick;
            struct npc_timerevent_list *timer_event;
            int label_list_num;
            struct npc_label_list *label_list;
            int src_id;
        } scr;
        struct npc_item_list shop_item[1];
        struct
        {
            short xs, ys;
            short x, y;
            char name[16];
        } warp;
        char *message;          // for MESSAGE: only send this message
    } u;
    // ここにメンバを追加してはならない(shop_itemが可変長の為)

    char eventqueue[MAX_EVENTQUEUE][50];
    int eventtimer[MAX_EVENTTIMER];
    short arenaflag;
};

#define MOB_MODE_SUMMONED                       0x1000
#define MOB_MODE_TURNS_AGAINST_BAD_MASTER       0x2000

#define MOB_SENSIBLE_MASK 0xf000    // fate: mob mode flags that I actually understand

#define MOB_XP_BONUS_BASE  1024
#define MOB_XP_BONUS_SHIFT 10

struct mob_data
{
    struct block_list bl;
    short n;
    short base_class, mob_class, dir, mode;
    short m, x0, y0, xs, ys;
    char name[24];
    int spawndelay1, spawndelay2;
    struct
    {
        MS state;
        MSS skillstate;
        unsigned attackable:1;
        unsigned steal_flag:1;
        unsigned steal_coin_flag:1;
        unsigned skillcastcancel:1;
        unsigned master_check:1;
        unsigned change_walk_target:1;
        unsigned walk_easy:1;
        unsigned special_mob_ai:3;
    } state;
    int timer;
    short to_x, to_y;
    int hp;
    int target_id, attacked_id;
    ATK target_lv;
    struct walkpath_data walkpath;
    unsigned int next_walktime;
    unsigned int attackabletime;
    unsigned int last_deadtime, last_spawntime, last_thinktime;
    unsigned int canmove_tick;
    short move_fail_count;
    struct
    {
        int id;
        int dmg;
    } dmglog[DAMAGELOG_SIZE];
    struct item *lootitem;
    short lootitem_count;

    earray<struct status_change, StatusChange, MAX_STATUSCHANGE> sc_data;
    short sc_count;
    Opt1 opt1;
    Opt2 opt2;
    Opt3 opt3;
    Option option;
    short min_chase;
    short sg_count;
    int deletetimer;

    int skilltimer;
    int skilltarget;
    short skillx, skilly;
    SkillID skillid;
    short skilllv, skillidx;
    unsigned int skilldelay[MAX_MOBSKILL];
    int def_ele;
    int master_id, master_dist;
    int exclusion_src, exclusion_party;
    struct skill_timerskill skilltimerskill[MAX_MOBSKILLTIMERSKILL];
    struct skill_unit_group skillunit[MAX_MOBSKILLUNITGROUP];
    struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
    char npc_event[50];
    // [Fate] mob-specific stats
    earray<unsigned short, mob_stat, mob_stat::LAST> stats;
    short size;
};

struct map_data
{
    char name[24];
    char alias[24];             // [MouseJstr]
    unsigned char *gat;         // NULLなら下のmap_data_other_serverとして扱う
    struct block_list **block;
    struct block_list **block_mob;
    int *block_count, *block_mob_count;
    int m;
    short xs, ys;
    short bxs, bys;
    int npc_num;
    int users;
    struct
    {
        unsigned alias:1;
        unsigned nomemo:1;
        unsigned noteleport:1;
        unsigned noreturn:1;
        unsigned monster_noteleport:1;
        unsigned nosave:1;
        unsigned nobranch:1;
        unsigned nopenalty:1;
        unsigned pvp:1;
        unsigned pvp_noparty:1;
        unsigned pvp_nightmaredrop:1;
        unsigned pvp_nocalcrank:1;
        unsigned nozenypenalty:1;
        unsigned notrade:1;
        unsigned noskill:1;
        unsigned nowarp:1;
        unsigned nowarpto:1;
        unsigned nopvp:1;       // [Valaris]
        unsigned noicewall:1;   // [Valaris]
        unsigned snow:1;        // [Valaris]
        unsigned fog:1;         // [Valaris]
        unsigned sakura:1;      // [Valaris]
        unsigned leaves:1;      // [Valaris]
        unsigned rain:1;        // [Valaris]
        unsigned no_player_drops:1; // [Jaxad0127]
        unsigned town:1;        // [remoitnane]
    } flag;
    struct point save;
    struct npc_data *npc[MAX_NPC_PER_MAP];
    struct
    {
        int drop_id;
        int drop_type;
        int drop_per;
    } drop_list[MAX_DROP_PER_MAP];
};
struct map_data_other_server
{
    char name[24];
    unsigned char *gat;         // NULL固定にして判断
    struct in_addr ip;
    unsigned int port;
};

extern struct map_data map[];
extern int map_num;

inline
uint8_t read_gatp(struct map_data *m, int x, int y)
{
    return m->gat[x + y * m->xs];
}
inline
uint8_t read_gat(int m, int x, int y)
{
    return read_gatp(&map[m], x, y);
}

struct flooritem_data
{
    struct block_list bl;
    short subx, suby;
    int cleartimer;
    int first_get_id, second_get_id, third_get_id;
    unsigned int first_get_tick, second_get_tick, third_get_tick;
    struct item item_data;
};

struct chat_data
{
    struct block_list bl;

    char pass[8];      /* password */
    char title[61];    /* room title max 60 */
    unsigned char limit;        /* join limit */
    unsigned char trigger;
    unsigned char users;        /* current users */
    unsigned char pub;          /* room attribute */
    struct map_session_data *usersd[20];
    struct block_list *owner_;
    struct block_list **owner;
    char npc_event[50];
};

extern int autosave_interval;
extern int save_settings;
extern int night_flag;          // 0=day, 1=night [Yor]

extern char motd_txt[];
extern char help_txt[];

extern char talkie_mes[];

extern char wisp_server_name[];

// 鯖全体情報
void map_setusers(int);
int map_getusers(void);
// block削除関連
int map_freeblock(void *bl);
int map_freeblock_lock(void);
int map_freeblock_unlock(void);
// block関連
int map_addblock(struct block_list *);
int map_delblock(struct block_list *);
void map_foreachinarea(std::function<void(struct block_list *)>,
        int,
        int, int, int, int,
        BL);
// -- moonsoul (added map_foreachincell)
void map_foreachincell(std::function<void(struct block_list *)>,
        int,
        int, int,
        BL);
void map_foreachinmovearea(std::function<void(struct block_list *)>,
        int,
        int, int, int, int,
        int, int,
        BL);
//block関連に追加
int map_count_oncell(int m, int x, int y);
// 一時的object関連
int map_addobject(struct block_list *);
int map_delobject(int, BL type);
int map_delobjectnofree(int id, BL type);
void map_foreachobject(std::function<void(struct block_list *)>,
        BL);
//
int map_quit(struct map_session_data *);
// npc
int map_addnpc(int, struct npc_data *);

void map_log(const_string line);
#define MAP_LOG(format, ...) \
    map_log(static_cast<const std::string&>(STRPRINTF(format, ## __VA_ARGS__)))

#define MAP_LOG_PC(sd, fmt, ...) MAP_LOG("PC%d %d:%d,%d " fmt, sd->status.char_id, sd->bl.m, sd->bl.x, sd->bl.y, ## __VA_ARGS__)

// 床アイテム関連
void map_clearflooritem_timer(timer_id, tick_t, custom_id_t, custom_data_t);
#define map_clearflooritem(id) map_clearflooritem_timer(0,0,id,1)
int map_addflooritem_any(struct item *, int amount, int m, int x, int y,
                           struct map_session_data **owners,
                           int *owner_protection,
                           int lifetime, int dispersal);
int map_addflooritem(struct item *, int, int, int, int,
                       struct map_session_data *, struct map_session_data *,
                       struct map_session_data *, int);

// キャラid＝＞キャラ名 変換関連
void map_addchariddb(int charid, const char *name);
int map_reqchariddb(struct map_session_data *sd, int charid);
char *map_charid2nick(int);

struct map_session_data *map_id2sd(int);
struct block_list *map_id2bl(int);
int map_mapname2mapid(const char *);
int map_mapname2ipport(const char *, struct in_addr *, int *);
int map_setipport(const char *name, struct in_addr ip, int port);
void map_addiddb(struct block_list *);
void map_deliddb(struct block_list *bl);
void map_foreachiddb(db_func_t);
void map_addnickdb(struct map_session_data *);
int map_scriptcont(struct map_session_data *sd, int id);  /* Continues a script either on a spell or on an NPC */
struct map_session_data *map_nick2sd(const char *);
int compare_item(struct item *a, struct item *b);

struct map_session_data *map_get_first_session(void);
struct map_session_data *map_get_last_session(void);
struct map_session_data *map_get_next_session(struct map_session_data
                                               *current);
struct map_session_data *map_get_prev_session(struct map_session_data
                                               *current);

// gat関連
int map_getcell(int, int, int);
int map_setcell(int, int, int, int);

// その他
int map_check_dir(int s_dir, int t_dir);
int map_calc_dir(struct block_list *src, int x, int y);

// path.cより
int path_search(struct walkpath_data *, int, int, int, int, int, int);
int path_blownpos(int m, int x0, int y0, int dx, int dy, int count);

#endif // MAP_HPP
