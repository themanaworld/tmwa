#ifndef TMWA_PROTO2_LOGIN_CHAR_HPP
#define TMWA_PROTO2_LOGIN_CHAR_HPP
//    login-char.hpp - TMWA network protocol: login/char
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

// This is a generated file, edit tools/protocol.py instead

# include "fwd.hpp"

# include "types.hpp"

// This is an internal protocol, and can be changed without notice

struct RPacket0x2709_Fixed
{
    uint16_t packet_id;
};
struct NetRPacket0x2709_Fixed
{
    Little16 packet_id;
};
static_assert(offsetof(NetRPacket0x2709_Fixed, packet_id) == 0, "offsetof(NetRPacket0x2709_Fixed, packet_id) == 0");
static_assert(sizeof(NetRPacket0x2709_Fixed) == 2, "sizeof(NetRPacket0x2709_Fixed) == 2");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2709_Fixed *network, RPacket0x2709_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2709_Fixed *native, NetRPacket0x2709_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    return rv;
}

struct RPacket0x2712_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
    uint32_t login_id1;
    uint32_t login_id2;
    SEX sex;
    IP4Address ip;
};
struct NetRPacket0x2712_Fixed
{
    Little16 packet_id;
    Little32 account_id;
    Little32 login_id1;
    Little32 login_id2;
    Byte sex;
    IP4Address ip;
};
static_assert(offsetof(NetRPacket0x2712_Fixed, packet_id) == 0, "offsetof(NetRPacket0x2712_Fixed, packet_id) == 0");
static_assert(offsetof(NetRPacket0x2712_Fixed, account_id) == 2, "offsetof(NetRPacket0x2712_Fixed, account_id) == 2");
static_assert(offsetof(NetRPacket0x2712_Fixed, login_id1) == 6, "offsetof(NetRPacket0x2712_Fixed, login_id1) == 6");
static_assert(offsetof(NetRPacket0x2712_Fixed, login_id2) == 10, "offsetof(NetRPacket0x2712_Fixed, login_id2) == 10");
static_assert(offsetof(NetRPacket0x2712_Fixed, sex) == 14, "offsetof(NetRPacket0x2712_Fixed, sex) == 14");
static_assert(offsetof(NetRPacket0x2712_Fixed, ip) == 15, "offsetof(NetRPacket0x2712_Fixed, ip) == 15");
static_assert(sizeof(NetRPacket0x2712_Fixed) == 19, "sizeof(NetRPacket0x2712_Fixed) == 19");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2712_Fixed *network, RPacket0x2712_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->login_id1, native.login_id1);
    rv &= native_to_network(&network->login_id2, native.login_id2);
    rv &= native_to_network(&network->sex, native.sex);
    rv &= native_to_network(&network->ip, native.ip);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2712_Fixed *native, NetRPacket0x2712_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->login_id1, network.login_id1);
    rv &= network_to_native(&native->login_id2, network.login_id2);
    rv &= network_to_native(&native->sex, network.sex);
    rv &= network_to_native(&native->ip, network.ip);
    return rv;
}

struct SPacket0x2713_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
    uint8_t invalid;
    AccountEmail email;
    TimeT connect_until;
};
struct NetSPacket0x2713_Fixed
{
    Little16 packet_id;
    Little32 account_id;
    Byte invalid;
    NetString<sizeof(AccountEmail)> email;
    Little32 connect_until;
};
static_assert(offsetof(NetSPacket0x2713_Fixed, packet_id) == 0, "offsetof(NetSPacket0x2713_Fixed, packet_id) == 0");
static_assert(offsetof(NetSPacket0x2713_Fixed, account_id) == 2, "offsetof(NetSPacket0x2713_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket0x2713_Fixed, invalid) == 6, "offsetof(NetSPacket0x2713_Fixed, invalid) == 6");
static_assert(offsetof(NetSPacket0x2713_Fixed, email) == 7, "offsetof(NetSPacket0x2713_Fixed, email) == 7");
static_assert(offsetof(NetSPacket0x2713_Fixed, connect_until) == 47, "offsetof(NetSPacket0x2713_Fixed, connect_until) == 47");
static_assert(sizeof(NetSPacket0x2713_Fixed) == 51, "sizeof(NetSPacket0x2713_Fixed) == 51");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x2713_Fixed *network, SPacket0x2713_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->invalid, native.invalid);
    rv &= native_to_network(&network->email, native.email);
    rv &= native_to_network(&network->connect_until, native.connect_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x2713_Fixed *native, NetSPacket0x2713_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->invalid, network.invalid);
    rv &= network_to_native(&native->email, network.email);
    rv &= network_to_native(&native->connect_until, network.connect_until);
    return rv;
}

