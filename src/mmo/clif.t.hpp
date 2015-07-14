#pragma once
//    clif.t.hpp - Network interface to the client.
//
//    Copyright © ????-2004 Athena Dev Teams
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

#include "fwd.hpp"

#include <cstdint>

#include "../ints/little.hpp"

#include "../compat/iter.hpp"

#include "../generic/enum.hpp"

#include "consts.hpp"
#include "enums.hpp"


namespace tmwa
{
namespace e
{
// [Fate] status.option properties.  These are persistent status changes.
// IDs that are not listed are not used in the code (to the best of my knowledge)
enum class Opt0 : uint16_t
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

ENUM_BITWISE_OPERATORS(Opt0)
ENUM_BITWISE_OPERATORS(Opt2)
ENUM_BITWISE_OPERATORS(Opt3)
}
using e::Opt0;
using e::Opt1;
using e::Opt2;
using e::Opt3;


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

enum class BeingRemoveWhy : uint8_t
{
    // general disappearance
    GONE = 0,
    // only case handled specially in client
    DEAD = 1,
    QUIT = 2,
    WARPED = 3,
    // handled specially in clif_clearchar - sent as 0 over network
    DISGUISE = 9,

    // handled speciall in mob_warp - not actually sent over network
    NEGATIVE1 = 0xff,
};

enum class PickupFail : uint8_t
{
    OKAY        = 0,
    BAD_ITEM    = 1,
    TOO_HEAVY   = 2,
    TOO_FAR     = 3,
    INV_FULL    = 4,
    STACK_FULL  = 5,
    DROP_STEAL  = 6,
};

// this is used for both input and output
// different values are valid in 0x0089 vs 0x008a
enum class DamageType : uint8_t
{
    NORMAL      = 0x00,
    TAKEITEM    = 0x01,
    SIT         = 0x02,
    STAND       = 0x03,
    RETURNED    = 0x04,
    CONTINUOUS  = 0x07,
    DOUBLED     = 0x08,
    CRITICAL    = 0x0a,
    FLEE2       = 0x0b,
};

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

// Note: there is also a typedef by this name in <dirent.h>
// but we should be fine since we never include it.
// (in the long term we should still rename this though)
enum class DIR : uint8_t
{
    S   = 0,
    SW  = 1,
    W   = 2,
    NW  = 3,
    N   = 4,
    NE  = 5,
    E   = 6,
    SE  = 7,

    COUNT,
};

constexpr
earray<int, DIR, DIR::COUNT> dirx //=
{{
    0, -1, -1, -1, 0, 1, 1, 1,
}}, diry //=
{{
    1, 1, 0, -1, -1, -1, 0, 1,
}};

constexpr
bool dir_is_diagonal(DIR d)
{
    return static_cast<uint8_t>(d) & 1;
}


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
#if 0
    KARMA                       = 3,
#endif

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

#if 0
    PARTNER                     = 57,
    CART                        = 58,
    FAME                        = 59,
    UNBREAKABLE                 = 60,
#endif

    DEAF                        = 70,

    // sent to client
    GM                          = 500,

    // sent to client
    ATTACKRANGE                 = 1000,
#if 0
    ATKELE                      = 1001,
#endif
#if 0
    DEFELE                      = 1002,
#endif
#if 0
    CASTRATE                    = 1003,
#endif
    MAXHPRATE                   = 1004,
#if 0
    MAXSPRATE                   = 1005,
#endif
#if 0
    SPRATE                      = 1006,
#endif

#if 0
    ADDEFF                      = 1012,
#endif
#if 0
    RESEFF                      = 1013,
#endif
    BASE_ATK                    = 1014,
    ASPD_RATE                   = 1015,
    HP_RECOV_RATE               = 1016,
#if 0
    SP_RECOV_RATE               = 1017,
#endif
#if 0
    SPEED_RATE                  = 1018,
#endif
    CRITICAL_DEF                = 1019,
#if 0
    NEAR_ATK_DEF                = 1020,
#endif
#if 0
    LONG_ATK_DEF                = 1021,
#endif
#if 0
    DOUBLE_RATE                 = 1022,
#endif
    DOUBLE_ADD_RATE             = 1023,
#if 0
    MATK                        = 1024,
#endif
#if 0
    MATK_RATE                   = 1025,
#endif
#if 0
    IGNORE_DEF_ELE              = 1026,
#endif
#if 0
    IGNORE_DEF_RACE             = 1027,
#endif
#if 0
    ATK_RATE                    = 1028,
#endif
    SPEED_ADDRATE               = 1029,
#if 0
    ASPD_ADDRATE                = 1030,
#endif
#if 0
    MAGIC_ATK_DEF               = 1031,
#endif
#if 0
    MISC_ATK_DEF                = 1032,
#endif
#if 0
    IGNORE_MDEF_ELE             = 1033,
#endif
#if 0
    IGNORE_MDEF_RACE            = 1034,
#endif

#if 0
    PERFECT_HIT_RATE            = 1038,
#endif
#if 0
    PERFECT_HIT_ADD_RATE        = 1039,
#endif
#if 0
    CRITICAL_RATE               = 1040,
#endif
#if 0
    GET_ZENY_NUM                = 1041,
#endif
#if 0
    ADD_GET_ZENY_NUM            = 1042,
#endif

#if 0
    ADD_MONSTER_DROP_ITEM       = 1047,
#endif
#if 0
    DEF_RATIO_ATK_ELE           = 1048,
#endif
#if 0
    DEF_RATIO_ATK_RACE          = 1049,
#endif
#if 0
    ADD_SPEED                   = 1050,
#endif
#if 0
    HIT_RATE                    = 1051,
#endif
#if 0
    FLEE_RATE                   = 1052,
#endif
#if 0
    FLEE2_RATE                  = 1053,
#endif
    DEF_RATE                    = 1054,
    DEF2_RATE                   = 1055,
#if 0
    MDEF_RATE                   = 1056,
#endif
#if 0
    MDEF2_RATE                  = 1057,
#endif
#if 0
    SPLASH_RANGE                = 1058,
#endif
#if 0
    SPLASH_ADD_RANGE            = 1059,
#endif

