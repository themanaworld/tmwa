#ifndef SKILL_T_HPP
#define SKILL_T_HPP

#include <cstdint>

#include "../common/utils.hpp"

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
    SC_PROVOKE          = 0,
#define SC_PROVOKE StatusChange::SC_PROVOKE
    SC_ENDURE           = 1,
#define SC_ENDURE StatusChange::SC_ENDURE
    SC_TWOHANDQUICKEN   = 2,
#define SC_TWOHANDQUICKEN StatusChange::SC_TWOHANDQUICKEN
    SC_CONCENTRATE      = 3,
#define SC_CONCENTRATE StatusChange::SC_CONCENTRATE
    SC_HIDING           = 4,
#define SC_HIDING StatusChange::SC_HIDING
    SC_CLOAKING         = 5,
#define SC_CLOAKING StatusChange::SC_CLOAKING
    SC_ENCPOISON        = 6,
#define SC_ENCPOISON StatusChange::SC_ENCPOISON
    SC_POISONREACT      = 7,
#define SC_POISONREACT StatusChange::SC_POISONREACT
    SC_QUAGMIRE         = 8,
#define SC_QUAGMIRE StatusChange::SC_QUAGMIRE
    SC_ANGELUS          = 9,
#define SC_ANGELUS StatusChange::SC_ANGELUS
    SC_BLESSING         = 10,
#define SC_BLESSING StatusChange::SC_BLESSING
    SC_SIGNUMCRUCIS     = 11,
#define SC_SIGNUMCRUCIS StatusChange::SC_SIGNUMCRUCIS
    SC_INCREASEAGI      = 12,
#define SC_INCREASEAGI StatusChange::SC_INCREASEAGI
    SC_DECREASEAGI      = 13,
#define SC_DECREASEAGI StatusChange::SC_DECREASEAGI
    SC_SLOWPOISON       = 14,   //
#define SC_SLOWPOISON StatusChange::SC_SLOWPOISON
    SC_IMPOSITIO        = 15,
#define SC_IMPOSITIO StatusChange::SC_IMPOSITIO
    SC_SUFFRAGIUM       = 16,
#define SC_SUFFRAGIUM StatusChange::SC_SUFFRAGIUM
    SC_ASPERSIO         = 17,
#define SC_ASPERSIO StatusChange::SC_ASPERSIO
    SC_BENEDICTIO       = 18,
#define SC_BENEDICTIO StatusChange::SC_BENEDICTIO
    SC_KYRIE            = 19,
#define SC_KYRIE StatusChange::SC_KYRIE
    SC_MAGNIFICAT       = 20,
#define SC_MAGNIFICAT StatusChange::SC_MAGNIFICAT
    SC_GLORIA           = 21,
#define SC_GLORIA StatusChange::SC_GLORIA
    SC_AETERNA          = 22,
#define SC_AETERNA StatusChange::SC_AETERNA
    SC_ADRENALINE       = 23,
#define SC_ADRENALINE StatusChange::SC_ADRENALINE
    SC_WEAPONPERFECTION = 24,
#define SC_WEAPONPERFECTION StatusChange::SC_WEAPONPERFECTION
    SC_OVERTHRUST       = 25,
#define SC_OVERTHRUST StatusChange::SC_OVERTHRUST
    SC_MAXIMIZEPOWER    = 26,
#define SC_MAXIMIZEPOWER StatusChange::SC_MAXIMIZEPOWER
    SC_RIDING           = 27,
#define SC_RIDING StatusChange::SC_RIDING
    SC_FALCON           = 28,
#define SC_FALCON StatusChange::SC_FALCON
    SC_TRICKDEAD        = 29,
#define SC_TRICKDEAD StatusChange::SC_TRICKDEAD
    SC_LOUD             = 30,
#define SC_LOUD StatusChange::SC_LOUD
    SC_ENERGYCOAT       = 31,
#define SC_ENERGYCOAT StatusChange::SC_ENERGYCOAT
    SC_BROKNARMOR       = 32,
