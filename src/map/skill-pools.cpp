#include "skill-pools.hpp"
#include "skill.hpp"
//    skill-pools.cpp - Additional support for focusable skills.
//
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

#include "../io/cxxstdio.hpp"

#include "../mmo/cxxstdio_enums.hpp"

#include "battle.hpp"
#include "pc.hpp"

#include "../poison.hpp"


namespace tmwa
{
Array<SkillID, MAX_POOL_SKILLS> skill_pool_skills;
int skill_pool_skills_size = 0;

void skill_pool_register(SkillID id)
{
    if (skill_pool_skills_size + 1 >= MAX_POOL_SKILLS)
    {
        FPRINTF(stderr,
                "Too many pool skills! Increase MAX_POOL_SKILLS and recompile."_fmt);
        return;
    }

    skill_pool_skills[skill_pool_skills_size++] = id;
}

int skill_pool(dumb_ptr<map_session_data> sd, SkillID *skills)
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

int skill_pool_size(dumb_ptr<map_session_data> sd)
{
    return skill_pool(sd, nullptr);
}

int skill_pool_max(dumb_ptr<map_session_data> sd)
{
    return sd->status.skill[SkillID::TMW_SKILLPOOL].lv;
}

int skill_pool_activate(dumb_ptr<map_session_data> sd, SkillID skill_id)
{
    if (bool(sd->status.skill[skill_id].flags & SkillFlags::POOL_ACTIVATED))
        return 0;               // Already there
    else if (sd->status.skill[skill_id].lv
             && (skill_pool_size(sd) < skill_pool_max(sd)))
    {
        sd->status.skill[skill_id].flags |= SkillFlags::POOL_ACTIVATED;
        pc_calcstatus(sd, 0);
        MAP_LOG_PC(sd, "SKILL-ACTIVATE %d %d %d"_fmt,
                skill_id, sd->status.skill[skill_id].lv,
                skill_power(sd, skill_id));
        return 0;
    }

    return 1;                   // failed
}

bool skill_pool_is_activated(dumb_ptr<map_session_data> sd, SkillID skill_id)
{
    return bool(sd->status.skill[skill_id].flags & SkillFlags::POOL_ACTIVATED);
}

int skill_pool_deactivate(dumb_ptr<map_session_data> sd, SkillID skill_id)
{
    if (bool(sd->status.skill[skill_id].flags & SkillFlags::POOL_ACTIVATED))
    {
        sd->status.skill[skill_id].flags &= ~SkillFlags::POOL_ACTIVATED;
        MAP_LOG_PC(sd, "SKILL-DEACTIVATE %d"_fmt, skill_id);
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

int skill_power(dumb_ptr<map_session_data> sd, SkillID skill_id)
{
    SP stat = skill_stat(skill_id);
    int stat_value, skill_value;
    int result;

    if (stat == SP::ZERO || !skill_pool_is_activated(sd, skill_id))
        return 0;

    stat_value = battle_get_stat(stat, sd);
    skill_value = sd->status.skill[skill_id].lv;

    if ((skill_value * 10) - 1 > stat_value)
        skill_value += (stat_value / 10);
    else
        skill_value *= 2;

    result = (skill_value * stat_value) / 10;

    return result;
}

int skill_power_bl(dumb_ptr<block_list> bl, SkillID skill)
{
    if (bl->bl_type == BL::PC)
        return skill_power(bl->is_player(), skill);
    else
        return 0;
}
} // namespace tmwa
