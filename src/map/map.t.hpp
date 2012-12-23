#ifndef MAP_T_HPP
#define MAP_T_HPP

#include "../common/mmo.hpp"
#include "../common/utils.hpp"

namespace e
{
// [Fate] status.option properties.  These are persistent status changes.
// IDs that are not listed are not used in the code (to the best of my knowledge)
enum class Option : uint16_t
{
    ZERO            = 0x0000,

    SIGHT           = 0x0001,
    // apparently some weaker non-GM hide
    HIDE2           = 0x0002,
    CLOAK           = 0x0004,
    CART1           = 0x0008,
    FALCON          = 0x0010,
    RIDING          = 0x0020,
    // [Fate] This is the GM `@hide' flag
    HIDE            = 0x0040,
    CART2           = 0x0080,
    CART3           = 0x0100,
    CART4           = 0x0200,
    CART5           = 0x0400,
    ORC_HEAD        = 0x0800,
    // [Fate] Complete invisibility to other clients
    INVISIBILITY    = 0x1000,
    _wedding        = 0x1000,
    // [Fate] Auto-logging of nearby comments
    SCRIBE          = 0x2000,
    _ruwach         = 0x2000,
    CHASEWALK       = 0x4000,
    sign            = 0x8000,


    // ?
    REAL_ANY_HIDE   = HIDE | CLOAK | HIDE2,
    OLD_ANY_HIDE    = CHASEWALK | CLOAK | HIDE2,
    CART_MASK       = CART1 | CART2 | CART3 | CART4 | CART5,
    MASK            = sign | CHASEWALK | _wedding | CART_MASK | RIDING | FALCON,
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
    _bladestop      = 0x0020,
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
#define BL_NUL BL::NUL
    PC,
#define BL_PC BL::PC
    NPC,
#define BL_NPC BL::NPC
    MOB,
#define BL_MOB BL::MOB
    ITEM,
#define BL_ITEM BL::ITEM
    CHAT,
#define BL_CHAT BL::CHAT
    SKILL,
#define BL_SKILL BL::SKILL
    SPELL,
#define BL_SPELL BL::SPELL
};
enum class NpcSubtype : uint8_t
{
    WARP,
#define WARP NpcSubtype::WARP
    SHOP,
#define SHOP NpcSubtype::SHOP
    SCRIPT,
#define SCRIPT NpcSubtype::SCRIPT
    MONS,
#define MONS NpcSubtype::MONS
    MESSAGE,
#define MESSAGE NpcSubtype::MESSAGE
};

enum class mob_stat
{
    LV,
#define MOB_LV mob_stat::LV
    MAX_HP,
#define MOB_MAX_HP mob_stat::MAX_HP
    STR,
#define MOB_STR mob_stat::STR
    AGI,
#define MOB_AGI mob_stat::AGI
    VIT,
#define MOB_VIT mob_stat::VIT
    INT,
#define MOB_INT mob_stat::INT
    DEX,
#define MOB_DEX mob_stat::DEX
    LUK,
#define MOB_LUK mob_stat::LUK
    // low and high attacks
    ATK1,
#define MOB_ATK1 mob_stat::ATK1
    ATK2,
#define MOB_ATK2 mob_stat::ATK2
    // attack delay
    ADELAY,
#define MOB_ADELAY mob_stat::ADELAY
    DEF,
#define MOB_DEF mob_stat::DEF
    MDEF,
#define MOB_MDEF mob_stat::MDEF
    SPEED,
#define MOB_SPEED mob_stat::SPEED
    // These must come last:
    // [Fate] Encoded as base to 1024: 1024 means 100%
    XP_BONUS,
#define MOB_XP_BONUS mob_stat::XP_BONUS
    LAST,
#define MOB_LAST mob_stat::LAST
};

enum class MS : uint8_t
{
    IDLE,
#define MS_IDLE MS::IDLE
    WALK,
#define MS_WALK MS::WALK
    ATTACK,
#define MS_ATTACK MS::ATTACK
    DEAD,
#define MS_DEAD MS::DEAD
    DELAY,
#define MS_DELAY MS::DELAY
};

enum class ATK
{
    ZERO = 0,

