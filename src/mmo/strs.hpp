#pragma once
//    strs.hpp - common string types
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

#include "../strings/vstring.hpp"


namespace tmwa
{
// affects CharName
#define NAME_IGNORING_CASE 1

struct AccountName : VString<23> {};
struct AccountPass : VString<23> {};
struct AccountCrypt : VString<39> {};
struct AccountEmail : VString<39> {};
struct ServerName : VString<19> {};
struct PartyName : VString<23> {};
struct GuildName : VString<23> {};
struct VarName : VString<31> {};

#define DEFAULT_EMAIL stringish<AccountEmail>("a@a.com"_s)

// It is decreed: a mapname shall not contain an extension
class MapName : public strings::_crtp_string<MapName, MapName, strings::ZPair>
{
    VString<15> _impl;
public:
    MapName() = default;
    MapName(VString<15> v) : _impl(v.xislice_h(std::find(v.begin(), v.end(), '.'))) {}

    iterator begin() const { return &*_impl.begin(); }
    iterator end() const { return &*_impl.end(); }
    const char *c_str() const { return _impl.c_str(); }

    operator RString() const { return _impl; }
    operator AString() const { return _impl; }
    operator TString() const { return _impl; }
    operator SString() const { return _impl; }
    operator ZString() const { return _impl; }
    operator XString() const { return _impl; }
};
template<>
inline
MapName stringish<MapName>(VString<15> iv)
{
    return iv;
}
inline
const char *decay_for_printf(const MapName& vs) { return vs.c_str(); }

// It is decreed: a charname is sometimes case sensitive
struct CharName
{
private:
    VString<23> _impl;
public:
    CharName() = default;
    explicit CharName(VString<23> name)
    : _impl(name)
    {}

    VString<23> to__actual() const
    {
        return _impl;
    }
    VString<23> to__lower() const
    {
        return _impl.to_lower();
    }
    VString<23> to__upper() const
    {
        return _impl.to_upper();
    }
    VString<23> to__canonical() const
    {
#if NAME_IGNORING_CASE == 0
        return to__actual();
#endif
#if NAME_IGNORING_CASE == 1
        return to__lower();
#endif
    }

    friend bool operator == (const CharName& l, const CharName& r)
    { return l.to__canonical() == r.to__canonical(); }
    friend bool operator != (const CharName& l, const CharName& r)
    { return l.to__canonical() != r.to__canonical(); }
    friend bool operator < (const CharName& l, const CharName& r)
    { return l.to__canonical() < r.to__canonical(); }
    friend bool operator <= (const CharName& l, const CharName& r)
    { return l.to__canonical() <= r.to__canonical(); }
    friend bool operator > (const CharName& l, const CharName& r)
    { return l.to__canonical() > r.to__canonical(); }
    friend bool operator >= (const CharName& l, const CharName& r)
    { return l.to__canonical() >= r.to__canonical(); }

    friend
    VString<23> convert_for_printf(const CharName& vs) { return vs.to__actual(); }
};
template<>
inline
CharName stringish<CharName>(VString<23> iv)
{
    return CharName(iv);
}

struct MobName : VString<23> {};
struct NpcName : VString<23> {};
struct ScriptLabel : VString<23> {};
struct ItemName : VString<23> {};

// formerly VString<49>, as name::label
struct NpcEvent
{
    NpcName npc;
    ScriptLabel label;

    explicit operator bool()
    {
        return npc || label;
    }
    bool operator !()
    {
        return !bool(*this);
    }

    friend bool operator == (const NpcEvent& l, const NpcEvent& r)
    {
        return l.npc == r.npc && l.label == r.label;
    }

    friend bool operator < (const NpcEvent& l, const NpcEvent& r)
    {
        return l.npc < r.npc || (l.npc == r.npc && l.label < r.label);
    }

    friend VString<49> convert_for_printf(NpcEvent ev);
};
} // namespace tmwa
