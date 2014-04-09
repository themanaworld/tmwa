#ifndef TMWA_MAP_BATTLE_T_HPP
#define TMWA_MAP_BATTLE_T_HPP
//    battle.t.hpp - Not so scary code.
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

# include "../sanity.hpp"

# include "../generic/enum.hpp"

namespace e
{
enum class BF : uint16_t
{
    ZERO        = 0x0000,
    NEGATIVE_1  = 0xffff,

    WEAPON      = 0x0001,
    MAGIC       = 0x0002,
    MISC        = 0x0004,
    SHORT       = 0x0010,
    LONG        = 0x0040,
    SKILL       = 0x0100,
    NORMAL      = 0x0200,
    WEAPONMASK  = 0x000f,
    RANGEMASK   = 0x00f0,
    SKILLMASK   = 0x0f00,
};
ENUM_BITWISE_OPERATORS(BF)
}
using e::BF;

namespace e
{
// not actually an enum, but put in the namespace to hide the operators
// from non-ADL.
struct BCT
{
                        // former representation:
    uint8_t lo;         // 0x 00 00 00 ff
    uint8_t mid;        // 0x 00 00 ff 00
    uint8_t classic:4;  // 0x 00 0f 00 00
    uint8_t level:4;    // 0x 00 f0 00 00
    uint8_t unused;     // 0x ff 00 00 00

    explicit
    operator bool() { return lo || mid || classic || level || unused; }
};

constexpr
BCT operator & (BCT l, BCT r) { return {uint8_t(l.lo & r.lo), uint8_t(l.mid & r.mid), uint8_t(l.classic & r.classic), uint8_t(l.level & r.level), uint8_t(l.unused & r.unused) }; }
constexpr
BCT operator | (BCT l, BCT r) { return {uint8_t(l.lo | r.lo), uint8_t(l.mid | r.mid), uint8_t(l.classic | r.classic), uint8_t(l.level | r.level), uint8_t(l.unused | r.unused) }; }
constexpr
BCT operator ^ (BCT l, BCT r) { return {uint8_t(l.lo ^ r.lo), uint8_t(l.mid ^ r.mid), uint8_t(l.classic ^ r.classic), uint8_t(l.level ^ r.level), uint8_t(l.unused ^ r.unused) }; }
inline
BCT& operator &= (BCT& l, BCT r) { return l = l & r; }
inline
BCT& operator |= (BCT& l, BCT r) { return l = l & r; }
inline
BCT& operator ^= (BCT& l, BCT r) { return l = l & r; }
// BCT operator ~(BCT r);

constexpr
bool operator == (BCT l, BCT r) { return l.lo == r.lo && l.mid == r.mid && l.classic == r.classic && l.level == r.level && l.unused == r.unused; }
constexpr
bool operator != (BCT l, BCT r) { return !(l == r); }
} // namespace e
using e::BCT;

constexpr
BCT BCT_NOENEMY = {0x00, 0x00, 0x0, 0x0, 0x00};
constexpr
BCT BCT_ZERO = BCT_NOENEMY;
constexpr
BCT BCT_PARTY   = {0x00, 0x00, 0x1, 0x0, 0x00};
constexpr
BCT BCT_ENEMY   = {0x00, 0x00, 0x4, 0x0, 0x00};
constexpr
BCT BCT_NOPARTY = {0x00, 0x00, 0x5, 0x0, 0x00};
constexpr
BCT BCT_ALL     = {0x00, 0x00, 0x2, 0x0, 0x00};
constexpr
BCT BCT_NOONE   = {0x00, 0x00, 0x6, 0x0, 0x00};

constexpr
BCT BCT_lo_x01  = {0x01, 0x00, 0x0, 0x0, 0x00};
constexpr
BCT BCT_lo_x02  = {0x02, 0x00, 0x0, 0x0, 0x00};
constexpr
BCT BCT_mid_x05 = {0x00, 0x05, 0x0, 0x0, 0x00};
constexpr
BCT BCT_mid_x80 = {0x00, 0x80, 0x0, 0x0, 0x00};

constexpr
BCT BCT_highnib = {0x00, 0x00, 0x0, 0xf, 0x00};

enum class Element : uint8_t
{
    neutral = 0,
    water   = 1,
    earth   = 2,
    fire    = 3,
    wind    = 4,
    poison  = 5,
    _holy   = 6,
    dark    = 7,
    _spirit = 8,
    undead  = 9,

    COUNT   = 10,
};

enum class Race : uint8_t
{
    formless    = 0,
    undead      = 1,
    _brute      = 2,
    plant       = 3,
    _insect     = 4,
    _fish       = 5,
    _demon      = 6,
    demihuman   = 7,
    _angel      = 8,
    _dragon     = 9,
    // special - one of these is applied in addition
    boss        = 10,
    other       = 11,

    COUNT       = 12,
};

struct LevelElement
{
    uint8_t level;
    Element element;

    static
    LevelElement unpack(int packed)
    {
        LevelElement le;
        le.element = static_cast<Element>(packed % 10);
        le.level = packed / 10;
        return le;
    }
    int pack() const
    {
        return level * 10 + static_cast<uint8_t>(element);
    }
};

namespace e
{
enum class Elements : uint16_t
{
    ZERO    = 0x0000,
    neutral = 1 << 0,
    water   = 1 << 1,
    earth   = 1 << 2,
    fire    = 1 << 3,
    wind    = 1 << 4,
    poison  = 1 << 5,
    _holy   = 1 << 6,
    dark    = 1 << 7,
    _spirit = 1 << 8,
    undead  = 1 << 9,
};
ENUM_BITWISE_OPERATORS(Elements)

enum class Races : uint16_t
{
    ZERO        = 0x0000,
    formless    = 1 << 0,
    undead      = 1 << 1,
    _brute      = 1 << 2,
    plant       = 1 << 3,
    _insect     = 1 << 4,
    _fish       = 1 << 5,
    _demon      = 1 << 6,
    demihuman   = 1 << 7,
    _angel      = 1 << 8,
    _dragon     = 1 << 9,
    // special - one of these is applied in addition
    boss        = 1 << 10,
    other       = 1 << 11,
};
ENUM_BITWISE_OPERATORS(Races)
}
using e::Elements;
using e::Races;

constexpr
earray<Elements, Element, Element::COUNT> element_shift //=
{{
    Elements::neutral,
    Elements::water,
    Elements::earth,
    Elements::fire,
    Elements::wind,
    Elements::poison,
    Elements::_holy,
    Elements::dark,
    Elements::_spirit,
    Elements::undead,
}};

constexpr
earray<Races, Race, Race::COUNT> race_shift //=
{{
    Races::formless,
    Races::undead,
    Races::_brute,
    Races::plant,
    Races::_insect,
    Races::_fish,
    Races::_demon,
    Races::demihuman,
    Races::_angel,
    Races::_dragon,
    Races::boss,
    Races::other,
}};

enum class DamageType : uint8_t
{
    NORMAL      = 0x00,
    TAKEITEM    = 0x01,
    RETURNED    = 0x04,
    DOUBLED     = 0x08,
    CRITICAL    = 0x0a,
    FLEE2       = 0x0b,
};

#endif // TMWA_MAP_BATTLE_T_HPP