    LUCKY,
#define ATK_LUCKY ATK::LUCKY
    FLEE,
#define ATK_FLEE ATK::FLEE
    DEF,
#define ATK_DEF ATK::DEF
};

enum class SP : uint16_t
{
    SPEED                       = 0,
#define SP_SPEED SP::SPEED

    // when used as "no stat"
    ZERO = 0,

    BASEEXP                     = 1,
#define SP_BASEEXP SP::BASEEXP
    JOBEXP                      = 2,
#define SP_JOBEXP SP::JOBEXP
    KARMA                       = 3,
#define SP_KARMA SP::KARMA
    HP                          = 5,
#define SP_HP SP::HP
    MAXHP                       = 6,
#define SP_MAXHP SP::MAXHP
    SP                          = 7,
#define SP_SP SP::SP
    MAXSP                       = 8,
#define SP_MAXSP SP::MAXSP
    STATUSPOINT                 = 9,
#define SP_STATUSPOINT SP::STATUSPOINT
    BASELEVEL                   = 11,
#define SP_BASELEVEL SP::BASELEVEL
    SKILLPOINT                  = 12,
#define SP_SKILLPOINT SP::SKILLPOINT
    STR                         = 13,
#define SP_STR SP::STR
    AGI                         = 14,
#define SP_AGI SP::AGI
    VIT                         = 15,
#define SP_VIT SP::VIT
    INT                         = 16,
#define SP_INT SP::INT
    DEX                         = 17,
#define SP_DEX SP::DEX
    LUK                         = 18,
#define SP_LUK SP::LUK
    CLASS                       = 19,
#define SP_CLASS SP::CLASS
    ZENY                        = 20,
#define SP_ZENY SP::ZENY
    SEX                         = 21,
#define SP_SEX SP::SEX
    NEXTBASEEXP                 = 22,
#define SP_NEXTBASEEXP SP::NEXTBASEEXP
    NEXTJOBEXP                  = 23,
#define SP_NEXTJOBEXP SP::NEXTJOBEXP
    WEIGHT                      = 24,
#define SP_WEIGHT SP::WEIGHT
    MAXWEIGHT                   = 25,
#define SP_MAXWEIGHT SP::MAXWEIGHT
    USTR                        = 32,
#define SP_USTR SP::USTR
    UAGI                        = 33,
#define SP_UAGI SP::UAGI
    UVIT                        = 34,
#define SP_UVIT SP::UVIT
    UINT                        = 35,
#define SP_UINT SP::UINT
    UDEX                        = 36,
#define SP_UDEX SP::UDEX
    ULUK                        = 37,
#define SP_ULUK SP::ULUK
    ATK1                        = 41,
#define SP_ATK1 SP::ATK1
    ATK2                        = 42,
#define SP_ATK2 SP::ATK2
    MATK1                       = 43,
#define SP_MATK1 SP::MATK1
    MATK2                       = 44,
#define SP_MATK2 SP::MATK2
    DEF1                        = 45,
#define SP_DEF1 SP::DEF1
    DEF2                        = 46,
#define SP_DEF2 SP::DEF2
    MDEF1                       = 47,
#define SP_MDEF1 SP::MDEF1
    MDEF2                       = 48,
#define SP_MDEF2 SP::MDEF2
    HIT                         = 49,
#define SP_HIT SP::HIT
    FLEE1                       = 50,
#define SP_FLEE1 SP::FLEE1
    FLEE2                       = 51,
#define SP_FLEE2 SP::FLEE2
    CRITICAL                    = 52,
#define SP_CRITICAL SP::CRITICAL
    ASPD                        = 53,
#define SP_ASPD SP::ASPD
    JOBLEVEL                    = 55,
#define SP_JOBLEVEL SP::JOBLEVEL
    UPPER                       = 56,
#define SP_UPPER SP::UPPER
    PARTNER                     = 57,
#define SP_PARTNER SP::PARTNER
    CART                        = 58,
#define SP_CART SP::CART
    FAME                        = 59,
#define SP_FAME SP::FAME
    UNBREAKABLE                 = 60,
#define SP_UNBREAKABLE SP::UNBREAKABLE
    DEAF                        = 70,
#define SP_DEAF SP::DEAF

    GM                          = 500,
#define SP_GM SP::GM