    HP_DRAIN_RATE               = 1061,
#if 0
    SP_DRAIN_RATE               = 1062,
#endif
#if 0
    SHORT_WEAPON_DAMAGE_RETURN  = 1063,
#endif
#if 0
    LONG_WEAPON_DAMAGE_RETURN   = 1064,
#endif

#if 0
    ADDEFF2                     = 1067,
#endif
    BREAK_WEAPON_RATE           = 1068,
    BREAK_ARMOR_RATE            = 1069,
    ADD_STEAL_RATE              = 1070,
    MAGIC_DAMAGE_RETURN         = 1071,
#if 0
    RANDOM_ATTACK_INCREASE      = 1072,
#endif

    POS_X                       = 1074,
    POS_Y                       = 1075,
    PVP_CHANNEL                 = 1076,
    BL_ID                       = 1077,
};

constexpr
SP attr_to_sp(ATTR attr)
{
    return static_cast<SP>(static_cast<uint16_t>(attr) + static_cast<uint16_t>(SP::STR));
}

constexpr
ATTR sp_to_attr(SP sp)
{
    return static_cast<ATTR>(static_cast<uint16_t>(sp) - static_cast<uint16_t>(SP::STR));
}

constexpr
SP attr_to_usp(ATTR attr)
{
    return static_cast<SP>(static_cast<uint16_t>(attr) + static_cast<uint16_t>(SP::USTR));
}