struct RPacket0x2714_Fixed
{
    uint16_t packet_id;
    uint32_t users;
};
struct NetRPacket0x2714_Fixed
{
    Little16 packet_id;
    Little32 users;
};
static_assert(offsetof(NetRPacket0x2714_Fixed, packet_id) == 0, "offsetof(NetRPacket0x2714_Fixed, packet_id) == 0");
static_assert(offsetof(NetRPacket0x2714_Fixed, users) == 2, "offsetof(NetRPacket0x2714_Fixed, users) == 2");
static_assert(sizeof(NetRPacket0x2714_Fixed) == 6, "sizeof(NetRPacket0x2714_Fixed) == 6");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2714_Fixed *network, RPacket0x2714_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->users, native.users);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2714_Fixed *native, NetRPacket0x2714_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->users, network.users);
    return rv;
}

struct RPacket0x2715_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
    AccountEmail email;
};
struct NetRPacket0x2715_Fixed
{
    Little16 packet_id;
    Little32 account_id;
    NetString<sizeof(AccountEmail)> email;
};
static_assert(offsetof(NetRPacket0x2715_Fixed, packet_id) == 0, "offsetof(NetRPacket0x2715_Fixed, packet_id) == 0");
static_assert(offsetof(NetRPacket0x2715_Fixed, account_id) == 2, "offsetof(NetRPacket0x2715_Fixed, account_id) == 2");
static_assert(offsetof(NetRPacket0x2715_Fixed, email) == 6, "offsetof(NetRPacket0x2715_Fixed, email) == 6");
static_assert(sizeof(NetRPacket0x2715_Fixed) == 46, "sizeof(NetRPacket0x2715_Fixed) == 46");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2715_Fixed *network, RPacket0x2715_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->email, native.email);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2715_Fixed *native, NetRPacket0x2715_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->email, network.email);
    return rv;
}

struct RPacket0x2716_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
};
struct NetRPacket0x2716_Fixed
{
    Little16 packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetRPacket0x2716_Fixed, packet_id) == 0, "offsetof(NetRPacket0x2716_Fixed, packet_id) == 0");
static_assert(offsetof(NetRPacket0x2716_Fixed, account_id) == 2, "offsetof(NetRPacket0x2716_Fixed, account_id) == 2");
static_assert(sizeof(NetRPacket0x2716_Fixed) == 6, "sizeof(NetRPacket0x2716_Fixed) == 6");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2716_Fixed *network, RPacket0x2716_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2716_Fixed *native, NetRPacket0x2716_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

struct SPacket0x2717_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
    AccountEmail email;
    TimeT connect_until;
};
struct NetSPacket0x2717_Fixed
{
    Little16 packet_id;
    Little32 account_id;
    NetString<sizeof(AccountEmail)> email;
    Little32 connect_until;
};
static_assert(offsetof(NetSPacket0x2717_Fixed, packet_id) == 0, "offsetof(NetSPacket0x2717_Fixed, packet_id) == 0");
static_assert(offsetof(NetSPacket0x2717_Fixed, account_id) == 2, "offsetof(NetSPacket0x2717_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket0x2717_Fixed, email) == 6, "offsetof(NetSPacket0x2717_Fixed, email) == 6");
static_assert(offsetof(NetSPacket0x2717_Fixed, connect_until) == 46, "offsetof(NetSPacket0x2717_Fixed, connect_until) == 46");
static_assert(sizeof(NetSPacket0x2717_Fixed) == 50, "sizeof(NetSPacket0x2717_Fixed) == 50");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x2717_Fixed *network, SPacket0x2717_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->email, native.email);
    rv &= native_to_network(&network->connect_until, native.connect_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x2717_Fixed *native, NetSPacket0x2717_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->email, network.email);
    rv &= network_to_native(&native->connect_until, network.connect_until);
    return rv;
}

