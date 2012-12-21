#ifndef MAP_T_HPP
#define MAP_T_HPP

#include "../common/utils.hpp"

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
    sign            = 0x8000,
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

    sign            = 0x8000,
};

ENUM_BITWISE_OPERATORS(Option)
ENUM_BITWISE_OPERATORS(Opt2)
ENUM_BITWISE_OPERATORS(Opt3)


enum
{ BL_NUL, BL_PC, BL_NPC, BL_MOB, BL_ITEM, BL_CHAT, BL_SKILL, BL_SPELL };
enum
{ WARP, SHOP, SCRIPT, MONS, MESSAGE };
#endif // MAP_T_HPP