#define SC_BROKNARMOR StatusChange::SC_BROKNARMOR
    SC_BROKNWEAPON      = 33,
#define SC_BROKNWEAPON StatusChange::SC_BROKNWEAPON
    SC_HALLUCINATION    = 34,
#define SC_HALLUCINATION StatusChange::SC_HALLUCINATION
    SC_WEIGHT50         = 35,
#define SC_WEIGHT50 StatusChange::SC_WEIGHT50
    SC_WEIGHT90         = 36,
#define SC_WEIGHT90 StatusChange::SC_WEIGHT90
    SC_SPEEDPOTION0     = 37,   //
#define SC_SPEEDPOTION0 StatusChange::SC_SPEEDPOTION0
    SC_SPEEDPOTION1     = 38,
#define SC_SPEEDPOTION1 StatusChange::SC_SPEEDPOTION1
    SC_SPEEDPOTION2     = 39,
#define SC_SPEEDPOTION2 StatusChange::SC_SPEEDPOTION2

    SC_STRIPWEAPON      = 50,
#define SC_STRIPWEAPON StatusChange::SC_STRIPWEAPON
    SC_STRIPSHIELD      = 51,
#define SC_STRIPSHIELD StatusChange::SC_STRIPSHIELD
    SC_STRIPARMOR       = 52,
#define SC_STRIPARMOR StatusChange::SC_STRIPARMOR
    SC_STRIPHELM        = 53,
#define SC_STRIPHELM StatusChange::SC_STRIPHELM
    SC_CP_WEAPON        = 54,
#define SC_CP_WEAPON StatusChange::SC_CP_WEAPON
    SC_CP_SHIELD        = 55,
#define SC_CP_SHIELD StatusChange::SC_CP_SHIELD
    SC_CP_ARMOR         = 56,
#define SC_CP_ARMOR StatusChange::SC_CP_ARMOR
    SC_CP_HELM          = 57,
#define SC_CP_HELM StatusChange::SC_CP_HELM
    SC_AUTOGUARD        = 58,
#define SC_AUTOGUARD StatusChange::SC_AUTOGUARD
    SC_REFLECTSHIELD    = 59,
#define SC_REFLECTSHIELD StatusChange::SC_REFLECTSHIELD
    SC_DEVOTION         = 60,
#define SC_DEVOTION StatusChange::SC_DEVOTION
    SC_PROVIDENCE       = 61,
#define SC_PROVIDENCE StatusChange::SC_PROVIDENCE
    SC_DEFENDER         = 62,
#define SC_DEFENDER StatusChange::SC_DEFENDER

    SC_SPEARSQUICKEN    = 68,
#define SC_SPEARSQUICKEN StatusChange::SC_SPEARSQUICKEN

    SC_HEALING          = 70,   //
#define SC_HEALING StatusChange::SC_HEALING

    SC_SIGHTTRASHER     = 73,
#define SC_SIGHTTRASHER StatusChange::SC_SIGHTTRASHER

    SC_EXPLOSIONSPIRITS = 86,
#define SC_EXPLOSIONSPIRITS StatusChange::SC_EXPLOSIONSPIRITS
    SC_STEELBODY        = 87,
#define SC_STEELBODY StatusChange::SC_STEELBODY

    SC_FLAMELAUNCHER    = 90,
#define SC_FLAMELAUNCHER StatusChange::SC_FLAMELAUNCHER
    SC_FROSTWEAPON      = 91,
#define SC_FROSTWEAPON StatusChange::SC_FROSTWEAPON
    SC_LIGHTNINGLOADER  = 92,
#define SC_LIGHTNINGLOADER StatusChange::SC_LIGHTNINGLOADER
    SC_SEISMICWEAPON    = 93,
#define SC_SEISMICWEAPON StatusChange::SC_SEISMICWEAPON

    SC_AURABLADE        = 103,
