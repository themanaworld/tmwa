
#include "mob.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <fstream>

#include "../strings/fstring.hpp"
#include "../strings/xstring.hpp"

#include "../common/cxxstdio.hpp"
#include "../common/extract.hpp"
#include "../common/io.hpp"
#include "../common/nullpo.hpp"
#include "../common/random.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"

#include "battle.hpp"
#include "clif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "path.hpp"
#include "pc.hpp"
#include "skill.hpp"

#include "../poison.hpp"

constexpr interval_t MIN_MOBTHINKTIME = std::chrono::milliseconds(100);

// Move probability in the negligent mode MOB (rate of 1000 minute)
constexpr random_::Fraction MOB_LAZYMOVEPERC {50, 1000};
// Warp probability in the negligent mode MOB (rate of 1000 minute)
constexpr random_::Fraction MOB_LAZYWARPPERC {20, 1000};

struct mob_db_ mob_db[2001];

/*==========================================
 * Local prototype declaration   (only required thing)
 *------------------------------------------
 */
static
int distance(int, int, int, int);
static
int mob_makedummymobdb(int);
static
void mob_timer(TimerData *, tick_t, int, unsigned char);
static
int mobskill_use_id(dumb_ptr<mob_data> md, dumb_ptr<block_list> target,
        mob_skill& skill_idx);

/*==========================================
 * Mob is searched with a name.
 *------------------------------------------
 */
int mobdb_searchname(MobName str)
{
    int i;

    for (i = 0; i < sizeof(mob_db) / sizeof(mob_db[0]); i++)
    {
        if (mob_db[i].name == str || mob_db[i].jname == str)
            return i;
    }

    return 0;
}

/*==========================================
 * Id Mob is checked.
 *------------------------------------------
 */
int mobdb_checkid(const int id)
{
    if (id <= 0 || id >= (sizeof(mob_db) / sizeof(mob_db[0]))
        || !mob_db[id].name)
        return 0;

    return id;
}

static
void mob_init(dumb_ptr<mob_data> md);

/*==========================================
 * The minimum data set for MOB spawning
 *------------------------------------------
 */
static
void mob_spawn_dataset(dumb_ptr<mob_data> md, MobName mobname, int mob_class)
{
    nullpo_retv(md);

    if (mobname == ENGLISH_NAME)
        md->name = mob_db[mob_class].name;
    else if (mobname == JAPANESE_NAME)
        md->name = mob_db[mob_class].jname;
    else
        md->name = mobname;

    md->bl_prev = NULL;
    md->bl_next = NULL;
    md->n = 0;
    md->mob_class = mob_class;
    md->bl_id = npc_get_new_npc_id();

    really_memzero_this(&md->state);
    // md->timer = nullptr;
    md->target_id = 0;
    md->attacked_id = 0;

    mob_init(md);
}

// Mutation values indicate how `valuable' a change to each stat is, XP wise.
// For one 256th of change, we give out that many 1024th fractions of XP change
// (i.e., 1024 means a 100% XP increase for a single point of adjustment, 4 means 100% XP bonus for doubling the value)
static
earray<int, mob_stat, mob_stat::XP_BONUS> mutation_value //=
{{
    2,                          // mob_stat::LV
    3,                          // mob_stat::MAX_HP
    1,                          // mob_stat::STR
    2,                          // mob_stat::AGI
    1,                          // mob_stat::VIT
    0,                          // mob_stat::INT
    2,                          // mob_stat::DEX
    2,                          // mob_stat::LUK
    1,                          // mob_stat::ATK1
    1,                          // mob_stat::ATK2
    2,                          // mob_stat::ADELAY
    2,                          // mob_stat::DEF
    2,                          // mob_stat::MDEF
    2,                          // mob_stat::SPEED
}};

// The mutation scale indicates how far `up' we can go, with 256 indicating 100%  Note that this may stack with multiple
// calls to `mutate'.
static
earray<int, mob_stat, mob_stat::XP_BONUS> mutation_scale //=
{{
    16,                         // mob_stat::LV
    256,                        // mob_stat::MAX_HP
    32,                         // mob_stat::STR
    48,                         // mob_stat::AGI
    48,                         // mob_stat::VIT
    48,                         // mob_stat::INT
    48,                         // mob_stat::DEX
    64,                         // mob_stat::LUK
    48,                         // mob_stat::ATK1
    48,                         // mob_stat::ATK2
    80,                         // mob_stat::ADELAY
    48,                         // mob_stat::DEF
    48,                         // mob_stat::MDEF
    80,                         // mob_stat::SPEED
}};

// The table below indicates the `average' value for each of the statistics, or -1 if there is none.
// This average is used to determine XP modifications for mutations.  The experience point bonus is
// based on mutation_value and mutation_base as follows:
// (1) first, compute the percentage change of the attribute (p0)
// (2) second, determine the absolute stat change
// (3) third, compute the percentage stat change relative to mutation_base (p1)
// (4) fourth, compute the XP mofication based on the smaller of (p0, p1).
static
earray<int, mob_stat, mob_stat::XP_BONUS> mutation_base //=
{{
    30,                         // mob_stat::LV
    -1,                         // mob_stat::MAX_HP
    20,                         // mob_stat::STR
    20,                         // mob_stat::AGI
    20,                         // mob_stat::VIT
    20,                         // mob_stat::INT
    20,                         // mob_stat::DEX
    20,                         // mob_stat::LUK
    -1,                         // mob_stat::ATK1
    -1,                         // mob_stat::ATK2
    -1,                         // mob_stat::ADELAY
    -1,                         // mob_stat::DEF
    20,                         // mob_stat::MDEF
    -1,                         // mob_stat::SPEED
}};

/*========================================
 * Mutates a MOB.  For large `direction' values, calling this multiple times will give bigger XP boni.
 *----------------------------------------
 */
// intensity: positive: strengthen, negative: weaken.  256 = 100%.
static
void mob_mutate(dumb_ptr<mob_data> md, mob_stat stat, int intensity)
{
    int old_stat;
    int new_stat;
    int real_intensity;        // relative intensity
    const int mut_base = mutation_base[stat];
    int sign = 1;

    if (!md || stat >= mob_stat::XP_BONUS || intensity == 0)
        return;

    while (intensity > mutation_scale[stat])
    {
        mob_mutate(md, stat, mutation_scale[stat]);    // give better XP assignments
        intensity -= mutation_scale[stat];
    }
    while (intensity < -mutation_scale[stat])
    {
        mob_mutate(md, stat, mutation_scale[stat]);    // give better XP assignments
        intensity += mutation_scale[stat];
    }

    if (!intensity)
        return;

    // mob_stat::ADELAY and mob_stat::SPEED are special because going DOWN is good here.
    if (stat == mob_stat::ADELAY || stat == mob_stat::SPEED)
        sign = -1;

    // Now compute the new stat
    old_stat = md->stats[stat];
    new_stat = old_stat + ((old_stat * sign * intensity) / 256);

    if (new_stat < 0)
        new_stat = 0;

    if (old_stat == 0)
        real_intensity = 0;
    else
        real_intensity = (((new_stat - old_stat) << 8) / old_stat);

    if (mut_base != -1)
    {
        // Now compute the mutation intensity relative to an absolute value.
        // Take the lesser of the two effects.
        int real_intensity2 = (((new_stat - old_stat) << 8) / mut_base);

        if (real_intensity < 0)
            if (real_intensity2 > real_intensity)
                real_intensity = real_intensity2;

        if (real_intensity > 0)
            if (real_intensity2 < real_intensity)
                real_intensity = real_intensity2;
    }

    real_intensity *= sign;

    md->stats[stat] = new_stat;

    // Adjust XP value
    md->stats[mob_stat::XP_BONUS] += mutation_value[stat] * real_intensity;
    if (md->stats[mob_stat::XP_BONUS] <= 0)
        md->stats[mob_stat::XP_BONUS] = 1;

    // Sanitise
    if (md->stats[mob_stat::ATK1] > md->stats[mob_stat::ATK2])
    {
        int swap = md->stats[mob_stat::ATK2];
        md->stats[mob_stat::ATK2] = md->stats[mob_stat::ATK1];
        md->stats[mob_stat::ATK1] = swap;
    }
}

// This calculates the exp of a given mob
static
int mob_gen_exp(mob_db_ *mob)
{
    if (mob->max_hp <= 1)
        return 1;
    double mod_def = 100 - mob->def;
    if (100 == mob->def)
        mod_def = 1;
    double effective_hp =
        ((50 - mob->attrs[ATTR::LUK]) * mob->max_hp / 50.0) +
        (2 * mob->attrs[ATTR::LUK] * mob->max_hp / mod_def);
    double attack_factor =
        (mob->atk1 + mob->atk2 + mob->attrs[ATTR::STR] / 3.0 + mob->attrs[ATTR::DEX] / 2.0 +
         mob->attrs[ATTR::LUK]) * (1872.0 / mob->adelay) / 4;
    double dodge_factor =
        pow(mob->lv + mob->attrs[ATTR::AGI] + mob->attrs[ATTR::LUK] / 2.0, 4.0 / 3.0);
    // TODO s/persuit/pursuit/g sometime when I'm not worried about diffs
    double persuit_factor =
        (3 + mob->range) * bool(mob->mode & MobMode::CAN_MOVE) * 1000 / mob->speed;
    double aggression_factor =
        bool(mob->mode & MobMode::AGGRESSIVE)
        ? 10.0 / 9.0
        : 1.0;
    int xp = floor(effective_hp * pow(sqrt(attack_factor)
            + sqrt(dodge_factor)
            + sqrt(persuit_factor) + 55, 3)
        * aggression_factor / 2000000.0
        * static_cast<double>(battle_config.base_exp_rate) / 100.);
    if (xp < 1)
        xp = 1;
    PRINTF("Exp for mob '%s' generated: %d\n", mob->name, xp);
    return xp;
}

static
void mob_init(dumb_ptr<mob_data> md)
{
    int i;
    const int mob_class = md->mob_class;
    const int mutations_nr = mob_db[mob_class].mutations_nr;
    const int mutation_power = mob_db[mob_class].mutation_power;

    md->stats[mob_stat::LV] = mob_db[mob_class].lv;
    md->stats[mob_stat::MAX_HP] = mob_db[mob_class].max_hp;
    md->stats[mob_stat::STR] = mob_db[mob_class].attrs[ATTR::STR];
    md->stats[mob_stat::AGI] = mob_db[mob_class].attrs[ATTR::AGI];
    md->stats[mob_stat::VIT] = mob_db[mob_class].attrs[ATTR::VIT];
    md->stats[mob_stat::INT] = mob_db[mob_class].attrs[ATTR::INT];
    md->stats[mob_stat::DEX] = mob_db[mob_class].attrs[ATTR::DEX];
    md->stats[mob_stat::LUK] = mob_db[mob_class].attrs[ATTR::LUK];
    md->stats[mob_stat::ATK1] = mob_db[mob_class].atk1;
    md->stats[mob_stat::ATK2] = mob_db[mob_class].atk2;
    md->stats[mob_stat::ADELAY] = mob_db[mob_class].adelay;
    md->stats[mob_stat::DEF] = mob_db[mob_class].def;
    md->stats[mob_stat::MDEF] = mob_db[mob_class].mdef;
    md->stats[mob_stat::SPEED] = mob_db[mob_class].speed;
    md->stats[mob_stat::XP_BONUS] = MOB_XP_BONUS_BASE;

    for (i = 0; i < mutations_nr; i++)
    {
        mob_stat stat_nr = random_::choice({
            mob_stat::LV,
            mob_stat::MAX_HP,
            mob_stat::STR,
            mob_stat::AGI,
            mob_stat::VIT,
            mob_stat::INT,
            mob_stat::DEX,
            mob_stat::LUK,
            mob_stat::ATK1,
            mob_stat::ATK2,
            mob_stat::ADELAY,
            mob_stat::DEF,
            mob_stat::MDEF,
            mob_stat::SPEED,
            // double chance to modify hp
            mob_stat::MAX_HP,
        });
        // TODO: if I don't remove this entirely, look into
        // normal distributions.
        int strength = (random_::to(mutation_power / 2)
                + random_::to(mutation_power / 2)
                + 2)
            * mutation_scale[stat_nr] / 100;

        if (random_::coin())
            strength = -strength;

        if (strength < -240)
            strength = -240;    /* Don't go too close to zero */

        mob_mutate(md, stat_nr, strength);
    }
}

/*==========================================
 * The MOB appearance for one time (for scripts)
 *------------------------------------------
 */
int mob_once_spawn(dumb_ptr<map_session_data> sd,
        MapName mapname, int x, int y,
        MobName mobname, int mob_class, int amount,
        NpcEvent event)
{
    dumb_ptr<mob_data> md = NULL;
    map_local *m;
    int count, r = mob_class;

    if (sd && mapname == MOB_THIS_MAP)
        m = sd->bl_m;
    else
        m = map_mapname2mapid(mapname);

    if (m == nullptr || amount <= 0 || (mob_class >= 0 && mob_class <= 1000) || mob_class > 2000)  // 値が異常なら召喚を止める
        return 0;

    if (sd)
    {
        if (x <= 0)
            x = sd->bl_x;
        if (y <= 0)
            y = sd->bl_y;
    }
    else if (x <= 0 || y <= 0)
    {
        PRINTF("mob_once_spawn: ??\n");
    }

    for (count = 0; count < amount; count++)
    {
        md.new_();
        md->lootitemv.clear();

        mob_spawn_dataset(md, mobname, mob_class);
        md->bl_m = m;
        md->bl_x = x;
        md->bl_y = y;
        if (r < 0 && battle_config.dead_branch_active == 1)
            //移動してアクティブで反撃する
            md->mode = MobMode::war;
        md->spawn.m = m;
        md->spawn.x0 = x;
        md->spawn.y0 = y;
        md->spawn.xs = 0;
        md->spawn.ys = 0;
        md->spawn.delay1 = static_cast<interval_t>(-1);   // Only once is a flag.
        md->spawn.delay2 = static_cast<interval_t>(-1);   // Only once is a flag.

        md->npc_event = event;

        md->bl_type = BL::MOB;
        map_addiddb(md);
        mob_spawn(md->bl_id);
    }
    return (amount > 0) ? md->bl_id : 0;
}

