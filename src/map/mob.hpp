#ifndef TMWA_MAP_MOB_HPP
#define TMWA_MAP_MOB_HPP

# include "mob.t.hpp"

# include "../generic/random.t.hpp"

# include "../mmo/mmo.hpp"
# include "../mmo/timer.t.hpp"

# include "clif.t.hpp"
# include "map.hpp"
# include "skill.t.hpp"

# define ENGLISH_NAME stringish<MobName>("--en--")
# define JAPANESE_NAME stringish<MobName>("--ja--")
# define MOB_THIS_MAP stringish<MapName>("this")

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
    int speed, adelay, amotion, dmotion;
    int mutations_nr, mutation_power;
    struct
    {
        int nameid;
        random_::Fixed<int, 10000> p;
    } dropitem[8];
    short hair, hair_color, weapon, shield, head_top, head_mid, head_buttom, option, clothes_color; // [Valaris]
    int equip;                 // [Valaris]
    std::vector<struct mob_skill> skills;
};
extern struct mob_db_ mob_db[];

int mobdb_searchname(MobName str);
int mobdb_checkid(const int id);
int mob_once_spawn(dumb_ptr<map_session_data> sd,
        MapName mapname, int x, int y,
        MobName mobname, int class_, int amount,
        NpcEvent event);
int mob_once_spawn_area(dumb_ptr<map_session_data> sd,
        MapName mapname, int x0, int y0, int x1, int y1,
        MobName mobname, int class_, int amount,
        NpcEvent event);

int mob_target(dumb_ptr<mob_data> md, dumb_ptr<block_list> bl, int dist);
int mob_stop_walking(dumb_ptr<mob_data> md, int type);
int mob_stopattack(dumb_ptr<mob_data>);
int mob_spawn(int);
int mob_damage(dumb_ptr<block_list>, dumb_ptr<mob_data>, int, int);
int mob_heal(dumb_ptr<mob_data>, int);
short mob_get_hair(int);
short mob_get_hair_color(int);
short mob_get_weapon(int);
short mob_get_shield(int);
short mob_get_head_top(int);
short mob_get_head_mid(int);
short mob_get_head_buttom(int);
short mob_get_clothes_color(int);  //player mob dye [Valaris]
int mob_get_equip(int);       // mob equip [Valaris]

bool mob_readdb(ZString filename);
bool mob_readskilldb(ZString filename);
void do_init_mob2(void);

int mob_delete(dumb_ptr<mob_data> md);
int mob_catch_delete(dumb_ptr<mob_data> md, BeingRemoveWhy type);
void mob_timer_delete(TimerData *, tick_t, int);

int mob_deleteslave(dumb_ptr<mob_data> md);

int mob_counttargeted(dumb_ptr<mob_data> md, dumb_ptr<block_list> src,
        ATK target_lv);

int mob_warp(dumb_ptr<mob_data> md, map_local *m, int x, int y, BeingRemoveWhy type);

int mobskill_use(dumb_ptr<mob_data> md, tick_t tick, MobSkillCondition event);
int mobskill_event(dumb_ptr<mob_data> md, BF flag);
void mobskill_castend_id(TimerData *tid, tick_t tick, int id);
void mobskill_castend_pos(TimerData *tid, tick_t tick, int id);
int mob_summonslave(dumb_ptr<mob_data> md2, int *value, int amount, int flag);

void mob_reload(void);

#endif // TMWA_MAP_MOB_HPP