#define SC_AURABLADE StatusChange::SC_AURABLADE
    SC_PARRYING         = 104,
#define SC_PARRYING StatusChange::SC_PARRYING
    SC_CONCENTRATION    = 105,
#define SC_CONCENTRATION StatusChange::SC_CONCENTRATION
    SC_TENSIONRELAX     = 106,
#define SC_TENSIONRELAX StatusChange::SC_TENSIONRELAX
    SC_BERSERK          = 107,
#define SC_BERSERK StatusChange::SC_BERSERK

    SC_ASSUMPTIO        = 110,
#define SC_ASSUMPTIO StatusChange::SC_ASSUMPTIO

    SC_MAGICPOWER       = 113,
#define SC_MAGICPOWER StatusChange::SC_MAGICPOWER

    SC_TRUESIGHT        = 115,
#define SC_TRUESIGHT StatusChange::SC_TRUESIGHT
    SC_WINDWALK         = 116,
#define SC_WINDWALK StatusChange::SC_WINDWALK
    SC_MELTDOWN         = 117,
#define SC_MELTDOWN StatusChange::SC_MELTDOWN
    SC_CARTBOOST        = 118,
#define SC_CARTBOOST StatusChange::SC_CARTBOOST

    SC_REJECTSWORD      = 120,
#define SC_REJECTSWORD StatusChange::SC_REJECTSWORD
    SC_MARIONETTE       = 121,
#define SC_MARIONETTE StatusChange::SC_MARIONETTE

    SC_HEADCRUSH        = 124,
#define SC_HEADCRUSH StatusChange::SC_HEADCRUSH
    SC_JOINTBEAT        = 125,
#define SC_JOINTBEAT StatusChange::SC_JOINTBEAT
    SC_BASILICA         = SC_JOINTBEAT,
#define SC_BASILICA StatusChange::SC_BASILICA

    SC_STONE            = 128,
#define SC_STONE StatusChange::SC_STONE
    SC_FREEZE           = 129,
#define SC_FREEZE StatusChange::SC_FREEZE
    SC_STAN             = 130,
#define SC_STAN StatusChange::SC_STAN
    SC_SLEEP            = 131,
#define SC_SLEEP StatusChange::SC_SLEEP
    SC_POISON           = 132,  //
#define SC_POISON StatusChange::SC_POISON
    SC_CURSE            = 133,
#define SC_CURSE StatusChange::SC_CURSE
    SC_SILENCE          = 134,
#define SC_SILENCE StatusChange::SC_SILENCE
    SC_DIVINA           = SC_SILENCE,
#define SC_DIVINA StatusChange::SC_DIVINA
    SC_CONFUSION        = 135,
#define SC_CONFUSION StatusChange::SC_CONFUSION
    SC_BLIND            = 136,
#define SC_BLIND StatusChange::SC_BLIND

    SC_SAFETYWALL       = 140,
#define SC_SAFETYWALL StatusChange::SC_SAFETYWALL
    SC_PNEUMA           = 141,
#define SC_PNEUMA StatusChange::SC_PNEUMA

    SC_ANKLE            = 143,
#define SC_ANKLE StatusChange::SC_ANKLE
    SC_DANCING          = 144,
#define SC_DANCING StatusChange::SC_DANCING
    SC_KEEPING          = 145,
#define SC_KEEPING StatusChange::SC_KEEPING
    SC_BARRIER          = 146,
#define SC_BARRIER StatusChange::SC_BARRIER

    SC_MAGICROD         = 149,
#define SC_MAGICROD StatusChange::SC_MAGICROD
    SC_SIGHT            = 150,
#define SC_SIGHT StatusChange::SC_SIGHT

    SC_VOLCANO          = 153,
#define SC_VOLCANO StatusChange::SC_VOLCANO
    SC_DELUGE           = 154,
#define SC_DELUGE StatusChange::SC_DELUGE
    SC_VIOLENTGALE      = 155,
