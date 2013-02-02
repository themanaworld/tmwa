#ifndef SKILL_T_HPP
#define SKILL_T_HPP

#include <cstdint>

#include "../common/utils2.hpp"

// TODO remove most of these as their corresponding SkillIDs get deleted.
enum class StatusChange : uint16_t
{
    // indices into (map_session_data).status_change
    SC_SENDMAX          = 256,
#define SC_SENDMAX StatusChange::SC_SENDMAX

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
#define CLIF_OPTION_SC_INVISIBILITY StatusChange::CLIF_OPTION_SC_INVISIBILITY
    CLIF_OPTION_SC_SCRIBE = 0x1001,
#define CLIF_OPTION_SC_SCRIBE StatusChange::CLIF_OPTION_SC_SCRIBE

    // the rest are the normal effects
    SC_HIDING           = 4,    // ? (opt) ?bad
#define SC_HIDING StatusChange::SC_HIDING

    SC_SLOWPOISON       = 14,   // item script
#define SC_SLOWPOISON StatusChange::SC_SLOWPOISON

    SC_BROKNARMOR       = 32,   // ?
#define SC_BROKNARMOR StatusChange::SC_BROKNARMOR
    SC_BROKNWEAPON      = 33,   // ?
#define SC_BROKNWEAPON StatusChange::SC_BROKNWEAPON

    SC_WEIGHT50         = 35,   // ? sort of used
#define SC_WEIGHT50 StatusChange::SC_WEIGHT50
    SC_WEIGHT90         = 36,   // definitely used
#define SC_WEIGHT90 StatusChange::SC_WEIGHT90
    SC_SPEEDPOTION0     = 37,   // item script
#define SC_SPEEDPOTION0 StatusChange::SC_SPEEDPOTION0

    SC_HEALING          = 70,   // item script
#define SC_HEALING StatusChange::SC_HEALING

    SC_STONE            = 128,  // ?bad
#define SC_STONE StatusChange::SC_STONE
    SC_FREEZE           = 129,  // ?bad
#define SC_FREEZE StatusChange::SC_FREEZE
    SC_STAN             = 130,  // ?bad
#define SC_STAN StatusChange::SC_STAN
    SC_SLEEP            = 131,  // ?bad
#define SC_SLEEP StatusChange::SC_SLEEP
    SC_POISON           = 132,  // bad; actually used
#define SC_POISON StatusChange::SC_POISON
    SC_CURSE            = 133,  // ?bad
#define SC_CURSE StatusChange::SC_CURSE
    SC_SILENCE          = 134,  // ?bad
#define SC_SILENCE StatusChange::SC_SILENCE
    SC_CONFUSION        = 135,  // ?bad
#define SC_CONFUSION StatusChange::SC_CONFUSION
    SC_BLIND            = 136,  // ?bad
#define SC_BLIND StatusChange::SC_BLIND

    SC_SAFETYWALL       = 140,  // ?skill.cpp skill_unit thingies
#define SC_SAFETYWALL StatusChange::SC_SAFETYWALL
    SC_PNEUMA           = 141,  // ?skill.cpp skill_unit thingies
#define SC_PNEUMA StatusChange::SC_PNEUMA

    SC_ANKLE            = 143,  // ?skill.cpp skill_unit thingies
#define SC_ANKLE StatusChange::SC_ANKLE

    SC_SIGHT            = 150,  // ?unbad
#define SC_SIGHT StatusChange::SC_SIGHT

    SC_ATKPOT           = 185,  // item script
#define SC_ATKPOT StatusChange::SC_ATKPOT
    SC_MATKPOT          = 186,  // unused, but kept for parallel
#define SC_MATKPOT StatusChange::SC_MATKPOT

    SC_NOCHAT           = 188,  // ? something with manner
#define SC_NOCHAT StatusChange::SC_NOCHAT

    SC_SELFDESTRUCTION  = 190,  // see table - maybe used, maybe not
#define SC_SELFDESTRUCTION StatusChange::SC_SELFDESTRUCTION

// Added for Fate's spells
    SC_HIDE             = 194,  // Hide from `detect' magic (PCs only)
#define SC_HIDE StatusChange::SC_HIDE
    SC_SHEARED          = 194,  // Has been sheared (mobs only)
#define SC_SHEARED StatusChange::SC_SHEARED
    SC_HALT_REGENERATE  = 195,  // Suspend regeneration
#define SC_HALT_REGENERATE StatusChange::SC_HALT_REGENERATE
    SC_FLYING_BACKPACK  = 196,  // Flying backpack
#define SC_FLYING_BACKPACK StatusChange::SC_FLYING_BACKPACK
    SC_MBARRIER         = 197,  // Magical barrier, magic resistance (val1 : power (%))
#define SC_MBARRIER StatusChange::SC_MBARRIER
    SC_HASTE            = 198,  // `Haste' spell (val1 : power)
#define SC_HASTE StatusChange::SC_HASTE
    SC_PHYS_SHIELD      = 199,  // `Protect' spell, reduce damage (val1: power)
#define SC_PHYS_SHIELD StatusChange::SC_PHYS_SHIELD
    MAX_STATUSCHANGE    = 200,
#define MAX_STATUSCHANGE StatusChange::MAX_STATUSCHANGE
};