struct RPacket0x2720_Head
{
    uint16_t packet_id;
    uint16_t packet_length;
    uint32_t account_id;
};
struct NetRPacket0x2720_Head
{
    Little16 packet_id;
    Little16 packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetRPacket0x2720_Head, packet_id) == 0, "offsetof(NetRPacket0x2720_Head, packet_id) == 0");
static_assert(offsetof(NetRPacket0x2720_Head, packet_length) == 2, "offsetof(NetRPacket0x2720_Head, packet_length) == 2");
static_assert(offsetof(NetRPacket0x2720_Head, account_id) == 4, "offsetof(NetRPacket0x2720_Head, account_id) == 4");
static_assert(sizeof(NetRPacket0x2720_Head) == 8, "sizeof(NetRPacket0x2720_Head) == 8");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2720_Head *network, RPacket0x2720_Head native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->packet_length, native.packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2720_Head *native, NetRPacket0x2720_Head network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->packet_length, network.packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

struct RPacket0x2720_Repeat
{
    uint8_t c;
};
struct NetRPacket0x2720_Repeat
{
    Byte c;
};
static_assert(offsetof(NetRPacket0x2720_Repeat, c) == 0, "offsetof(NetRPacket0x2720_Repeat, c) == 0");
static_assert(sizeof(NetRPacket0x2720_Repeat) == 1, "sizeof(NetRPacket0x2720_Repeat) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2720_Repeat *network, RPacket0x2720_Repeat native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2720_Repeat *native, NetRPacket0x2720_Repeat network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

struct SPacket0x2721_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
    GmLevel gm_level;
};
struct NetSPacket0x2721_Fixed
{
    Little16 packet_id;
    Little32 account_id;
    Little32 gm_level;
};
static_assert(offsetof(NetSPacket0x2721_Fixed, packet_id) == 0, "offsetof(NetSPacket0x2721_Fixed, packet_id) == 0");
static_assert(offsetof(NetSPacket0x2721_Fixed, account_id) == 2, "offsetof(NetSPacket0x2721_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket0x2721_Fixed, gm_level) == 6, "offsetof(NetSPacket0x2721_Fixed, gm_level) == 6");
static_assert(sizeof(NetSPacket0x2721_Fixed) == 10, "sizeof(NetSPacket0x2721_Fixed) == 10");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x2721_Fixed *network, SPacket0x2721_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->gm_level, native.gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x2721_Fixed *native, NetSPacket0x2721_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->gm_level, network.gm_level);
    return rv;
}

struct RPacket0x2722_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
    AccountEmail old_email;
    AccountEmail new_email;
};
struct NetRPacket0x2722_Fixed
{
    Little16 packet_id;
    Little32 account_id;
    NetString<sizeof(AccountEmail)> old_email;
    NetString<sizeof(AccountEmail)> new_email;
};
static_assert(offsetof(NetRPacket0x2722_Fixed, packet_id) == 0, "offsetof(NetRPacket0x2722_Fixed, packet_id) == 0");
static_assert(offsetof(NetRPacket0x2722_Fixed, account_id) == 2, "offsetof(NetRPacket0x2722_Fixed, account_id) == 2");
static_assert(offsetof(NetRPacket0x2722_Fixed, old_email) == 6, "offsetof(NetRPacket0x2722_Fixed, old_email) == 6");
static_assert(offsetof(NetRPacket0x2722_Fixed, new_email) == 46, "offsetof(NetRPacket0x2722_Fixed, new_email) == 46");
static_assert(sizeof(NetRPacket0x2722_Fixed) == 86, "sizeof(NetRPacket0x2722_Fixed) == 86");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2722_Fixed *network, RPacket0x2722_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->old_email, native.old_email);
    rv &= native_to_network(&network->new_email, native.new_email);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2722_Fixed *native, NetRPacket0x2722_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->old_email, network.old_email);
    rv &= network_to_native(&native->new_email, network.new_email);
    return rv;
}

