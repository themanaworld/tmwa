#pragma once
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

#include "fwd.hpp"

#include <vector>

#include "../ints/little.hpp"

#include "../compat/cast.hpp"

#include "../proto2/fwd.hpp"

#include "../net/socket.hpp"


namespace tmwa
{
struct Buffer
{
    std::vector<Byte> bytes;
};

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
void packet_dump(Session *s);

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

inline
void send_buffer(Session *s, const Buffer& buffer)
{
    bool ok = !buffer.bytes.empty() && packet_send(s, buffer.bytes.data(), buffer.bytes.size());
    if (!ok)
        s->set_eof();
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
RecvResult net_recv_ppacket(Session *s, NetPacket_Payload<id>& payload)
{
    bool ok = packet_fetch(s, 0, reinterpret_cast<Byte *>(&payload), sizeof(NetPacket_Payload<id>));
    if (ok)
    {
        packet_discard(s, sizeof(NetPacket_Payload<id>));
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

template<uint16_t id>
__attribute__((warn_unused_result))
RecvResult net_recv_opacket(Session *s, NetPacket_Head<id>& head, bool *has_opt, NetPacket_Option<id>& opt)
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
        if (bytes_repeat % sizeof(NetPacket_Option<id>))
            return RecvResult::Error;
        size_t has_opt_pls = bytes_repeat / sizeof(NetPacket_Option<id>);
        if (has_opt_pls > 1)
            return RecvResult::Error;
        *has_opt = has_opt_pls;
        if (!*has_opt || packet_fetch(s, sizeof(NetPacket_Head<id>), reinterpret_cast<Byte *>(&opt), sizeof(NetPacket_Option<id>)))
        {
            packet_discard(s, nat.magic_packet_length);
            return RecvResult::Complete;
        }
        return RecvResult::Incomplete;
    }
    return RecvResult::Incomplete;
}


template<uint16_t id, uint16_t size>
Buffer create_fpacket(const Packet_Fixed<id>& fixed)
{
    static_assert(id == Packet_Fixed<id>::PACKET_ID, "Packet_Fixed<id>::PACKET_ID");
    static_assert(size == sizeof(NetPacket_Fixed<id>), "sizeof(NetPacket_Fixed<id>)");

    Buffer buf;
    buf.bytes.resize(sizeof(NetPacket_Fixed<id>));
    auto& net_fixed = reinterpret_cast<NetPacket_Fixed<id>&>(
            *(buf.bytes.begin() + 0));
    if (!native_to_network(&net_fixed, fixed))
    {
        return Buffer();
    }
    return buf;
}

template<uint16_t id>
Buffer create_ppacket(Packet_Payload<id>& payload)
{
    static_assert(id == Packet_Payload<id>::PACKET_ID, "Packet_Payload<id>::PACKET_ID");

    if (id != 0x8000)
        payload.magic_packet_length = sizeof(NetPacket_Payload<id>);

    Buffer buf;
    buf.bytes.resize(sizeof(NetPacket_Payload<id>));
    auto& net_payload = reinterpret_cast<NetPacket_Payload<id>&>(
            *(buf.bytes.begin() + 0));
    if (!native_to_network(&net_payload, payload))
    {
        return Buffer();
    }
    return buf;
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
Buffer create_vpacket(Packet_Head<id>& head, const std::vector<Packet_Repeat<id>>& repeat)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "sizeof(NetPacket_Head<id>)");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "sizeof(NetPacket_Repeat<id>)");

    // since these are already allocated, can't overflow address space
    size_t total_size = sizeof(NetPacket_Head<id>) + repeat.size() * sizeof(NetPacket_Repeat<id>);
    // truncates
    head.magic_packet_length = total_size;
    if (head.magic_packet_length != total_size)
    {
        return Buffer();
    }

    Buffer buf;
    buf.bytes.resize(total_size);
    auto& net_head = reinterpret_cast<NetPacket_Head<id>&>(
            *(buf.bytes.begin() + 0));
    if (!native_to_network(&net_head, head))
    {
        return Buffer();
    }
    for (size_t i = 0; i < repeat.size(); ++i)
    {
        auto& net_repeat_i = reinterpret_cast<NetPacket_Repeat<id>&>(
                *(buf.bytes.begin()
                    + sizeof(NetPacket_Head<id>)
                    + i * sizeof(NetPacket_Repeat<id>)));
        if (!native_to_network(&net_repeat_i, repeat[i]))
        {
            return Buffer();
        }
    }
    return buf;
}