#define SC_VIOLENTGALE StatusChange::SC_VIOLENTGALE

    SC_EXTREMITYFIST    = 158,
#define SC_EXTREMITYFIST StatusChange::SC_EXTREMITYFIST

    SC_ENSEMBLE         = 159,
#define SC_ENSEMBLE StatusChange::SC_ENSEMBLE

    SC_LULLABY          = 160,
#define SC_LULLABY StatusChange::SC_LULLABY
    SC_RICHMANKIM       = 161,
#define SC_RICHMANKIM StatusChange::SC_RICHMANKIM
    SC_ETERNALCHAOS     = 162,
#define SC_ETERNALCHAOS StatusChange::SC_ETERNALCHAOS
    SC_DRUMBATTLE       = 163,
#define SC_DRUMBATTLE StatusChange::SC_DRUMBATTLE
    SC_NIBELUNGEN       = 164,
#define SC_NIBELUNGEN StatusChange::SC_NIBELUNGEN
    SC_ROKISWEIL        = 165,
#define SC_ROKISWEIL StatusChange::SC_ROKISWEIL
    SC_INTOABYSS        = 166,
#define SC_INTOABYSS StatusChange::SC_INTOABYSS
    SC_SIEGFRIED        = 167,
#define SC_SIEGFRIED StatusChange::SC_SIEGFRIED
    SC_DISSONANCE       = 168,
#define SC_DISSONANCE StatusChange::SC_DISSONANCE
    SC_WHISTLE          = 169,
#define SC_WHISTLE StatusChange::SC_WHISTLE
    SC_ASSNCROS         = 170,
#define SC_ASSNCROS StatusChange::SC_ASSNCROS
    SC_POEMBRAGI        = 171,
#define SC_POEMBRAGI StatusChange::SC_POEMBRAGI
    SC_APPLEIDUN        = 172,
#define SC_APPLEIDUN StatusChange::SC_APPLEIDUN
    SC_UGLYDANCE        = 173,
#define SC_UGLYDANCE StatusChange::SC_UGLYDANCE
    SC_HUMMING          = 174,
#define SC_HUMMING StatusChange::SC_HUMMING
    SC_DONTFORGETME     = 175,
#define SC_DONTFORGETME StatusChange::SC_DONTFORGETME
    SC_FORTUNE          = 176,
#define SC_FORTUNE StatusChange::SC_FORTUNE
    SC_SERVICE4U        = 177,
#define SC_SERVICE4U StatusChange::SC_SERVICE4U
    SC_FOGWALL          = 178,
#define SC_FOGWALL StatusChange::SC_FOGWALL
    SC_GOSPEL           = 179,
#define SC_GOSPEL StatusChange::SC_GOSPEL
    SC_SPIDERWEB        = 180,
#define SC_SPIDERWEB StatusChange::SC_SPIDERWEB
    SC_MEMORIZE         = 181,
#define SC_MEMORIZE StatusChange::SC_MEMORIZE
    SC_LANDPROTECTOR    = 182,
#define SC_LANDPROTECTOR StatusChange::SC_LANDPROTECTOR
    SC_ADAPTATION       = 183,
#define SC_ADAPTATION StatusChange::SC_ADAPTATION
    SC_CHASEWALK        = 184,
#define SC_CHASEWALK StatusChange::SC_CHASEWALK
    SC_ATKPOT           = 185,  //
#define SC_ATKPOT StatusChange::SC_ATKPOT
    SC_MATKPOT          = 186,
#define SC_MATKPOT StatusChange::SC_MATKPOT
    SC_WEDDING          = 187,
#define SC_WEDDING StatusChange::SC_WEDDING
    SC_NOCHAT           = 188,
#define SC_NOCHAT StatusChange::SC_NOCHAT
    SC_SPLASHER         = 189,  // ?
#define SC_SPLASHER StatusChange::SC_SPLASHER
    SC_SELFDESTRUCTION  = 190,
#define SC_SELFDESTRUCTION StatusChange::SC_SELFDESTRUCTION
    SC_MINDBREAKER      = 191,
