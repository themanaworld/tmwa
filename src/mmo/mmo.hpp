#ifndef TMWA_MMO_MMO_HPP
#define TMWA_MMO_MMO_HPP
//    mmo.hpp - Huge mess of structures and constants.
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

# include "fwd.hpp"

# include "../compat/memory.hpp"

# include "../strings/vstring.hpp"

# include "../generic/enum.hpp"

# include "ids.hpp"
# include "timer.t.hpp"

// affects CharName
# define NAME_IGNORING_CASE 1

constexpr int FIFOSIZE_SERVERLINK = 256 * 1024;

constexpr int MAX_MAP_PER_SERVER = 512;
constexpr int MAX_INVENTORY = 100;
constexpr int MAX_AMOUNT = 30000;
constexpr int MAX_ZENY = 1000000000;     // 1G zeny
constexpr int TRADE_MAX = 10;

enum class SkillID : uint16_t;
constexpr SkillID MAX_SKILL = SkillID(474); // not 450
constexpr SkillID get_enum_min_value(SkillID) { return SkillID(); }
constexpr SkillID get_enum_max_value(SkillID) { return MAX_SKILL; }

constexpr int GLOBAL_REG_NUM = 96;
constexpr int ACCOUNT_REG_NUM = 16;
constexpr int ACCOUNT_REG2_NUM = 16;
constexpr interval_t DEFAULT_WALK_SPEED = std::chrono::milliseconds(150);
constexpr interval_t MIN_WALK_SPEED = interval_t::zero();
constexpr interval_t MAX_WALK_SPEED = std::chrono::seconds(1);
constexpr int MAX_STORAGE = 300;
constexpr int MAX_PARTY = 12;

# define MIN_HAIR_STYLE battle_config.min_hair_style
# define MAX_HAIR_STYLE battle_config.max_hair_style
# define MIN_HAIR_COLOR battle_config.min_hair_color
# define MAX_HAIR_COLOR battle_config.max_hair_color
# define MIN_CLOTH_COLOR battle_config.min_cloth_color
# define MAX_CLOTH_COLOR battle_config.max_cloth_color

template<class T, size_t n>
struct Array
{
    T data[n];
public:
    T& operator [](size_t i) { assert (i < n); return data[i]; }
    const T& operator [](size_t i) const { assert (i < n); return data[i]; }

    T *begin() { return data + 0; }
    T *end() { return data + n; }
    const T *begin() const { return data + 0; }
    const T *end() const { return data + n; }
};

struct AccountName : VString<23> {};
struct AccountPass : VString<23> {};
struct AccountCrypt : VString<39> {};
struct AccountEmail : VString<39> {};
struct ServerName : VString<19> {};
struct PartyName : VString<23> {};
struct VarName : VString<31> {};

# define DEFAULT_EMAIL stringish<AccountEmail>("a@a.com"_s)

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
# if NAME_IGNORING_CASE == 0
        return to__actual();
# endif
# if NAME_IGNORING_CASE == 1
        return to__lower();
# endif
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

namespace e
{
enum class EPOS : uint16_t
{
    ZERO    = 0x0000,

    LEGS    = 0x0001,
    WEAPON  = 0x0002,
    GLOVES  = 0x0004,
    CAPE    = 0x0008,
    MISC1   = 0x0010,
    SHIELD  = 0x0020,
    SHOES   = 0x0040,
    MISC2   = 0x0080,
    HAT     = 0x0100,
    TORSO   = 0x0200,

    ARROW   = 0x8000,
};
ENUM_BITWISE_OPERATORS(EPOS)

constexpr EPOS get_enum_min_value(EPOS) { return EPOS(0x0000); }
constexpr EPOS get_enum_max_value(EPOS) { return EPOS(0xffff); }
}
using e::EPOS;

struct item
{
    ItemNameId nameid;
    short amount;
    EPOS equip;
};

struct point
{
    MapName map_;
    short x, y;
};

namespace e
{
enum class SkillFlags : uint16_t;
}
using e::SkillFlags;

struct skill_value
{
    unsigned short lv;
    SkillFlags flags;