struct SPacket0x2723_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
    SEX sex;
};
struct NetSPacket0x2723_Fixed
{
    Little16 packet_id;
    Little32 account_id;
    Byte sex;
};
static_assert(offsetof(NetSPacket0x2723_Fixed, packet_id) == 0, "offsetof(NetSPacket0x2723_Fixed, packet_id) == 0");
static_assert(offsetof(NetSPacket0x2723_Fixed, account_id) == 2, "offsetof(NetSPacket0x2723_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket0x2723_Fixed, sex) == 6, "offsetof(NetSPacket0x2723_Fixed, sex) == 6");
static_assert(sizeof(NetSPacket0x2723_Fixed) == 7, "sizeof(NetSPacket0x2723_Fixed) == 7");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x2723_Fixed *network, SPacket0x2723_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->sex, native.sex);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x2723_Fixed *native, NetSPacket0x2723_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->sex, network.sex);
    return rv;
}

struct RPacket0x2724_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
    uint32_t status;
};
struct NetRPacket0x2724_Fixed
{
    Little16 packet_id;
    Little32 account_id;
    Little32 status;
};
static_assert(offsetof(NetRPacket0x2724_Fixed, packet_id) == 0, "offsetof(NetRPacket0x2724_Fixed, packet_id) == 0");
static_assert(offsetof(NetRPacket0x2724_Fixed, account_id) == 2, "offsetof(NetRPacket0x2724_Fixed, account_id) == 2");
static_assert(offsetof(NetRPacket0x2724_Fixed, status) == 6, "offsetof(NetRPacket0x2724_Fixed, status) == 6");
static_assert(sizeof(NetRPacket0x2724_Fixed) == 10, "sizeof(NetRPacket0x2724_Fixed) == 10");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2724_Fixed *network, RPacket0x2724_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->status, native.status);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2724_Fixed *native, NetRPacket0x2724_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->status, network.status);
    return rv;
}

struct RPacket0x2725_Head
{
    uint16_t packet_id;
    uint32_t account_id;
    HumanTimeDiff deltas;
};
struct NetRPacket0x2725_Head
{
    Little16 packet_id;
    Little32 account_id;
    NetHumanTimeDiff deltas;
};
static_assert(offsetof(NetRPacket0x2725_Head, packet_id) == 0, "offsetof(NetRPacket0x2725_Head, packet_id) == 0");
static_assert(offsetof(NetRPacket0x2725_Head, account_id) == 2, "offsetof(NetRPacket0x2725_Head, account_id) == 2");
static_assert(offsetof(NetRPacket0x2725_Head, deltas) == 6, "offsetof(NetRPacket0x2725_Head, deltas) == 6");
static_assert(sizeof(NetRPacket0x2725_Head) == 18, "sizeof(NetRPacket0x2725_Head) == 18");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2725_Head *network, RPacket0x2725_Head native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->deltas, native.deltas);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2725_Head *native, NetRPacket0x2725_Head network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->deltas, network.deltas);
    return rv;
}

struct RPacket0x2725_Repeat
{
    uint8_t c;
};
struct NetRPacket0x2725_Repeat
{
    Byte c;
};
static_assert(offsetof(NetRPacket0x2725_Repeat, c) == 0, "offsetof(NetRPacket0x2725_Repeat, c) == 0");
static_assert(sizeof(NetRPacket0x2725_Repeat) == 1, "sizeof(NetRPacket0x2725_Repeat) == 1");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2725_Repeat *network, RPacket0x2725_Repeat native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2725_Repeat *native, NetRPacket0x2725_Repeat network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