template<uint16_t id, uint16_t headsize, uint16_t optsize>
Buffer create_opacket(Packet_Head<id>& head, bool has_opt, const Packet_Option<id>& opt)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "sizeof(NetPacket_Head<id>)");
    static_assert(id == Packet_Option<id>::PACKET_ID, "Packet_Option<id>::PACKET_ID");
    static_assert(optsize == sizeof(NetPacket_Option<id>), "sizeof(NetPacket_Option<id>)");

    // since these are already allocated, can't overflow address space
    size_t total_size = sizeof(NetPacket_Head<id>) + has_opt * sizeof(NetPacket_Option<id>);
    // truncates
    head.magic_packet_length = total_size;
    if (head.magic_packet_length != total_size)
    {
        return Buffer();
    }

    Buffer buf;
    buf.bytes.resize(total_size);

    auto& net_head = reinterpret_cast<NetPacket_Head<id>&>(
            *(buf.bytes.begin() + 0));
    if (!native_to_network(&net_head, head))
    {
        return Buffer();
    }
    if (has_opt)
    {
        auto& net_opt = reinterpret_cast<NetPacket_Option<id>&>(
                *(buf.bytes.begin()
                    + sizeof(NetPacket_Head<id>)));
        if (!native_to_network(&net_opt, opt))
        {
            return Buffer();
        }
    }

    return buf;
}

template<uint16_t id, uint16_t size>
void send_fpacket(Session *s, const Packet_Fixed<id>& fixed)
{
    Buffer pkt = create_fpacket<id, size>(fixed);
    send_buffer(s, pkt);
}

template<uint16_t id>
void send_ppacket(Session *s, Packet_Payload<id>& payload)
{
    Buffer pkt = create_ppacket<id>(payload);
    send_buffer(s, pkt);
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
void send_vpacket(Session *s, Packet_Head<id>& head, const std::vector<Packet_Repeat<id>>& repeat)
{
    Buffer pkt = create_vpacket<id, headsize, repeatsize>(head, repeat);
    send_buffer(s, pkt);
}

template<uint16_t id, uint16_t headsize, uint16_t optsize>
void send_opacket(Session *s, Packet_Head<id>& head, bool has_opt, const Packet_Option<id>& opt)
{
    Buffer pkt = create_opacket<id, headsize, optsize>(head, has_opt, opt);
    send_buffer(s, pkt);
}

template<uint16_t id, uint16_t size>
__attribute__((warn_unused_result))
RecvResult recv_fpacket(Session *s, Packet_Fixed<id>& fixed)
{
    static_assert(id == Packet_Fixed<id>::PACKET_ID, "Packet_Fixed<id>::PACKET_ID");
    static_assert(size == sizeof(NetPacket_Fixed<id>), "NetPacket_Fixed<id>");

    NetPacket_Fixed<id> net_fixed;
    RecvResult rv = net_recv_fpacket(s, net_fixed);
    if (rv == RecvResult::Complete)
    {
        if (!network_to_native(&fixed, net_fixed))
            return RecvResult::Error;
        assert (fixed.magic_packet_id == Packet_Fixed<id>::PACKET_ID);
    }
    return rv;
}

template<uint16_t id>
__attribute__((warn_unused_result))
RecvResult recv_ppacket(Session *s, Packet_Payload<id>& payload)
{
    static_assert(id == Packet_Payload<id>::PACKET_ID, "Packet_Payload<id>::PACKET_ID");

    NetPacket_Payload<id> net_payload;
    RecvResult rv = net_recv_ppacket(s, net_payload);
    if (rv == RecvResult::Complete)
    {
        if (!network_to_native(&payload, net_payload))
            return RecvResult::Error;
        assert (payload.magic_packet_id == Packet_Payload<id>::PACKET_ID);
        if (id == 0x8000)
        {
            // 0x8000 is special
            if (packet_avail(s) < payload.magic_packet_length)
                return RecvResult::Incomplete;
            payload.magic_packet_length = 4;
            return RecvResult::Complete;
        }
        if (payload.magic_packet_length != sizeof(net_payload))
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
    if (rv == RecvResult::Complete)
    {
        if (!network_to_native(&head, net_head))
            return RecvResult::Error;
        assert (head.magic_packet_id == Packet_Head<id>::PACKET_ID);

        repeat.resize(net_repeat.size());
        for (size_t i = 0; i < net_repeat.size(); ++i)
        {
            if (!network_to_native(&repeat[i], net_repeat[i]))
                return RecvResult::Error;
        }
    }
    return rv;
}

template<uint16_t id, uint16_t headsize, uint16_t optsize>
__attribute__((warn_unused_result))
RecvResult recv_opacket(Session *s, Packet_Head<id>& head, bool *has_opt, Packet_Option<id>& opt)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "NetPacket_Head<id>");
    static_assert(id == Packet_Option<id>::PACKET_ID, "Packet_Option<id>::PACKET_ID");
    static_assert(optsize == sizeof(NetPacket_Option<id>), "NetPacket_Option<id>");

    NetPacket_Head<id> net_head;
    NetPacket_Option<id> net_opt;
    RecvResult rv = net_recv_opacket(s, net_head, has_opt, net_opt);
    if (rv == RecvResult::Complete)
    {
        if (!network_to_native(&head, net_head))
            return RecvResult::Error;
        assert (head.magic_packet_id == Packet_Head<id>::PACKET_ID);

        if (*has_opt)
        {
            if (!network_to_native(&opt, net_opt))
                return RecvResult::Error;
        }
    }
    return rv;
}


