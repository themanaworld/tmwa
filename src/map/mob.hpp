#pragma once
//    mob.hpp - Really scary code.
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

#include "mob.t.hpp"

#include "fwd.hpp"

#include "../generic/enum.hpp"
#include "../generic/random.t.hpp"

#include "../net/timer.t.hpp"

#include "battle.t.hpp"
#include "../mmo/clif.t.hpp"
#include "map.hpp"
#include "../mmo/skill.t.hpp"


namespace tmwa
{
namespace map
{
#define ENGLISH_NAME stringish<MobName>("--en--"_s)
#define JAPANESE_NAME stringish<MobName>("--ja--"_s)
#define MOB_THIS_MAP stringish<MapName>("this"_s)

struct mob_skill
{
    MobSkillState state;
    SkillID skill_id;
    short skill_lv;
    short permillage;
    interval_t casttime, delay;
    bool cancel;
    MobSkillCondition cond1;
    int cond2i;
    MobSkillTarget target;
    int val[5];
    short emotion;
};

struct mob_db_
{
    MobName name, jname;
    int lv;
    int max_hp, max_sp;
    int base_exp, job_exp;
    int atk1, atk2;
    int def, mdef;
    earray<int, ATTR, ATTR::COUNT> attrs;
    int range, range2, range3;
    // always 1
    int size;
    Race race;
    LevelElement element;
    MobMode mode;
    interval_t speed, adelay, amotion, dmotion;
    int mutations_nr, mutation_power;
    struct
    {
        ItemNameId nameid;
        random_::Fixed<int, 10000> p;
    } dropitem[8];
    short hair, hair_color, weapon;
    ItemNameId shield, head_top, head_mid, head_buttom;
    short option, clothes_color; // [Valaris]
    int equip;                 // [Valaris]
    std::vector<struct mob_skill> skills;
};
struct mob_db_& get_mob_db(Species);

Species mobdb_searchname(MobName str);
Species mobdb_checkid(Species id);
BlockId mob_once_spawn(dumb_ptr<map_session_data> sd,
        MapName mapname, int x, int y,
        MobName mobname, Species class_, int amount,
        NpcEvent event);
BlockId mob_once_spawn_area(dumb_ptr<map_session_data> sd,
        MapName mapname, int x0, int y0, int x1, int y1,
        MobName mobname, Species class_, int amount,
        NpcEvent event);

int mob_target(dumb_ptr<mob_data> md, dumb_ptr<block_list> bl, int dist);
int mob_aggravate(dumb_ptr<mob_data> md, dumb_ptr<block_list> bl);
int mob_stop_walking(dumb_ptr<mob_data> md, int type);
int mob_stopattack(dumb_ptr<mob_data>);
int mob_spawn(BlockId);
int mob_damage(dumb_ptr<block_list>, dumb_ptr<mob_data>, int, int);
int mob_heal(dumb_ptr<mob_data>, int);
short mob_get_hair(Species);
short mob_get_hair_color(Species);
short mob_get_weapon(Species);
ItemNameId mob_get_shield(Species);
ItemNameId mob_get_head_top(Species);
ItemNameId mob_get_head_mid(Species);
ItemNameId mob_get_head_buttom(Species);
short mob_get_clothes_color(Species);  //player mob dye [Valaris]
int mob_get_equip(Species);       // mob equip [Valaris]

bool mob_readdb(ZString filename);
bool mob_readskilldb(ZString filename);
void do_init_mob2(void);

int mob_delete(dumb_ptr<mob_data> md);
int mob_catch_delete(dumb_ptr<mob_data> md, BeingRemoveWhy type);
void mob_timer_delete(TimerData *, tick_t, BlockId);

int mob_deleteslave(dumb_ptr<mob_data> md);

int mob_counttargeted(dumb_ptr<mob_data> md, dumb_ptr<block_list> src,
        ATK target_lv);

int mob_warp(dumb_ptr<mob_data> md, Option<Borrowed<map_local>> m, int x, int y, BeingRemoveWhy type);

int mobskill_use(dumb_ptr<mob_data> md, tick_t tick, MobSkillCondition event);
int mobskill_event(dumb_ptr<mob_data> md, BF flag);
void mobskill_castend_id(TimerData *tid, tick_t tick, BlockId id);
void mobskill_castend_pos(TimerData *tid, tick_t tick, BlockId id);
int mob_summonslave(dumb_ptr<mob_data> md2, int *value, int amount, int flag);

void mob_reload(void);
} // namespace map
} // namespace tmwa