/*==========================================
 * The MOB appearance for one time (& area specification for scripts)
 *------------------------------------------
 */
int mob_once_spawn_area(dumb_ptr<map_session_data> sd,
        MapName mapname, int x0, int y0, int x1, int y1,
        MobName mobname, int mob_class, int amount,
        NpcEvent event)
{
    int x, y, i, max, lx = -1, ly = -1, id = 0;
    map_local *m;

    if (mapname == MOB_THIS_MAP)
        m = sd->bl_m;
    else
        m = map_mapname2mapid(mapname);

    max = (y1 - y0 + 1) * (x1 - x0 + 1) * 3;
    if (max > 1000)
        max = 1000;

    if (m == nullptr || amount <= 0 || (mob_class >= 0 && mob_class <= 1000) || mob_class > 2000)  // A summon is stopped if a value is unusual
        return 0;

    for (i = 0; i < amount; i++)
    {
        int j = 0;
        do
        {
            x = random_::in(x0, x1);
            y = random_::in(y0, y1);
        }
        while (bool(map_getcell(m, x, y) & MapCell::UNWALKABLE)
             && (++j) < max);
        if (j >= max)
        {
            if (lx >= 0)
            {                   // Since reference went wrong, the place which boiled before is used.
                x = lx;
                y = ly;
            }
            else
                return 0;       // Since reference of the place which boils first went wrong, it stops.
        }
        id = mob_once_spawn(sd, mapname, x, y, mobname, mob_class, 1, event);
        lx = x;
        ly = y;
    }
    return id;
}

// TODO: deprecate these
short mob_get_hair(int mob_class)
{
    return mob_db[mob_class].hair;
}

short mob_get_hair_color(int mob_class)
{
    return mob_db[mob_class].hair_color;
}

short mob_get_weapon(int mob_class)
{
    return mob_db[mob_class].weapon;
}

short mob_get_shield(int mob_class)
{
    return mob_db[mob_class].shield;
}

short mob_get_head_top(int mob_class)
{
    return mob_db[mob_class].head_top;
}

short mob_get_head_mid(int mob_class)
{
    return mob_db[mob_class].head_mid;
}

short mob_get_head_buttom(int mob_class)
{
    return mob_db[mob_class].head_buttom;
}

short mob_get_clothes_color(int mob_class) // Add for player monster dye - Valaris
{
    return mob_db[mob_class].clothes_color; // End
}

int mob_get_equip(int mob_class)   // mob equip [Valaris]
{
    return mob_db[mob_class].equip;
}

/*==========================================
 * Is MOB in the state in which the present movement is possible or not?
 *------------------------------------------
 */
static
int mob_can_move(dumb_ptr<mob_data> md)
{
    nullpo_ret(md);

    if (md->canmove_tick > gettick()
        || (bool(md->opt1) && md->opt1 != Opt1::_stone6))
        return 0;

    return 1;
}

/*==========================================
 * Time calculation concerning one step next to mob
 *------------------------------------------
 */
static
interval_t calc_next_walk_step(dumb_ptr<mob_data> md)
{
    nullpo_retr(interval_t::zero(), md);

    if (md->walkpath.path_pos >= md->walkpath.path_len)
        return static_cast<interval_t>(-1);
    if (dir_is_diagonal(md->walkpath.path[md->walkpath.path_pos]))
        return battle_get_speed(md) * 14 / 10;
    return battle_get_speed(md);
}

static
int mob_walktoxy_sub(dumb_ptr<mob_data> md);

/*==========================================
 * Mob Walk processing
 *------------------------------------------
 */
static
int mob_walk(dumb_ptr<mob_data> md, tick_t tick, unsigned char data)
{
    int moveblock;
    int x, y, dx, dy;

    nullpo_ret(md);

    md->state.state = MS::IDLE;
    if (md->walkpath.path_pos >= md->walkpath.path_len
        || md->walkpath.path_pos != data)
        return 0;

    md->walkpath.path_half ^= 1;
    if (md->walkpath.path_half == 0)
    {
        md->walkpath.path_pos++;
        if (md->state.change_walk_target)
        {
            mob_walktoxy_sub(md);
            return 0;
        }
    }
    else
    {
        if (md->walkpath.path[md->walkpath.path_pos] >= DIR::COUNT)
            return 1;

        x = md->bl_x;
        y = md->bl_y;
        if (bool(map_getcell(md->bl_m, x, y) & MapCell::UNWALKABLE))
        {
            mob_stop_walking(md, 1);
            return 0;
        }
        md->dir = md->walkpath.path[md->walkpath.path_pos];
        dx = dirx[md->dir];
        dy = diry[md->dir];

        if (bool(map_getcell(md->bl_m, x + dx, y + dy)
                & MapCell::UNWALKABLE))
        {
            mob_walktoxy_sub(md);
            return 0;
        }

        moveblock = (x / BLOCK_SIZE != (x + dx) / BLOCK_SIZE
                     || y / BLOCK_SIZE != (y + dy) / BLOCK_SIZE);

        md->state.state = MS::WALK;
        map_foreachinmovearea(std::bind(clif_moboutsight, ph::_1, md),
                md->bl_m,
                x - AREA_SIZE, y - AREA_SIZE,
                x + AREA_SIZE, y + AREA_SIZE,
                dx, dy,
                BL::PC);

        x += dx;
        y += dy;
        if (md->min_chase > 13)
            md->min_chase--;

        if (moveblock)
            map_delblock(md);
        md->bl_x = x;
        md->bl_y = y;
        if (moveblock)
            map_addblock(md);

        map_foreachinmovearea(std::bind(clif_mobinsight, ph::_1, md),
                md->bl_m,
                x - AREA_SIZE, y - AREA_SIZE,
                x + AREA_SIZE, y + AREA_SIZE,
                -dx, -dy,
                BL::PC);
        md->state.state = MS::IDLE;
    }
    interval_t i = calc_next_walk_step(md);
    if (i > interval_t::zero())
    {
        i = i / 2;
        if (md->walkpath.path_half == 0)
            i = std::max(i, std::chrono::milliseconds(1));
        md->timer = Timer(tick + i,
                std::bind(mob_timer, ph::_1, ph::_2,
                    md->bl_id, md->walkpath.path_pos));
        md->state.state = MS::WALK;

        if (md->walkpath.path_pos >= md->walkpath.path_len)
            clif_fixmobpos(md);    // When mob stops, retransmission current of a position.
    }
    return 0;
}

/*==========================================
 * Check if mob should be attempting to attack
 *------------------------------------------
 */
static
int mob_check_attack(dumb_ptr<mob_data> md)
{
    dumb_ptr<block_list> tbl = NULL;
    dumb_ptr<map_session_data> tsd = NULL;
    dumb_ptr<mob_data> tmd = NULL;

    MobMode mode;
    int range;

    nullpo_ret(md);

    md->min_chase = 13;
    md->state.state = MS::IDLE;
    md->state.skillstate = MobSkillState::MSS_IDLE;

    if (md->skilltimer)
        return 0;

    if (bool(md->opt1))
        return 0;

    if ((tbl = map_id2bl(md->target_id)) == NULL)
    {
        md->target_id = 0;
        md->state.attackable = false;
        return 0;
    }

    if (tbl->bl_type == BL::PC)
        tsd = tbl->is_player();
    else if (tbl->bl_type == BL::MOB)
        tmd = tbl->is_mob();
    else
        return 0;

    if (tsd)
    {
        if (pc_isdead(tsd) || tsd->invincible_timer
            || pc_isinvisible(tsd) || md->bl_m != tbl->bl_m || tbl->bl_prev == NULL
            || distance(md->bl_x, md->bl_y, tbl->bl_x, tbl->bl_y) >= 13)
        {
            md->target_id = 0;
            md->state.attackable = false;
            return 0;
        }
    }
    if (tmd)
    {
        if (md->bl_m != tbl->bl_m || tbl->bl_prev == NULL
            || distance(md->bl_x, md->bl_y, tbl->bl_x, tbl->bl_y) >= 13)
        {
            md->target_id = 0;
            md->state.attackable = false;
            return 0;
        }
    }

    if (md->mode == MobMode::ZERO)
        mode = mob_db[md->mob_class].mode;
    else
        mode = md->mode;

    Race race = mob_db[md->mob_class].race;
    if (!bool(mode & MobMode::CAN_ATTACK))
    {
        md->target_id = 0;
        md->state.attackable = false;
        return 0;
    }
    if (tsd
        && !bool(mode & MobMode::BOSS)
        && (tsd->state.gangsterparadise
            && race != Race::_insect
            && race != Race::_demon))
    {
        md->target_id = 0;
        md->state.attackable = false;
        return 0;
    }

    range = mob_db[md->mob_class].range;
    if (bool(mode & MobMode::CAN_MOVE))
        range++;
    if (distance(md->bl_x, md->bl_y, tbl->bl_x, tbl->bl_y) > range)
        return 0;

    return 1;
}

static
void mob_ancillary_attack(dumb_ptr<block_list> bl,
        dumb_ptr<block_list> mdbl, dumb_ptr<block_list> tbl, tick_t tick)
{
    if (bl != tbl)
        battle_weapon_attack(mdbl, bl, tick);
}

/*==========================================
 * Attack processing of mob
 *------------------------------------------
 */
static
int mob_attack(dumb_ptr<mob_data> md, tick_t tick)
{
    dumb_ptr<block_list> tbl = NULL;

    nullpo_ret(md);

    if ((tbl = map_id2bl(md->target_id)) == NULL)
        return 0;

    if (!mob_check_attack(md))
        return 0;

    if (battle_config.monster_attack_direction_change)
        md->dir = map_calc_dir(md, tbl->bl_x, tbl->bl_y);   // 向き設定

    //clif_fixmobpos(md);

    md->state.skillstate = MobSkillState::MSS_ATTACK;
    if (mobskill_use(md, tick, MobSkillCondition::NEVER_EQUAL))
        return 0;

    md->target_lv = battle_weapon_attack(md, tbl, tick);
    // If you are reading this, please note:
    // it is highly platform-specific that this even works at all.
    int radius = battle_config.mob_splash_radius;
    if (radius >= 0 && tbl->bl_type == BL::PC && !tbl->bl_m->flag.town)
        map_foreachinarea(std::bind(mob_ancillary_attack, ph::_1, md, tbl, tick),
                tbl->bl_m,
                tbl->bl_x - radius, tbl->bl_y - radius,
                tbl->bl_x + radius, tbl->bl_y + radius,
                BL::PC);

    md->attackabletime = tick + battle_get_adelay(md);

    md->timer = Timer(md->attackabletime,
            std::bind(mob_timer, ph::_1, ph::_2,
                md->bl_id, 0));
    md->state.state = MS::ATTACK;

    return 0;
}

/*==========================================
 * The attack of PC which is attacking id is stopped.
 * The callback function of clif_foreachclient
 *------------------------------------------
 */
static
void mob_stopattacked(dumb_ptr<map_session_data> sd, int id)
{
    nullpo_retv(sd);

    if (sd->attacktarget == id)
        pc_stopattack(sd);
}

/*==========================================
 * The timer in which the mob's states changes
 *------------------------------------------
 */
static
int mob_changestate(dumb_ptr<mob_data> md, MS state, bool type)
{
    nullpo_ret(md);

    md->timer.cancel();
    md->state.state = state;

    switch (state)
    {
        case MS::WALK:
        {
            interval_t i = calc_next_walk_step(md);
            if (i > interval_t::zero())
            {
                i = i / 4;
                md->timer = Timer(gettick() + i,
                        std::bind(mob_timer, ph::_1, ph::_2,
                            md->bl_id, 0));
            }
            else
                md->state.state = MS::IDLE;
        }
            break;
        case MS::ATTACK:
        {
            tick_t tick = gettick();
            interval_t i = md->attackabletime - tick;
            if (i > interval_t::zero() && i < std::chrono::seconds(2))
                md->timer = Timer(md->attackabletime,
                        std::bind(mob_timer, ph::_1, ph::_2,
                            md->bl_id, 0));
            else if (type)
            {
                md->attackabletime = tick + battle_get_amotion(md);
                md->timer = Timer(md->attackabletime,
                        std::bind(mob_timer, ph::_1, ph::_2,
                            md->bl_id, 0));
            }
            else
            {
                md->attackabletime = tick + std::chrono::milliseconds(1);
                md->timer = Timer(md->attackabletime,
                        std::bind(mob_timer, ph::_1, ph::_2,
                            md->bl_id, 0));
            }
        }
            break;
        case MS::DEAD:
        {
            skill_castcancel(md, 0);
            md->state.skillstate = MobSkillState::MSS_DEAD;
            md->last_deadtime = gettick();
            // Since it died, all aggressors' attack to this mob is stopped.
            clif_foreachclient(std::bind(mob_stopattacked, ph::_1, md->bl_id));
            skill_status_change_clear(md, 2); // The abnormalities in status are canceled.
            md->deletetimer.cancel();
            md->hp = md->target_id = md->attacked_id = 0;
            md->state.attackable = false;
        }
            break;
    }

    return 0;
}

/*==========================================
 * timer processing of mob (timer function)
 * It branches to a walk and an attack.
 *------------------------------------------
 */