#define SC_MINDBREAKER StatusChange::SC_MINDBREAKER
    SC_SPELLBREAKER     = 192,
#define SC_SPELLBREAKER StatusChange::SC_SPELLBREAKER

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

    AL_TELEPORT         = 28,   //
#define AL_TELEPORT SkillID::AL_TELEPORT
    AL_HEAL             = 30,   //
#define AL_HEAL SkillID::AL_HEAL

    AC_OWL              = 45,   // Mallard's Eye
#define AC_OWL SkillID::AC_OWL

    NV_FIRSTAID         = 144,  //
#define NV_FIRSTAID SkillID::NV_FIRSTAID
    NV_TRICKDEAD        = 145,
#define NV_TRICKDEAD SkillID::NV_TRICKDEAD

    NPC_PIERCINGATT     = 160,
#define NPC_PIERCINGATT SkillID::NPC_PIERCINGATT
    NPC_MENTALBREAKER   = 161,
#define NPC_MENTALBREAKER SkillID::NPC_MENTALBREAKER
    NPC_RANGEATTACK     = 162,
#define NPC_RANGEATTACK SkillID::NPC_RANGEATTACK
    NPC_ATTRICHANGE     = 163,
#define NPC_ATTRICHANGE SkillID::NPC_ATTRICHANGE
    NPC_CHANGEWATER     = 164,
#define NPC_CHANGEWATER SkillID::NPC_CHANGEWATER
    NPC_CHANGEGROUND    = 165,
#define NPC_CHANGEGROUND SkillID::NPC_CHANGEGROUND
    NPC_CHANGEFIRE      = 166,
#define NPC_CHANGEFIRE SkillID::NPC_CHANGEFIRE
    NPC_CHANGEWIND      = 167,
#define NPC_CHANGEWIND SkillID::NPC_CHANGEWIND
    NPC_CHANGEPOISON    = 168,
#define NPC_CHANGEPOISON SkillID::NPC_CHANGEPOISON
    NPC_CHANGEHOLY      = 169,
#define NPC_CHANGEHOLY SkillID::NPC_CHANGEHOLY
    NPC_CHANGEDARKNESS  = 170,
#define NPC_CHANGEDARKNESS SkillID::NPC_CHANGEDARKNESS
    NPC_CHANGETELEKINESIS = 171,
#define NPC_CHANGETELEKINESIS SkillID::NPC_CHANGETELEKINESIS
    NPC_CRITICALSLASH   = 172,
#define NPC_CRITICALSLASH SkillID::NPC_CRITICALSLASH
    NPC_COMBOATTACK     = 173,
#define NPC_COMBOATTACK SkillID::NPC_COMBOATTACK
    NPC_GUIDEDATTACK    = 174,
#define NPC_GUIDEDATTACK SkillID::NPC_GUIDEDATTACK
    NPC_SELFDESTRUCTION = 175,  //
#define NPC_SELFDESTRUCTION SkillID::NPC_SELFDESTRUCTION
    NPC_SPLASHATTACK    = 176,
#define NPC_SPLASHATTACK SkillID::NPC_SPLASHATTACK
    NPC_SUICIDE         = 177,
#define NPC_SUICIDE SkillID::NPC_SUICIDE
    NPC_POISON          = 178,  //
#define NPC_POISON SkillID::NPC_POISON
    NPC_BLINDATTACK     = 179,
#define NPC_BLINDATTACK SkillID::NPC_BLINDATTACK
    NPC_SILENCEATTACK   = 180,
#define NPC_SILENCEATTACK SkillID::NPC_SILENCEATTACK
    NPC_STUNATTACK      = 181,
#define NPC_STUNATTACK SkillID::NPC_STUNATTACK
    NPC_PETRIFYATTACK   = 182,
#define NPC_PETRIFYATTACK SkillID::NPC_PETRIFYATTACK
    NPC_CURSEATTACK     = 183,
