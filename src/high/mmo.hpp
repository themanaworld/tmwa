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

#include "../compat/borrow.hpp"
#include "../compat/memory.hpp"

#include "../proto2/net-CharData.hpp"
#include "../proto2/net-CharKey.hpp"
#include "../proto2/net-PartyMost.hpp"
#include "../proto2/net-GuildMost.hpp"
#include "../proto2/net-SkillValue.hpp"


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
    PartyId party_id;
    Borrowed<PartyMost> party_most;

    PartyMost& operator *() const { return *party_most; }
    Borrowed<PartyMost> operator->() const { return party_most; }
};

struct GuildPair
{
    GuildId guild_id;
    Borrowed<GuildMost> guild_most;

    GuildMost& operator *() const { return *guild_most; }
    Borrowed<GuildMost> operator->() const { return guild_most; }
};
} // namespace tmwa