struct RPacket0x2727_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
};
struct NetRPacket0x2727_Fixed
{
    Little16 packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetRPacket0x2727_Fixed, packet_id) == 0, "offsetof(NetRPacket0x2727_Fixed, packet_id) == 0");
static_assert(offsetof(NetRPacket0x2727_Fixed, account_id) == 2, "offsetof(NetRPacket0x2727_Fixed, account_id) == 2");
static_assert(sizeof(NetRPacket0x2727_Fixed) == 6, "sizeof(NetRPacket0x2727_Fixed) == 6");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2727_Fixed *network, RPacket0x2727_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2727_Fixed *native, NetRPacket0x2727_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

struct RPacket0x2728_Head
{
    uint16_t packet_id;
    uint16_t packet_length;
    uint32_t account_id;
};
struct NetRPacket0x2728_Head
{
    Little16 packet_id;
    Little16 packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetRPacket0x2728_Head, packet_id) == 0, "offsetof(NetRPacket0x2728_Head, packet_id) == 0");
static_assert(offsetof(NetRPacket0x2728_Head, packet_length) == 2, "offsetof(NetRPacket0x2728_Head, packet_length) == 2");
static_assert(offsetof(NetRPacket0x2728_Head, account_id) == 4, "offsetof(NetRPacket0x2728_Head, account_id) == 4");
static_assert(sizeof(NetRPacket0x2728_Head) == 8, "sizeof(NetRPacket0x2728_Head) == 8");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2728_Head *network, RPacket0x2728_Head native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->packet_length, native.packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2728_Head *native, NetRPacket0x2728_Head network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->packet_length, network.packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

struct RPacket0x2728_Repeat
{
    VarName name;
    uint32_t value;
};
struct NetRPacket0x2728_Repeat
{
    NetString<sizeof(VarName)> name;
    Little32 value;
};
static_assert(offsetof(NetRPacket0x2728_Repeat, name) == 0, "offsetof(NetRPacket0x2728_Repeat, name) == 0");
static_assert(offsetof(NetRPacket0x2728_Repeat, value) == 32, "offsetof(NetRPacket0x2728_Repeat, value) == 32");
static_assert(sizeof(NetRPacket0x2728_Repeat) == 36, "sizeof(NetRPacket0x2728_Repeat) == 36");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2728_Repeat *network, RPacket0x2728_Repeat native)
{
    bool rv = true;
    rv &= native_to_network(&network->name, native.name);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2728_Repeat *native, NetRPacket0x2728_Repeat network)
{
    bool rv = true;
    rv &= network_to_native(&native->name, network.name);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

struct SPacket0x2729_Head
{
    uint16_t packet_id;
    uint16_t packet_length;
    uint32_t account_id;
};
struct NetSPacket0x2729_Head
{
    Little16 packet_id;
    Little16 packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetSPacket0x2729_Head, packet_id) == 0, "offsetof(NetSPacket0x2729_Head, packet_id) == 0");
static_assert(offsetof(NetSPacket0x2729_Head, packet_length) == 2, "offsetof(NetSPacket0x2729_Head, packet_length) == 2");
static_assert(offsetof(NetSPacket0x2729_Head, account_id) == 4, "offsetof(NetSPacket0x2729_Head, account_id) == 4");
static_assert(sizeof(NetSPacket0x2729_Head) == 8, "sizeof(NetSPacket0x2729_Head) == 8");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x2729_Head *network, SPacket0x2729_Head native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->packet_length, native.packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x2729_Head *native, NetSPacket0x2729_Head network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->packet_length, network.packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

