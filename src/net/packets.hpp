#ifndef TMWA_NET_PACKETS_HPP
#define TMWA_NET_PACKETS_HPP
//    packets.hpp - palatable socket buffer accessors
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

# include "../compat/cast.hpp"

# include "../ints/little.hpp"

# include "../io/fwd.hpp"

// ordering violation, should invert
# include "../proto2/fwd.hpp"

# include "socket.hpp"

enum class RecvResult
{
    Incomplete,
    Complete,
    Error,
};

enum class SendResult
{
    Success,
    Fail,
};


size_t packet_avail(Session *s);
void packet_dump(io::WriteFile& out, Session *s);

bool packet_fetch(Session *s, size_t offset, Byte *data, size_t sz);
void packet_discard(Session *s, size_t sz);
bool packet_send(Session *s, const Byte *data, size_t sz);

inline
bool packet_peek_id(Session *s, uint16_t *packet_id)
{
    Little16 id;
    bool okay = packet_fetch(s, 0, reinterpret_cast<Byte *>(&id), 2);
    if (okay)
    {
        if (!network_to_native(packet_id, id))
        {
            s->set_eof();
            return false;
        }
    }
    return okay;
}

template<uint16_t id>
__attribute__((warn_unused_result))
SendResult net_send_fpacket(Session *s, const NetPacket_Fixed<id>& fixed)
{
    bool ok = packet_send(s, reinterpret_cast<const Byte *>(&fixed), sizeof(NetPacket_Fixed<id>));
    return ok ? SendResult::Success : SendResult::Fail;
}

template<uint16_t id>
__attribute__((warn_unused_result))
SendResult net_send_vpacket(Session *s, const NetPacket_Head<id>& head, const std::vector<NetPacket_Repeat<id>>& repeat)
{
    bool ok = packet_send(s, reinterpret_cast<const Byte *>(&head), sizeof(NetPacket_Head<id>));
    ok &= packet_send(s, reinterpret_cast<const Byte *>(repeat.data()), repeat.size() * sizeof(NetPacket_Repeat<id>));
    return ok ? SendResult::Success : SendResult::Fail;
}

template<uint16_t id>
__attribute__((warn_unused_result))
RecvResult net_recv_fpacket(Session *s, NetPacket_Fixed<id>& fixed)
{
    bool ok = packet_fetch(s, 0, reinterpret_cast<Byte *>(&fixed), sizeof(NetPacket_Fixed<id>));
    if (ok)
    {
        packet_discard(s, sizeof(NetPacket_Fixed<id>));
        return RecvResult::Complete;
    }
    return RecvResult::Incomplete;
}

template<uint16_t id>
__attribute__((warn_unused_result))
RecvResult net_recv_vpacket(Session *s, NetPacket_Head<id>& head, std::vector<NetPacket_Repeat<id>>& repeat)
{
    bool ok = packet_fetch(s, 0, reinterpret_cast<Byte *>(&head), sizeof(NetPacket_Head<id>));
    if (ok)
    {
        Packet_Head<id> nat;
        if (!network_to_native(&nat, head))
            return RecvResult::Error;
        if (packet_avail(s) < nat.magic_packet_length)
            return RecvResult::Incomplete;
        if (nat.magic_packet_length < sizeof(NetPacket_Head<id>))
            return RecvResult::Error;
        size_t bytes_repeat = nat.magic_packet_length - sizeof(NetPacket_Head<id>);
        if (bytes_repeat % sizeof(NetPacket_Repeat<id>))
            return RecvResult::Error;
        repeat.resize(bytes_repeat / sizeof(NetPacket_Repeat<id>));
        if (packet_fetch(s, sizeof(NetPacket_Head<id>), reinterpret_cast<Byte *>(repeat.data()), bytes_repeat))
        {
            packet_discard(s, nat.magic_packet_length);
            return RecvResult::Complete;
        }
        return RecvResult::Incomplete;
    }
    return RecvResult::Incomplete;
}


template<uint16_t id, uint16_t size>
void send_fpacket(Session *s, const Packet_Fixed<id>& fixed)
{
    static_assert(id == Packet_Fixed<id>::PACKET_ID, "Packet_Fixed<id>::PACKET_ID");
    static_assert(size == sizeof(NetPacket_Fixed<id>), "sizeof(NetPacket_Fixed<id>)");

    NetPacket_Fixed<id> net_fixed;
    if (!native_to_network(&net_fixed, fixed))
    {
        s->set_eof();
        return;
    }
    SendResult rv = net_send_fpacket(s, net_fixed);
    if (rv != SendResult::Success)
        s->set_eof();
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
void send_vpacket(Session *s, Packet_Head<id>& head, const std::vector<Packet_Repeat<id>>& repeat)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "sizeof(NetPacket_Head<id>)");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "sizeof(NetPacket_Repeat<id>)");

    NetPacket_Head<id> net_head;
    // since these are already allocated, can't overflow address space
    size_t total_size = sizeof(NetPacket_Head<id>) + repeat.size() * sizeof(NetPacket_Repeat<id>);
    // truncates
    head.magic_packet_length = total_size;
    if (head.magic_packet_length != total_size)
    {
        s->set_eof();
        return;
    }
    // TODO potentially avoid the allocation
    std::vector<NetPacket_Repeat<id>> net_repeat(repeat.size());
    if (!native_to_network(&net_head, head))
    {
        s->set_eof();
        return;
    }
    for (size_t i = 0; i < repeat.size(); ++i)
    {
        if (!native_to_network(&net_repeat[i], repeat[i]))
        {
            s->set_eof();
            return;
        }
    }
    SendResult rv = net_send_vpacket(s, net_head, net_repeat);
    if (rv != SendResult::Success)
        s->set_eof();
}