constexpr
StatusChange MAJOR_STATUS_EFFECTS[] =
{
    SC_STONE,
    SC_FREEZE,
    SC_STAN,
    SC_SLEEP,
    SC_POISON,
    SC_CURSE,
    SC_SILENCE,
    SC_CONFUSION,
    SC_BLIND,
};

constexpr
StatusChange MAJOR_STATUS_EFFECTS_1[] =
{
    SC_STONE,
    SC_FREEZE,
    SC_STAN,
    SC_SLEEP,
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
};

constexpr
StatusChange BadSC_to_SC(BadSC bsc)
{
    return StatusChange(uint16_t(SC_STONE) + int(bsc));
}

constexpr
BadSC BadSC_from_SC(StatusChange sc)
{
    return BadSC(uint16_t(sc) - uint16_t(SC_STONE));
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
#define NV_EMOTE SkillID::NV_EMOTE
    NV_TRADE            = 2,    //
#define NV_TRADE SkillID::NV_TRADE
    NV_PARTY            = 3,    //
#define NV_PARTY SkillID::NV_PARTY

    AC_OWL              = 45,   // Mallard's Eye
#define AC_OWL SkillID::AC_OWL

    NPC_SELFDESTRUCTION = 175,  //
#define NPC_SELFDESTRUCTION SkillID::NPC_SELFDESTRUCTION

    NPC_POISON          = 178,  //
#define NPC_POISON SkillID::NPC_POISON

    NPC_SUMMONSLAVE     = 198,  //
#define NPC_SUMMONSLAVE SkillID::NPC_SUMMONSLAVE
    NPC_EMOTION         = 199,  //
#define NPC_EMOTION SkillID::NPC_EMOTION

    TMW_SKILLPOOL       = 339,  // skill pool size
#define TMW_SKILLPOOL SkillID::TMW_SKILLPOOL

    // magic skills
    TMW_MAGIC           = 340,  //
#define TMW_MAGIC SkillID::TMW_MAGIC
    TMW_MAGIC_LIFE      = 341,  //
#define TMW_MAGIC_LIFE SkillID::TMW_MAGIC_LIFE
    TMW_MAGIC_WAR       = 342,  //
#define TMW_MAGIC_WAR SkillID::TMW_MAGIC_WAR
    TMW_MAGIC_TRANSMUTE = 343,  //
#define TMW_MAGIC_TRANSMUTE SkillID::TMW_MAGIC_TRANSMUTE
    TMW_MAGIC_NATURE    = 344,  //
#define TMW_MAGIC_NATURE SkillID::TMW_MAGIC_NATURE
    TMW_MAGIC_ETHER     = 345,  //
#define TMW_MAGIC_ETHER SkillID::TMW_MAGIC_ETHER
    TMW_MAGIC_DARK      = 346,  //
#define TMW_MAGIC_DARK SkillID::TMW_MAGIC_DARK
    TMW_MAGIC_LIGHT     = 347,  //
#define TMW_MAGIC_LIGHT SkillID::TMW_MAGIC_LIGHT

    // focusable skills
    TMW_BRAWLING        = 350,  //
#define TMW_BRAWLING SkillID::TMW_BRAWLING
    TMW_LUCKY_COUNTER   = 351,  //
#define TMW_LUCKY_COUNTER SkillID::TMW_LUCKY_COUNTER
    TMW_SPEED           = 352,  //
#define TMW_SPEED SkillID::TMW_SPEED
    TMW_RESIST_POISON   = 353,  //
#define TMW_RESIST_POISON SkillID::TMW_RESIST_POISON
    TMW_ASTRAL_SOUL     = 354,  //
#define TMW_ASTRAL_SOUL SkillID::TMW_ASTRAL_SOUL
    TMW_RAGING          = 355,  //
#define TMW_RAGING SkillID::TMW_RAGING

    // Note: this value is also hard-coded in common/mmo.hpp
    MAX_SKILL_DB        = 474, // not 450
#define MAX_SKILL_DB SkillID::MAX_SKILL_DB
};

namespace e
{
enum class SkillFlags : uint16_t
{
    ZERO        = 0x00,
    // is a pool skill
    FLAG        = 0x01,
#define SKILL_POOL_FLAG SkillFlags::FLAG
    // is an active pool skill
    ACTIVE      = 0x02,
#define SKILL_POOL_ACTIVE SkillFlags::ACTIVE
    // pool skill has been activated (used for clif)
    ACTIVATED   = 0x04,
#define SKILL_POOL_ACTIVATED SkillFlags::ACTIVATED
};
ENUM_BITWISE_OPERATORS(SkillFlags)
}
using e::SkillFlags;

#endif // SKILL_T_HPP
