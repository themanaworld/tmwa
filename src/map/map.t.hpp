#ifndef TMWA_MAP_MAP_T_HPP
#define TMWA_MAP_MAP_T_HPP

# include "../strings/vstring.hpp"

# include "../mmo/mmo.hpp"

namespace e
{
// [Fate] status.option properties.  These are persistent status changes.
// IDs that are not listed are not used in the code (to the best of my knowledge)
enum class Option : uint16_t
{
    ZERO            = 0x0000,

    // [Fate] This is the GM `@hide' flag
    HIDE            = 0x0040,
    // [Fate] Complete invisibility to other clients
    INVISIBILITY    = 0x1000,

    // ?
    REAL_ANY_HIDE   = HIDE,
};
enum class Opt1 : uint16_t
{
    ZERO            = 0,
    _stone1         = 1,
    _freeze         = 2,
    _stan           = 3,
    _sleep          = 4,
    _stone6         = 6,
};
enum class Opt2 : uint16_t
{
    ZERO            = 0x0000,
    _poison         = 0x0001,
    _curse          = 0x0002,
    _silence        = 0x0004,
    BLIND           = 0x0010,
    _speedpotion0   = 0x0020,
    _signumcrucis   = 0x0040,
    _atkpot         = 0x0080,
    _heal           = 0x0100,
    _slowpoison     = 0x0200,
};
enum class Opt3 : uint16_t
{
    ZERO            = 0x0000,
    _concentration  = 0x0001,
    _overthrust     = 0x0002,
    _energycoat     = 0x0004,
    _explosionspirits = 0x0008,
    _steelbody      = 0x0010,
    _berserk        = 0x0080,

    _marionette     = 0x0400,
    _assumptio      = 0x0800,
};

ENUM_BITWISE_OPERATORS(Option)
ENUM_BITWISE_OPERATORS(Opt2)
ENUM_BITWISE_OPERATORS(Opt3)
}
using e::Option;
using e::Opt1;
using e::Opt2;
using e::Opt3;

enum class BL : uint8_t
{
    NUL,
    PC,
    NPC,
    MOB,
    ITEM,
    SPELL,
};
enum class NpcSubtype : uint8_t
{
    WARP,
    SHOP,
    SCRIPT,
    MESSAGE,
};

enum class mob_stat
{
    LV,
    MAX_HP,
    STR,
    AGI,
    VIT,
    INT,
    DEX,
    LUK,
    // low and high attacks
    ATK1,
    ATK2,
    // attack delay
    ADELAY,
    DEF,
    MDEF,
    SPEED,
    // These must come last:
    // [Fate] Encoded as base to 1024: 1024 means 100%
    XP_BONUS,
    LAST,
};

enum class MS : uint8_t
{
    IDLE,
    WALK,
    ATTACK,
    DEAD,
};

enum class ATK
{
    ZERO = 0,

    LUCKY,
    FLEE,
    DEF,
};

enum class SP : uint16_t
{
    // sent to client
    SPEED                       = 0,

    // when used as "no stat"
    ZERO = 0,

    // sent to client
    BASEEXP                     = 1,
    // sent to client
    JOBEXP                      = 2,
# if 0
    KARMA                       = 3,
# endif

    // sent to client
    HP                          = 5,
    // sent to client
    MAXHP                       = 6,
    // sent to client
    SP                          = 7,
    // sent to client
    MAXSP                       = 8,
    // sent to client
    STATUSPOINT                 = 9,

    // sent to client
    BASELEVEL                   = 11,
    // sent to client
    SKILLPOINT                  = 12,
    // sent to client
    STR                         = 13,
    // sent to client
    AGI                         = 14,
    // sent to client
    VIT                         = 15,
    // sent to client
    INT                         = 16,
    // sent to client
    DEX                         = 17,
    // sent to client
    LUK                         = 18,
    CLASS                       = 19,
    // sent to client
    ZENY                        = 20,
    SEX                         = 21,
    // sent to client
    NEXTBASEEXP                 = 22,
    // sent to client
    NEXTJOBEXP                  = 23,
    // sent to client
    WEIGHT                      = 24,
    // sent to client
    MAXWEIGHT                   = 25,

    // sent to client
    USTR                        = 32,
    // sent to client
    UAGI                        = 33,
    // sent to client
    UVIT                        = 34,
    // sent to client
    UINT                        = 35,
    // sent to client
    UDEX                        = 36,
    // sent to client
    ULUK                        = 37,