template<uint16_t id, uint16_t size>
__attribute__((warn_unused_result))
RecvResult recv_fpacket(Session *s, Packet_Fixed<id>& fixed)
{
    static_assert(id == Packet_Fixed<id>::PACKET_ID, "Packet_Fixed<id>::PACKET_ID");
    static_assert(size == sizeof(NetPacket_Fixed<id>), "NetPacket_Fixed<id>");

    NetPacket_Fixed<id> net_fixed;
    RecvResult rv = net_recv_fpacket(s, net_fixed);
    assert (fixed.magic_packet_id == Packet_Fixed<id>::PACKET_ID);
    if (rv == RecvResult::Complete)
    {
        if (!network_to_native(&fixed, net_fixed))
            return RecvResult::Error;
    }
    return rv;
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
__attribute__((warn_unused_result))
RecvResult recv_vpacket(Session *s, Packet_Head<id>& head, std::vector<Packet_Repeat<id>>& repeat)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "NetPacket_Head<id>");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "NetPacket_Repeat<id>");

    NetPacket_Head<id> net_head;
    std::vector<NetPacket_Repeat<id>> net_repeat;
    RecvResult rv = net_recv_vpacket(s, net_head, net_repeat);
    assert (head.magic_packet_id == Packet_Head<id>::PACKET_ID);
    if (rv == RecvResult::Complete)
    {
        if (!network_to_native(&head, net_head))
            return RecvResult::Error;
        repeat.resize(net_repeat.size());
        for (size_t i = 0; i < net_repeat.size(); ++i)
        {
            if (!network_to_native(&repeat[i], net_repeat[i]))
                return RecvResult::Error;
        }
    }
    return rv;
}

// convenience for trailing strings

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
void send_vpacket(Session *s, Packet_Head<id>& head, const XString& repeat)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "NetPacket_Head<id>");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "NetPacket_Repeat<id>");
    static_assert(repeatsize == 1, "repeatsize");

    NetPacket_Head<id> net_head;
    // since it's already allocated, it can't overflow address space
    size_t total_length = sizeof(NetPacket_Head<id>) + (repeat.size() + 1) * sizeof(NetPacket_Repeat<id>);
    head.magic_packet_length = total_length;
    if (head.magic_packet_length != total_length)
    {
        s->set_eof();
        return;
    }
    // TODO potentially avoid the allocation
    std::vector<NetPacket_Repeat<id>> net_repeat(repeat.size() + 1);
    if (!native_to_network(&net_head, head))
    {
        s->set_eof();
        return;
    }
    for (size_t i = 0; i < repeat.size(); ++i)
    {
        net_repeat[i].c = Byte{static_cast<uint8_t>(repeat[i])};
    }
    net_repeat[repeat.size()].c = Byte{static_cast<uint8_t>('\0')};
    SendResult rv = net_send_vpacket(s, net_head, net_repeat);
    if (rv != SendResult::Success)
        s->set_eof();
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
__attribute__((warn_unused_result))
RecvResult recv_vpacket(Session *s, Packet_Head<id>& head, AString& repeat)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "NetPacket_Head<id>");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "NetPacket_Repeat<id>");
    static_assert(repeatsize == 1, "repeatsize");

    NetPacket_Head<id> net_head;
    std::vector<NetPacket_Repeat<id>> net_repeat;
    RecvResult rv = net_recv_vpacket(s, net_head, net_repeat);
    assert (head.magic_packet_id == Packet_Head<id>::PACKET_ID);
    if (rv == RecvResult::Complete)
    {
        if (!network_to_native(&head, net_head))
            return RecvResult::Error;
        // reinterpret_cast is needed to correctly handle an empty vector
        const char *begin = sign_cast<const char *>(net_repeat.data());
        const char *end = begin + repeat.size();
        end = std::find(begin, end, '\0');
        repeat = XString(begin, end, nullptr);
    }
    return rv;
}


// if there is nothing in the head but the id and length, use the below

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
void send_packet_repeatonly(Session *s, const std::vector<Packet_Repeat<id>>& v)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "repeat headsize");
    static_assert(headsize == 4, "repeat headsize");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "sizeof(NetPacket_Repeat<id>)");

    Packet_Head<id> head;
    send_vpacket<id, 4, repeatsize>(s, head, v);
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
__attribute__((warn_unused_result))
RecvResult recv_packet_repeatonly(Session *s, std::vector<Packet_Repeat<id>>& v)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "repeat headsize");
    static_assert(headsize == 4, "repeat headsize");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "sizeof(NetPacket_Repeat<id>)");

    Packet_Head<id> head;
    return recv_vpacket<id, 4, repeatsize>(s, head, v);
}

#endif // TMWA_NET_PACKETS_HPP