struct SPacket0x2729_Repeat
{
    VarName name;
    uint32_t value;
};
struct NetSPacket0x2729_Repeat
{
    NetString<sizeof(VarName)> name;
    Little32 value;
};
static_assert(offsetof(NetSPacket0x2729_Repeat, name) == 0, "offsetof(NetSPacket0x2729_Repeat, name) == 0");
static_assert(offsetof(NetSPacket0x2729_Repeat, value) == 32, "offsetof(NetSPacket0x2729_Repeat, value) == 32");
static_assert(sizeof(NetSPacket0x2729_Repeat) == 36, "sizeof(NetSPacket0x2729_Repeat) == 36");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x2729_Repeat *network, SPacket0x2729_Repeat native)
{
    bool rv = true;
    rv &= native_to_network(&network->name, native.name);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x2729_Repeat *native, NetSPacket0x2729_Repeat network)
{
    bool rv = true;
    rv &= network_to_native(&native->name, network.name);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

struct RPacket0x272a_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
};
struct NetRPacket0x272a_Fixed
{
    Little16 packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetRPacket0x272a_Fixed, packet_id) == 0, "offsetof(NetRPacket0x272a_Fixed, packet_id) == 0");
static_assert(offsetof(NetRPacket0x272a_Fixed, account_id) == 2, "offsetof(NetRPacket0x272a_Fixed, account_id) == 2");
static_assert(sizeof(NetRPacket0x272a_Fixed) == 6, "sizeof(NetRPacket0x272a_Fixed) == 6");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x272a_Fixed *network, RPacket0x272a_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x272a_Fixed *native, NetRPacket0x272a_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

struct SPacket0x2730_Fixed
{
    uint16_t packet_id;
};
struct NetSPacket0x2730_Fixed
{
    Little16 packet_id;
};
static_assert(offsetof(NetSPacket0x2730_Fixed, packet_id) == 0, "offsetof(NetSPacket0x2730_Fixed, packet_id) == 0");
static_assert(sizeof(NetSPacket0x2730_Fixed) == 2, "sizeof(NetSPacket0x2730_Fixed) == 2");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x2730_Fixed *network, SPacket0x2730_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x2730_Fixed *native, NetSPacket0x2730_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    return rv;
}

struct SPacket0x2731_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
    uint8_t ban_not_status;
    TimeT status_or_ban_until;
};
struct NetSPacket0x2731_Fixed
{
    Little16 packet_id;
    Little32 account_id;
    Byte ban_not_status;
    Little32 status_or_ban_until;
};
static_assert(offsetof(NetSPacket0x2731_Fixed, packet_id) == 0, "offsetof(NetSPacket0x2731_Fixed, packet_id) == 0");
static_assert(offsetof(NetSPacket0x2731_Fixed, account_id) == 2, "offsetof(NetSPacket0x2731_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket0x2731_Fixed, ban_not_status) == 6, "offsetof(NetSPacket0x2731_Fixed, ban_not_status) == 6");
static_assert(offsetof(NetSPacket0x2731_Fixed, status_or_ban_until) == 7, "offsetof(NetSPacket0x2731_Fixed, status_or_ban_until) == 7");
static_assert(sizeof(NetSPacket0x2731_Fixed) == 11, "sizeof(NetSPacket0x2731_Fixed) == 11");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x2731_Fixed *network, SPacket0x2731_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->ban_not_status, native.ban_not_status);
    rv &= native_to_network(&network->status_or_ban_until, native.status_or_ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x2731_Fixed *native, NetSPacket0x2731_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->ban_not_status, network.ban_not_status);
    rv &= network_to_native(&native->status_or_ban_until, network.status_or_ban_until);
    return rv;
}