#define NPC_CURSEATTACK SkillID::NPC_CURSEATTACK
    NPC_SLEEPATTACK     = 184,
#define NPC_SLEEPATTACK SkillID::NPC_SLEEPATTACK
    NPC_RANDOMATTACK    = 185,
#define NPC_RANDOMATTACK SkillID::NPC_RANDOMATTACK
    NPC_WATERATTACK     = 186,
#define NPC_WATERATTACK SkillID::NPC_WATERATTACK
    NPC_GROUNDATTACK    = 187,
#define NPC_GROUNDATTACK SkillID::NPC_GROUNDATTACK
    NPC_FIREATTACK      = 188,
#define NPC_FIREATTACK SkillID::NPC_FIREATTACK
    NPC_WINDATTACK      = 189,  // ?
#define NPC_WINDATTACK SkillID::NPC_WINDATTACK

    NPC_POISONATTACK    = 190,  //
#define NPC_POISONATTACK SkillID::NPC_POISONATTACK
    NPC_HOLYATTACK      = 191,
#define NPC_HOLYATTACK SkillID::NPC_HOLYATTACK
    NPC_DARKNESSATTACK  = 192,
#define NPC_DARKNESSATTACK SkillID::NPC_DARKNESSATTACK
    NPC_TELEKINESISATTACK = 193,
#define NPC_TELEKINESISATTACK SkillID::NPC_TELEKINESISATTACK
    NPC_MAGICALATTACK   = 194,
#define NPC_MAGICALATTACK SkillID::NPC_MAGICALATTACK
    NPC_METAMORPHOSIS   = 195,
#define NPC_METAMORPHOSIS SkillID::NPC_METAMORPHOSIS
    NPC_PROVOCATION     = 196,
#define NPC_PROVOCATION SkillID::NPC_PROVOCATION
    NPC_SMOKING         = 197,
#define NPC_SMOKING SkillID::NPC_SMOKING
    NPC_SUMMONSLAVE     = 198,  //
#define NPC_SUMMONSLAVE SkillID::NPC_SUMMONSLAVE
    NPC_EMOTION         = 199,  //
#define NPC_EMOTION SkillID::NPC_EMOTION
    NPC_TRANSFORMATION  = 200,
#define NPC_TRANSFORMATION SkillID::NPC_TRANSFORMATION
    NPC_BLOODDRAIN      = 201,
#define NPC_BLOODDRAIN SkillID::NPC_BLOODDRAIN
    NPC_ENERGYDRAIN     = 202,
#define NPC_ENERGYDRAIN SkillID::NPC_ENERGYDRAIN
    NPC_KEEPING         = 203,
#define NPC_KEEPING SkillID::NPC_KEEPING
    NPC_DARKBREATH      = 204,
#define NPC_DARKBREATH SkillID::NPC_DARKBREATH
    NPC_DARKBLESSING    = 205,
#define NPC_DARKBLESSING SkillID::NPC_DARKBLESSING
    NPC_BARRIER         = 206,
#define NPC_BARRIER SkillID::NPC_BARRIER
    NPC_DEFENDER        = 207,
#define NPC_DEFENDER SkillID::NPC_DEFENDER
    NPC_LICK            = 208,
#define NPC_LICK SkillID::NPC_LICK
    NPC_HALLUCINATION   = 209,
#define NPC_HALLUCINATION SkillID::NPC_HALLUCINATION
    NPC_REBIRTH         = 210,
#define NPC_REBIRTH SkillID::NPC_REBIRTH
    NPC_SUMMONMONSTER   = 211,
#define NPC_SUMMONMONSTER SkillID::NPC_SUMMONMONSTER

    NPC_SELFDESTRUCTION2 = 333,
#define NPC_SELFDESTRUCTION2 SkillID::NPC_SELFDESTRUCTION2

    NPC_DARKCROSS       = 338,
#define NPC_DARKCROSS SkillID::NPC_DARKCROSS

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
