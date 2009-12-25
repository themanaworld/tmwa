#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "timer.h"
#include "nullpo.h"
#include "malloc.h"
#include "magic.h"

#include "battle.h"
#include "clif.h"
#include "intif.h"
#include "itemdb.h"
#include "map.h"
#include "mob.h"
#include "party.h"
#include "pc.h"
#include "script.h"
#include "skill.h"
#include "../common/socket.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

int  skill_pool_skills[MAX_POOL_SKILLS];
int  skill_pool_skills_size = 0;

extern void skill_pool_register (int id)
{
    if (skill_pool_skills_size + 1 >= MAX_POOL_SKILLS)
    {
        fprintf (stderr,
                 "Too many pool skills! Increase MAX_POOL_SKILLS and recompile.");
        return;
    }

    skill_pool_skills[skill_pool_skills_size++] = id;
}

char *skill_name (int skill)
{
    if (skill > 0 && skill < MAX_SKILL_DB)
        return skill_names[skill].desc;
    else
        return NULL;
}

int skill_pool (struct map_session_data *sd, int *skills)
{
    int  i, count = 0;

    for (i = 0; count < MAX_SKILL_POOL && i < skill_pool_skills_size; i++)
    {
        int  skill_id = skill_pool_skills[i];
        if (sd->status.skill[skill_id].flags & SKILL_POOL_ACTIVATED)
        {
            if (skills)
                skills[count] = skill_id;
            ++count;
        }
    }

    return count;
}

void skill_pool_empty (struct map_session_data *sd)
{
    int  i;

    for (i = 0; i < skill_pool_skills_size; i++)
    {
        int  skill_id = skill_pool_skills[i];
        sd->status.skill[skill_id].flags = 0;
    }
}

int skill_pool_size (struct map_session_data *sd)
{
    return skill_pool (sd, NULL);
}

int skill_pool_max (struct map_session_data *sd)
{
    return sd->status.skill[TMW_SKILLPOOL].lv;
}

int skill_pool_activate (struct map_session_data *sd, int skill_id)
{
    if (sd->status.skill[skill_id].flags & SKILL_POOL_ACTIVATED)
        return 0;               // Already there
    else if (sd->status.skill[skill_id].id == skill_id  // knows the skill
             && (skill_pool_size (sd) < skill_pool_max (sd)))
    {
        sd->status.skill[skill_id].flags |= SKILL_POOL_ACTIVATED;
        pc_calcstatus (sd, 0);
        MAP_LOG_PC (sd, "SKILL-ACTIVATE %d %d %d", skill_id,
                    sd->status.skill[skill_id].lv, skill_power (sd,
                                                                skill_id));
        return 0;
    }

    return 1;                   // failed
}

int skill_pool_is_activated (struct map_session_data *sd, int skill_id)
{
    return sd->status.skill[skill_id].flags & SKILL_POOL_ACTIVATED;
}

int skill_pool_deactivate (struct map_session_data *sd, int skill_id)
{
    if (sd->status.skill[skill_id].flags & SKILL_POOL_ACTIVATED)
    {
        sd->status.skill[skill_id].flags &= ~SKILL_POOL_ACTIVATED;
        MAP_LOG_PC (sd, "SKILL-DEACTIVATE %d", skill_id);
        pc_calcstatus (sd, 0);
        return 0;
    }

    return 1;
}

int skill_stat (int skill_id)
{
    return skill_db[skill_id].stat;
}

int skill_power (struct map_session_data *sd, int skill_id)
{
    int  stat = skill_stat (skill_id);
    int  stat_value, skill_value;
    int  result;

    if (stat == 0 || !skill_pool_is_activated (sd, skill_id))
        return 0;

    stat_value = battle_get_stat (stat, &(sd->bl));
    skill_value = sd->status.skill[skill_id].lv;

    if ((skill_value * 10) - 1 > stat_value)
        skill_value += (stat_value / 10);
    else
        skill_value *= 2;

    result = (skill_value * stat_value) / 10;

    return result;
}

int skill_power_bl (struct block_list *bl, int skill)
{
    if (bl->type == BL_PC)
        return skill_power ((struct map_session_data *) bl, skill);
    else
        return 0;
}
