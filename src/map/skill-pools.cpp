#include "skill.hpp"

#include "../common/cxxstdio.hpp"

#include "battle.hpp"
#include "pc.hpp"

#include "../poison.hpp"

SkillID skill_pool_skills[MAX_POOL_SKILLS];
int skill_pool_skills_size = 0;

void skill_pool_register(SkillID id)
{
    if (skill_pool_skills_size + 1 >= MAX_POOL_SKILLS)
    {
        FPRINTF(stderr,
                 "Too many pool skills! Increase MAX_POOL_SKILLS and recompile.");
        return;
    }

    skill_pool_skills[skill_pool_skills_size++] = id;
}

int skill_pool(struct map_session_data *sd, SkillID *skills)
{
    int i, count = 0;

    for (i = 0; count < MAX_SKILL_POOL && i < skill_pool_skills_size; i++)
    {
        SkillID skill_id = skill_pool_skills[i];
        if (bool(sd->status.skill[skill_id].flags & SkillFlags::POOL_ACTIVATED))
        {
            if (skills)
                skills[count] = skill_id;
            ++count;
        }
    }

    return count;
}

int skill_pool_size(struct map_session_data *sd)
{
    return skill_pool(sd, NULL);
}

int skill_pool_max(struct map_session_data *sd)
{
    return sd->status.skill[SkillID::TMW_SKILLPOOL].lv;
}

int skill_pool_activate(struct map_session_data *sd, SkillID skill_id)
{
    if (bool(sd->status.skill[skill_id].flags & SkillFlags::POOL_ACTIVATED))
        return 0;               // Already there
    else if (sd->status.skill[skill_id].lv
             && (skill_pool_size(sd) < skill_pool_max(sd)))
    {
        sd->status.skill[skill_id].flags |= SkillFlags::POOL_ACTIVATED;
        pc_calcstatus(sd, 0);
        MAP_LOG_PC(sd, "SKILL-ACTIVATE %d %d %d",
                skill_id, sd->status.skill[skill_id].lv,
                skill_power(sd, skill_id));
        return 0;
    }

    return 1;                   // failed
}

bool skill_pool_is_activated(struct map_session_data *sd, SkillID skill_id)
{
    return bool(sd->status.skill[skill_id].flags & SkillFlags::POOL_ACTIVATED);
}

int skill_pool_deactivate(struct map_session_data *sd, SkillID skill_id)
{
    if (bool(sd->status.skill[skill_id].flags & SkillFlags::POOL_ACTIVATED))
    {
        sd->status.skill[skill_id].flags &= ~SkillFlags::POOL_ACTIVATED;
        MAP_LOG_PC(sd, "SKILL-DEACTIVATE %d", skill_id);
        pc_calcstatus(sd, 0);
        return 0;
    }

    return 1;
}

// Yields the stat associated with a skill.
// Returns zero if none, or SP::STR, SP::VIT, ... otherwise
static
SP skill_stat(SkillID skill_id)
{
    return skill_db[skill_id].stat;
}

int skill_power(struct map_session_data *sd, SkillID skill_id)
{
    SP stat = skill_stat(skill_id);
    int stat_value, skill_value;
    int result;

    if (stat == SP::ZERO || !skill_pool_is_activated(sd, skill_id))
        return 0;

    stat_value = battle_get_stat(stat, &(sd->bl));
    skill_value = sd->status.skill[skill_id].lv;

    if ((skill_value * 10) - 1 > stat_value)
        skill_value += (stat_value / 10);
    else
        skill_value *= 2;

    result = (skill_value * stat_value) / 10;

    return result;
}

int skill_power_bl(struct block_list *bl, SkillID skill)
{
    if (bl->type == BL::PC)
        return skill_power((struct map_session_data *) bl, skill);
    else
        return 0;
}