// convenience for trailing strings

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
Buffer create_vpacket(Packet_Head<id>& head, const XString& repeat)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "NetPacket_Head<id>");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "NetPacket_Repeat<id>");
    static_assert(repeatsize == 1, "repeatsize");

    // since it's already allocated, it can't overflow address space
    size_t total_length = sizeof(NetPacket_Head<id>) + (repeat.size() + 1) * sizeof(NetPacket_Repeat<id>);
    head.magic_packet_length = total_length;
    if (head.magic_packet_length != total_length)
    {
        return Buffer();
    }

    Buffer buf;
    buf.bytes.resize(total_length);
    auto& net_head = reinterpret_cast<NetPacket_Head<id>&>(
            *(buf.bytes.begin() + 0));
    std::vector<NetPacket_Repeat<id>> net_repeat(repeat.size() + 1);
    if (!native_to_network(&net_head, head))
    {
        return Buffer();
    }
    for (size_t i = 0; i < repeat.size(); ++i)
    {
        auto& net_repeat_i = reinterpret_cast<NetPacket_Repeat<id>&>(
                *(buf.bytes.begin()
                    + sizeof(NetPacket_Head<id>)
                    + i));
        net_repeat_i.c = Byte{static_cast<uint8_t>(repeat[i])};
    }
    auto& net_repeat_repeat_size = reinterpret_cast<NetPacket_Repeat<id>&>(
            *(buf.bytes.begin()
                + sizeof(NetPacket_Head<id>)
                + repeat.size()));
    net_repeat_repeat_size.c = Byte{static_cast<uint8_t>('\0')};
    return buf;
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
void send_vpacket(Session *s, Packet_Head<id>& head, const XString& repeat)
{
    Buffer pkt = create_vpacket<id, headsize, repeatsize>(head, repeat);
    send_buffer(s, pkt);
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
        const char *end = begin + net_repeat.size();
        end = std::find(begin, end, '\0');
        repeat = XString(begin, end, nullptr);
    }
    return rv;
}


// if there is nothing in the head but the id and length, use the below

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
Buffer create_packet_repeatonly(const std::vector<Packet_Repeat<id>>& v)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "repeat headsize");
    static_assert(headsize == 4, "repeat headsize");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "sizeof(NetPacket_Repeat<id>)");

    Packet_Head<id> head;
    return create_vpacket<id, 4, repeatsize>(head, v);
}

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


// and the combination of both of the above

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
Buffer create_packet_repeatonly(const XString& repeat)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "repeat headsize");
    static_assert(headsize == 4, "repeat headsize");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "sizeof(NetPacket_Repeat<id>)");
    static_assert(repeatsize == 1, "repeatsize");

    Packet_Head<id> head;
    return create_vpacket<id, 4, repeatsize>(head, repeat);
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
void send_packet_repeatonly(Session *s, const XString& repeat)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "repeat headsize");
    static_assert(headsize == 4, "repeat headsize");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "sizeof(NetPacket_Repeat<id>)");
    static_assert(repeatsize == 1, "repeatsize");

    Packet_Head<id> head;
    send_vpacket<id, 4, repeatsize>(s, head, repeat);
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize>
__attribute__((warn_unused_result))
RecvResult recv_packet_repeatonly(Session *s, AString& repeat)
{
    static_assert(id == Packet_Head<id>::PACKET_ID, "Packet_Head<id>::PACKET_ID");
    static_assert(headsize == sizeof(NetPacket_Head<id>), "repeat headsize");
    static_assert(headsize == 4, "repeat headsize");
    static_assert(id == Packet_Repeat<id>::PACKET_ID, "Packet_Repeat<id>::PACKET_ID");
    static_assert(repeatsize == sizeof(NetPacket_Repeat<id>), "sizeof(NetPacket_Repeat<id>)");
    static_assert(repeatsize == 1, "repeatsize");

    Packet_Head<id> head;
    return recv_vpacket<id, 4, repeatsize>(s, head, repeat);
}
} // namespace tmwa
