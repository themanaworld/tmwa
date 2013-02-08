#ifndef SKILL_T_HPP
#define SKILL_T_HPP

#include <cstdint>

#include "../common/utils2.hpp"

// TODO remove most of these as their corresponding SkillIDs get deleted.
enum class StatusChange : uint16_t
{
    // indices into (map_session_data).status_change
    SC_SENDMAX          = 256,

    // sometimes means "none", sometimes not
    NEGATIVE1           = 0xffff,
    ANY_BAD             = NEGATIVE1,

    // these ones are used by clif_status_change,
    // e.g. by the magic system
    ZERO                = 0,
    ATTACK_ICON_GENERIC = 2000,
    ATTACK_ICON_SHEARING = 2001,
    CART                = 0x0c,
    CLIF_OPTION_SC_INVISIBILITY = 0x1000,
    CLIF_OPTION_SC_SCRIBE = 0x1001,

    // the rest are the normal effects
    SC_SLOWPOISON       = 14,   // item script

    SC_BROKNARMOR       = 32,   // ?
    SC_BROKNWEAPON      = 33,   // ?

    SC_WEIGHT50         = 35,   // ? sort of used
    SC_WEIGHT90         = 36,   // definitely used
    SC_SPEEDPOTION0     = 37,   // item script

    SC_HEALING          = 70,   // item script

    SC_STONE            = 128,  // ?bad
    SC_FREEZE           = 129,  // ?bad
    SC_STAN             = 130,  // ?bad
    SC_SLEEP            = 131,  // ?bad
    SC_POISON           = 132,  // bad; actually used
    SC_CURSE            = 133,  // ?bad
    SC_SILENCE          = 134,  // ?bad
    SC_CONFUSION        = 135,  // ?bad
    SC_BLIND            = 136,  // ?bad

    SC_ATKPOT           = 185,  // item script
    SC_MATKPOT          = 186,  // unused, but kept for parallel

    SC_NOCHAT           = 188,  // ? something with manner

    SC_SELFDESTRUCTION  = 190,  // see table - maybe used, maybe not

// Added for Fate's spells
    SC_HIDE             = 194,  // Hide from `detect' magic (PCs only)
    SC_SHEARED          = 194,  // Has been sheared (mobs only)
    SC_HALT_REGENERATE  = 195,  // Suspend regeneration
    SC_FLYING_BACKPACK  = 196,  // Flying backpack
    SC_MBARRIER         = 197,  // Magical barrier, magic resistance (val1 : power (%))
    SC_HASTE            = 198,  // `Haste' spell (val1 : power)
    SC_PHYS_SHIELD      = 199,  // `Protect' spell, reduce damage (val1: power)
    MAX_STATUSCHANGE    = 200,
};

constexpr
StatusChange MAJOR_STATUS_EFFECTS[] =
{
    StatusChange::SC_STONE,
    StatusChange::SC_FREEZE,
    StatusChange::SC_STAN,
    StatusChange::SC_SLEEP,
    StatusChange::SC_POISON,
    StatusChange::SC_CURSE,
    StatusChange::SC_SILENCE,
    StatusChange::SC_CONFUSION,
    StatusChange::SC_BLIND,
};

constexpr
StatusChange MAJOR_STATUS_EFFECTS_1[] =
{
    StatusChange::SC_STONE,
    StatusChange::SC_FREEZE,
    StatusChange::SC_STAN,
    StatusChange::SC_SLEEP,
};

// needed to work around some subtractative indexing
// I think it *might* be able to be totally removed.
enum class BadSC
{
    STONE       = 0,
    FREEZE      = 1,
    STAN        = 2,
    SLEEP       = 3,
    POISON      = 4,
    CURSE       = 5,
    SILENCE     = 6,
    CONFUSION   = 7,
    BLIND       = 8,

    COUNT       = 9, // formerly 10,
    // there is apocryphal evidence of a "bleeding" effect
};

constexpr
StatusChange BadSC_to_SC(BadSC bsc)
{
    return StatusChange(uint16_t(StatusChange::SC_STONE) + int(bsc));
}

constexpr
BadSC BadSC_from_SC(StatusChange sc)
{
    return BadSC(uint16_t(sc) - uint16_t(StatusChange::SC_STONE));
}

// TODO remove most of these
enum class SkillID : uint16_t
{
    // TODO: Remove these!
    NEGATIVE            = 0xffff,
    ZERO                = 0x0000,
    ONE                 = 0x0001,

    // Basic skills.
    // These should probably be made unconditional.
    NV_EMOTE            = 1,    //
    NV_TRADE            = 2,    //
    NV_PARTY            = 3,    //

    AC_OWL              = 45,   // Mallard's Eye

    NPC_SELFDESTRUCTION = 175,  //

    NPC_POISON          = 178,  //

    NPC_SUMMONSLAVE     = 198,  //
    NPC_EMOTION         = 199,  //

    TMW_SKILLPOOL       = 339,  // skill pool size

    // magic skills
    TMW_MAGIC           = 340,  //
    TMW_MAGIC_LIFE      = 341,  //
    TMW_MAGIC_WAR       = 342,  //
    TMW_MAGIC_TRANSMUTE = 343,  //
    TMW_MAGIC_NATURE    = 344,  //
    TMW_MAGIC_ETHER     = 345,  //
    TMW_MAGIC_DARK      = 346,  //
    TMW_MAGIC_LIGHT     = 347,  //

    // focusable skills
    TMW_BRAWLING        = 350,  //
    TMW_LUCKY_COUNTER   = 351,  //
    TMW_SPEED           = 352,  //
    TMW_RESIST_POISON   = 353,  //
    TMW_ASTRAL_SOUL     = 354,  //
    TMW_RAGING          = 355,  //

    // Note: this value is also hard-coded in common/mmo.hpp
    MAX_SKILL_DB        = 474, // not 450
};

namespace e
{
enum class SkillFlags : uint16_t
{
    ZERO            = 0x00,
    // is a pool skill
    POOL_FLAG       = 0x01,
    // is an active pool skill
    POOL_ACTIVE     = 0x02,
    // pool skill has been activated (used for clif)
    POOL_ACTIVATED  = 0x04,
};
ENUM_BITWISE_OPERATORS(SkillFlags)
}
using e::SkillFlags;

#endif // SKILL_T_HPP