struct SPacket0x2732_Head
{
    uint16_t packet_id;
    uint16_t packet_length;
};
struct NetSPacket0x2732_Head
{
    Little16 packet_id;
    Little16 packet_length;
};
static_assert(offsetof(NetSPacket0x2732_Head, packet_id) == 0, "offsetof(NetSPacket0x2732_Head, packet_id) == 0");
static_assert(offsetof(NetSPacket0x2732_Head, packet_length) == 2, "offsetof(NetSPacket0x2732_Head, packet_length) == 2");
static_assert(sizeof(NetSPacket0x2732_Head) == 4, "sizeof(NetSPacket0x2732_Head) == 4");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x2732_Head *network, SPacket0x2732_Head native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->packet_length, native.packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x2732_Head *native, NetSPacket0x2732_Head network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->packet_length, network.packet_length);
    return rv;
}

struct SPacket0x2732_Repeat
{
    uint32_t account_id;
    GmLevel gm_level;
};
struct NetSPacket0x2732_Repeat
{
    Little32 account_id;
    Byte gm_level;
};
static_assert(offsetof(NetSPacket0x2732_Repeat, account_id) == 0, "offsetof(NetSPacket0x2732_Repeat, account_id) == 0");
static_assert(offsetof(NetSPacket0x2732_Repeat, gm_level) == 4, "offsetof(NetSPacket0x2732_Repeat, gm_level) == 4");
static_assert(sizeof(NetSPacket0x2732_Repeat) == 5, "sizeof(NetSPacket0x2732_Repeat) == 5");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x2732_Repeat *network, SPacket0x2732_Repeat native)
{
    bool rv = true;
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->gm_level, native.gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x2732_Repeat *native, NetSPacket0x2732_Repeat network)
{
    bool rv = true;
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->gm_level, network.gm_level);
    return rv;
}

struct RPacket0x2740_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
    AccountPass old_pass;
    AccountPass new_pass;
};
struct NetRPacket0x2740_Fixed
{
    Little16 packet_id;
    Little32 account_id;
    NetString<sizeof(AccountPass)> old_pass;
    NetString<sizeof(AccountPass)> new_pass;
};
static_assert(offsetof(NetRPacket0x2740_Fixed, packet_id) == 0, "offsetof(NetRPacket0x2740_Fixed, packet_id) == 0");
static_assert(offsetof(NetRPacket0x2740_Fixed, account_id) == 2, "offsetof(NetRPacket0x2740_Fixed, account_id) == 2");
static_assert(offsetof(NetRPacket0x2740_Fixed, old_pass) == 6, "offsetof(NetRPacket0x2740_Fixed, old_pass) == 6");
static_assert(offsetof(NetRPacket0x2740_Fixed, new_pass) == 30, "offsetof(NetRPacket0x2740_Fixed, new_pass) == 30");
static_assert(sizeof(NetRPacket0x2740_Fixed) == 54, "sizeof(NetRPacket0x2740_Fixed) == 54");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x2740_Fixed *network, RPacket0x2740_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->old_pass, native.old_pass);
    rv &= native_to_network(&network->new_pass, native.new_pass);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x2740_Fixed *native, NetRPacket0x2740_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->old_pass, network.old_pass);
    rv &= network_to_native(&native->new_pass, network.new_pass);
    return rv;
}

struct SPacket0x2741_Fixed
{
    uint16_t packet_id;
    uint32_t account_id;
    uint8_t status;
};
struct NetSPacket0x2741_Fixed
{
    Little16 packet_id;
    Little32 account_id;
    Byte status;
};
static_assert(offsetof(NetSPacket0x2741_Fixed, packet_id) == 0, "offsetof(NetSPacket0x2741_Fixed, packet_id) == 0");
static_assert(offsetof(NetSPacket0x2741_Fixed, account_id) == 2, "offsetof(NetSPacket0x2741_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket0x2741_Fixed, status) == 6, "offsetof(NetSPacket0x2741_Fixed, status) == 6");
static_assert(sizeof(NetSPacket0x2741_Fixed) == 7, "sizeof(NetSPacket0x2741_Fixed) == 7");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x2741_Fixed *network, SPacket0x2741_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->status, native.status);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x2741_Fixed *native, NetSPacket0x2741_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->status, network.status);
    return rv;
}


#endif // TMWA_PROTO2_LOGIN_CHAR_HPP
