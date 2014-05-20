#ifndef TMWA_PROTO2_TYPES_HPP
#define TMWA_PROTO2_TYPES_HPP
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

# include "fwd.hpp"

//TODO split the includes
# include <cstdint>
# include "../ints/little.hpp"
# include "../strings/vstring.hpp"
# include "../net/ip.hpp"
# include "../mmo/enums.hpp"
# include "../mmo/human_time_diff.hpp"
# include "../mmo/ids.hpp"
# include "../mmo/strs.hpp"
# include "../mmo/utils.hpp"
# include "../mmo/version.hpp"
# include "../login/types.hpp"
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
    uint16_t year = native.year; rv &= native_to_network(&network->year, year);
    uint16_t month = native.month; rv &= native_to_network(&network->month, month);
    uint16_t day = native.day; rv &= native_to_network(&network->day, day);
    uint16_t hour = native.hour; rv &= native_to_network(&network->hour, hour);
    uint16_t minute = native.minute; rv &= native_to_network(&network->minute, minute);
    uint16_t second = native.second; rv &= native_to_network(&network->second, second);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(HumanTimeDiff *native, NetHumanTimeDiff network)
{
    bool rv = true;
    uint16_t year; rv &= network_to_native(&year, network.year); native->year = year;
    uint16_t month; rv &= network_to_native(&month, network.month); native->month = month;
    uint16_t day; rv &= network_to_native(&day, network.day); native->day = day;
    uint16_t hour; rv &= network_to_native(&hour, network.hour); native->hour = hour;
    uint16_t minute; rv &= network_to_native(&minute, network.minute); native->minute = minute;
    uint16_t second; rv &= network_to_native(&second, network.second); native->second = second;
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
#endif // TMWA_PROTO2_TYPES_HPP
