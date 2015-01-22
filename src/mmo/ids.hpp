#pragma once
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

#include "fwd.hpp"

#include "../ints/little.hpp"
#include "../ints/wrap.hpp"


namespace tmwa
{
class Species : public Wrapped<uint16_t> { public: explicit operator bool() const = delete; bool operator !() const = delete; constexpr Species() : Wrapped<uint16_t>() {} protected: constexpr explicit Species(uint16_t a) : Wrapped<uint16_t>(a) {} };

constexpr Species NEGATIVE_SPECIES = Species();

bool impl_extract(XString str, Species *w);


class AccountId : public Wrapped<uint32_t> { public: constexpr AccountId() : Wrapped<uint32_t>() {} protected: constexpr explicit AccountId(uint32_t a) : Wrapped<uint32_t>(a) {} };
class CharId : public Wrapped<uint32_t> { public: constexpr CharId() : Wrapped<uint32_t>() {} protected: constexpr explicit CharId(uint32_t a) : Wrapped<uint32_t>(a) {} };
// important note: slave mobs synthesize PartyId as -BlockId of master
class PartyId : public Wrapped<uint32_t> { public: constexpr PartyId() : Wrapped<uint32_t>() {} protected: constexpr explicit PartyId(uint32_t a) : Wrapped<uint32_t>(a) {} };
class ItemNameId : public Wrapped<uint16_t> { public: constexpr ItemNameId() : Wrapped<uint16_t>() {} protected: constexpr explicit ItemNameId(uint16_t a) : Wrapped<uint16_t>(a) {} };

class BlockId : public Wrapped<uint32_t> { public: constexpr BlockId() : Wrapped<uint32_t>() {} protected: constexpr explicit BlockId(uint32_t a) : Wrapped<uint32_t>(a) {} };

bool impl_extract(XString str, GmLevel *lvl);
class GmLevel
{
    uint32_t bits;

    friend bool impl_extract(XString str, GmLevel *lvl);
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

    friend
    bool native_to_network(Byte *network, GmLevel native)
    {
        network->value = native.bits;
        return true; // LIES. But this code is going away soon anyway
    }
    friend
    bool network_to_native(GmLevel *native, Byte network)
    {
        native->bits = network.value;
        return true; // LIES. But this code is going away soon anyway
    }

    // TODO kill this code too
    friend
    bool native_to_network(Little16 *network, GmLevel native)
    {
        uint16_t tmp = native.bits;
        return native_to_network(network, tmp);
    }
    friend
    bool network_to_native(GmLevel *native, Little16 network)
    {
        uint16_t tmp;
        bool rv = network_to_native(&tmp, network);
        native->bits = tmp;
        return rv;
    }

    friend
    bool native_to_network(Little32 *network, GmLevel native)
    {
        return native_to_network(network, native.bits);
    }
    friend
    bool network_to_native(GmLevel *native, Little32 network)
    {
        return network_to_native(&native->bits, network);
    }
};

inline
uint32_t convert_for_printf(GmLevel g)
{
    return g.get_all_bits();
}
} // namespace tmwa