    friend bool operator == (const skill_value& l, const skill_value& r)
    {
        return l.lv == r.lv && l.flags == r.flags;
    }
    friend bool operator != (const skill_value& l, const skill_value& r)
    {
        return !(l == r);
    }
};

struct global_reg
{
    VarName str;
    int value;
};

// Option and Opt1..3 in map.hpp
namespace e
{
enum class Option : uint16_t;
constexpr Option get_enum_min_value(Option) { return Option(0x0000); }
constexpr Option get_enum_max_value(Option) { return Option(0xffff); }
}
using e::Option;

enum class ATTR
{
    STR = 0,
    AGI = 1,
    VIT = 2,
    INT = 3,
    DEX = 4,
    LUK = 5,

    COUNT = 6,
};

constexpr ATTR ATTRs[6] =
{
    ATTR::STR,
    ATTR::AGI,
    ATTR::VIT,
    ATTR::INT,
    ATTR::DEX,
    ATTR::LUK,
};

enum class ItemLook : uint16_t
{
    NONE = 0,
    BLADE = 1, // or some other common weapons
    _2,
    SETZER_AND_SCYTHE = 3,
    _6,
    STAFF = 10,
    BOW = 11,
    _13 = 13,
    _14 = 14,
    _16 = 16,
    SINGLE_HANDED_COUNT = 17,

    DUAL_BLADE = 0x11,
    DUAL_2 = 0x12,
    DUAL_6 = 0x13,
    DUAL_12 = 0x14,
    DUAL_16 = 0x15,
    DUAL_26 = 0x16,
};

enum class SEX : uint8_t
{
    FEMALE = 0,
    MALE = 1,
    // For items. This is also used as error, sometime.
    NEUTRAL = 2,
};
inline
char sex_to_char(SEX sex)
{
    switch (sex)
    {
    case SEX::FEMALE: return 'F';
    case SEX::MALE: return 'M';
    default: return '\0';
    }
}
inline
SEX sex_from_char(char c)
{
    switch (c)
    {
    case 'F': return SEX::FEMALE;
    case 'M': return SEX::MALE;
    default: return SEX::NEUTRAL;
    }
}

struct CharKey
{
    CharName name;
    AccountId account_id;
    CharId char_id;
    unsigned char char_num;
};

struct CharData
{
    CharId partner_id;

    int base_exp, job_exp, zeny;

    Species species;
    short status_point, skill_point;
    int hp, max_hp, sp, max_sp;
    Option option;
    short karma, manner;
    short hair, hair_color, clothes_color;
    PartyId party_id;

    ItemLook weapon;
    ItemNameId shield;
    ItemNameId head_top, head_mid, head_bottom;

    unsigned char base_level, job_level;
    earray<short, ATTR, ATTR::COUNT> attrs;
    SEX sex;

    unsigned long mapip;
    unsigned int mapport;

    struct point last_point, save_point;
    Array<struct item, MAX_INVENTORY> inventory;
    earray<skill_value, SkillID, MAX_SKILL> skill;
    int global_reg_num;
    Array<struct global_reg, GLOBAL_REG_NUM> global_reg;
    int account_reg_num;
    Array<struct global_reg, ACCOUNT_REG_NUM> account_reg;
    int account_reg2_num;
    Array<struct global_reg, ACCOUNT_REG2_NUM> account_reg2;
};

struct CharPair
{
    CharKey key;
    std::unique_ptr<CharData> data;

    CharPair()
    : key{}, data(make_unique<CharData>())
    {}
};

struct storage
{
    bool dirty;
    AccountId account_id;
    short storage_status;
    short storage_amount;
    Array<struct item, MAX_STORAGE> storage_;
};

struct GM_Account
{
    AccountId account_id;
    GmLevel level;
};

struct party_member
{
    AccountId account_id;
    CharName name;
    MapName map;
    int leader, online, lv;
    struct map_session_data *sd;
};

struct party
{
    PartyId party_id;
    PartyName name;
    int exp;
    int item;
    Array<struct party_member, MAX_PARTY> member;
};

#endif // TMWA_MMO_MMO_HPP
