#ifndef MOB_T_HPP
#define MOB_T_HPP

#include <cstdint>

enum class MobSkillTarget
{
    MST_TARGET = 0,
#define MST_TARGET MobSkillTarget::MST_TARGET
    MST_SELF,
#define MST_SELF MobSkillTarget::MST_SELF
};

/// Used as a condition when trying to apply the chosen mob skill.
enum class MobSkillCondition : uint16_t
{
    // used as something that never compares true
    NEVER_EQUAL = 0xfffe,
    ANY = 0xffff,

    MSC_ALWAYS = 0x0000,
#define MSC_ALWAYS MobSkillCondition::MSC_ALWAYS
    MSC_MYHPLTMAXRATE = 0x0001,
#define MSC_MYHPLTMAXRATE MobSkillCondition::MSC_MYHPLTMAXRATE

    MSC_NOTINTOWN = 0x0032,
#define MSC_NOTINTOWN MobSkillCondition::MSC_NOTINTOWN

    MSC_SLAVELT = 0x0110,
#define MSC_SLAVELT MobSkillCondition::MSC_SLAVELT
    MSC_SLAVELE = 0x0111,
#define MSC_SLAVELE MobSkillCondition::MSC_SLAVELE
};

/// Used as a filter when trying to choose a mob skill to use.
enum class MobSkillState : uint8_t
{
    ANY = 0xff,

    MSS_IDLE = 0,
#define MSS_IDLE MobSkillState::MSS_IDLE
    MSS_WALK,
#define MSS_WALK MobSkillState::MSS_WALK
    MSS_ATTACK,
#define MSS_ATTACK MobSkillState::MSS_ATTACK
    MSS_DEAD,
#define MSS_DEAD MobSkillState::MSS_DEAD
    MSS_LOOT,
#define MSS_LOOT MobSkillState::MSS_LOOT
    MSS_CHASE,
#define MSS_CHASE MobSkillState::MSS_CHASE
};

#endif // MOB_T_HPP