    // sent to client
    ATK1                        = 41,
    // sent to client
    ATK2                        = 42,
    // sent to client
    MATK1                       = 43,
    // sent to client
    MATK2                       = 44,
    // sent to client
    DEF1                        = 45,
    // sent to client
    DEF2                        = 46,
    // sent to client
    MDEF1                       = 47,
    // sent to client
    MDEF2                       = 48,
    // sent to client
    HIT                         = 49,
    // sent to client
    FLEE1                       = 50,
    // sent to client
    FLEE2                       = 51,
    // sent to client
    CRITICAL                    = 52,
    // sent to client
    ASPD                        = 53,

    // sent to client
    JOBLEVEL                    = 55,

# if 0
    PARTNER                     = 57,
    CART                        = 58,
    FAME                        = 59,
    UNBREAKABLE                 = 60,
# endif

    DEAF                        = 70,

    // sent to client
    GM                          = 500,

    // sent to client
    ATTACKRANGE                 = 1000,
# if 0
    ATKELE                      = 1001,
# endif
# if 0
    DEFELE                      = 1002,
# endif
# if 0
    CASTRATE                    = 1003,
# endif
    MAXHPRATE                   = 1004,
# if 0
    MAXSPRATE                   = 1005,
# endif
# if 0
    SPRATE                      = 1006,
# endif

# if 0
    ADDEFF                      = 1012,
# endif
# if 0
    RESEFF                      = 1013,
# endif
    BASE_ATK                    = 1014,
    ASPD_RATE                   = 1015,
    HP_RECOV_RATE               = 1016,
# if 0
    SP_RECOV_RATE               = 1017,
# endif
# if 0
    SPEED_RATE                  = 1018,
# endif
    CRITICAL_DEF                = 1019,
# if 0
    NEAR_ATK_DEF                = 1020,
# endif
# if 0
    LONG_ATK_DEF                = 1021,
# endif
# if 0
    DOUBLE_RATE                 = 1022,
# endif
    DOUBLE_ADD_RATE             = 1023,
# if 0
    MATK                        = 1024,
# endif
# if 0
    MATK_RATE                   = 1025,
# endif
# if 0
    IGNORE_DEF_ELE              = 1026,
# endif
# if 0
    IGNORE_DEF_RACE             = 1027,
# endif
# if 0
    ATK_RATE                    = 1028,
# endif
    SPEED_ADDRATE               = 1029,
# if 0
    ASPD_ADDRATE                = 1030,
# endif
# if 0
    MAGIC_ATK_DEF               = 1031,
# endif
# if 0
    MISC_ATK_DEF                = 1032,
# endif
# if 0
    IGNORE_MDEF_ELE             = 1033,
# endif
# if 0
    IGNORE_MDEF_RACE            = 1034,
# endif

# if 0
    PERFECT_HIT_RATE            = 1038,
# endif
# if 0
    PERFECT_HIT_ADD_RATE        = 1039,
# endif
# if 0
    CRITICAL_RATE               = 1040,
# endif
# if 0
    GET_ZENY_NUM                = 1041,
# endif
# if 0
    ADD_GET_ZENY_NUM            = 1042,
# endif

# if 0
    ADD_MONSTER_DROP_ITEM       = 1047,
# endif
# if 0
    DEF_RATIO_ATK_ELE           = 1048,
# endif
# if 0
    DEF_RATIO_ATK_RACE          = 1049,
# endif
# if 0
    ADD_SPEED                   = 1050,
# endif
# if 0
    HIT_RATE                    = 1051,
# endif
# if 0
    FLEE_RATE                   = 1052,
# endif
# if 0
    FLEE2_RATE                  = 1053,
# endif
    DEF_RATE                    = 1054,
    DEF2_RATE                   = 1055,
# if 0
    MDEF_RATE                   = 1056,
# endif
# if 0
    MDEF2_RATE                  = 1057,
# endif
# if 0
    SPLASH_RANGE                = 1058,
# endif
# if 0
    SPLASH_ADD_RANGE            = 1059,
# endif

    HP_DRAIN_RATE               = 1061,
# if 0
    SP_DRAIN_RATE               = 1062,
# endif
# if 0
    SHORT_WEAPON_DAMAGE_RETURN  = 1063,
# endif
# if 0
    LONG_WEAPON_DAMAGE_RETURN   = 1064,
# endif

# if 0
    ADDEFF2                     = 1067,
# endif
    BREAK_WEAPON_RATE           = 1068,
    BREAK_ARMOR_RATE            = 1069,
    ADD_STEAL_RATE              = 1070,
    MAGIC_DAMAGE_RETURN         = 1071,
# if 0
    RANDOM_ATTACK_INCREASE      = 1072,
# endif
};

constexpr
SP attr_to_sp(ATTR attr)
{
    return SP(uint16_t(attr) + uint16_t(SP::STR));
}