    ATTACKRANGE                 = 1000,
#define SP_ATTACKRANGE SP::ATTACKRANGE
    ATKELE                      = 1001,
#define SP_ATKELE SP::ATKELE
    DEFELE                      = 1002,
#define SP_DEFELE SP::DEFELE
    CASTRATE                    = 1003,
#define SP_CASTRATE SP::CASTRATE
    MAXHPRATE                   = 1004,
#define SP_MAXHPRATE SP::MAXHPRATE
    MAXSPRATE                   = 1005,
#define SP_MAXSPRATE SP::MAXSPRATE
    SPRATE                      = 1006,
#define SP_SPRATE SP::SPRATE
    ADDELE                      = 1007,
#define SP_ADDELE SP::ADDELE
    ADDRACE                     = 1008,
#define SP_ADDRACE SP::ADDRACE
    ADDSIZE                     = 1009,
#define SP_ADDSIZE SP::ADDSIZE
    SUBELE                      = 1010,
#define SP_SUBELE SP::SUBELE
    SUBRACE                     = 1011,
#define SP_SUBRACE SP::SUBRACE
    ADDEFF                      = 1012,
#define SP_ADDEFF SP::ADDEFF
    RESEFF                      = 1013,
#define SP_RESEFF SP::RESEFF
    BASE_ATK                    = 1014,
#define SP_BASE_ATK SP::BASE_ATK
    ASPD_RATE                   = 1015,
#define SP_ASPD_RATE SP::ASPD_RATE
    HP_RECOV_RATE               = 1016,
#define SP_HP_RECOV_RATE SP::HP_RECOV_RATE
    SP_RECOV_RATE               = 1017,
#define SP_SP_RECOV_RATE SP::SP_RECOV_RATE
    SPEED_RATE                  = 1018,
#define SP_SPEED_RATE SP::SPEED_RATE
    CRITICAL_DEF                = 1019,
#define SP_CRITICAL_DEF SP::CRITICAL_DEF
    NEAR_ATK_DEF                = 1020,
#define SP_NEAR_ATK_DEF SP::NEAR_ATK_DEF
    LONG_ATK_DEF                = 1021,
#define SP_LONG_ATK_DEF SP::LONG_ATK_DEF
    DOUBLE_RATE                 = 1022,
#define SP_DOUBLE_RATE SP::DOUBLE_RATE
    DOUBLE_ADD_RATE             = 1023,
#define SP_DOUBLE_ADD_RATE SP::DOUBLE_ADD_RATE
    MATK                        = 1024,
#define SP_MATK SP::MATK
    MATK_RATE                   = 1025,
#define SP_MATK_RATE SP::MATK_RATE
    IGNORE_DEF_ELE              = 1026,
#define SP_IGNORE_DEF_ELE SP::IGNORE_DEF_ELE
    IGNORE_DEF_RACE             = 1027,
#define SP_IGNORE_DEF_RACE SP::IGNORE_DEF_RACE
    ATK_RATE                    = 1028,
#define SP_ATK_RATE SP::ATK_RATE
    SPEED_ADDRATE               = 1029,
#define SP_SPEED_ADDRATE SP::SPEED_ADDRATE
    ASPD_ADDRATE                = 1030,
#define SP_ASPD_ADDRATE SP::ASPD_ADDRATE
    MAGIC_ATK_DEF               = 1031,
#define SP_MAGIC_ATK_DEF SP::MAGIC_ATK_DEF
    MISC_ATK_DEF                = 1032,
#define SP_MISC_ATK_DEF SP::MISC_ATK_DEF
    IGNORE_MDEF_ELE             = 1033,
#define SP_IGNORE_MDEF_ELE SP::IGNORE_MDEF_ELE
    IGNORE_MDEF_RACE            = 1034,
#define SP_IGNORE_MDEF_RACE SP::IGNORE_MDEF_RACE
    MAGIC_ADDELE                = 1035,
#define SP_MAGIC_ADDELE SP::MAGIC_ADDELE
    MAGIC_ADDRACE               = 1036,
#define SP_MAGIC_ADDRACE SP::MAGIC_ADDRACE
    MAGIC_SUBRACE               = 1037,
#define SP_MAGIC_SUBRACE SP::MAGIC_SUBRACE
    PERFECT_HIT_RATE            = 1038,
#define SP_PERFECT_HIT_RATE SP::PERFECT_HIT_RATE
    PERFECT_HIT_ADD_RATE        = 1039,
#define SP_PERFECT_HIT_ADD_RATE SP::PERFECT_HIT_ADD_RATE
    CRITICAL_RATE               = 1040,
#define SP_CRITICAL_RATE SP::CRITICAL_RATE
    GET_ZENY_NUM                = 1041,
#define SP_GET_ZENY_NUM SP::GET_ZENY_NUM
    ADD_GET_ZENY_NUM            = 1042,
#define SP_ADD_GET_ZENY_NUM SP::ADD_GET_ZENY_NUM
    ADD_DAMAGE_CLASS            = 1043,
#define SP_ADD_DAMAGE_CLASS SP::ADD_DAMAGE_CLASS
    ADD_MAGIC_DAMAGE_CLASS      = 1044,
#define SP_ADD_MAGIC_DAMAGE_CLASS SP::ADD_MAGIC_DAMAGE_CLASS
    ADD_DEF_CLASS               = 1045,
#define SP_ADD_DEF_CLASS SP::ADD_DEF_CLASS
    ADD_MDEF_CLASS              = 1046,
#define SP_ADD_MDEF_CLASS SP::ADD_MDEF_CLASS
    ADD_MONSTER_DROP_ITEM       = 1047,
#define SP_ADD_MONSTER_DROP_ITEM SP::ADD_MONSTER_DROP_ITEM
    DEF_RATIO_ATK_ELE           = 1048,
#define SP_DEF_RATIO_ATK_ELE SP::DEF_RATIO_ATK_ELE
    DEF_RATIO_ATK_RACE          = 1049,
#define SP_DEF_RATIO_ATK_RACE SP::DEF_RATIO_ATK_RACE
    ADD_SPEED                   = 1050,
#define SP_ADD_SPEED SP::ADD_SPEED
    HIT_RATE                    = 1051,
#define SP_HIT_RATE SP::HIT_RATE
    FLEE_RATE                   = 1052,
#define SP_FLEE_RATE SP::FLEE_RATE
    FLEE2_RATE                  = 1053,
#define SP_FLEE2_RATE SP::FLEE2_RATE
    DEF_RATE                    = 1054,
#define SP_DEF_RATE SP::DEF_RATE
    DEF2_RATE                   = 1055,
#define SP_DEF2_RATE SP::DEF2_RATE
    MDEF_RATE                   = 1056,
#define SP_MDEF_RATE SP::MDEF_RATE
    MDEF2_RATE                  = 1057,
#define SP_MDEF2_RATE SP::MDEF2_RATE
    SPLASH_RANGE                = 1058,
#define SP_SPLASH_RANGE SP::SPLASH_RANGE
    SPLASH_ADD_RANGE            = 1059,
#define SP_SPLASH_ADD_RANGE SP::SPLASH_ADD_RANGE
    AUTOSPELL                   = 1060,
#define SP_AUTOSPELL SP::AUTOSPELL
    HP_DRAIN_RATE               = 1061,
#define SP_HP_DRAIN_RATE SP::HP_DRAIN_RATE
    SP_DRAIN_RATE               = 1062,
#define SP_SP_DRAIN_RATE SP::SP_DRAIN_RATE
    SHORT_WEAPON_DAMAGE_RETURN  = 1063,
#define SP_SHORT_WEAPON_DAMAGE_RETURN SP::SHORT_WEAPON_DAMAGE_RETURN
    LONG_WEAPON_DAMAGE_RETURN   = 1064,
#define SP_LONG_WEAPON_DAMAGE_RETURN SP::LONG_WEAPON_DAMAGE_RETURN
    WEAPON_COMA_ELE             = 1065,
#define SP_WEAPON_COMA_ELE SP::WEAPON_COMA_ELE
    WEAPON_COMA_RACE            = 1066,
#define SP_WEAPON_COMA_RACE SP::WEAPON_COMA_RACE
    ADDEFF2                     = 1067,
#define SP_ADDEFF2 SP::ADDEFF2
    BREAK_WEAPON_RATE           = 1068,
#define SP_BREAK_WEAPON_RATE SP::BREAK_WEAPON_RATE
    BREAK_ARMOR_RATE            = 1069,
#define SP_BREAK_ARMOR_RATE SP::BREAK_ARMOR_RATE
    ADD_STEAL_RATE              = 1070,
#define SP_ADD_STEAL_RATE SP::ADD_STEAL_RATE
    MAGIC_DAMAGE_RETURN         = 1071,
#define SP_MAGIC_DAMAGE_RETURN SP::MAGIC_DAMAGE_RETURN
    RANDOM_ATTACK_INCREASE      = 1072,
#define SP_RANDOM_ATTACK_INCREASE SP::RANDOM_ATTACK_INCREASE
    ALL_STATS                   = 1073,
#define SP_ALL_STATS SP::ALL_STATS
    AGI_VIT                     = 1074,
#define SP_AGI_VIT SP::AGI_VIT
    AGI_DEX_STR                 = 1075,
#define SP_AGI_DEX_STR SP::AGI_DEX_STR
    PERFECT_HIDE                = 1076,
#define SP_PERFECT_HIDE SP::PERFECT_HIDE
    DISGUISE                    = 1077,
#define SP_DISGUISE SP::DISGUISE

