#ifndef TMWA_MMO_IDS_HPP
#define TMWA_MMO_IDS_HPP
//    ids.hpp - special integer classes for various object IDs
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

# include "fwd.hpp"

# include "../ints/wrap.hpp"

# include "extract.hpp"

class Species : public Wrapped<uint16_t> { public: explicit operator bool() const = delete; bool operator !() const = delete; Species() : Wrapped<uint16_t>() {} protected: constexpr explicit Species(uint16_t a) : Wrapped<uint16_t>(a) {} };

class AccountId : public Wrapped<uint32_t> { public: AccountId() : Wrapped<uint32_t>() {} protected: constexpr explicit AccountId(uint32_t a) : Wrapped<uint32_t>(a) {} };
class CharId : public Wrapped<uint32_t> { public: CharId() : Wrapped<uint32_t>() {} protected: constexpr explicit CharId(uint32_t a) : Wrapped<uint32_t>(a) {} };
// important note: slave mobs synthesize PartyId as -BlockId of master
class PartyId : public Wrapped<uint32_t> { public: PartyId() : Wrapped<uint32_t>() {} protected: constexpr explicit PartyId(uint32_t a) : Wrapped<uint32_t>(a) {} };
class ItemNameId : public Wrapped<uint16_t> { public: ItemNameId() : Wrapped<uint16_t>() {} protected: constexpr explicit ItemNameId(uint16_t a) : Wrapped<uint16_t>(a) {} };

class GmLevel
{
    uint32_t bits;

    friend bool extract(XString str, GmLevel *lvl) { return extract(str, &lvl->bits); }
    constexpr explicit
    GmLevel(uint32_t b) : bits(b) {}
    constexpr explicit
    operator uint32_t() const { return bits; }

    template<class T>
    explicit
    GmLevel(T) = delete;
    template<class T, typename=typename std::enable_if<!std::is_same<T, uint32_t>::value && !std::is_same<T, bool>::value>::type>
    explicit
    operator T() = delete;
public:
    constexpr
    GmLevel() : bits() {}
    constexpr static
    GmLevel from(uint32_t bits) { return GmLevel(bits); }
    template<class T>
    constexpr static
    GmLevel from(T) = delete;

    constexpr explicit
    operator bool() const { return bits; }
    constexpr
    bool operator !() const { return !bits; }

    // the argument is the level of a command
    constexpr
    bool satisfies(GmLevel perm) const { return bits >= perm.bits; }
    // the argument is another player's gm level, for info commands
    constexpr
    bool detects(GmLevel other) const { return bits >= other.bits; }
    // the argument is another player's gm level, for aggressive commands
    constexpr
    bool overwhelms(GmLevel other) const { return bits >= other.bits; }
    // the argument is another potential permission level
    constexpr
    bool obsoletes(GmLevel plvl) const { return bits >= plvl.bits; }

    constexpr
    uint16_t get_public_word() const
    {
        return (bits == 60 || bits == 99) ? 0x0080 : 0;
    }

    constexpr
    uint32_t get_all_bits() const
    {
        return bits;
    }

    friend constexpr
    bool operator == (GmLevel l, GmLevel r)
    {
        return l.bits == r.bits;
    }
    friend constexpr
    bool operator != (GmLevel l, GmLevel r)
    {
        return l.bits != r.bits;
    }
};

inline
uint32_t convert_for_printf(GmLevel g)
{
    return g.get_all_bits();
}

#endif // TMWA_MMO_IDS_HPP