constexpr
ATTR sp_to_attr(SP sp)
{
    return ATTR(uint16_t(sp) - uint16_t(SP::STR));
}

constexpr
SP attr_to_usp(ATTR attr)
{
    return SP(uint16_t(attr) + uint16_t(SP::USTR));
}

constexpr
ATTR usp_to_attr(SP sp)
{
    return ATTR(uint16_t(sp) - uint16_t(SP::USTR));
}

constexpr
SP sp_to_usp(SP sp)
{
    return attr_to_usp(sp_to_attr(sp));
}

constexpr
SP usp_to_sp(SP sp)
{
    return attr_to_sp(usp_to_attr(sp));
}


enum class LOOK : uint8_t
{
    BASE            = 0,
    HAIR            = 1,
    WEAPON          = 2,
    HEAD_BOTTOM     = 3,
    HEAD_TOP        = 4,
    HEAD_MID        = 5,
    HAIR_COLOR      = 6,
    CLOTHES_COLOR   = 7,
    SHIELD          = 8,
    SHOES           = 9,
    GLOVES          = 10,
    CAPE            = 11,
    MISC1           = 12,
    MISC2           = 13,

    COUNT,
};

enum class EQUIP
{
    NONE    = -1,
    MISC2   = 0,
    CAPE    = 1,
    SHOES   = 2,
    GLOVES  = 3,
    LEGS    = 4, // also called "head bottom"
    TORSO   = 5, // also called "head middle"
    HAT     = 6, // also called "head top"
    MISC1   = 7,
    SHIELD  = 8,
    WEAPON  = 9,
    ARROW   = 10,
    COUNT   = 11,
};

constexpr
EQUIP EQUIPs[] =
{
    EQUIP::MISC2,
    EQUIP::CAPE,
    EQUIP::SHOES,
    EQUIP::GLOVES,
    EQUIP::LEGS,
    EQUIP::TORSO,
    EQUIP::HAT,
    EQUIP::MISC1,
    EQUIP::SHIELD,
    EQUIP::WEAPON,
    EQUIP::ARROW,
};

constexpr
EQUIP EQUIPs_noarrow[] =
{
    EQUIP::MISC2,
    EQUIP::CAPE,
    EQUIP::SHOES,
    EQUIP::GLOVES,
    EQUIP::LEGS,
    EQUIP::TORSO,
    EQUIP::HAT,
    EQUIP::MISC1,
    EQUIP::SHIELD,
    EQUIP::WEAPON,
};

enum class ItemType : uint8_t
{
    USE     = 0,    // in eA, healing only
    _1      = 1,    // unused
    _2      = 2,    // in eA, other usable items
    JUNK    = 3,    // "useless" items (e.g. quests)
    WEAPON  = 4,    // all weapons
    ARMOR   = 5,    // all other equipment
    _6      = 6,    // in eA, card
    _7      = 7,    // in eA, pet egg
    _8      = 8,    // in eA, pet equipment
    _9      = 9,    // unused
    ARROW   = 10,   // ammo
    _11     = 11,   // in eA, delayed use (special script)
};

namespace e
{
enum class MobMode : uint16_t
{
    ZERO                        = 0x0000,

    CAN_MOVE                    = 0x0001,
    LOOTER                      = 0x0002,
    AGGRESSIVE                  = 0x0004,
    ASSIST                      = 0x0008,

    CAST_SENSOR                 = 0x0010,
    BOSS                        = 0x0020,
    // sometimes also called "robust"
    PLANT                       = 0x0040,
    CAN_ATTACK                  = 0x0080,

    DETECTOR                    = 0x0100,
    CHANGE_TARGET               = 0x0200,

    war                         = CAN_MOVE | AGGRESSIVE | CAN_ATTACK,

    SUMMONED                    = 0x1000,
    TURNS_AGAINST_BAD_MASTER    = 0x2000,

    // mob mode flags that Fate actually understood
    SENSIBLE_MASK               = 0xf000,
};

ENUM_BITWISE_OPERATORS(MobMode)
}
using e::MobMode;

namespace e
{
enum class MapCell : uint8_t
{
    // the usual thing
    UNWALKABLE  = 0x01,
    // not in tmwa data
    _range      = 0x04,
    // set in code, not imported
    NPC_NEAR    = 0x80,
};
ENUM_BITWISE_OPERATORS(MapCell)
}
using e::MapCell;

struct MobName : VString<23> {};
struct NpcName : VString<23> {};
struct ScriptLabel : VString<23> {};
struct ItemName : VString<23> {};

#endif // TMWA_MAP_MAP_T_HPP
