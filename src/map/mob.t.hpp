#ifndef TMWA_MAP_MOB_T_HPP
#define TMWA_MAP_MOB_T_HPP

# include <cstdint>

enum class MobSkillTarget
{
    MST_TARGET = 0,
    MST_SELF,
};

/// Used as a condition when trying to apply the chosen mob skill.
enum class MobSkillCondition : uint16_t
{
    // used as something that never compares true
    NEVER_EQUAL = 0xfffe,
    ANY = 0xffff,

    MSC_ALWAYS = 0x0000,
    MSC_MYHPLTMAXRATE = 0x0001,

    MSC_NOTINTOWN = 0x0032,

    MSC_SLAVELT = 0x0110,
    MSC_SLAVELE = 0x0111,
};

/// Used as a filter when trying to choose a mob skill to use.
enum class MobSkillState : uint8_t
{
    ANY = 0xff,

    MSS_IDLE = 0,
    MSS_WALK,
    MSS_ATTACK,
    MSS_DEAD,
    MSS_LOOT,
    MSS_CHASE,
};

#endif // TMWA_MAP_MOB_T_HPP
