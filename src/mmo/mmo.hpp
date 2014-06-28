#pragma once
//    mmo.hpp - Huge mess of structures.
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

#include "../compat/memory.hpp"

#include "../proto2/types.hpp"


namespace tmwa
{
inline
bool operator == (const SkillValue& l, const SkillValue& r)
{
    return l.lv == r.lv && l.flags == r.flags;
}
inline
bool operator != (const SkillValue& l, const SkillValue& r)
{
    return !(l == r);
}

struct CharPair
{
    CharKey key;
    std::unique_ptr<CharData> data;

    CharPair()
    : key{}, data(make_unique<CharData>())
    {}
};

struct GM_Account
{
    AccountId account_id;
    GmLevel level;
};

struct PartyPair
{
    PartyId party_id = {};
    PartyMost *party_most = {};

    explicit
    operator bool() const { return party_most; }
    bool operator !() const { return !party_most; }
    PartyMost *operator->() const { return party_most; }
};
} // namespace tmwa
