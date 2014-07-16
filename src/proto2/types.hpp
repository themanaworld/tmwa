#pragma once
//    proto2/types.hpp - Forward declarations of packet component types
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "fwd.hpp"

#include "../generic/array.hpp"
#include "../mmo/consts.hpp"

//TODO split the includes
#include <cstdint>
#include "../ints/little.hpp"
#include "../strings/vstring.hpp"
#include "../net/ip.hpp"
#include "../net/timer.t.hpp"
#include "../mmo/consts.hpp"
#include "../mmo/enums.hpp"
#include "../mmo/human_time_diff.hpp"
#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"
#include "../mmo/utils.hpp"
#include "../mmo/version.hpp"
#include "../login/login.t.hpp"
#include "../map/clif.t.hpp"
#include "../map/skill.t.hpp"

namespace tmwa
{
template<class T>
bool native_to_network(T *network, T native)
{
    *network = native;
    return true;
}
template<class T>
bool network_to_native(T *native, T network)
{
    *native = network;
    return true;
}
template<class T, size_t N>
struct NetArray
{
    T data[N];
};
template<class T, class U, class I>
bool native_to_network(NetArray<T, I::alloc_size> *network, GenericArray<U, I> native)
{
    for (size_t i = 0; i < I::alloc_size; ++i)
    {
        if (!native_to_network(&(*network).data[i], native[I::offset_to_index(i)]))
            return false;
    }
    return true;
}
template<class T, class U, class I>
bool network_to_native(GenericArray<U, I> *native, NetArray<T, I::alloc_size> network)
{
    for (size_t i = 0; i < I::alloc_size; ++i)
    {
        if (!network_to_native(&(*native)[I::offset_to_index(i)], network.data[i]))
            return false;
    }
    return true;
}

template<size_t N>
struct NetString
{
    char data[N];
};
template<size_t N>
bool native_to_network(NetString<N> *network, VString<N-1> native)
{
    // basically WBUF_STRING
    char *const begin = network->data;
    char *const end = begin + N;
    char *const mid = std::copy(native.begin(), native.end(), begin);
    std::fill(mid, end, '\0');
    return true;
}
template<size_t N>
bool network_to_native(VString<N-1> *native, NetString<N> network)
{
    // basically RBUF_STRING
    const char *const begin = network.data;
    const char *const end = begin + N;
    const char *const mid = std::find(begin, end, '\0');
    *native = XString(begin, mid, nullptr);
    return true;
}

inline
bool native_to_network(NetString<24> *network, CharName native)
{
    VString<23> tmp = native.to__actual();
    bool rv = native_to_network(network, tmp);
    return rv;
}
inline
bool network_to_native(CharName *native, NetString<24> network)
{
    VString<23> tmp;
    bool rv = network_to_native(&tmp, network);
    *native = stringish<CharName>(tmp);
    return rv;
}

inline
bool native_to_network(NetString<16> *network, MapName native)
{
    XString tmp = native;
    bool rv = native_to_network(network, VString<15>(tmp));
    return rv;
}
inline
bool network_to_native(MapName *native, NetString<16> network)
{
    VString<15> tmp;
    bool rv = network_to_native(&tmp, network);
    *native = stringish<MapName>(tmp);
    return rv;
}

template<class T, size_t N>
struct SkewedLength
{
    T data;
};
template<class T, size_t N, class U>
bool native_to_network(SkewedLength<T, N> *network, U native)
{
    native -= N;
    return native_to_network(&network->data, native);
}
template<class T, size_t N, class U>
bool network_to_native(U *native, SkewedLength<T, N> network)
{
    bool rv = network_to_native(native, network.data);
    *native += N;
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(Byte *network, DIR native)
{
    bool rv = true;
    uint8_t tmp = static_cast<uint8_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(DIR *native, Byte network)
{
    bool rv = true;
    uint8_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<DIR>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Byte *network, BeingRemoveWhy native)
{
    bool rv = true;
    uint8_t tmp = static_cast<uint8_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(BeingRemoveWhy *native, Byte network)
{
    bool rv = true;
    uint8_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<BeingRemoveWhy>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, Opt1 native)
{
    bool rv = true;
    uint16_t tmp = static_cast<uint16_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Opt1 *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<Opt1>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, Opt2 native)
{
    bool rv = true;
    uint16_t tmp = static_cast<uint16_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Opt2 *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<Opt2>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, Opt3 native)
{
    bool rv = true;
    uint16_t tmp = static_cast<uint16_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Opt3 *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<Opt3>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Byte *network, ItemType native)
{
    bool rv = true;
    uint8_t tmp = static_cast<uint8_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(ItemType *native, Byte network)
{
    bool rv = true;
    uint8_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<ItemType>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Byte *network, PickupFail native)
{
    bool rv = true;
    uint8_t tmp = static_cast<uint8_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(PickupFail *native, Byte network)
{
    bool rv = true;
    uint8_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<PickupFail>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Byte *network, DamageType native)
{
    bool rv = true;
    uint8_t tmp = static_cast<uint8_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(DamageType *native, Byte network)
{
    bool rv = true;
    uint8_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<DamageType>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, SP native)
{
    bool rv = true;
    uint16_t tmp = static_cast<uint16_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SP *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<SP>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Byte *network, LOOK native)
{
    bool rv = true;
    uint8_t tmp = static_cast<uint8_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(LOOK *native, Byte network)
{
    bool rv = true;
    uint8_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<LOOK>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, SkillID native)
{
    bool rv = true;
    uint16_t tmp = static_cast<uint16_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SkillID *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<SkillID>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, StatusChange native)
{
    bool rv = true;
    uint16_t tmp = static_cast<uint16_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(StatusChange *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<StatusChange>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, SkillFlags native)
{
    bool rv = true;
    uint16_t tmp = static_cast<uint16_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SkillFlags *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<SkillFlags>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Byte *network, SEX native)
{
    bool rv = true;
    uint8_t tmp = static_cast<uint8_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SEX *native, Byte network)
{
    bool rv = true;
    uint8_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<SEX>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, Option native)
{
    bool rv = true;
    uint16_t tmp = static_cast<uint16_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Option *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<Option>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, EPOS native)
{
    bool rv = true;
    uint16_t tmp = static_cast<uint16_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(EPOS *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<EPOS>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, ItemLook native)
{
    bool rv = true;
    uint16_t tmp = static_cast<uint16_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(ItemLook *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<ItemLook>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, Species native)
{
    bool rv = true;
    uint16_t tmp = unwrap<Species>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Species *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = wrap<Species>(tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little32 *network, AccountId native)
{
    bool rv = true;
    uint32_t tmp = unwrap<AccountId>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(AccountId *native, Little32 network)
{
    bool rv = true;
    uint32_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = wrap<AccountId>(tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little32 *network, CharId native)
{
    bool rv = true;
    uint32_t tmp = unwrap<CharId>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(CharId *native, Little32 network)
{
    bool rv = true;
    uint32_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = wrap<CharId>(tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little32 *network, PartyId native)
{
    bool rv = true;
    uint32_t tmp = unwrap<PartyId>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(PartyId *native, Little32 network)
{
    bool rv = true;
    uint32_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = wrap<PartyId>(tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little16 *network, ItemNameId native)
{
    bool rv = true;
    uint16_t tmp = unwrap<ItemNameId>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(ItemNameId *native, Little16 network)
{
    bool rv = true;
    uint16_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = wrap<ItemNameId>(tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little32 *network, ItemNameId native)
{
    bool rv = true;
    uint32_t tmp = unwrap<ItemNameId>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(ItemNameId *native, Little32 network)
{
    bool rv = true;
    uint32_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = wrap<ItemNameId>(tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(Little32 *network, BlockId native)
{
    bool rv = true;
    uint32_t tmp = unwrap<BlockId>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(BlockId *native, Little32 network)
{
    bool rv = true;
    uint32_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = wrap<BlockId>(tmp);
    return rv;
}
struct NetHumanTimeDiff
{
    Little16 year;
    Little16 month;
    Little16 day;
    Little16 hour;
    Little16 minute;
    Little16 second;
};
inline __attribute__((warn_unused_result))
bool native_to_network(NetHumanTimeDiff *network, HumanTimeDiff native)
{
    bool rv = true;
    int16_t year = native.year; rv &= native_to_network(&network->year, year);
    int16_t month = native.month; rv &= native_to_network(&network->month, month);
    int16_t day = native.day; rv &= native_to_network(&network->day, day);
    int16_t hour = native.hour; rv &= native_to_network(&network->hour, hour);
    int16_t minute = native.minute; rv &= native_to_network(&network->minute, minute);
    int16_t second = native.second; rv &= native_to_network(&network->second, second);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(HumanTimeDiff *native, NetHumanTimeDiff network)
{
    bool rv = true;
    int16_t year; rv &= network_to_native(&year, network.year); native->year = year;
    int16_t month; rv &= network_to_native(&month, network.month); native->month = month;
    int16_t day; rv &= network_to_native(&day, network.day); native->day = day;
    int16_t hour; rv &= network_to_native(&hour, network.hour); native->hour = hour;
    int16_t minute; rv &= network_to_native(&minute, network.minute); native->minute = minute;
    int16_t second; rv &= network_to_native(&second, network.second); native->second = second;
    return rv;
}

struct NetVersion
{
    Byte major;
    Byte minor;
    Byte patch;
    Byte devel;
    Byte flags;
    Byte which;
    Little16 vend;
};
inline __attribute__((warn_unused_result))
bool native_to_network(NetVersion *network, Version native)
{
    bool rv = true;
    uint8_t major = native.major; rv &= native_to_network(&network->major, major);
    uint8_t minor = native.minor; rv &= native_to_network(&network->minor, minor);
    uint8_t patch = native.patch; rv &= native_to_network(&network->patch, patch);
    uint8_t devel = native.devel; rv &= native_to_network(&network->devel, devel);
    uint8_t flags = native.flags; rv &= native_to_network(&network->flags, flags);
    uint8_t which = native.which; rv &= native_to_network(&network->which, which);
    uint16_t vend = native.vend; rv &= native_to_network(&network->vend, vend);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Version *native, NetVersion network)
{
    bool rv = true;
    uint8_t major; rv &= network_to_native(&major, network.major); native->major = major;
    uint8_t minor; rv &= network_to_native(&minor, network.minor); native->minor = minor;
    uint8_t patch; rv &= network_to_native(&patch, network.patch); native->patch = patch;
    uint8_t devel; rv &= network_to_native(&devel, network.devel); native->devel = devel;
    uint8_t flags; rv &= network_to_native(&flags, network.flags); native->flags = flags;
    uint8_t which; rv &= network_to_native(&which, network.which); native->which = which;
    uint16_t vend; rv &= network_to_native(&vend, network.vend); native->vend = vend;
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(Byte *network, VERSION_2 native)
{
    bool rv = true;
    uint8_t tmp = static_cast<uint8_t>(native);
    rv &= native_to_network(network, tmp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(VERSION_2 *native, Byte network)
{
    bool rv = true;
    uint8_t tmp;
    rv &= network_to_native(&tmp, network);
    *native = static_cast<VERSION_2>(tmp);
    // TODO this is what really should be doing a checked cast
    return rv;
}
struct Stats6
{
    uint8_t str = {};
    uint8_t agi = {};
    uint8_t vit = {};
    uint8_t int_ = {};
    uint8_t dex = {};
    uint8_t luk = {};
};
struct NetStats6
{
    Byte str;
    Byte agi;
    Byte vit;
    Byte int_;
    Byte dex;
    Byte luk;
};
static_assert(offsetof(NetStats6, str) == 0, "offsetof(NetStats6, str) == 0");
static_assert(offsetof(NetStats6, agi) == 1, "offsetof(NetStats6, agi) == 1");
static_assert(offsetof(NetStats6, vit) == 2, "offsetof(NetStats6, vit) == 2");
static_assert(offsetof(NetStats6, int_) == 3, "offsetof(NetStats6, int_) == 3");
static_assert(offsetof(NetStats6, dex) == 4, "offsetof(NetStats6, dex) == 4");
static_assert(offsetof(NetStats6, luk) == 5, "offsetof(NetStats6, luk) == 5");
static_assert(sizeof(NetStats6) == 6, "sizeof(NetStats6) == 6");
static_assert(alignof(NetStats6) == 1, "alignof(NetStats6) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetStats6 *network, Stats6 native)
{
    bool rv = true;
    rv &= native_to_network(&network->str, native.str);
    rv &= native_to_network(&network->agi, native.agi);
    rv &= native_to_network(&network->vit, native.vit);
    rv &= native_to_network(&network->int_, native.int_);
    rv &= native_to_network(&network->dex, native.dex);
    rv &= native_to_network(&network->luk, native.luk);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Stats6 *native, NetStats6 network)
{
    bool rv = true;
    rv &= network_to_native(&native->str, network.str);
    rv &= network_to_native(&native->agi, network.agi);
    rv &= network_to_native(&native->vit, network.vit);
    rv &= network_to_native(&native->int_, network.int_);
    rv &= network_to_native(&native->dex, network.dex);
    rv &= network_to_native(&native->luk, network.luk);
    return rv;
}

struct CharSelect
{
    CharId char_id = {};
    uint32_t base_exp = {};
    uint32_t zeny = {};
    uint32_t job_exp = {};
    uint32_t job_level = {};
    ItemNameId shoes = {};
    ItemNameId gloves = {};
    ItemNameId cape = {};
    ItemNameId misc1 = {};
    Option option = {};
    uint16_t unused = {};
    uint32_t karma = {};
    uint32_t manner = {};
    uint16_t status_point = {};
    uint16_t hp = {};
    uint16_t max_hp = {};
    uint16_t sp = {};
    uint16_t max_sp = {};
    uint16_t speed = {};
    Species species = {};
    uint16_t hair_style = {};
    uint16_t weapon = {};
    uint16_t base_level = {};
    uint16_t skill_point = {};
    ItemNameId head_bottom = {};
    ItemNameId shield = {};
    ItemNameId head_top = {};
    ItemNameId head_mid = {};
    uint16_t hair_color = {};
    ItemNameId misc2 = {};
    CharName char_name = {};
    Stats6 stats = {};
    uint8_t char_num = {};
    uint8_t unused2 = {};
};
struct NetCharSelect
{
    Little32 char_id;
    Little32 base_exp;
    Little32 zeny;
    Little32 job_exp;
    Little32 job_level;
    Little16 shoes;
    Little16 gloves;
    Little16 cape;
    Little16 misc1;
    Little16 option;
    Little16 unused;
    Little32 karma;
    Little32 manner;
    Little16 status_point;
    Little16 hp;
    Little16 max_hp;
    Little16 sp;
    Little16 max_sp;
    Little16 speed;
    Little16 species;
    Little16 hair_style;
    Little16 weapon;
    Little16 base_level;
    Little16 skill_point;
    Little16 head_bottom;
    Little16 shield;
    Little16 head_top;
    Little16 head_mid;
    Little16 hair_color;
    Little16 misc2;
    NetString<sizeof(CharName)> char_name;
    NetStats6 stats;
    Byte char_num;
    Byte unused2;
};
static_assert(offsetof(NetCharSelect, char_id) == 0, "offsetof(NetCharSelect, char_id) == 0");
static_assert(offsetof(NetCharSelect, base_exp) == 4, "offsetof(NetCharSelect, base_exp) == 4");
static_assert(offsetof(NetCharSelect, zeny) == 8, "offsetof(NetCharSelect, zeny) == 8");
static_assert(offsetof(NetCharSelect, job_exp) == 12, "offsetof(NetCharSelect, job_exp) == 12");
static_assert(offsetof(NetCharSelect, job_level) == 16, "offsetof(NetCharSelect, job_level) == 16");
static_assert(offsetof(NetCharSelect, shoes) == 20, "offsetof(NetCharSelect, shoes) == 20");
static_assert(offsetof(NetCharSelect, gloves) == 22, "offsetof(NetCharSelect, gloves) == 22");
static_assert(offsetof(NetCharSelect, cape) == 24, "offsetof(NetCharSelect, cape) == 24");
static_assert(offsetof(NetCharSelect, misc1) == 26, "offsetof(NetCharSelect, misc1) == 26");
static_assert(offsetof(NetCharSelect, option) == 28, "offsetof(NetCharSelect, option) == 28");
static_assert(offsetof(NetCharSelect, unused) == 30, "offsetof(NetCharSelect, unused) == 30");
static_assert(offsetof(NetCharSelect, karma) == 32, "offsetof(NetCharSelect, karma) == 32");
static_assert(offsetof(NetCharSelect, manner) == 36, "offsetof(NetCharSelect, manner) == 36");
static_assert(offsetof(NetCharSelect, status_point) == 40, "offsetof(NetCharSelect, status_point) == 40");
static_assert(offsetof(NetCharSelect, hp) == 42, "offsetof(NetCharSelect, hp) == 42");
static_assert(offsetof(NetCharSelect, max_hp) == 44, "offsetof(NetCharSelect, max_hp) == 44");
static_assert(offsetof(NetCharSelect, sp) == 46, "offsetof(NetCharSelect, sp) == 46");
static_assert(offsetof(NetCharSelect, max_sp) == 48, "offsetof(NetCharSelect, max_sp) == 48");
static_assert(offsetof(NetCharSelect, speed) == 50, "offsetof(NetCharSelect, speed) == 50");
static_assert(offsetof(NetCharSelect, species) == 52, "offsetof(NetCharSelect, species) == 52");
static_assert(offsetof(NetCharSelect, hair_style) == 54, "offsetof(NetCharSelect, hair_style) == 54");
static_assert(offsetof(NetCharSelect, weapon) == 56, "offsetof(NetCharSelect, weapon) == 56");
static_assert(offsetof(NetCharSelect, base_level) == 58, "offsetof(NetCharSelect, base_level) == 58");
static_assert(offsetof(NetCharSelect, skill_point) == 60, "offsetof(NetCharSelect, skill_point) == 60");
static_assert(offsetof(NetCharSelect, head_bottom) == 62, "offsetof(NetCharSelect, head_bottom) == 62");
static_assert(offsetof(NetCharSelect, shield) == 64, "offsetof(NetCharSelect, shield) == 64");
static_assert(offsetof(NetCharSelect, head_top) == 66, "offsetof(NetCharSelect, head_top) == 66");
static_assert(offsetof(NetCharSelect, head_mid) == 68, "offsetof(NetCharSelect, head_mid) == 68");
static_assert(offsetof(NetCharSelect, hair_color) == 70, "offsetof(NetCharSelect, hair_color) == 70");
static_assert(offsetof(NetCharSelect, misc2) == 72, "offsetof(NetCharSelect, misc2) == 72");
static_assert(offsetof(NetCharSelect, char_name) == 74, "offsetof(NetCharSelect, char_name) == 74");
static_assert(offsetof(NetCharSelect, stats) == 98, "offsetof(NetCharSelect, stats) == 98");
static_assert(offsetof(NetCharSelect, char_num) == 104, "offsetof(NetCharSelect, char_num) == 104");
static_assert(offsetof(NetCharSelect, unused2) == 105, "offsetof(NetCharSelect, unused2) == 105");
static_assert(sizeof(NetCharSelect) == 106, "sizeof(NetCharSelect) == 106");
static_assert(alignof(NetCharSelect) == 1, "alignof(NetCharSelect) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetCharSelect *network, CharSelect native)
{
    bool rv = true;
    rv &= native_to_network(&network->char_id, native.char_id);
    rv &= native_to_network(&network->base_exp, native.base_exp);
    rv &= native_to_network(&network->zeny, native.zeny);
    rv &= native_to_network(&network->job_exp, native.job_exp);
    rv &= native_to_network(&network->job_level, native.job_level);
    rv &= native_to_network(&network->shoes, native.shoes);
    rv &= native_to_network(&network->gloves, native.gloves);
    rv &= native_to_network(&network->cape, native.cape);
    rv &= native_to_network(&network->misc1, native.misc1);
    rv &= native_to_network(&network->option, native.option);
    rv &= native_to_network(&network->unused, native.unused);
    rv &= native_to_network(&network->karma, native.karma);
    rv &= native_to_network(&network->manner, native.manner);
    rv &= native_to_network(&network->status_point, native.status_point);
    rv &= native_to_network(&network->hp, native.hp);
    rv &= native_to_network(&network->max_hp, native.max_hp);
    rv &= native_to_network(&network->sp, native.sp);
    rv &= native_to_network(&network->max_sp, native.max_sp);
    rv &= native_to_network(&network->speed, native.speed);
    rv &= native_to_network(&network->species, native.species);
    rv &= native_to_network(&network->hair_style, native.hair_style);
    rv &= native_to_network(&network->weapon, native.weapon);
    rv &= native_to_network(&network->base_level, native.base_level);
    rv &= native_to_network(&network->skill_point, native.skill_point);
    rv &= native_to_network(&network->head_bottom, native.head_bottom);
    rv &= native_to_network(&network->shield, native.shield);
    rv &= native_to_network(&network->head_top, native.head_top);
    rv &= native_to_network(&network->head_mid, native.head_mid);
    rv &= native_to_network(&network->hair_color, native.hair_color);
    rv &= native_to_network(&network->misc2, native.misc2);
    rv &= native_to_network(&network->char_name, native.char_name);
    rv &= native_to_network(&network->stats, native.stats);
    rv &= native_to_network(&network->char_num, native.char_num);
    rv &= native_to_network(&network->unused2, native.unused2);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(CharSelect *native, NetCharSelect network)
{
    bool rv = true;
    rv &= network_to_native(&native->char_id, network.char_id);
    rv &= network_to_native(&native->base_exp, network.base_exp);
    rv &= network_to_native(&native->zeny, network.zeny);
    rv &= network_to_native(&native->job_exp, network.job_exp);
    rv &= network_to_native(&native->job_level, network.job_level);
    rv &= network_to_native(&native->shoes, network.shoes);
    rv &= network_to_native(&native->gloves, network.gloves);
    rv &= network_to_native(&native->cape, network.cape);
    rv &= network_to_native(&native->misc1, network.misc1);
    rv &= network_to_native(&native->option, network.option);
    rv &= network_to_native(&native->unused, network.unused);
    rv &= network_to_native(&native->karma, network.karma);
    rv &= network_to_native(&native->manner, network.manner);
    rv &= network_to_native(&native->status_point, network.status_point);
    rv &= network_to_native(&native->hp, network.hp);
    rv &= network_to_native(&native->max_hp, network.max_hp);
    rv &= network_to_native(&native->sp, network.sp);
    rv &= network_to_native(&native->max_sp, network.max_sp);
    rv &= network_to_native(&native->speed, network.speed);
    rv &= network_to_native(&native->species, network.species);
    rv &= network_to_native(&native->hair_style, network.hair_style);
    rv &= network_to_native(&native->weapon, network.weapon);
    rv &= network_to_native(&native->base_level, network.base_level);
    rv &= network_to_native(&native->skill_point, network.skill_point);
    rv &= network_to_native(&native->head_bottom, network.head_bottom);
    rv &= network_to_native(&native->shield, network.shield);
    rv &= network_to_native(&native->head_top, network.head_top);
    rv &= network_to_native(&native->head_mid, network.head_mid);
    rv &= network_to_native(&native->hair_color, network.hair_color);
    rv &= network_to_native(&native->misc2, network.misc2);
    rv &= network_to_native(&native->char_name, network.char_name);
    rv &= network_to_native(&native->stats, network.stats);
    rv &= network_to_native(&native->char_num, network.char_num);
    rv &= network_to_native(&native->unused2, network.unused2);
    return rv;
}

struct SkillInfo
{
    SkillID skill_id = {};
    uint16_t type_or_inf = {};
    SkillFlags flags = {};
    uint16_t level = {};
    uint16_t sp = {};
    uint16_t range = {};
    VString<23> unused = {};
    uint8_t can_raise = {};
};
struct NetSkillInfo
{
    Little16 skill_id;
    Little16 type_or_inf;
    Little16 flags;
    Little16 level;
    Little16 sp;
    Little16 range;
    NetString<sizeof(VString<23>)> unused;
    Byte can_raise;
};
static_assert(offsetof(NetSkillInfo, skill_id) == 0, "offsetof(NetSkillInfo, skill_id) == 0");
static_assert(offsetof(NetSkillInfo, type_or_inf) == 2, "offsetof(NetSkillInfo, type_or_inf) == 2");
static_assert(offsetof(NetSkillInfo, flags) == 4, "offsetof(NetSkillInfo, flags) == 4");
static_assert(offsetof(NetSkillInfo, level) == 6, "offsetof(NetSkillInfo, level) == 6");
static_assert(offsetof(NetSkillInfo, sp) == 8, "offsetof(NetSkillInfo, sp) == 8");
static_assert(offsetof(NetSkillInfo, range) == 10, "offsetof(NetSkillInfo, range) == 10");
static_assert(offsetof(NetSkillInfo, unused) == 12, "offsetof(NetSkillInfo, unused) == 12");
static_assert(offsetof(NetSkillInfo, can_raise) == 36, "offsetof(NetSkillInfo, can_raise) == 36");
static_assert(sizeof(NetSkillInfo) == 37, "sizeof(NetSkillInfo) == 37");
static_assert(alignof(NetSkillInfo) == 1, "alignof(NetSkillInfo) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSkillInfo *network, SkillInfo native)
{
    bool rv = true;
    rv &= native_to_network(&network->skill_id, native.skill_id);
    rv &= native_to_network(&network->type_or_inf, native.type_or_inf);
    rv &= native_to_network(&network->flags, native.flags);
    rv &= native_to_network(&network->level, native.level);
    rv &= native_to_network(&network->sp, native.sp);
    rv &= native_to_network(&network->range, native.range);
    rv &= native_to_network(&network->unused, native.unused);
    rv &= native_to_network(&network->can_raise, native.can_raise);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SkillInfo *native, NetSkillInfo network)
{
    bool rv = true;
    rv &= network_to_native(&native->skill_id, network.skill_id);
    rv &= network_to_native(&native->type_or_inf, network.type_or_inf);
    rv &= network_to_native(&native->flags, network.flags);
    rv &= network_to_native(&native->level, network.level);
    rv &= network_to_native(&native->sp, network.sp);
    rv &= network_to_native(&native->range, network.range);
    rv &= network_to_native(&native->unused, network.unused);
    rv &= network_to_native(&native->can_raise, network.can_raise);
    return rv;
}

struct Item
{
    ItemNameId nameid = {};
    int16_t amount = {};
    EPOS equip = {};
};
struct NetItem
{
    Little16 nameid;
    Little16 amount;
    Little16 equip;
};
static_assert(alignof(NetItem) == 1, "alignof(NetItem) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetItem *network, Item native)
{
    bool rv = true;
    rv &= native_to_network(&network->nameid, native.nameid);
    rv &= native_to_network(&network->amount, native.amount);
    rv &= native_to_network(&network->equip, native.equip);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Item *native, NetItem network)
{
    bool rv = true;
    rv &= network_to_native(&native->nameid, network.nameid);
    rv &= network_to_native(&native->amount, network.amount);
    rv &= network_to_native(&native->equip, network.equip);
    return rv;
}

struct Point
{
    MapName map_ = {};
    int16_t x = {};
    int16_t y = {};
    Point() = default;
    Point(MapName _map_, int16_t _x, int16_t _y) : map_(_map_), x(_x), y(_y) {}
};
struct NetPoint
{
    NetString<sizeof(MapName)> map_;
    Little16 x;
    Little16 y;
};
static_assert(alignof(NetPoint) == 1, "alignof(NetPoint) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetPoint *network, Point native)
{
    bool rv = true;
    rv &= native_to_network(&network->map_, native.map_);
    rv &= native_to_network(&network->x, native.x);
    rv &= native_to_network(&network->y, native.y);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Point *native, NetPoint network)
{
    bool rv = true;
    rv &= network_to_native(&native->map_, network.map_);
    rv &= network_to_native(&native->x, network.x);
    rv &= network_to_native(&native->y, network.y);
    return rv;
}

struct SkillValue
{
    uint16_t lv = {};
    SkillFlags flags = {};
};
struct NetSkillValue
{
    Little16 lv;
    Little16 flags;
};
static_assert(alignof(NetSkillValue) == 1, "alignof(NetSkillValue) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSkillValue *network, SkillValue native)
{
    bool rv = true;
    rv &= native_to_network(&network->lv, native.lv);
    rv &= native_to_network(&network->flags, native.flags);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SkillValue *native, NetSkillValue network)
{
    bool rv = true;
    rv &= network_to_native(&native->lv, network.lv);
    rv &= network_to_native(&native->flags, network.flags);
    return rv;
}

struct GlobalReg
{
    VarName str = {};
    int32_t value = {};
};
struct NetGlobalReg
{
    NetString<sizeof(VarName)> str;
    Little32 value;
};
static_assert(alignof(NetGlobalReg) == 1, "alignof(NetGlobalReg) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetGlobalReg *network, GlobalReg native)
{
    bool rv = true;
    rv &= native_to_network(&network->str, native.str);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(GlobalReg *native, NetGlobalReg network)
{
    bool rv = true;
    rv &= network_to_native(&native->str, network.str);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

struct CharKey
{
    CharName name = {};
    AccountId account_id = {};
    CharId char_id = {};
    uint8_t char_num = {};
};
struct NetCharKey
{
    NetString<sizeof(CharName)> name;
    Little32 account_id;
    Little32 char_id;
    Byte char_num;
};
static_assert(alignof(NetCharKey) == 1, "alignof(NetCharKey) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetCharKey *network, CharKey native)
{
    bool rv = true;
    rv &= native_to_network(&network->name, native.name);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->char_id, native.char_id);
    rv &= native_to_network(&network->char_num, native.char_num);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(CharKey *native, NetCharKey network)
{
    bool rv = true;
    rv &= network_to_native(&native->name, network.name);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->char_id, network.char_id);
    rv &= network_to_native(&native->char_num, network.char_num);
    return rv;
}

struct CharData
{
    CharId partner_id = {};
    int32_t base_exp = {};
    int32_t job_exp = {};
    int32_t zeny = {};
    Species species = {};
    int16_t status_point = {};
    int16_t skill_point = {};
    int32_t hp = {};
    int32_t max_hp = {};
    int32_t sp = {};
    int32_t max_sp = {};
    Option option = {};
    int16_t karma = {};
    int16_t manner = {};
    int16_t hair = {};
    int16_t hair_color = {};
    int16_t clothes_color = {};
    PartyId party_id = {};
    ItemLook weapon = {};
    ItemNameId shield = {};
    ItemNameId head_top = {};
    ItemNameId head_mid = {};
    ItemNameId head_bottom = {};
    uint8_t base_level = {};
    uint8_t job_level = {};
    earray<int16_t, ATTR, ATTR::COUNT> attrs = {};
    SEX sex = {};
    IP4Address mapip = {};
    uint16_t mapport = {};
    Point last_point = {};
    Point save_point = {};
    GenericArray<Item, InventoryIndexing<IOff0, MAX_INVENTORY>> inventory = {};
    earray<SkillValue, SkillID, MAX_SKILL> skill = {};
    int32_t global_reg_num = {};
    Array<GlobalReg, GLOBAL_REG_NUM> global_reg = {};
    int32_t account_reg_num = {};
    Array<GlobalReg, ACCOUNT_REG_NUM> account_reg = {};
    int32_t account_reg2_num = {};
    Array<GlobalReg, ACCOUNT_REG2_NUM> account_reg2 = {};
};
struct NetCharData
{
    Little32 partner_id;
    Little32 base_exp;
    Little32 job_exp;
    Little32 zeny;
    Little16 species;
    Little16 status_point;
    Little16 skill_point;
    Little32 hp;
    Little32 max_hp;
    Little32 sp;
    Little32 max_sp;
    Little16 option;
    Little16 karma;
    Little16 manner;
    Little16 hair;
    Little16 hair_color;
    Little16 clothes_color;
    Little32 party_id;
    Little16 weapon;
    Little16 shield;
    Little16 head_top;
    Little16 head_mid;
    Little16 head_bottom;
    Byte base_level;
    Byte job_level;
    NetArray<Little16, static_cast<size_t>(ATTR::COUNT)> attrs;
    Byte sex;
    IP4Address mapip;
    Little16 mapport;
    NetPoint last_point;
    NetPoint save_point;
    NetArray<NetItem, MAX_INVENTORY> inventory;
    NetArray<NetSkillValue, static_cast<size_t>(MAX_SKILL)> skill;
    Little32 global_reg_num;
    NetArray<NetGlobalReg, GLOBAL_REG_NUM> global_reg;
    Little32 account_reg_num;
    NetArray<NetGlobalReg, ACCOUNT_REG_NUM> account_reg;
    Little32 account_reg2_num;
    NetArray<NetGlobalReg, ACCOUNT_REG2_NUM> account_reg2;
};
static_assert(alignof(NetCharData) == 1, "alignof(NetCharData) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetCharData *network, CharData native)
{
    bool rv = true;
    rv &= native_to_network(&network->partner_id, native.partner_id);
    rv &= native_to_network(&network->base_exp, native.base_exp);
    rv &= native_to_network(&network->job_exp, native.job_exp);
    rv &= native_to_network(&network->zeny, native.zeny);
    rv &= native_to_network(&network->species, native.species);
    rv &= native_to_network(&network->status_point, native.status_point);
    rv &= native_to_network(&network->skill_point, native.skill_point);
    rv &= native_to_network(&network->hp, native.hp);
    rv &= native_to_network(&network->max_hp, native.max_hp);
    rv &= native_to_network(&network->sp, native.sp);
    rv &= native_to_network(&network->max_sp, native.max_sp);
    rv &= native_to_network(&network->option, native.option);
    rv &= native_to_network(&network->karma, native.karma);
    rv &= native_to_network(&network->manner, native.manner);
    rv &= native_to_network(&network->hair, native.hair);
    rv &= native_to_network(&network->hair_color, native.hair_color);
    rv &= native_to_network(&network->clothes_color, native.clothes_color);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->weapon, native.weapon);
    rv &= native_to_network(&network->shield, native.shield);
    rv &= native_to_network(&network->head_top, native.head_top);
    rv &= native_to_network(&network->head_mid, native.head_mid);
    rv &= native_to_network(&network->head_bottom, native.head_bottom);
    rv &= native_to_network(&network->base_level, native.base_level);
    rv &= native_to_network(&network->job_level, native.job_level);
    rv &= native_to_network(&network->attrs, native.attrs);
    rv &= native_to_network(&network->sex, native.sex);
    rv &= native_to_network(&network->mapip, native.mapip);
    rv &= native_to_network(&network->mapport, native.mapport);
    rv &= native_to_network(&network->last_point, native.last_point);
    rv &= native_to_network(&network->save_point, native.save_point);
    rv &= native_to_network(&network->inventory, native.inventory);
    rv &= native_to_network(&network->skill, native.skill);
    rv &= native_to_network(&network->global_reg_num, native.global_reg_num);
    rv &= native_to_network(&network->global_reg, native.global_reg);
    rv &= native_to_network(&network->account_reg_num, native.account_reg_num);
    rv &= native_to_network(&network->account_reg, native.account_reg);
    rv &= native_to_network(&network->account_reg2_num, native.account_reg2_num);
    rv &= native_to_network(&network->account_reg2, native.account_reg2);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(CharData *native, NetCharData network)
{
    bool rv = true;
    rv &= network_to_native(&native->partner_id, network.partner_id);
    rv &= network_to_native(&native->base_exp, network.base_exp);
    rv &= network_to_native(&native->job_exp, network.job_exp);
    rv &= network_to_native(&native->zeny, network.zeny);
    rv &= network_to_native(&native->species, network.species);
    rv &= network_to_native(&native->status_point, network.status_point);
    rv &= network_to_native(&native->skill_point, network.skill_point);
    rv &= network_to_native(&native->hp, network.hp);
    rv &= network_to_native(&native->max_hp, network.max_hp);
    rv &= network_to_native(&native->sp, network.sp);
    rv &= network_to_native(&native->max_sp, network.max_sp);
    rv &= network_to_native(&native->option, network.option);
    rv &= network_to_native(&native->karma, network.karma);
    rv &= network_to_native(&native->manner, network.manner);
    rv &= network_to_native(&native->hair, network.hair);
    rv &= network_to_native(&native->hair_color, network.hair_color);
    rv &= network_to_native(&native->clothes_color, network.clothes_color);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->weapon, network.weapon);
    rv &= network_to_native(&native->shield, network.shield);
    rv &= network_to_native(&native->head_top, network.head_top);
    rv &= network_to_native(&native->head_mid, network.head_mid);
    rv &= network_to_native(&native->head_bottom, network.head_bottom);
    rv &= network_to_native(&native->base_level, network.base_level);
    rv &= network_to_native(&native->job_level, network.job_level);
    rv &= network_to_native(&native->attrs, network.attrs);
    rv &= network_to_native(&native->sex, network.sex);
    rv &= network_to_native(&native->mapip, network.mapip);
    rv &= network_to_native(&native->mapport, network.mapport);
    rv &= network_to_native(&native->last_point, network.last_point);
    rv &= network_to_native(&native->save_point, network.save_point);
    rv &= network_to_native(&native->inventory, network.inventory);
    rv &= network_to_native(&native->skill, network.skill);
    rv &= network_to_native(&native->global_reg_num, network.global_reg_num);
    rv &= network_to_native(&native->global_reg, network.global_reg);
    rv &= network_to_native(&native->account_reg_num, network.account_reg_num);
    rv &= network_to_native(&native->account_reg, network.account_reg);
    rv &= network_to_native(&native->account_reg2_num, network.account_reg2_num);
    rv &= network_to_native(&native->account_reg2, network.account_reg2);
    return rv;
}

struct NetPartyMember
{
    Little32 account_id;
    NetString<sizeof(CharName)> name;
    NetString<sizeof(MapName)> map;
    Little32 leader;
    Little32 online;
    Little32 lv;
};
inline __attribute__((warn_unused_result))
bool native_to_network(NetPartyMember *network, PartyMember native)
{
    bool rv = true;
    AccountId account_id = native.account_id; rv &= native_to_network(&network->account_id, account_id);
    CharName name = native.name; rv &= native_to_network(&network->name, name);
    MapName map = native.map; rv &= native_to_network(&network->map, map);
    int32_t leader = native.leader; rv &= native_to_network(&network->leader, leader);
    int32_t online = native.online; rv &= native_to_network(&network->online, online);
    int32_t lv = native.lv; rv &= native_to_network(&network->lv, lv);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(PartyMember *native, NetPartyMember network)
{
    bool rv = true;
    AccountId account_id; rv &= network_to_native(&account_id, network.account_id); native->account_id = account_id;
    CharName name; rv &= network_to_native(&name, network.name); native->name = name;
    MapName map; rv &= network_to_native(&map, network.map); native->map = map;
    int32_t leader; rv &= network_to_native(&leader, network.leader); native->leader = leader;
    int32_t online; rv &= network_to_native(&online, network.online); native->online = online;
    int32_t lv; rv &= network_to_native(&lv, network.lv); native->lv = lv;
    return rv;
}

struct PartyMost
{
    PartyName name = {};
    int32_t exp = {};
    int32_t item = {};
    Array<PartyMember, MAX_PARTY> member = {};
};
struct NetPartyMost
{
    NetString<sizeof(PartyName)> name;
    Little32 exp;
    Little32 item;
    NetArray<NetPartyMember, MAX_PARTY> member;
};
static_assert(alignof(NetPartyMost) == 1, "alignof(NetPartyMost) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetPartyMost *network, PartyMost native)
{
    bool rv = true;
    rv &= native_to_network(&network->name, native.name);
    rv &= native_to_network(&network->exp, native.exp);
    rv &= native_to_network(&network->item, native.item);
    rv &= native_to_network(&network->member, native.member);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(PartyMost *native, NetPartyMost network)
{
    bool rv = true;
    rv &= network_to_native(&native->name, network.name);
    rv &= network_to_native(&native->exp, network.exp);
    rv &= network_to_native(&native->item, network.item);
    rv &= network_to_native(&native->member, network.member);
    return rv;
}

struct Storage
{
    bool dirty = {};
    AccountId account_id = {};
    int16_t storage_status = {};
    int16_t storage_amount = {};
    GenericArray<Item, InventoryIndexing<SOff0, MAX_STORAGE>> storage_ = {};
};
struct NetStorage
{
    bool dirty;
    Little32 account_id;
    Little16 storage_status;
    Little16 storage_amount;
    NetArray<NetItem, MAX_STORAGE> storage_;
};
static_assert(alignof(NetStorage) == 1, "alignof(NetStorage) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetStorage *network, Storage native)
{
    bool rv = true;
    rv &= native_to_network(&network->dirty, native.dirty);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->storage_status, native.storage_status);
    rv &= native_to_network(&network->storage_amount, native.storage_amount);
    rv &= native_to_network(&network->storage_, native.storage_);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Storage *native, NetStorage network)
{
    bool rv = true;
    rv &= network_to_native(&native->dirty, network.dirty);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->storage_status, network.storage_status);
    rv &= network_to_native(&native->storage_amount, network.storage_amount);
    rv &= network_to_native(&native->storage_, network.storage_);
    return rv;
}

} // namespace tmwa