static
void mob_timer(TimerData *, tick_t tick, int id, unsigned char data)
{
    dumb_ptr<mob_data> md;
    dumb_ptr<block_list> bl;
    bl = map_id2bl(id);
    if (bl == NULL)
    {                           //攻撃してきた敵がもういないのは正常のようだ
        return;
    }

    if (bl->bl_type == BL::NUL || bl->bl_type != BL::MOB)
        return;

    md = bl->is_mob();

    if (md->bl_prev == NULL || md->state.state == MS::DEAD)
        return;

    MapBlockLock lock;
    switch (md->state.state)
    {
        case MS::WALK:
            mob_check_attack(md);
            mob_walk(md, tick, data);
            break;
        case MS::ATTACK:
            mob_attack(md, tick);
            break;
        default:
            if (battle_config.error_log == 1)
                PRINTF("mob_timer : %d ?\n",
                        md->state.state);
            break;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
int mob_walktoxy_sub(dumb_ptr<mob_data> md)
{
    struct walkpath_data wpd;

    nullpo_ret(md);

    if (path_search(&wpd, md->bl_m, md->bl_x, md->bl_y, md->to_x, md->to_y,
         md->state.walk_easy))
        return 1;
    md->walkpath = wpd;

    md->state.change_walk_target = 0;
    mob_changestate(md, MS::WALK, 0);
    clif_movemob(md);

    return 0;
}

/*==========================================
 * mob move start
 *------------------------------------------
 */
static
int mob_walktoxy(dumb_ptr<mob_data> md, int x, int y, int easy)
{
    struct walkpath_data wpd;

    nullpo_ret(md);

    if (md->state.state == MS::WALK
        && path_search(&wpd, md->bl_m, md->bl_x, md->bl_y, x, y, easy))
        return 1;

    md->state.walk_easy = easy;
    md->to_x = x;
    md->to_y = y;
    if (md->state.state == MS::WALK)
    {
        md->state.change_walk_target = 1;
    }
    else
    {
        return mob_walktoxy_sub(md);
    }

    return 0;
}

/*==========================================
 * mob spawn with delay (timer function)
 *------------------------------------------
 */
static
void mob_delayspawn(TimerData *, tick_t, int m)
{
    mob_spawn(m);
}

/*==========================================
 * spawn timing calculation
 *------------------------------------------
 */
static
int mob_setdelayspawn(int id)
{
    dumb_ptr<mob_data> md;
    dumb_ptr<block_list> bl;

    if ((bl = map_id2bl(id)) == NULL)
        return -1;

    if (!bl || bl->bl_type == BL::NUL || bl->bl_type != BL::MOB)
        return -1;

    md = bl->is_mob();
    nullpo_retr(-1, md);

    if (!md || md->bl_type != BL::MOB)
        return -1;

    // Processing of MOB which is not revitalized
    if (md->spawn.delay1 == static_cast<interval_t>(-1)
        && md->spawn.delay2 == static_cast<interval_t>(-1)
        && md->n == 0)
    {
        map_deliddb(md);
        md->lootitemv.clear();
        MapBlockLock::freeblock(md);
        return 0;
    }

    tick_t spawntime1 = md->last_spawntime + md->spawn.delay1;
    tick_t spawntime2 = md->last_deadtime + md->spawn.delay2;
    tick_t spawntime3 = gettick() + std::chrono::seconds(5);
    tick_t spawntime = std::max({spawntime1, spawntime2, spawntime3});

    Timer(spawntime,
            std::bind(mob_delayspawn, ph::_1, ph::_2,
                id)
    ).detach();
    return 0;
}

/*==========================================
 * Mob spawning. Initialization is also variously here.
 *------------------------------------------
 */
int mob_spawn(int id)
{
    int x = 0, y = 0;
    tick_t tick = gettick();
    dumb_ptr<mob_data> md;
    dumb_ptr<block_list> bl;

    bl = map_id2bl(id);
    nullpo_retr(-1, bl);

    if (!bl || bl->bl_type == BL::NUL || bl->bl_type != BL::MOB)
        return -1;

    md = bl->is_mob();
    nullpo_retr(-1, md);

    if (!md || md->bl_type == BL::NUL || md->bl_type != BL::MOB)
        return -1;

    md->last_spawntime = tick;
    if (md->bl_prev != NULL)
    {
        map_delblock(md);
    }

    md->bl_m = md->spawn.m;
    {
        int i = 0;
        do
        {
            if (md->spawn.x0 == 0 && md->spawn.y0 == 0)
            {
                x = random_::in(1, md->bl_m->xs - 2);
                y = random_::in(1, md->bl_m->ys - 2);
            }
            else
            {
                // TODO: move this logic earlier - possibly all the way
                // into the data files
                x = md->spawn.x0 - md->spawn.xs / 2 + random_::in(0, md->spawn.xs);
                y = md->spawn.y0 - md->spawn.ys / 2 + random_::in(0, md->spawn.ys);
            }
            i++;
        }
        while (bool(map_getcell(md->bl_m, x, y) & MapCell::UNWALKABLE)
            && i < 50);

        if (i >= 50)
        {
    //      if(battle_config.error_log==1)
    //          PRINTF("MOB spawn error %d @ %s\n",id,map[md->bl_m].name);
            Timer(tick + std::chrono::seconds(5),
                    std::bind(mob_delayspawn, ph::_1, ph::_2,
                        id)
            ).detach();
            return 1;
        }
    }

    md->to_x = md->bl_x = x;
    md->to_y = md->bl_y = y;
    md->dir = DIR::S;

    map_addblock(md);

    really_memzero_this(&md->state);
    md->attacked_id = 0;
    md->target_id = 0;
    md->move_fail_count = 0;
    mob_init(md);

    if (!md->stats[mob_stat::SPEED])
        md->stats[mob_stat::SPEED] = mob_db[md->mob_class].speed;
    md->def_ele = mob_db[md->mob_class].element;
    md->master_id = 0;
    md->master_dist = 0;

    md->state.state = MS::IDLE;
    md->state.skillstate = MobSkillState::MSS_IDLE;
    assert (!md->timer);
    md->last_thinktime = tick;
    md->next_walktime = tick + std::chrono::seconds(5) + std::chrono::milliseconds(random_::to(50));
    md->attackabletime = tick;
    md->canmove_tick = tick;

    // md->deletetimer = nullptr;

    // md->skilltimer = nullptr;
    md->skilldelayup = make_unique<tick_t[]>(mob_db[md->mob_class].skills.size());
    for (size_t i = 0; i < mob_db[md->mob_class].skills.size(); i++)
        md->skilldelayup[i] = tick - std::chrono::hours(10);
    md->skillid = SkillID();
    md->skilllv = 0;

    md->dmglogv.clear();
    md->lootitemv.clear();

    for (StatusChange i : erange(StatusChange(), StatusChange::MAX_STATUSCHANGE))
    {
        assert (!md->sc_data[i].timer);
        md->sc_data[i].val1 = 0;
    }
    md->sc_count = 0;
    md->opt1 = Opt1::ZERO;
    md->opt2 = Opt2::ZERO;
    md->opt3 = Opt3::ZERO;
    md->option = Option::ZERO;

    md->hp = battle_get_max_hp(md);
    if (md->hp <= 0)
    {
        mob_makedummymobdb(md->mob_class);
        md->hp = battle_get_max_hp(md);
    }

    clif_spawnmob(md);

    return 0;
}

/*==========================================
 * Distance calculation between two points
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
 * The stop of MOB's attack
 *------------------------------------------
 */
int mob_stopattack(dumb_ptr<mob_data> md)
{
    md->target_id = 0;
    md->state.attackable = false;
    md->attacked_id = 0;
    return 0;
}

/*==========================================
 * The stop of MOB's walking
 *------------------------------------------
 */
int mob_stop_walking(dumb_ptr<mob_data> md, int type)
{
    nullpo_ret(md);

    if (md->state.state == MS::WALK || md->state.state == MS::IDLE)
    {
        int dx = 0, dy = 0;

        md->walkpath.path_len = 0;
        if (type & 4)
        {
            dx = md->to_x - md->bl_x;
            if (dx < 0)
                dx = -1;
            else if (dx > 0)
                dx = 1;
            dy = md->to_y - md->bl_y;
            if (dy < 0)
                dy = -1;
            else if (dy > 0)
                dy = 1;
        }
        md->to_x = md->bl_x + dx;
        md->to_y = md->bl_y + dy;
        if (dx != 0 || dy != 0)
        {
            mob_walktoxy_sub(md);
            return 0;
        }
        mob_changestate(md, MS::IDLE, 0);
    }
    if (type & 0x01)
        clif_fixmobpos(md);
    if (type & 0x02)
    {
        interval_t delay = battle_get_dmotion(md);
        tick_t tick = gettick();
        if (md->canmove_tick < tick)
            md->canmove_tick = tick + delay;
    }

    return 0;
}

/*==========================================
 * Reachability to a Specification ID existence place
 *------------------------------------------
 */
static
int mob_can_reach(dumb_ptr<mob_data> md, dumb_ptr<block_list> bl, int range)
{
    int dx, dy;
    struct walkpath_data wpd;
    int i;

    nullpo_ret(md);
    nullpo_ret(bl);

    dx = abs(bl->bl_x - md->bl_x);
    dy = abs(bl->bl_y - md->bl_y);

    if (bl->bl_type == BL::PC && battle_config.monsters_ignore_gm == 1)
    {                           // option to have monsters ignore GMs [Valaris]
        dumb_ptr<map_session_data> sd = bl->is_player();
        if (pc_isGM(sd))
            return 0;
    }

    if (md->bl_m != bl->bl_m)      // 違うャbプ
        return 0;

    if (range > 0 && range < ((dx > dy) ? dx : dy)) // 遠すぎる
        return 0;

    if (md->bl_x == bl->bl_x && md->bl_y == bl->bl_y) // 同じャX
        return 1;

    // Obstacle judging
    wpd.path_len = 0;
    wpd.path_pos = 0;
    wpd.path_half = 0;
    if (path_search(&wpd, md->bl_m, md->bl_x, md->bl_y, bl->bl_x, bl->bl_y, 0) !=
        -1)
        return 1;

    if (bl->bl_type != BL::PC && bl->bl_type != BL::MOB)
        return 0;

    // It judges whether it can adjoin or not.
    dx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    dy = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
    if (path_search(&wpd, md->bl_m, md->bl_x, md->bl_y, bl->bl_x - dx, bl->bl_y - dy, 0) != -1)
        return 1;
    for (i = 0; i < 9; i++)
    {
        if (path_search(&wpd, md->bl_m, md->bl_x, md->bl_y, bl->bl_x - 1 + i / 3,
             bl->bl_y - 1 + i % 3, 0) != -1)
            return 1;
    }
    return 0;
}

/*==========================================
 * Determination for an attack of a monster
 *------------------------------------------
 */
int mob_target(dumb_ptr<mob_data> md, dumb_ptr<block_list> bl, int dist)
{
    dumb_ptr<map_session_data> sd;
    eptr<struct status_change, StatusChange> sc_data;
    MobMode mode;

    nullpo_ret(md);
    nullpo_ret(bl);

    sc_data = battle_get_sc_data(bl);
    Option *option = battle_get_option(bl);
    Race race = mob_db[md->mob_class].race;

    if (md->mode == MobMode::ZERO)
    {
        mode = mob_db[md->mob_class].mode;
    }
    else
    {
        mode = md->mode;
    }
    if (!bool(mode & MobMode::CAN_ATTACK))
    {
        md->target_id = 0;
        return 0;
    }
    // Nothing will be carried out if there is no mind of changing TAGE by TAGE ending.
    if ((md->target_id > 0 && md->state.attackable)
        && (!bool(mode & MobMode::AGGRESSIVE)
            || !random_::chance({25 + 1, 100})))
        return 0;

    // Coercion is exerted if it is MVPMOB.
    if (bool(mode & MobMode::BOSS)
        || (option != NULL
            || race == Race::_insect
            || race == Race::_demon))
    {
        if (bl->bl_type == BL::PC)
        {
            sd = bl->is_player();
            nullpo_ret(sd);
            if (sd->invincible_timer || pc_isinvisible(sd))
                return 0;
            if (!bool(mode & MobMode::BOSS) && race != Race::_insect && race != Race::_demon
                && sd->state.gangsterparadise)
                return 0;
        }

        md->target_id = bl->bl_id; // Since there was no disturbance, it locks on to target.bl_
        if (bl->bl_type == BL::PC || bl->bl_type == BL::MOB)
            md->state.attackable = true;
        else
            md->state.attackable = false;
        md->min_chase = dist + 13;
        if (md->min_chase > 26)
            md->min_chase = 26;
    }
    return 0;
}

/*==========================================
 * The ?? routine of an active monster
 *------------------------------------------
 */
static
void mob_ai_sub_hard_activesearch(dumb_ptr<block_list> bl,
        dumb_ptr<mob_data> smd, int *pcc)
{
    dumb_ptr<map_session_data> tsd = NULL;
    dumb_ptr<mob_data> tmd = NULL;
    MobMode mode;
    int dist;

    nullpo_retv(bl);
    nullpo_retv(smd);
    nullpo_retv(pcc);

    if (bl->bl_type == BL::PC)
        tsd = bl->is_player();
    else if (bl->bl_type == BL::MOB)
        tmd = bl->is_mob();
    else
        return;

    //敵味方判定
    if (battle_check_target(smd, bl, BCT_ENEMY) == 0)
        return;

    if (smd->mode == MobMode::ZERO)
        mode = mob_db[smd->mob_class].mode;
    else
        mode = smd->mode;

    // アクティブでターゲット射程内にいるなら、ロックする
    if (bool(mode & MobMode::AGGRESSIVE))
    {
        Race race = mob_db[smd->mob_class].race;
        //対象がPCの場合
        if (tsd &&
            !pc_isdead(tsd) &&
            tsd->bl_m == smd->bl_m &&
            !tsd->invincible_timer &&
            !pc_isinvisible(tsd) &&
            (dist =
             distance(smd->bl_x, smd->bl_y, tsd->bl_x, tsd->bl_y)) < 9)
        {
            if (bool(mode & MobMode::BOSS)
                || (!tsd->state.gangsterparadise
                    || race == Race::_insect
                    || race == Race::_demon))
            {
                // 妨害がないか判定
                // 到達可能性判定
                if (mob_can_reach(smd, bl, 12)
                    && random_::chance({1, ++*pcc}))
                {
                    // 範囲内PCで等確率にする
                    smd->target_id = tsd->bl_id;
                    smd->state.attackable = true;
                    smd->min_chase = 13;
                }
            }
        }
        //対象がMobの場合
        else if (tmd &&
                 tmd->bl_m == smd->bl_m &&
                 (dist =
                  distance(smd->bl_x, smd->bl_y, tmd->bl_x, tmd->bl_y)) < 9)
        {
            // 到達可能性判定
            if (mob_can_reach(smd, bl, 12)
                && random_::chance({1, ++*pcc}))
            {
                // 範囲内で等確率にする
                smd->target_id = bl->bl_id;
                smd->state.attackable = true;
                smd->min_chase = 13;
            }
        }
    }
}

/*==========================================
 * loot monster item search
 *------------------------------------------
 */
static
void mob_ai_sub_hard_lootsearch(dumb_ptr<block_list> bl, dumb_ptr<mob_data> md, int *itc)
{
    MobMode mode;
    int dist;

    nullpo_retv(bl);

    if (md->mode == MobMode::ZERO)
    {
        mode = mob_db[md->mob_class].mode;
    }
    else
    {
        mode = md->mode;
    }

    if (!md->target_id && bool(mode & MobMode::LOOTER))
    {
        if (bl->bl_m == md->bl_m
            && (dist = distance(md->bl_x, md->bl_y, bl->bl_x, bl->bl_y)) < 9)
        {
            // Reachability judging
            if (mob_can_reach(md, bl, 12)
                && random_::chance({1, ++*itc}))
            {
                // It is made a probability, such as within the limits PC.
                md->target_id = bl->bl_id;
                md->state.attackable = false;
                md->min_chase = 13;
            }
        }
    }
}

/*==========================================
 * The ?? routine of a link monster
 *------------------------------------------
 */
static
void mob_ai_sub_hard_linksearch(dumb_ptr<block_list> bl, dumb_ptr<mob_data> md, dumb_ptr<block_list> target)
{
    dumb_ptr<mob_data> tmd;

    nullpo_retv(bl);
    tmd = bl->is_mob();
    nullpo_retv(md);
    nullpo_retv(target);

    if (md->attacked_id > 0
        && bool(mob_db[md->mob_class].mode & MobMode::ASSIST))
    {
        if (tmd->mob_class == md->mob_class
            && tmd->bl_m == md->bl_m
            && (!tmd->target_id || !md->state.attackable))
        {
            if (mob_can_reach(tmd, target, 12))
            {
                // Reachability judging
                tmd->target_id = md->attacked_id;
                tmd->state.attackable = true;
                tmd->min_chase = 13;
            }
        }
    }
}

/*==========================================
 * Processing of slave monsters
 *------------------------------------------
 */
static
int mob_ai_sub_hard_slavemob(dumb_ptr<mob_data> md, tick_t tick)
{
    dumb_ptr<mob_data> mmd = NULL;
    dumb_ptr<block_list> bl;
    MobMode mode;
    int old_dist;

    nullpo_ret(md);

    if ((bl = map_id2bl(md->master_id)) != NULL)
        mmd = bl->is_mob();

    mode = mob_db[md->mob_class].mode;

    // It is not main monster/leader.
    if (!mmd || mmd->bl_type != BL::MOB || mmd->bl_id != md->master_id)
        return 0;

    // Since it is in the map on which the master is not, teleport is carried out and it pursues.
    if (mmd->bl_m != md->bl_m)
    {
        mob_warp(md, mmd->bl_m, mmd->bl_x, mmd->bl_y, BeingRemoveWhy::WARPED);
        md->state.master_check = 1;
        return 0;
    }

    // Distance with between slave and master is measured.
    old_dist = md->master_dist;
    md->master_dist = distance(md->bl_x, md->bl_y, mmd->bl_x, mmd->bl_y);

    // Since the master was in near immediately before, teleport is carried out and it pursues.
    if (old_dist < 10 && md->master_dist > 18)
    {
        mob_warp(md, nullptr, mmd->bl_x, mmd->bl_y, BeingRemoveWhy::WARPED);
        md->state.master_check = 1;
        return 0;
    }

    // Although there is the master, since it is somewhat far, it approaches.
    if ((!md->target_id || !md->state.attackable)
        && mob_can_move(md)
        && (md->walkpath.path_pos >= md->walkpath.path_len
            || md->walkpath.path_len == 0) && md->master_dist < 15)
    {
        int i = 0, dx, dy, ret;
        if (md->master_dist > 4)
        {
            do
            {
                if (i <= 5)
                {
                    dx = mmd->bl_x - md->bl_x;
                    dy = mmd->bl_y - md->bl_y;
                    if (dx < 0)
                        dx += random_::in(1, std::min(3, -dx));
                    else if (dx > 0)
                        dx -= random_::in(1, std::min(3, dx));
                    if (dy < 0)
                        dy += random_::in(1, std::min(3, -dy));
                    else if (dy > 0)
                        dy -= random_::in(1, std::min(3, dy));
                }
                else
                {
                    dx = mmd->bl_x - md->bl_x + random_::in(-3, 3);
                    dy = mmd->bl_y - md->bl_y + random_::in(-3, 3);
                }

                ret = mob_walktoxy(md, md->bl_x + dx, md->bl_y + dy, 0);
                i++;
            }
            while (ret && i < 10);
        }
        else
        {
            do
            {
                // changed to do what it was obviously supposed to do,
                // instead of what it was doing ...
                dx = random_::in(-4, 4);
                dy = random_::in(-4, 4);
                if (dx == 0 && dy == 0)
                {
                    dx = random_::coin() ? 1 : -1;
                    dy = random_::coin() ? 1 : -1;
                }
                dx += mmd->bl_x;
                dy += mmd->bl_y;

                ret = mob_walktoxy(md, mmd->bl_x + dx, mmd->bl_y + dy, 0);
                i++;
            }
            while (ret && i < 10);
        }

        md->next_walktime = tick + std::chrono::milliseconds(500);
        md->state.master_check = 1;
    }

    // There is the master, the master locks a target and he does not lock.
    if ((mmd->target_id > 0 && mmd->state.attackable)
        && (!md->target_id || !md->state.attackable))
    {
        dumb_ptr<map_session_data> sd = map_id2sd(mmd->target_id);
        if (sd != NULL && !pc_isdead(sd) && !sd->invincible_timer
            && !pc_isinvisible(sd))
        {

            Race race = mob_db[md->mob_class].race;
            if (bool(mode & MobMode::BOSS)
                || (!sd->state.gangsterparadise
                    || race == Race::_insect
                    || race == Race::_demon))
            {                   // 妨害がないか判定

                md->target_id = sd->bl_id;
                md->state.attackable = true;
                md->min_chase =
                    5 + distance(md->bl_x, md->bl_y, sd->bl_x, sd->bl_y);
                md->state.master_check = 1;
            }
        }
    }

    return 0;
}

/*==========================================
 * A lock of target is stopped and mob moves to a standby state.
 *------------------------------------------
 */
static
int mob_unlocktarget(dumb_ptr<mob_data> md, tick_t tick)
{
    nullpo_ret(md);

    md->target_id = 0;
    md->state.attackable = false;
    md->state.skillstate = MobSkillState::MSS_IDLE;
    md->next_walktime = tick + std::chrono::seconds(3) + std::chrono::milliseconds(random_::to(3000));
    return 0;
}

/*==========================================
 * Random walk
 *------------------------------------------
 */
static
int mob_randomwalk(dumb_ptr<mob_data> md, tick_t tick)
{
    const int retrycount = 20;

    nullpo_ret(md);

    interval_t speed = battle_get_speed(md);
    if (md->next_walktime < tick)
    {
        int i, x, y, d = 12 - md->move_fail_count;
        if (d < 5)
            d = 5;
        for (i = 0; i < retrycount; i++)
        {
            // Search of a movable place
            x = md->bl_x + random_::in(-d, d);
            y = md->bl_y + random_::in(-d, d);
            if (!bool(map_getcell(md->bl_m, x, y) & MapCell::UNWALKABLE)
                && mob_walktoxy(md, x, y, 1) == 0)
            {
                md->move_fail_count = 0;
                break;
            }
            if (i + 1 >= retrycount)
            {
                md->move_fail_count++;
                if (md->move_fail_count > 1000)
                {
                    if (battle_config.error_log == 1)
                        PRINTF("MOB cant move. random spawn %d, mob_class = %d\n",
                             md->bl_id, md->mob_class);
                    md->move_fail_count = 0;
                    mob_spawn(md->bl_id);
                }
            }
        }
        interval_t c = interval_t::zero();
        for (i = 0; i < md->walkpath.path_len; i++)
        {
            // The next walk start time is calculated.
            if (dir_is_diagonal(md->walkpath.path[i]))
                c += speed * 14 / 10;
            else
                c += speed;
        }
        md->next_walktime = tick + std::chrono::seconds(3) + std::chrono::milliseconds(random_::to(3000)) + c;
        md->state.skillstate = MobSkillState::MSS_WALK;
        return 1;
    }
    return 0;
}

/*==========================================
 * AI of MOB whose is near a Player
 *------------------------------------------
 */
static
void mob_ai_sub_hard(dumb_ptr<block_list> bl, tick_t tick)
{
    dumb_ptr<mob_data> md, tmd = NULL;
    dumb_ptr<map_session_data> tsd = NULL;
    dumb_ptr<block_list> tbl = NULL;
    dumb_ptr<flooritem_data> fitem;
    int i, dx, dy, ret, dist;
    int attack_type = 0;
    MobMode mode;

    nullpo_retv(bl);
    md = bl->is_mob();

    if (tick < md->last_thinktime + MIN_MOBTHINKTIME)
        return;
    md->last_thinktime = tick;

    if (md->skilltimer || md->bl_prev == NULL)
    {
        // Under a skill aria and death
        if (tick > md->next_walktime + MIN_MOBTHINKTIME)
            md->next_walktime = tick;
        return;
    }

    if (md->mode == MobMode::ZERO)
        mode = mob_db[md->mob_class].mode;
    else
        mode = md->mode;

    Race race = mob_db[md->mob_class].race;

    // Abnormalities
    if (bool(md->opt1) && md->opt1 != Opt1::_stone6)
        return;

    if (!bool(mode & MobMode::CAN_ATTACK) && md->target_id > 0)
        md->target_id = 0;

    if (md->attacked_id > 0 && bool(mode & MobMode::ASSIST))
    {                           // Link monster
        dumb_ptr<map_session_data> asd = map_id2sd(md->attacked_id);
        if (asd)
        {
            if (!asd->invincible_timer && !pc_isinvisible(asd))
            {
                map_foreachinarea(std::bind(mob_ai_sub_hard_linksearch, ph::_1, md, asd),
                        md->bl_m,
                        md->bl_x - 13, md->bl_y - 13,
                        md->bl_x + 13, md->bl_y + 13,
                        BL::MOB);
            }
        }
    }

    // It checks to see it was attacked first (if active, it is target change at 25% of probability).
    if (mode != MobMode::ZERO && md->attacked_id > 0
        && (!md->target_id || !md->state.attackable
            || (bool(mode & MobMode::AGGRESSIVE) && random_::chance({25, 100}))))
    {
        dumb_ptr<block_list> abl = map_id2bl(md->attacked_id);
        dumb_ptr<map_session_data> asd = NULL;
        if (abl)
        {
            if (abl->bl_type == BL::PC)
                asd = abl->is_player();
            if (asd == NULL || md->bl_m != abl->bl_m || abl->bl_prev == NULL
                || asd->invincible_timer || pc_isinvisible(asd)
                || (dist =
                    distance(md->bl_x, md->bl_y, abl->bl_x, abl->bl_y)) >= 32
                || battle_check_target(bl, abl, BCT_ENEMY) == 0)
                md->attacked_id = 0;
            else
            {
                md->target_id = md->attacked_id;    // set target
                md->state.attackable = true;
                attack_type = 1;
                md->attacked_id = 0;
                md->min_chase = dist + 13;
                if (md->min_chase > 26)
                    md->min_chase = 26;
            }
        }
    }

    md->state.master_check = 0;
    // Processing of slave monster
    if (md->master_id > 0 && md->state.special_mob_ai == 0)
        mob_ai_sub_hard_slavemob(md, tick);

    // アクティヴモンスターの策敵 (?? of a bitter taste TIVU monster)
    if ((!md->target_id || !md->state.attackable)
        && bool(mode & MobMode::AGGRESSIVE) && !md->state.master_check
        && battle_config.monster_active_enable == 1)
    {
        i = 0;
        if (md->state.special_mob_ai)
        {
            map_foreachinarea(std::bind(mob_ai_sub_hard_activesearch, ph::_1, md, &i),
                    md->bl_m,
                    md->bl_x - AREA_SIZE * 2, md->bl_y - AREA_SIZE * 2,
                    md->bl_x + AREA_SIZE * 2, md->bl_y + AREA_SIZE * 2,
                    BL::NUL);
        }
        else
        {
            map_foreachinarea(std::bind(mob_ai_sub_hard_activesearch, ph::_1, md, &i),
                    md->bl_m,
                    md->bl_x - AREA_SIZE * 2, md->bl_y - AREA_SIZE * 2,
                    md->bl_x + AREA_SIZE * 2, md->bl_y + AREA_SIZE * 2,
                    BL::PC);
        }
    }

    // The item search of a route monster
    if (!md->target_id
        && bool(mode & MobMode::LOOTER)
        && !md->state.master_check)
    {
        i = 0;
        map_foreachinarea(std::bind(mob_ai_sub_hard_lootsearch, ph::_1, md, &i),
                md->bl_m,
                md->bl_x - AREA_SIZE * 2, md->bl_y - AREA_SIZE * 2,
                md->bl_x + AREA_SIZE * 2, md->bl_y + AREA_SIZE * 2,
                BL::ITEM);
    }

    // It will attack, if the candidate for an attack is.
    if (md->target_id > 0)
    {
        if ((tbl = map_id2bl(md->target_id)))
        {
            if (tbl->bl_type == BL::PC)
                tsd = tbl->is_player();
            else if (tbl->bl_type == BL::MOB)
                tmd = tbl->is_mob();
            if (tsd || tmd)
            {
                if (tbl->bl_m != md->bl_m || tbl->bl_prev == NULL
                    || (dist =
                        distance(md->bl_x, md->bl_y, tbl->bl_x,
                                  tbl->bl_y)) >= md->min_chase)
                    mob_unlocktarget(md, tick);    // 別マップか、視界外
                else if (tsd && !bool(mode & MobMode::BOSS)
                         && (tsd->state.gangsterparadise
                             && race != Race::_insect
                             && race != Race::_demon))
                    mob_unlocktarget(md, tick);    // スキルなどによる策敵妨害
                else if (!battle_check_range(md, tbl, mob_db[md->mob_class].range))
                {
                    // 攻撃範囲外なので移動
                    if (!bool(mode & MobMode::CAN_MOVE))
                    {           // 移動しないモード
                        mob_unlocktarget(md, tick);
                        return;
                    }
                    if (!mob_can_move(md)) // 動けない状態にある
                        return;
                    md->state.skillstate = MobSkillState::MSS_CHASE;   // 突撃時スキル
                    mobskill_use(md, tick, MobSkillCondition::ANY);
                    if (md->timer && md->state.state != MS::ATTACK
                        && (md->next_walktime < tick
                            || distance(md->to_x, md->to_y, tbl->bl_x, tbl->bl_y) < 2))
                        return;   // 既に移動中
                    if (!mob_can_reach(md, tbl, (md->min_chase > 13) ? md->min_chase : 13))
                        mob_unlocktarget(md, tick);    // 移動できないのでタゲ解除（IWとか？）
                    else
                    {
                        // 追跡
                        md->next_walktime = tick + std::chrono::milliseconds(500);
                        i = 0;
                        do
                        {
                            if (i == 0)
                            {
                                // 最初はAEGISと同じ方法で検索
                                dx = tbl->bl_x - md->bl_x;
                                dy = tbl->bl_y - md->bl_y;
                                if (dx < 0)
                                    dx++;
                                else if (dx > 0)
                                    dx--;
                                if (dy < 0)
                                    dy++;
                                else if (dy > 0)
                                    dy--;
                            }
                            else
                            {
                                // だめならAthena式(ランダム)
                                // {0 1 2}
                                dx = tbl->bl_x - md->bl_x + random_::in(-1, 1);
                                dy = tbl->bl_y - md->bl_y + random_::in(-1, 1);
                            }
                            ret = mob_walktoxy(md, md->bl_x + dx, md->bl_y + dy, 0);
                            i++;
                        }
                        while (ret && i < 5);

                        if (ret)
                        {       // 移動不可能な所からの攻撃なら2歩下る
                            if (dx < 0)
                                dx = 2;
                            else if (dx > 0)
                                dx = -2;
                            if (dy < 0)
                                dy = 2;
                            else if (dy > 0)
                                dy = -2;
                            mob_walktoxy(md, md->bl_x + dx, md->bl_y + dy,
                                          0);
                        }
                    }
                }
                else
                {               // 攻撃射程範囲内
                    md->state.skillstate = MobSkillState::MSS_ATTACK;
                    if (md->state.state == MS::WALK)
                        mob_stop_walking(md, 1);   // 歩行中なら停止
                    if (md->state.state == MS::ATTACK)
                        return;   // 既に攻撃中
                    mob_changestate(md, MS::ATTACK, attack_type);
                }
                return;
            }
            else
            {                   // ルートモンスター処理
                if (tbl == NULL || tbl->bl_type != BL::ITEM || tbl->bl_m != md->bl_m
                    || (dist =
                        distance(md->bl_x, md->bl_y, tbl->bl_x,
                                  tbl->bl_y)) >= md->min_chase
                    || !bool(mob_db[md->mob_class].mode & MobMode::LOOTER))
                {
                    // 遠すぎるかアイテムがなくなった
                    mob_unlocktarget(md, tick);
                    if (md->state.state == MS::WALK)
                        mob_stop_walking(md, 1);   // 歩行中なら停止
                }
                else if (dist)
                {
                    if (!bool(mode & MobMode::CAN_MOVE))
                    {           // 移動しないモード
                        mob_unlocktarget(md, tick);
                        return;
                    }
                    if (!mob_can_move(md)) // 動けない状態にある
                        return;
                    md->state.skillstate = MobSkillState::MSS_LOOT;    // ルート時スキル使用
                    mobskill_use(md, tick, MobSkillCondition::ANY);
                    if (md->timer && md->state.state != MS::ATTACK
                        && (md->next_walktime < tick
                            || distance(md->to_x, md->to_y, tbl->bl_x, tbl->bl_y) <= 0))
                        return;   // 既に移動中
                    md->next_walktime = tick + std::chrono::milliseconds(500);
                    dx = tbl->bl_x - md->bl_x;
                    dy = tbl->bl_y - md->bl_y;
                    ret = mob_walktoxy(md, md->bl_x + dx, md->bl_y + dy, 0);
                    if (ret)
                        mob_unlocktarget(md, tick);    // 移動できないのでタゲ解除（IWとか？）
                }
                else
                {               // アイテムまでたどり着いた
                    if (md->state.state == MS::ATTACK)
                        return;   // 攻撃中
                    if (md->state.state == MS::WALK)
                        mob_stop_walking(md, 1);   // 歩行中なら停止
                    fitem = tbl->is_item();
                    md->lootitemv.push_back(fitem->item_data);
                    map_clearflooritem(tbl->bl_id);
                    mob_unlocktarget(md, tick);
                }
                return;
            }
        }
        else
        {
            mob_unlocktarget(md, tick);
            if (md->state.state == MS::WALK)
                mob_stop_walking(md, 4);   // 歩行中なら停止
            return;
        }
    }

    // It is skill use at the time of /standby at the time of a walk.
    if (mobskill_use(md, tick, MobSkillCondition::ANY))
        return;

    // mobs that are not slaves can random-walk
    if (bool(mode & MobMode::CAN_MOVE)
        && mob_can_move(md)
        && (md->master_id == 0 || md->state.special_mob_ai
            || md->master_dist > 10))
    {
        // if walktime is more than 7 seconds in the future,
        // set it to somewhere between 3 and 5 seconds
        if (md->next_walktime > tick + std::chrono::seconds(7)
            && (md->walkpath.path_len == 0
                || md->walkpath.path_pos >= md->walkpath.path_len))
        {
            md->next_walktime = tick + std::chrono::seconds(3)
                + std::chrono::milliseconds(random_::to(2000));
        }

        // Random movement
        if (mob_randomwalk(md, tick))
            return;
    }

    // Since he has finished walking, it stands by.
    if (md->walkpath.path_len == 0
        || md->walkpath.path_pos >= md->walkpath.path_len)
        md->state.skillstate = MobSkillState::MSS_IDLE;
}

/*==========================================
 * Serious processing for mob in PC field of view (foreachclient)
 *------------------------------------------
 */
static
void mob_ai_sub_foreachclient(dumb_ptr<map_session_data> sd, tick_t tick)
{
    nullpo_retv(sd);

    map_foreachinarea(std::bind(mob_ai_sub_hard, ph::_1, tick),
            sd->bl_m,
            sd->bl_x - AREA_SIZE * 2, sd->bl_y - AREA_SIZE * 2,
            sd->bl_x + AREA_SIZE * 2, sd->bl_y + AREA_SIZE * 2,
            BL::MOB);
}

/*==========================================
 * Serious processing for mob in PC field of view   (interval timer function)
 *------------------------------------------
 */
static
void mob_ai_hard(TimerData *, tick_t tick)
{
    clif_foreachclient(std::bind(mob_ai_sub_foreachclient, ph::_1, tick));
}

/*==========================================
 * Negligent mode MOB AI (PC is not in near)
 *------------------------------------------
 */
static
void mob_ai_sub_lazy(dumb_ptr<block_list> bl, tick_t tick)
{
    nullpo_retv(bl);

    if (bl->bl_type != BL::MOB)
        return;

    dumb_ptr<mob_data> md = bl->is_mob();

    if (tick < md->last_thinktime + MIN_MOBTHINKTIME * 10)
        return;
    md->last_thinktime = tick;

    if (md->bl_prev == NULL || md->skilltimer)
    {
        if (tick > md->next_walktime + MIN_MOBTHINKTIME * 10)
            md->next_walktime = tick;
        return;
    }

    if (md->next_walktime < tick
        && bool(mob_db[md->mob_class].mode & MobMode::CAN_MOVE)
        && mob_can_move(md))
    {

        if (md->bl_m->users > 0)
        {
            // Since PC is in the same map, somewhat better negligent processing is carried out.

            // It sometimes moves.
            if (random_::chance(MOB_LAZYMOVEPERC))
                mob_randomwalk(md, tick);

            // MOB which is not not the summons MOB but BOSS, either sometimes reboils.
            else if (random_::chance(MOB_LAZYWARPPERC)
                    && md->spawn.x0 <= 0
                    && md->master_id != 0
                    && !bool(mob_db[md->mob_class].mode & MobMode::BOSS))
                mob_spawn(md->bl_id);

        }
        else
        {
            // Since PC is not even in the same map, suitable processing is carried out even if it takes.

            // MOB which is not BOSS which is not Summons MOB, either -- a case -- sometimes -- leaping
            if (random_::chance(MOB_LAZYWARPPERC)
                && md->spawn.x0 <= 0
                && md->master_id != 0
                && !bool(mob_db[md->mob_class].mode & MobMode::BOSS))
                mob_warp(md, nullptr, -1, -1, BeingRemoveWhy::NEGATIVE1);
        }

        md->next_walktime = tick + std::chrono::seconds(5) + std::chrono::milliseconds(random_::to(10 * 1000));
    }
}

/*==========================================
 * Negligent processing for mob outside PC field of view   (interval timer function)
 *------------------------------------------
 */
static
void mob_ai_lazy(TimerData *, tick_t tick)
{
    for (auto& pair : id_db)
        mob_ai_sub_lazy(pair.second, tick);
}

/*==========================================
 * The structure object for item drop with delay
 * Since it is only two being able to pass [ int ] a timer function
 * Data is put in and passed to this structure object.
 *------------------------------------------
 */
struct delay_item_drop
{
    map_local *m;
    int x, y;
    int nameid, amount;
    dumb_ptr<map_session_data> first_sd, second_sd, third_sd;
};

struct delay_item_drop2
{
    map_local *m;
    int x, y;
    struct item item_data;
    dumb_ptr<map_session_data> first_sd, second_sd, third_sd;
};

/*==========================================
 * item drop with delay (timer function)
 *------------------------------------------
 */
static
void mob_delay_item_drop(TimerData *, tick_t, struct delay_item_drop ditem)
{
    struct item temp_item {};
    PickupFail flag;

    temp_item.nameid = ditem.nameid;
    temp_item.amount = ditem.amount;
    temp_item.identify = !itemdb_isequip3(temp_item.nameid);

    if (battle_config.item_auto_get == 1)
    {
        if (ditem.first_sd
            && (flag =
                pc_additem(ditem.first_sd, &temp_item, ditem.amount))
            != PickupFail::OKAY)
        {
            clif_additem(ditem.first_sd, 0, 0, flag);
            map_addflooritem(&temp_item, 1,
                    ditem.m, ditem.x, ditem.y,
                    ditem.first_sd, ditem.second_sd, ditem.third_sd);
        }
        return;
    }

    map_addflooritem(&temp_item, 1,
            ditem.m, ditem.x, ditem.y,
            ditem.first_sd, ditem.second_sd, ditem.third_sd);
}

/*==========================================
 * item drop (timer function)-lootitem with delay
 *------------------------------------------
 */
static
void mob_delay_item_drop2(TimerData *, tick_t, struct delay_item_drop2 ditem)
{
    PickupFail flag;

    if (battle_config.item_auto_get == 1)
    {
        if (ditem.first_sd
            && (flag =
                pc_additem(ditem.first_sd, &ditem.item_data,
                            ditem.item_data.amount))
            != PickupFail::OKAY)
        {
            clif_additem(ditem.first_sd, 0, 0, flag);
            map_addflooritem(&ditem.item_data, ditem.item_data.amount,
                    ditem.m, ditem.x, ditem.y,
                    ditem.first_sd, ditem.second_sd, ditem.third_sd);
        }
        return;
    }

    map_addflooritem(&ditem.item_data, ditem.item_data.amount,
            ditem.m, ditem.x, ditem.y,
            ditem.first_sd, ditem.second_sd, ditem.third_sd);
}

/*==========================================
 * mob data is erased.
 *------------------------------------------
 */
int mob_delete(dumb_ptr<mob_data> md)
{
    nullpo_retr(1, md);

    if (md->bl_prev == NULL)
        return 1;
    mob_changestate(md, MS::DEAD, 0);
    clif_clearchar(md, BeingRemoveWhy::DEAD);
    map_delblock(md);
    mob_deleteslave(md);
    mob_setdelayspawn(md->bl_id);
    return 0;
}

int mob_catch_delete(dumb_ptr<mob_data> md, BeingRemoveWhy type)
{
    nullpo_retr(1, md);

    if (md->bl_prev == NULL)
        return 1;
    mob_changestate(md, MS::DEAD, 0);
    clif_clearchar(md, type);
    map_delblock(md);
    mob_setdelayspawn(md->bl_id);
    return 0;
}

void mob_timer_delete(TimerData *, tick_t, int id)
{
    dumb_ptr<block_list> bl = map_id2bl(id);
    dumb_ptr<mob_data> md;

    nullpo_retv(bl);

    md = bl->is_mob();
    mob_catch_delete(md, BeingRemoveWhy::WARPED);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void mob_deleteslave_sub(dumb_ptr<block_list> bl, int id)
{
    dumb_ptr<mob_data> md;

    nullpo_retv(bl);
    md = bl->is_mob();

    if (md->master_id > 0 && md->master_id == id)
        mob_damage(NULL, md, md->hp, 1);
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_deleteslave(dumb_ptr<mob_data> md)
{
    nullpo_ret(md);

    map_foreachinarea(std::bind(mob_deleteslave_sub, ph::_1, md->bl_id),
            md->bl_m,
            0, 0,
            md->bl_m->xs, md->bl_m->ys,
            BL::MOB);
    return 0;
}

// max. number of players to account for
constexpr int DAMAGE_BONUS_COUNT = 6;
const static
double damage_bonus_factor[DAMAGE_BONUS_COUNT + 1] =
{
    1.0, 1.0, 2.0, 2.5, 2.75, 2.9, 3.0
};

/*==========================================
 * It is the damage of sd to damage to md.
 *------------------------------------------
 */
int mob_damage(dumb_ptr<block_list> src, dumb_ptr<mob_data> md, int damage,
                int type)
{
    dumb_ptr<map_session_data> sd = NULL;
    int max_hp;
    tick_t tick = gettick();
    dumb_ptr<map_session_data> mvp_sd = NULL, second_sd = NULL, third_sd = NULL;

    nullpo_ret(md);        //srcはNULLで呼ばれる場合もあるので、他でチェック

    if (src && src->bl_id == md->master_id
        && bool(md->mode & MobMode::TURNS_AGAINST_BAD_MASTER))
    {
        /* If the master hits a monster, have the monster turn against him */
        md->master_id = 0;
        md->mode = MobMode::war;        /* Regular war mode */
        md->target_id = src->bl_id;
        md->attacked_id = src->bl_id;
    }

    max_hp = battle_get_max_hp(md);

    if (src && src->bl_type == BL::PC)
    {
        sd = src->is_player();
        mvp_sd = sd;
    }

//  if(battle_config.battle_log)
//      PRINTF("mob_damage %d %d %d\n",md->hp,max_hp,damage);
    if (md->bl_prev == NULL)
    {
        if (battle_config.error_log == 1)
            PRINTF("mob_damage : BlockError!!\n");
        return 0;
    }

    if (md->state.state == MS::DEAD || md->hp <= 0)
    {
        if (md->bl_prev != NULL)
        {
            mob_changestate(md, MS::DEAD, 0);
            // It is skill at the time of death.
            mobskill_use(md, tick, MobSkillCondition::ANY);

            clif_clearchar(md, BeingRemoveWhy::DEAD);
            map_delblock(md);
            mob_setdelayspawn(md->bl_id);
        }
        return 0;
    }

    mob_stop_walking(md, 3);
    if (damage > max_hp >> 2)
        skill_stop_dancing(md, 0);

    if (md->hp > max_hp)
        md->hp = max_hp;

    // The amount of overkill rounds to hp.
    if (damage > md->hp)
        damage = md->hp;

    if (!(type & 2))
    {
        if (sd != NULL)
        {
            for (mob_data::DmgLogEntry& dle : md->dmglogv)
            {
                if (dle.id == sd->bl_id)
                {
                    dle.dmg += damage;
                    goto damage_logged_pc;
                }
            }
            //else
            {
                mob_data::DmgLogEntry app;
                app.id = sd->bl_id;
                app.dmg = damage;
                md->dmglogv.push_back(app);
            }
        damage_logged_pc:

            if (md->attacked_id <= 0 && md->state.special_mob_ai == 0)
                md->attacked_id = sd->bl_id;
        }
        if (src && src->bl_type == BL::MOB
            && src->is_mob()->state.special_mob_ai)
        {
            dumb_ptr<mob_data> md2 = src->is_mob();
            dumb_ptr<block_list> master_bl = map_id2bl(md2->master_id);
            if (master_bl && master_bl->bl_type == BL::PC)
            {
                MAP_LOG_PC(master_bl->is_player(),
                            "MOB-TO-MOB-DMG FROM MOB%d %d TO MOB%d %d FOR %d",
                            md2->bl_id, md2->mob_class, md->bl_id, md->mob_class,
                            damage);
            }

            nullpo_ret(md2);
            for (mob_data::DmgLogEntry& dle : md->dmglogv)
            {
                if (dle.id == md2->master_id)
                {
                    dle.dmg += damage;
                    goto damage_logged_slave;
                }
            }
            //else
            {
                mob_data::DmgLogEntry app;
                app.id = md2->master_id;
                app.dmg = damage;
                md->dmglogv.push_back(app);

                if (md->attacked_id <= 0 && md->state.special_mob_ai == 0)
                    md->attacked_id = md2->master_id;
            }
        damage_logged_slave:
            ;
        }
    }

    md->hp -= damage;

    if (md->hp > 0)
    {
        return 0;
    }

    MAP_LOG("MOB%d DEAD", md->bl_id);

    // ----- ここから死亡処理 -----

    MapBlockLock lock;
    // cancels timers
    mob_changestate(md, MS::DEAD, 0);
    mobskill_use(md, tick, MobSkillCondition::ANY);

    max_hp = battle_get_max_hp(md);

    if (src && src->bl_type == BL::MOB)
        mob_unlocktarget(src->is_mob(), tick);

    // map外に消えた人は計算から除くので
    // overkill分は無いけどsumはmax_hpとは違う

    // snip a prelude loop, now merged

    std::sort(md->dmglogv.begin(), md->dmglogv.end(),
            [](const mob_data::DmgLogEntry& l, const mob_data::DmgLogEntry& r) -> bool
            {
                // reversed
                return l.dmg > r.dmg;
            }
    );

    {
        struct DmgLogParty
        {
            struct party *p;
            int base_exp, job_exp;
        };
        std::vector<DmgLogParty> ptv;

        int mvp_dmg = 0, second_dmg = 0, third_dmg = 0;
        for (mob_data::DmgLogEntry& dle : md->dmglogv)
        {
            dumb_ptr<map_session_data> tmpsdi = map_id2sd(dle.id);
            int tmpdmg = dle.dmg;
            if (tmpsdi == NULL)
                continue;
            if (tmpsdi->bl_m != md->bl_m || pc_isdead(tmpsdi))
                continue;

            // this way is actually fair, unlike the old way
            // that refers to the subsequents ... was buggy though
            if (tmpdmg > mvp_dmg)
            {
                third_sd = second_sd;
                third_dmg = second_dmg;
                second_sd = mvp_sd;
                second_dmg = mvp_dmg;
                mvp_sd = tmpsdi;
                mvp_dmg = tmpdmg;
            }
            else if (tmpdmg > second_dmg)
            {
                third_sd = second_sd;
                third_dmg = second_dmg;
                second_sd = tmpsdi;
                second_dmg = tmpdmg;
            }
            else if (tmpdmg > third_dmg)
            {
                third_sd = tmpsdi;
                third_dmg = tmpdmg;
            }

            int base_exp, job_exp, flag = 1;
            double per;
            struct party *p;

            // [Fate] The above is the old formula.  We do a more involved computation below.
            // [o11c] Look in git history for old code, you idiot!
            // 256 = 100% of the score
            per = static_cast<double>(dle.dmg) * 256 / static_cast<double>(max_hp);
            size_t count = md->dmglogv.size();
            if (count > DAMAGE_BONUS_COUNT)
                count = DAMAGE_BONUS_COUNT;
            per *= damage_bonus_factor[count];    // Bonus for party attack
            if (per > 512)
                per = 512;      // [Fate] Retained from before.  The maximum a single individual can get is double the original value.
            if (per < 1)
                per = 1;

            base_exp =
                ((mob_db[md->mob_class].base_exp *
                  md->stats[mob_stat::XP_BONUS]) >> MOB_XP_BONUS_SHIFT) * per / 256;
            if (base_exp < 1)
                base_exp = 1;
            if (sd && md && battle_config.pk_mode == 1
                && (mob_db[md->mob_class].lv - sd->status.base_level >= 20))
            {
                base_exp *= 1.15;   // pk_mode additional exp if monster >20 levels [Valaris]
            }
            if (md->state.special_mob_ai >= 1
                && battle_config.alchemist_summon_reward != 1)
                base_exp = 0;   // Added [Valaris]
            job_exp = mob_db[md->mob_class].job_exp * per / 256;
            if (job_exp < 1)
                job_exp = 1;
            if (sd && md && battle_config.pk_mode == 1
                && (mob_db[md->mob_class].lv - sd->status.base_level >= 20))
            {
                job_exp *= 1.15;    // pk_mode additional exp if monster >20 levels [Valaris]
            }
            if (md->state.special_mob_ai >= 1
                && battle_config.alchemist_summon_reward != 1)
                job_exp = 0;    // Added [Valaris]

            int pid = tmpsdi->status.party_id;
            if (pid > 0)
            {
                std::vector<DmgLogParty>::iterator it = std::find_if(ptv.begin(), ptv.end(),
                        [pid](const DmgLogParty& dlp)
                        {
                            return dlp.p->party_id == pid;
                        }
                );
                if (it == ptv.end())
                {
                    p = party_search(pid);
                    if (p != NULL && p->exp != 0)
                    {
                        DmgLogParty pn {};
                        pn.p = p;
                        pn.base_exp = base_exp;
                        pn.job_exp = job_exp;
                        ptv.push_back(pn);
                        flag = 0;
                    }
                }
                else
                {
                    it->base_exp += base_exp;
                    it->job_exp += job_exp;
                    flag = 0;
                }
            }
            if (flag)
                // not sharing
                pc_gainexp_reason(tmpsdi, base_exp, job_exp,
                                  PC_GAINEXP_REASON::KILLING);
        }
        for (DmgLogParty& pti : ptv)
            party_exp_share(pti.p, md->bl_m, pti.base_exp, pti.job_exp);

        // item drop
        if (!(type & 1))
        {
            for (int i = 0; i < 8; i++)
            {
                if (md->state.special_mob_ai >= 1 && battle_config.alchemist_summon_reward != 1)    // Added [Valaris]
                    break;      // End

                if (mob_db[md->mob_class].dropitem[i].nameid <= 0)
                    continue;
                random_::Fixed<int, 10000> drop_rate = mob_db[md->mob_class].dropitem[i].p;
                if (battle_config.drops_by_luk > 0 && sd && md)
                    drop_rate.num += (sd->status.attrs[ATTR::LUK] * battle_config.drops_by_luk) / 100;   // drops affected by luk [Valaris]
                if (sd && md && battle_config.pk_mode == 1
                    && (mob_db[md->mob_class].lv - sd->status.base_level >= 20))
                    drop_rate.num *= 1.25;  // pk_mode increase drops if 20 level difference [Valaris]
                if (!random_::chance(drop_rate))
                    continue;

                struct delay_item_drop ditem {};
                ditem.nameid = mob_db[md->mob_class].dropitem[i].nameid;
                ditem.amount = 1;
                ditem.m = md->bl_m;
                ditem.x = md->bl_x;
                ditem.y = md->bl_y;
                ditem.first_sd = mvp_sd;
                ditem.second_sd = second_sd;
                ditem.third_sd = third_sd;
                Timer(tick + std::chrono::milliseconds(500) + static_cast<interval_t>(i),
                        std::bind(mob_delay_item_drop, ph::_1, ph::_2,
                            ditem)
                ).detach();
            }
            {
                int i = 0;
                for (struct item lit : md->lootitemv)
                {
                    struct delay_item_drop2 ditem {};
                    ditem.item_data = lit;
                    ditem.m = md->bl_m;
                    ditem.x = md->bl_x;
                    ditem.y = md->bl_y;
                    ditem.first_sd = mvp_sd;
                    ditem.second_sd = second_sd;
                    ditem.third_sd = third_sd;
                    // ?
                    Timer(tick + std::chrono::milliseconds(540) + static_cast<interval_t>(i),
                            std::bind(mob_delay_item_drop2, ph::_1, ph::_2,
                                ditem)
                    ).detach();
                    i++;
                }
            }
        }
    }                           // [MouseJstr]

    // SCRIPT実行
    if (md->npc_event)
    {
        if (sd == NULL)
        {
            if (mvp_sd != NULL)
                sd = mvp_sd;
            else
            {
                for (int i = 0; i < fd_max; i++)
                {
                    if (!session[i])
                        continue;
                    dumb_ptr<map_session_data> tmp_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
                    if (tmp_sd && tmp_sd->state.auth)
                    {
                        if (md->bl_m == tmp_sd->bl_m)
                        {
                            sd = tmp_sd;
                            break;
                        }
                    }
                }
            }
        }
        if (sd)
            npc_event(sd, md->npc_event, 0);
    }

    clif_clearchar(md, BeingRemoveWhy::DEAD);
    map_delblock(md);
    mob_deleteslave(md);
    mob_setdelayspawn(md->bl_id);

    return 0;
}

/*==========================================
 * mob回復
 *------------------------------------------
 */
int mob_heal(dumb_ptr<mob_data> md, int heal)
{
    int max_hp = battle_get_max_hp(md);

    nullpo_ret(md);

    md->hp += heal;
    if (max_hp < md->hp)
        md->hp = max_hp;

    return 0;
}

/*==========================================
 * Added by RoVeRT
 *------------------------------------------
 */
static
void mob_warpslave_sub(dumb_ptr<block_list> bl, int id, int x, int y)
{
    dumb_ptr<mob_data> md = bl->is_mob();

    if (md->master_id == id)
    {
        mob_warp(md, nullptr, x, y, BeingRemoveWhy::QUIT);
    }
}

/*==========================================
 * Added by RoVeRT
 *------------------------------------------
 */
static
int mob_warpslave(dumb_ptr<mob_data> md, int x, int y)
{
//PRINTF("warp slave\n");
    map_foreachinarea(std::bind(mob_warpslave_sub, ph::_1, md->bl_id, md->bl_x, md->bl_y),
            md->bl_m,
            x - AREA_SIZE, y - AREA_SIZE,
            x + AREA_SIZE, y + AREA_SIZE,
            BL::MOB);
    return 0;
}

/*==========================================
 * mobワープ
 *------------------------------------------
 */
int mob_warp(dumb_ptr<mob_data> md, map_local *m, int x, int y, BeingRemoveWhy type)
{
    int i = 0, xs = 0, ys = 0, bx = x, by = y;

    nullpo_ret(md);

    if (md->bl_prev == NULL)
        return 0;

    if (m == nullptr)
        m = md->bl_m;

    if (type != BeingRemoveWhy::NEGATIVE1)
    {
        if (md->bl_m->flag.monster_noteleport)
            return 0;
        clif_clearchar(md, type);
    }
    map_delblock(md);

    if (bx > 0 && by > 0)
    {                           // 位置指定の場合周囲９セルを探索
        xs = ys = 9;
    }

    while ((x < 0
            || y < 0
            || bool(read_gatp(m, x, y) & MapCell::UNWALKABLE))
       && (i++) < 1000)
    {
        if (xs > 0 && ys > 0 && i < 250)
        {
            // 指定位置付近の探索
            x = bx + random_::to(xs) - xs / 2;
            y = by + random_::to(ys) - ys / 2;
        }
        else
        {
            // 完全ランダム探索
            x = random_::in(1, m->xs - 2);
            y = random_::in(1, m->ys - 2);
        }
    }
    md->dir = DIR::S;
    if (i < 1000)
    {
        md->bl_x = md->to_x = x;
        md->bl_y = md->to_y = y;
        md->bl_m = m;
    }
    else
    {
        if (battle_config.error_log == 1)
            PRINTF("MOB %d warp failed, mob_class = %d\n", md->bl_id, md->mob_class);
    }

    md->target_id = 0;          // タゲを解除する
    md->state.attackable = false;
    md->attacked_id = 0;
    md->state.skillstate = MobSkillState::MSS_IDLE;
    mob_changestate(md, MS::IDLE, 0);

    if (type != BeingRemoveWhy::GONE && type != BeingRemoveWhy::NEGATIVE1
        && i == 1000)
    {
        if (battle_config.battle_log == 1)
            PRINTF("MOB %d warp to (%d,%d), mob_class = %d\n", md->bl_id, x, y,
                    md->mob_class);
    }

    map_addblock(md);
    if (type != BeingRemoveWhy::GONE && type != BeingRemoveWhy::NEGATIVE1)
    {
        clif_spawnmob(md);
        mob_warpslave(md, md->bl_x, md->bl_y);
    }

    return 0;
}

/*==========================================
 * 画面内の取り巻きの数計算用(foreachinarea)
 *------------------------------------------
 */
static
void mob_countslave_sub(dumb_ptr<block_list> bl, int id, int *c)
{
    dumb_ptr<mob_data> md;

    nullpo_retv(bl);
    md = bl->is_mob();

    if (md->master_id == id)
        (*c)++;
}

/*==========================================
 * 画面内の取り巻きの数計算
 *------------------------------------------
 */
static
int mob_countslave(dumb_ptr<mob_data> md)
{
    int c = 0;

    nullpo_ret(md);

    map_foreachinarea(std::bind(mob_countslave_sub, ph::_1, md->bl_id, &c),
            md->bl_m,
            0, 0,
            md->bl_m->xs - 1, md->bl_m->ys - 1,
            BL::MOB);
    return c;
}

/*==========================================
 * 手下MOB召喚
 *------------------------------------------
 */
int mob_summonslave(dumb_ptr<mob_data> md2, int *value, int amount, int flag)
{
    dumb_ptr<mob_data> md;
    int bx, by, count = 0, mob_class, k, a = amount;

    nullpo_ret(md2);
    nullpo_ret(value);

    bx = md2->bl_x;
    by = md2->bl_y;
    map_local *m = md2->bl_m;

    if (value[0] <= 1000 || value[0] > 2000)    // 値が異常なら召喚を止める
        return 0;
    while (count < 5 && value[count] > 1000 && value[count] <= 2000)
        count++;
    if (count < 1)
        return 0;

    for (k = 0; k < count; k++)
    {
        amount = a;
        mob_class = value[k];
        if (mob_class <= 1000 || mob_class > 2000)
            continue;
        for (; amount > 0; amount--)
        {
            int x = 0, y = 0, i = 0;
            md.new_();
            if (bool(mob_db[mob_class].mode & MobMode::LOOTER))
                md->lootitemv.clear();

            while ((x <= 0
                    || y <= 0
                    || bool(map_getcell(m, x, y) & MapCell::UNWALKABLE))
                && (i++) < 100)
            {
                x = bx + random_::in(-4, 4);
                y = by + random_::in(-4, 4);
            }
            if (i >= 100)
            {
                x = bx;
                y = by;
            }

            mob_spawn_dataset(md, JAPANESE_NAME, mob_class);
            md->bl_prev = NULL;
            md->bl_next = NULL;
            md->bl_m = m;
            md->bl_x = x;
            md->bl_y = y;

            md->spawn.m = m;
            md->spawn.x0 = x;
            md->spawn.y0 = y;
            md->spawn.xs = 0;
            md->spawn.ys = 0;
            md->stats[mob_stat::SPEED] = md2->stats[mob_stat::SPEED];
            md->spawn.delay1 = static_cast<interval_t>(-1);   // 一度のみフラグ
            md->spawn.delay2 = static_cast<interval_t>(-1);   // 一度のみフラグ

            md->npc_event = NpcEvent();
            md->bl_type = BL::MOB;
            map_addiddb(md);
            mob_spawn(md->bl_id);

            if (flag)
                md->master_id = md2->bl_id;
        }
    }
    return 0;
}

/*==========================================
 * 自分をロックしているPCの数を数える(foreachclient)
 *------------------------------------------
 */
static
void mob_counttargeted_sub(dumb_ptr<block_list> bl,
        int id, int *c, dumb_ptr<block_list> src, ATK target_lv)
{
    nullpo_retv(bl);
    nullpo_retv(c);

    if (id == bl->bl_id || (src && id == src->bl_id))
        return;
    if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = bl->is_player();
        if (sd && sd->attacktarget == id && sd->attacktimer
            && sd->attacktarget_lv >= target_lv)
            (*c)++;
    }
    else if (bl->bl_type == BL::MOB)
    {
        dumb_ptr<mob_data> md = bl->is_mob();
        if (md && md->target_id == id && md->timer
            && md->state.state == MS::ATTACK && md->target_lv >= target_lv)
            (*c)++;
    }
}

/*==========================================
 * 自分をロックしているPCの数を数える
 *------------------------------------------
 */
int mob_counttargeted(dumb_ptr<mob_data> md, dumb_ptr<block_list> src,
        ATK target_lv)
{
    int c = 0;

    nullpo_ret(md);

    map_foreachinarea(std::bind(mob_counttargeted_sub, ph::_1, md->bl_id, &c, src, target_lv),
            md->bl_m,
            md->bl_x - AREA_SIZE, md->bl_y - AREA_SIZE,
            md->bl_x + AREA_SIZE, md->bl_y + AREA_SIZE,
            BL::NUL);
    return c;
}

//
// MOBスキル
//

/*==========================================
 * スキル使用（詠唱完了、ID指定）
 *------------------------------------------
 */
void mobskill_castend_id(TimerData *, tick_t tick, int id)
{
    dumb_ptr<mob_data> md = NULL;
    dumb_ptr<block_list> bl;
    dumb_ptr<block_list> mbl;
    int range;

    if ((mbl = map_id2bl(id)) == NULL) //詠唱したMobがもういないというのは良くある正常処理
        return;
    if ((md = mbl->is_mob()) == NULL)
    {
        PRINTF("mobskill_castend_id nullpo mbl->bl_id:%d\n", mbl->bl_id);
        return;
    }
    if (md->bl_type != BL::MOB || md->bl_prev == NULL)
        return;

    if (bool(md->opt1))
        return;

    if (md->skillid != SkillID::NPC_EMOTION)
        md->last_thinktime = tick + battle_get_adelay(md);

    if ((bl = map_id2bl(md->skilltarget)) == NULL || bl->bl_prev == NULL)
    {                           //スキルターゲットが存在しない
        //PRINTF("mobskill_castend_id nullpo\n");//ターゲットがいないときはnullpoじゃなくて普通に終了
        return;
    }
    if (md->bl_m != bl->bl_m)
        return;

    if (((skill_get_inf(md->skillid) & 1) || (skill_get_inf2(md->skillid) & 4)) &&    // 彼我敵対関係チェック
        battle_check_target(md, bl, BCT_ENEMY) <= 0)
        return;
    range = skill_get_range(md->skillid, md->skilllv);
    if (range < 0)
        range = battle_get_range(md) - (range + 1);
    if (range + battle_config.monster_skill_add_range < distance(md->bl_x, md->bl_y, bl->bl_x, bl->bl_y))
        return;

    md->skilldelayup[md->skillidx - &mob_db[md->mob_class].skills.front()] = tick;

    if (battle_config.monster_skill_log == 1)
        PRINTF("MOB skill castend skill=%d, mob_class = %d\n",
                md->skillid, md->mob_class);
    mob_stop_walking(md, 0);

    switch (skill_get_nk(md->skillid))
    {
            // 攻撃系/吹き飛ばし系
        case 0:
        case 2:
            skill_castend_damage_id(md, bl,
                    md->skillid, md->skilllv,
                    tick, BCT_ZERO);
            break;
        case 1:                // 支援系
            skill_castend_nodamage_id(md, bl,
                    md->skillid, md->skilllv);
            break;
    }
}

/*==========================================
 * スキル使用（詠唱完了、場所指定）
 *------------------------------------------
 */
void mobskill_castend_pos(TimerData *, tick_t tick, int id)
{
    dumb_ptr<mob_data> md = NULL;
    dumb_ptr<block_list> bl;
    int range;

    //mobskill_castend_id同様詠唱したMobが詠唱完了時にもういないというのはありそうなのでnullpoから除外
    if ((bl = map_id2bl(id)) == NULL)
        return;

    md = bl->is_mob();
    nullpo_retv(md);

    if (md->bl_type != BL::MOB || md->bl_prev == NULL)
        return;

    if (bool(md->opt1))
        return;

    range = skill_get_range(md->skillid, md->skilllv);
    if (range < 0)
        range = battle_get_range(md) - (range + 1);
    if (range + battle_config.monster_skill_add_range < distance(md->bl_x, md->bl_y, md->skillx, md->skilly))
        return;
    md->skilldelayup[md->skillidx - &mob_db[md->mob_class].skills.front()] = tick;

    if (battle_config.monster_skill_log == 1)
        PRINTF("MOB skill castend skill=%d, mob_class = %d\n",
                md->skillid, md->mob_class);
    mob_stop_walking(md, 0);
}

/*==========================================
 * Skill use (an aria start, ID specification)
 *------------------------------------------
 */
int mobskill_use_id(dumb_ptr<mob_data> md, dumb_ptr<block_list> target,
        mob_skill& skill_idx)
{
    int range;
    struct mob_skill *ms;
    SkillID skill_id;
    int skill_lv;

    nullpo_ret(md);
    ms = &skill_idx;

    if (target == NULL && (target = map_id2bl(md->target_id)) == NULL)
        return 0;

    if (target->bl_prev == NULL || md->bl_prev == NULL)
        return 0;

    skill_id = ms->skill_id;
    skill_lv = ms->skill_lv;

    if (bool(md->opt1))
        return 0;

    if (skill_get_inf2(skill_id) & 0x200 && md->bl_id == target->bl_id)
        return 0;

    // 射程と障害物チェック
    range = skill_get_range(skill_id, skill_lv);
    if (range < 0)
        range = battle_get_range(md) - (range + 1);

    if (!battle_check_range(md, target, range))
        return 0;

//  delay=skill_delayfix(md, skill_get_delay( skill_id,skill_lv) );

    interval_t casttime = skill_castfix(md, ms->casttime);
    md->state.skillcastcancel = ms->cancel;
    md->skilldelayup[ms - &mob_db[md->mob_class].skills.front()] = gettick();

    if (battle_config.monster_skill_log == 1)
        PRINTF("MOB skill use target_id=%d skill=%d lv=%d cast=%d, mob_class = %d\n",
                target->bl_id, skill_id, skill_lv,
                static_cast<uint32_t>(casttime.count()), md->mob_class);

    if (casttime <= interval_t::zero())          // 詠唱の無いものはキャンセルされない
        md->state.skillcastcancel = 0;

    md->skilltarget = target->bl_id;
    md->skillx = 0;
    md->skilly = 0;
    md->skillid = skill_id;
    md->skilllv = skill_lv;
    md->skillidx = &skill_idx;

    if (casttime > interval_t::zero())
    {
        md->skilltimer = Timer(gettick() + casttime,
                std::bind(mobskill_castend_id, ph::_1, ph::_2,
                    md->bl_id));
    }
    else
    {
        assert (!md->skilltimer);
        mobskill_castend_id(nullptr, gettick(), md->bl_id);
    }

    return 1;
}

/*==========================================
 * スキル使用（場所指定）
 *------------------------------------------
 */
static
int mobskill_use_pos(dumb_ptr<mob_data> md,
        int skill_x, int skill_y, mob_skill& skill_idx)
{
    int range;
    struct mob_skill *ms;
    struct block_list bl;
    int skill_lv;

    nullpo_ret(md);
    ms = &skill_idx;

    if (md->bl_prev == NULL)
        return 0;

    SkillID skill_id = ms->skill_id;
    skill_lv = ms->skill_lv;

    if (bool(md->opt1))
        return 0;

    // 射程と障害物チェック
    bl.bl_type = BL::NUL;
    bl.bl_m = md->bl_m;
    bl.bl_x = skill_x;
    bl.bl_y = skill_y;
    range = skill_get_range(skill_id, skill_lv);
    if (range < 0)
        range = battle_get_range(md) - (range + 1);
    if (!battle_check_range(md, dumb_ptr<block_list>(&bl), range))
        return 0;

//  delay=skill_delayfix(sd, skill_get_delay( skill_id,skill_lv) );
    interval_t casttime = skill_castfix(md, ms->casttime);
    md->skilldelayup[ms - &mob_db[md->mob_class].skills.front()] = gettick();
    md->state.skillcastcancel = ms->cancel;

    if (battle_config.monster_skill_log == 1)
        PRINTF("MOB skill use target_pos= (%d,%d) skill=%d lv=%d cast=%d, mob_class = %d\n",
             skill_x, skill_y, skill_id, skill_lv,
             static_cast<uint32_t>(casttime.count()), md->mob_class);

    if (casttime <= interval_t::zero())
        // A skill without a cast time wont be cancelled.
        md->state.skillcastcancel = 0;

    md->skillx = skill_x;
    md->skilly = skill_y;
    md->skilltarget = 0;
    md->skillid = skill_id;
    md->skilllv = skill_lv;
    md->skillidx = &skill_idx;
    if (casttime > interval_t::zero())
    {
        md->skilltimer = Timer(gettick() + casttime,
                std::bind(mobskill_castend_pos, ph::_1, ph::_2,
                    md->bl_id));
    }
    else
    {
        assert (!md->skilltimer);
        mobskill_castend_pos(nullptr, gettick(), md->bl_id);
    }

    return 1;
}

/*==========================================
 * Skill use judging
 *------------------------------------------
 */
int mobskill_use(dumb_ptr<mob_data> md, tick_t tick,
        MobSkillCondition event)
{
    int max_hp;

    nullpo_ret(md);
    std::vector<mob_skill>& ms = mob_db[md->mob_class].skills;

    max_hp = battle_get_max_hp(md);

    if (battle_config.mob_skill_use == 0 || md->skilltimer)
        return 0;

    if (md->state.special_mob_ai)
        return 0;

    for (mob_skill& msii : ms)
    {
        tick_t& sdii = md->skilldelayup[&msii - &ms.front()];
        int flag = 0;

        // ディレイ中
        if (tick < sdii + msii.delay)
            continue;

        // 状態判定
        if (msii.state != MobSkillState::ANY && msii.state != md->state.skillstate)
            continue;

        // Note: these *may* both be MobSkillCondition::ANY
        flag = (event == msii.cond1);
        if (!flag)
        {
            switch (msii.cond1)
            {
                case MobSkillCondition::MSC_ALWAYS:
                    flag = 1;
                    break;
                case MobSkillCondition::MSC_MYHPLTMAXRATE:    // HP< maxhp%
                    flag = (md->hp < max_hp * msii.cond2i / 100);
                    break;
                case MobSkillCondition::MSC_NOTINTOWN:     // Only outside of towns.
                    flag = !md->bl_m->flag.town;
                    break;
                case MobSkillCondition::MSC_SLAVELT:  // slave < num
                    flag = (mob_countslave(md) < msii.cond2i);
                    break;
                case MobSkillCondition::MSC_SLAVELE:  // slave <= num
                    flag = (mob_countslave(md) <= msii.cond2i);
                    break;
            }
        }

        // 確率判定
        if (flag && random_::chance({msii.permillage, 10000}))
        {

            if (skill_get_inf(msii.skill_id) & 2)
            {
                // 場所指定
                dumb_ptr<block_list> bl = NULL;
                int x = 0, y = 0;
                {
                    if (msii.target == MobSkillTarget::MST_TARGET)
                        bl = map_id2bl(md->target_id);
                    else
                        bl = md;

                    if (bl)
                    {
                        x = bl->bl_x;
                        y = bl->bl_y;
                    }
                }
                if (x <= 0 || y <= 0)
                    continue;
                if (!mobskill_use_pos(md, x, y, msii))
                    return 0;
            }
            else
            {
                {
                    dumb_ptr<block_list> bl = NULL;
                    if (msii.target == MobSkillTarget::MST_TARGET)
                        bl = map_id2bl(md->target_id);
                    else
                        bl = md;
                    if (bl && !mobskill_use_id(md, bl, msii))
                        return 0;
                }
            }
            if (msii.emotion >= 0)
                clif_emotion(md, msii.emotion);
            return 1;
        }
    }

    return 0;
}

/*==========================================
 * Skill use event processing
 *------------------------------------------
 */
int mobskill_event(dumb_ptr<mob_data> md, BF flag)
{
    nullpo_ret(md);

    if (flag == BF::NEGATIVE_1
        && mobskill_use(md, gettick(), MobSkillCondition::ANY))
        return 1;
    if (bool(flag & BF::SHORT)
        && mobskill_use(md, gettick(), MobSkillCondition::ANY))
        return 1;
    if (bool(flag & BF::LONG)
        && mobskill_use(md, gettick(), MobSkillCondition::ANY))
        return 1;
    return 0;
}

//
// 初期化
//
/*==========================================
 * Since un-setting [ mob ] up was used, it is an initial provisional value setup.
 *------------------------------------------
 */
static
int mob_makedummymobdb(int mob_class)
{
    int i;

    SNPRINTF(mob_db[mob_class].name, 24, "mob%d", mob_class);
    SNPRINTF(mob_db[mob_class].jname, 24, "mob%d", mob_class);
    mob_db[mob_class].lv = 1;
    mob_db[mob_class].max_hp = 1000;
    mob_db[mob_class].max_sp = 1;
    mob_db[mob_class].base_exp = 2;
    mob_db[mob_class].job_exp = 1;
    mob_db[mob_class].range = 1;
    mob_db[mob_class].atk1 = 7;
    mob_db[mob_class].atk2 = 10;
    mob_db[mob_class].def = 0;
    mob_db[mob_class].mdef = 0;
    mob_db[mob_class].attrs[ATTR::STR] = 1;
    mob_db[mob_class].attrs[ATTR::AGI] = 1;
    mob_db[mob_class].attrs[ATTR::VIT] = 1;
    mob_db[mob_class].attrs[ATTR::INT] = 1;
    mob_db[mob_class].attrs[ATTR::DEX] = 6;
    mob_db[mob_class].attrs[ATTR::LUK] = 2;
    mob_db[mob_class].range2 = 10;
    mob_db[mob_class].range3 = 10;
    mob_db[mob_class].size = 0; // 1
    mob_db[mob_class].race = Race::formless;
    mob_db[mob_class].element = LevelElement{0, Element::neutral};
    mob_db[mob_class].mode = MobMode::ZERO;
    mob_db[mob_class].speed = 300;
    mob_db[mob_class].adelay = 1000;
    mob_db[mob_class].amotion = 500;
    mob_db[mob_class].dmotion = 500;
    for (i = 0; i < 8; i++)
    {
        mob_db[mob_class].dropitem[i].nameid = 0;
        mob_db[mob_class].dropitem[i].p.num = 0;
    }
    return 0;
}

static
bool extract(XString str, LevelElement *le)
{
    int tmp;
    if (extract(str, &tmp))
    {
        *le = LevelElement::unpack(tmp);
        return true;
    }
    return false;
}

/*==========================================
 * db/mob_db.txt reading
 *------------------------------------------
 */
static
int mob_readdb(void)
{
    const char *filename = "db/mob_db.txt";

    for (mob_db_& e : mob_db)
        e = mob_db_{};

    {
        std::ifstream in(filename);
        if (!in.is_open())
        {
            return -1;
        }
        FString line;
        while (io::getline(in, line))
        {
            int mob_class;

            if (!line || line.startswith("//"))
                continue;
            struct mob_db_ mdbv {};

            XString ignore;

            bool okay = extract(line, record<','>(
                        &mob_class,
                        lstripping(&mdbv.name),
                        lstripping(&mdbv.jname),
                        lstripping(&mdbv.lv),
                        lstripping(&mdbv.max_hp),
                        lstripping(&mdbv.max_sp),
                        lstripping(&mdbv.base_exp),
                        lstripping(&mdbv.job_exp),
                        lstripping(&mdbv.range),
                        lstripping(&mdbv.atk1),
                        lstripping(&mdbv.atk2),
                        lstripping(&mdbv.def),
                        lstripping(&mdbv.mdef),
                        lstripping(&mdbv.attrs[ATTR::STR]),
                        lstripping(&mdbv.attrs[ATTR::AGI]),
                        lstripping(&mdbv.attrs[ATTR::VIT]),
                        lstripping(&mdbv.attrs[ATTR::INT]),
                        lstripping(&mdbv.attrs[ATTR::DEX]),
                        lstripping(&mdbv.attrs[ATTR::LUK]),
                        lstripping(&mdbv.range2),
                        lstripping(&mdbv.range3),
                        lstripping(&mdbv.size),
                        lstripping(&mdbv.race),
                        lstripping(&mdbv.element),
                        lstripping(&mdbv.mode),
                        lstripping(&mdbv.speed),
                        lstripping(&mdbv.adelay),
                        lstripping(&mdbv.amotion),
                        lstripping(&mdbv.dmotion),
                        lstripping(&mdbv.dropitem[0].nameid),
                        lstripping(&mdbv.dropitem[0].p.num),
                        lstripping(&mdbv.dropitem[1].nameid),
                        lstripping(&mdbv.dropitem[1].p.num),
                        lstripping(&mdbv.dropitem[2].nameid),
                        lstripping(&mdbv.dropitem[2].p.num),
                        lstripping(&mdbv.dropitem[3].nameid),
                        lstripping(&mdbv.dropitem[3].p.num),
                        lstripping(&mdbv.dropitem[4].nameid),
                        lstripping(&mdbv.dropitem[4].p.num),
                        lstripping(&mdbv.dropitem[5].nameid),
                        lstripping(&mdbv.dropitem[5].p.num),
                        lstripping(&mdbv.dropitem[6].nameid),
                        lstripping(&mdbv.dropitem[6].p.num),
                        lstripping(&mdbv.dropitem[7].nameid),
                        lstripping(&mdbv.dropitem[7].p.num),
                        &ignore,
                        &ignore,
                        &ignore,
                        &ignore,
                        &ignore,
                        &ignore,
                        &ignore,
                        &ignore,
                        &ignore,
                        &ignore,
                        lstripping(&mdbv.mutations_nr),
                        lstripping(&mdbv.mutation_power)
                    )
            );

            if (!okay || mob_class <= 1000 || mob_class > 2000)
            {
                PRINTF("bad mob line: %s\n", line);
                continue;
            }

            // TODO move this lower
            mob_db[mob_class] = std::move(mdbv);

            if (mob_db[mob_class].base_exp < 0)
                mob_db[mob_class].base_exp = 0;
            else if (mob_db[mob_class].base_exp > 0
                     && (mob_db[mob_class].base_exp *
                         battle_config.base_exp_rate / 100 > 1000000000
                         || mob_db[mob_class].base_exp *
                         battle_config.base_exp_rate / 100 < 0))
                mob_db[mob_class].base_exp = 1000000000;
            else
                mob_db[mob_class].base_exp = mob_db[mob_class].base_exp * battle_config.base_exp_rate / 100;

            if (mob_db[mob_class].job_exp < 0)
                mob_db[mob_class].job_exp = 0;
            else if (mob_db[mob_class].job_exp > 0
                     && (mob_db[mob_class].job_exp * battle_config.job_exp_rate /
                         100 > 1000000000
                         || mob_db[mob_class].job_exp *
                         battle_config.job_exp_rate / 100 < 0))
                mob_db[mob_class].job_exp = 1000000000;
            else
                mob_db[mob_class].job_exp = mob_db[mob_class].job_exp * battle_config.job_exp_rate / 100;

            for (int i = 0; i < 8; i++)
            {
                int rate = mob_db[mob_class].dropitem[i].p.num;
                if (rate < 1) rate = 1;
                if (rate > 10000) rate = 10000;
                mob_db[mob_class].dropitem[i].p.num = rate;
            }


            mob_db[mob_class].skills.clear();

            mob_db[mob_class].hair = 0;
            mob_db[mob_class].hair_color = 0;
            mob_db[mob_class].weapon = 0;
            mob_db[mob_class].shield = 0;
            mob_db[mob_class].head_top = 0;
            mob_db[mob_class].head_mid = 0;
            mob_db[mob_class].head_buttom = 0;
            mob_db[mob_class].clothes_color = 0;    //Add for player monster dye - Valaris

            if (mob_db[mob_class].base_exp == 0)
                mob_db[mob_class].base_exp = mob_gen_exp(&mob_db[mob_class]);
        }
        PRINTF("read %s done\n", filename);
    }
    return 0;
}

template<>
bool extract<MobSkillCondition, void, void>(XString str, MobSkillCondition *msc)
{
    const struct
    {
        char str[32];
        MobSkillCondition id;
    } cond1[] =
    {
        {"always", MobSkillCondition::MSC_ALWAYS},
        {"myhpltmaxrate", MobSkillCondition::MSC_MYHPLTMAXRATE},
        {"notintown", MobSkillCondition::MSC_NOTINTOWN},
        {"slavelt", MobSkillCondition::MSC_SLAVELT},
        {"slavele", MobSkillCondition::MSC_SLAVELE},
    };
    for (auto& pair : cond1)
        if (str == pair.str)
        {
            *msc = pair.id;
            return true;
        }
    return false;
}

template<>
bool extract<MobSkillState, void, void>(XString str, MobSkillState *mss)
{
    const struct
    {
        char str[32];
        MobSkillState id;
    } state[] =
    {
        {"any", MobSkillState::ANY},
        {"idle", MobSkillState::MSS_IDLE},
        {"walk", MobSkillState::MSS_WALK},
        {"attack", MobSkillState::MSS_ATTACK},
    };
    for (auto& pair : state)
        if (str == pair.str)
        {
            *mss = pair.id;
            return true;
        }
    return false;
}

template<>
bool extract<MobSkillTarget, void, void>(XString str, MobSkillTarget *mst)
{
    const struct
    {
        char str[32];
        MobSkillTarget id;
    } target[] =
    {
        {"target", MobSkillTarget::MST_TARGET},
        {"self", MobSkillTarget::MST_SELF},
    };
    for (auto& pair : target)
        if (str == pair.str)
        {
            *mst = pair.id;
            return true;
        }
    return false;
}

/*==========================================
 * db/mob_skill_db.txt reading
 *------------------------------------------
 */
static
int mob_readskilldb(void)
{
    const char *filename = "db/mob_skill_db.txt";

    {
        std::ifstream in(filename);
        if (!in.is_open())
        {
            PRINTF("can't read %s\n", filename);
            return 0;
        }
        FString line;
        while (io::getline(in, line))
        {
            int mob_id;

            if (line.startswith("//"))
                continue;

            XString blah;
            if (extract(line, record<','>(&mob_id, &blah)) && mob_id > 0 && blah == "clear")
            {
                mob_db[mob_id].skills.clear();
                continue;
            }

            struct mob_skill msv {};
            msv.emotion = -1;
            int casttime, delay;
            XString cancellable;

            if (!extract(
                        line,
                        record<',', 17>(
                            &mob_id,
                            &blah,
                            &msv.state,
                            &msv.skill_id,
                            &msv.skill_lv,
                            &msv.permillage,
                            &casttime,
                            &delay,
                            &cancellable,
                            &msv.target,
                            &msv.cond1,
                            &msv.cond2i,
                            &msv.val[0],
                            &msv.val[1],
                            &msv.val[2],
                            &msv.val[3],
                            &msv.val[4],

                            &msv.emotion
                        )
                    )
            )
                continue;
            if (cancellable == "yes")
                msv.cancel = true;
            else if (cancellable == "no")
                msv.cancel = false;
            else
                continue;

            msv.casttime = std::chrono::milliseconds(casttime);
            msv.delay = std::chrono::milliseconds(delay);

            if (mob_id <= 0)
                continue;

            mob_db[mob_id].skills.push_back(std::move(msv));
        }
        PRINTF("read %s done\n", filename);
    }
    return 0;
}

void mob_reload(void)
{
    /*
     *
     * <empty monster database>
     * mob_read();
     *
     */

    do_init_mob();
}

/*==========================================
 * Circumference initialization of mob
 *------------------------------------------
 */
int do_init_mob(void)
{
    mob_readdb();

    mob_readskilldb();

    Timer(gettick() + MIN_MOBTHINKTIME,
            mob_ai_hard,
            MIN_MOBTHINKTIME
    ).detach();
    Timer(gettick() + MIN_MOBTHINKTIME * 10,
            mob_ai_lazy,
            MIN_MOBTHINKTIME * 10
    ).detach();

    return 0;
}