constexpr
ATTR usp_to_attr(SP sp)
{
    return static_cast<ATTR>(static_cast<uint16_t>(sp) - static_cast<uint16_t>(SP::USTR));
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


// xxxx xxxx  xxyy yyyy  yyyy dddd
struct NetPosition1
{
    Byte data[3];
};

struct Position1
{
    uint16_t x, y;
    DIR dir;
};

inline
bool native_to_network(NetPosition1 *network, Position1 native)
{
    uint16_t x = native.x;
    uint16_t y = native.y;
    uint8_t d = static_cast<uint8_t>(native.dir);

    uint8_t *p = reinterpret_cast<uint8_t *>(network);
    p[0] = x >> 2;
    p[1] = (x << 6) | ((y >> 4) & 0x3f);
    p[2] = y << 4 | d;

    return x < 1024 && y < 1024 && d < 16;
}

inline
bool network_to_native(Position1 *native, NetPosition1 network)
{
    const uint8_t *p = reinterpret_cast<const uint8_t *>(&network);
    native->x = (p[0] & (0x3ff >> 2)) << 2 | p[1] >> (8 - 2);
    native->y = (p[1] & (0x3ff >> 4)) << 4 | p[2] >> (8 - 4);
    uint8_t d = p[2] & 0x0f;
    native->dir = static_cast<DIR>(d);
    return d < 8;
}

// x0xx xxxx  xxy0 yyyy  yyyy x1xx  xxxx xxy1  yyyy yyyy
struct NetPosition2
{
    Byte data[5];
};

struct Position2
{
    uint16_t x0, y0;
    uint16_t x1, y1;
};

inline
bool native_to_network(NetPosition2 *network, Position2 native)
{
    uint16_t x0 = native.x0;
    uint16_t y0 = native.y0;
    uint16_t x1 = native.x1;
    uint16_t y1 = native.y1;

    uint8_t *p = reinterpret_cast<uint8_t *>(network);
    p[0] = x0 >> 2;
    p[1] = (x0 << 6) | ((y0 >> 4) & 0x3f);
    p[2] = (y0 << 4) | ((x1 >> 6) & 0x0f);
    p[3] = (x1 << 2) | ((y1 >> 8) & 0x03);
    p[4] = y1;

    return x0 < 1024 && y0 < 1024 && x1 < 1024 && y1 < 1024;
}

inline
bool network_to_native(Position2 *native, NetPosition2 network)
{
    const uint8_t *p = reinterpret_cast<const uint8_t *>(&network);
    native->x0 = (p[0] & (0x3ff >> 2)) << 2 | p[1] >> (8 - 2);
    native->y0 = (p[1] & (0x3ff >> 4)) << 4 | p[2] >> (8 - 4);
    native->x1 = (p[2] & (0x3ff >> 6)) << 6 | p[3] >> (8 - 6);
    native->y1 = (p[3] & (0x3ff >> 8)) << 8 | p[4] >> (8 - 8);
    return true;
}

struct IOff2;
struct SOff1;

struct IOff0
{
    uint16_t index;

    bool ok() const
    { return get0() < MAX_INVENTORY; }
    uint16_t get0() const
    { return index; }
    static IOff0 from(uint16_t i)
    { return IOff0{i}; }
    static IteratorPair<ValueIterator<IOff0>> iter()
    { return {IOff0::from(0), IOff0::from(MAX_INVENTORY)}; }
    friend uint16_t convert_for_printf(IOff0 i0) { return i0.index; }

    IOff0& operator ++() { ++index; return *this; }
    friend bool operator == (IOff0 l, IOff0 r) { return l.index == r.index; }
    friend bool operator != (IOff0 l, IOff0 r) { return !(l == r); }
    IOff2 shift() const;

    IOff0() : index(0) {}
private:
    explicit IOff0(uint16_t i) : index(i) {}
};

struct SOff0
{
    uint16_t index;

    bool ok() const
    { return get0() < MAX_STORAGE; }
    uint16_t get0() const
    { return index; }
    static SOff0 from(uint16_t i)
    { return SOff0{i}; }
    static IteratorPair<ValueIterator<SOff0>> iter()
    { return {SOff0::from(0), SOff0::from(MAX_STORAGE)}; }
    friend uint16_t convert_for_printf(SOff0 s0) { return s0.index; }

    SOff0& operator ++() { ++index; return *this; }
    friend bool operator == (SOff0 l, SOff0 r) { return l.index == r.index; }
    friend bool operator != (SOff0 l, SOff0 r) { return !(l == r); }
    SOff1 shift() const;

    SOff0() : index(0) {}
private:
    explicit SOff0(uint16_t i) : index(i) {}
};

struct IOff2
{
    uint16_t index;

    bool ok() const
    { return get2() < MAX_INVENTORY; }
    uint16_t get2() const
    { return index - 2; }
    static IOff2 from(uint16_t i)
    { return IOff2{static_cast<uint16_t>(i + 2)}; }
    static IteratorPair<ValueIterator<IOff2>> iter()
    { return {IOff2::from(0), IOff2::from(MAX_INVENTORY)}; }

    IOff2& operator ++() { ++index; return *this; }
    friend bool operator == (IOff2 l, IOff2 r) { return l.index == r.index; }
    friend bool operator != (IOff2 l, IOff2 r) { return !(l == r); }
    IOff0 unshift() const
    { return IOff0::from(get2()); }

    IOff2() : index(0) {}
private:
    explicit IOff2(uint16_t i) : index(i) {}
};

struct SOff1
{
    uint16_t index;

    bool ok() const
    { return get1() < MAX_STORAGE; }
    uint16_t get1() const
    { return index - 1; }
    static SOff1 from(uint16_t i)
    { return SOff1{static_cast<uint16_t>(i + 1)}; }
    static IteratorPair<ValueIterator<SOff1>> iter()
    { return {SOff1::from(0), SOff1::from(MAX_STORAGE)}; }

    SOff1& operator ++() { ++index; return *this; }
    friend bool operator == (SOff1 l, SOff1 r) { return l.index == r.index; }
    friend bool operator != (SOff1 l, SOff1 r) { return !(l == r); }
    SOff0 unshift() const
    { return SOff0::from(get1()); }

    SOff1() : index(0) {}
private:
    explicit SOff1(uint16_t i) : index(i) {}
};

inline IOff2 IOff0::shift() const
{ return IOff2::from(get0()); }
inline SOff1 SOff0::shift() const
{ return SOff1::from(get0()); }

inline
bool native_to_network(Little16 *network, IOff2 native)
{
    return native_to_network(network, native.index);
}

inline
bool network_to_native(IOff2 *native, Little16 network)
{
    return network_to_native(&native->index, network);
}

inline
bool native_to_network(Little16 *network, SOff1 native)
{
    return native_to_network(network, native.index);
}

inline
bool network_to_native(SOff1 *native, Little16 network)
{
    return network_to_native(&native->index, network);
}
} // namespace tmwa