    RESTART_FULL_RECORVER       = 2000,
#define SP_RESTART_FULL_RECORVER SP::RESTART_FULL_RECORVER
    NO_CASTCANCEL               = 2001,
#define SP_NO_CASTCANCEL SP::NO_CASTCANCEL
    NO_SIZEFIX                  = 2002,
#define SP_NO_SIZEFIX SP::NO_SIZEFIX
    NO_MAGIC_DAMAGE             = 2003,
#define SP_NO_MAGIC_DAMAGE SP::NO_MAGIC_DAMAGE
    NO_WEAPON_DAMAGE            = 2004,
#define SP_NO_WEAPON_DAMAGE SP::NO_WEAPON_DAMAGE
    NO_GEMSTONE                 = 2005,
#define SP_NO_GEMSTONE SP::NO_GEMSTONE
    NO_CASTCANCEL2              = 2006,
#define SP_NO_CASTCANCEL2 SP::NO_CASTCANCEL2
    INFINITE_ENDURE             = 2007,
#define SP_INFINITE_ENDURE SP::INFINITE_ENDURE
    UNBREAKABLE_WEAPON          = 2008,
#define SP_UNBREAKABLE_WEAPON SP::UNBREAKABLE_WEAPON
    SP_UNBREAKABLE_ARMOR        = 2009,
#define SP_UNBREAKABLE_ARMOR SP::UNBREAKABLE_ARMOR
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
};

constexpr
SP usp_to_sp(SP sp)
{
    return attr_to_sp(usp_to_attr(sp));
};


enum class LOOK : uint8_t
{
    BASE            = 0,
#define LOOK_BASE LOOK::BASE
    HAIR            = 1,
#define LOOK_HAIR LOOK::HAIR
    WEAPON          = 2,
#define LOOK_WEAPON LOOK::WEAPON
    HEAD_BOTTOM     = 3,
#define LOOK_HEAD_BOTTOM LOOK::HEAD_BOTTOM
    HEAD_TOP        = 4,
#define LOOK_HEAD_TOP LOOK::HEAD_TOP
    HEAD_MID        = 5,
#define LOOK_HEAD_MID LOOK::HEAD_MID
    HAIR_COLOR      = 6,
#define LOOK_HAIR_COLOR LOOK::HAIR_COLOR
    CLOTHES_COLOR   = 7,
#define LOOK_CLOTHES_COLOR LOOK::CLOTHES_COLOR
    SHIELD          = 8,
#define LOOK_SHIELD LOOK::SHIELD
    SHOES           = 9,
#define LOOK_SHOES LOOK::SHOES
    GLOVES          = 10,
#define LOOK_GLOVES LOOK::GLOVES
    CAPE            = 11,
#define LOOK_CAPE LOOK::CAPE
    MISC1           = 12,
#define LOOK_MISC1 LOOK::MISC1
    MISC2           = 13,
#define LOOK_MISC2 LOOK::MISC2

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
#define EQUIP_SHIELD EQUIP::SHIELD
    WEAPON  = 9,
#define EQUIP_WEAPON EQUIP::WEAPON
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

#endif // MAP_T_HPP
