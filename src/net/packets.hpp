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

 #include "../io/fwd.hpp"

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

template<class F>
__attribute__((warn_unused_result))
SendResult net_send_fpacket(Session *s, const F& fixed)
{
    bool ok = packet_send(s, reinterpret_cast<const Byte *>(&fixed), sizeof(F));
    return ok ? SendResult::Success : SendResult::Fail;
}

template<class H, class R>
__attribute__((warn_unused_result))
SendResult net_send_vpacket(Session *s, const H& head, const std::vector<R>& repeat)
{
    bool ok = packet_send(s, reinterpret_cast<const Byte *>(&head), sizeof(H));
    ok &= packet_send(s, reinterpret_cast<const Byte *>(repeat.data()), repeat.size() * sizeof(R));
    return ok ? SendResult::Success : SendResult::Fail;
}

template<class F>
__attribute__((warn_unused_result))
RecvResult net_recv_fpacket(Session *s, F& fixed)
{
    bool ok = packet_fetch(s, 0, reinterpret_cast<Byte *>(&fixed), sizeof(F));
    if (ok)
    {
        packet_discard(s, sizeof(F));
        return RecvResult::Complete;
    }
    return RecvResult::Incomplete;
}

template<class HNat, class H, class R>
__attribute__((warn_unused_result))
RecvResult net_recv_vpacket(Session *s, H& head, std::vector<R>& repeat)
{
    bool ok = packet_fetch(s, 0, reinterpret_cast<Byte *>(&head), sizeof(H));
    if (ok)
    {
        HNat nat;
        if (!network_to_native(&nat, head))
            return RecvResult::Error;
        if (packet_avail(s) < nat.magic_packet_length)
            return RecvResult::Incomplete;
        if (nat.magic_packet_length < sizeof(H))
            return RecvResult::Error;
        size_t bytes_repeat = nat.magic_packet_length - sizeof(H);
        if (bytes_repeat % sizeof(R))
            return RecvResult::Error;
        repeat.resize(bytes_repeat / sizeof(R));
        if (packet_fetch(s, sizeof(H), reinterpret_cast<Byte *>(repeat.data()), bytes_repeat))
        {
            packet_discard(s, nat.magic_packet_length);
            return RecvResult::Complete;
        }
        return RecvResult::Incomplete;
    }
    return RecvResult::Incomplete;
}


template<uint16_t id, uint16_t size, class F>
void send_fpacket(Session *s, const F& fixed)
{
    static_assert(id == F::PACKET_ID, "F::PACKET_ID");
    static_assert(size == sizeof(typename F::NetType), "F::NetType");

    typename F::NetType net_fixed;
    if (!native_to_network(&net_fixed, fixed))
    {
        s->set_eof();
        return;
    }
    SendResult rv = net_send_fpacket(s, net_fixed);
    if (rv != SendResult::Success)
        s->set_eof();
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize, class H, class R>
void send_vpacket(Session *s, H& head, const std::vector<R>& repeat)
{
    static_assert(id == H::PACKET_ID, "H::PACKET_ID");
    static_assert(headsize == sizeof(typename H::NetType), "H::NetType");
    static_assert(id == R::PACKET_ID, "R::PACKET_ID");
    static_assert(repeatsize == sizeof(typename R::NetType), "R::NetType");

    typename H::NetType net_head;
    // since these are already allocated, can't overflow address space
    size_t total_size = sizeof(typename H::NetType) + repeat.size() * sizeof(typename R::NetType);
    // truncates
    head.magic_packet_length = total_size;
    if (head.magic_packet_length != total_size)
    {
        s->set_eof();
        return;
    }
    // TODO potentially avoid the allocation
    std::vector<typename R::NetType> net_repeat(repeat.size());
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

template<uint16_t id, uint16_t size, class F>
__attribute__((warn_unused_result))
RecvResult recv_fpacket(Session *s, F& fixed)
{
    static_assert(id == F::PACKET_ID, "F::PACKET_ID");
    static_assert(size == sizeof(typename F::NetType), "F::NetType");

    typename F::NetType net_fixed;
    RecvResult rv = net_recv_fpacket(s, net_fixed);
    assert (fixed.magic_packet_id == F::PACKET_ID);
    if (rv == RecvResult::Complete)
    {
        if (!network_to_native(&fixed, net_fixed))
            return RecvResult::Error;
    }
    return rv;
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize, class H, class R>
__attribute__((warn_unused_result))
RecvResult recv_vpacket(Session *s, H& head, std::vector<R>& repeat)
{
    static_assert(id == H::PACKET_ID, "H::PACKET_ID");
    static_assert(headsize == sizeof(typename H::NetType), "H::NetType");
    static_assert(id == R::PACKET_ID, "R::PACKET_ID");
    static_assert(repeatsize == sizeof(typename R::NetType), "R::NetType");

    typename H::NetType net_head;
    std::vector<typename R::NetType> net_repeat;
    RecvResult rv = net_recv_vpacket<H>(s, net_head, net_repeat);
    assert (head.magic_packet_id == H::PACKET_ID);
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

struct VarStringNetType
{
    char c;
};

template<uint16_t id, uint16_t headsize, uint16_t repeatsize, class H>
void send_vpacket(Session *s, H& head, const XString& repeat)
{
    static_assert(id == H::PACKET_ID, "H::PACKET_ID");
    static_assert(headsize == sizeof(typename H::NetType), "H::NetType");
    // static_assert(id == R::PACKET_ID, "R::PACKET_ID");
    static_assert(repeatsize == 1, "R::NetType");

    typename H::NetType net_head;
    // since it's already allocated, it can't overflow address space
    size_t total_length = sizeof(typename H::NetType) + (repeat.size() + 1) * sizeof(VarStringNetType);
    head.magic_packet_length = total_length;
    if (head.magic_packet_length != total_length)
    {
        s->set_eof();
        return;
    }
    // TODO potentially avoid the allocation
    std::vector<VarStringNetType> net_repeat(repeat.size() + 1);
    if (!native_to_network(&net_head, head))
    {
        s->set_eof();
        return;
    }
    for (size_t i = 0; i < repeat.size(); ++i)
    {
        net_repeat[i].c = repeat[i];
    }
    net_repeat[repeat.size()].c = '\0';
    SendResult rv = net_send_vpacket(s, net_head, net_repeat);
    if (rv != SendResult::Success)
        s->set_eof();
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize, class H>
__attribute__((warn_unused_result))
RecvResult recv_vpacket(Session *s, H& head, AString& repeat)
{
    static_assert(id == H::PACKET_ID, "H::PACKET_ID");
    static_assert(headsize == sizeof(typename H::NetType), "H::NetType");
    //static_assert(id == R::PACKET_ID, "R::PACKET_ID");
    static_assert(repeatsize == 1, "R::NetType");

    typename H::NetType net_head;
    std::vector<VarStringNetType> net_repeat;
    RecvResult rv = net_recv_vpacket<H>(s, net_head, net_repeat);
    assert (head.magic_packet_id == H::PACKET_ID);
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

// TODO make this go away with template specialization

template<uint16_t PKT_ID>
struct NetCommonPacketHead
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};

template<uint16_t PKT_ID>
struct CommonPacketHead
{
    using NetType = NetCommonPacketHead<PKT_ID>;
    static const uint16_t PACKET_ID = PKT_ID;

    uint16_t magic_packet_id = PACKET_ID;
    uint16_t magic_packet_length;
};

template<uint16_t PKT_ID>
bool native_to_network(NetCommonPacketHead<PKT_ID> *net, CommonPacketHead<PKT_ID> nat)
{
    return native_to_network(&net->magic_packet_id, nat.magic_packet_id)
        && native_to_network(&net->magic_packet_length, nat.magic_packet_length);
}

template<uint16_t PKT_ID>
bool network_to_native(CommonPacketHead<PKT_ID> *nat, NetCommonPacketHead<PKT_ID> net)
{
    return network_to_native(&nat->magic_packet_id, net.magic_packet_id)
        && network_to_native(&nat->magic_packet_length, net.magic_packet_length);
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize, class R>
void send_packet_repeatonly(Session *s, const std::vector<R>& v)
{
    static_assert(headsize == 4, "repeat headsize");
    static_assert(id == R::PACKET_ID, "R::PACKET_ID");
    static_assert(repeatsize == sizeof(typename R::NetType), "R::NetType");

    CommonPacketHead<R::PACKET_ID> head;
    send_vpacket<id, 4, repeatsize>(s, head, v);
}

template<uint16_t id, uint16_t headsize, uint16_t repeatsize, class R>
__attribute__((warn_unused_result))
RecvResult recv_packet_repeatonly(Session *s, std::vector<R>& v)
{
    static_assert(headsize == 4, "repeat headsize");
    static_assert(id == R::PACKET_ID, "R::PACKET_ID");
    static_assert(repeatsize == sizeof(typename R::NetType), "R::NetType");

    CommonPacketHead<R::PACKET_ID> head;
    return recv_vpacket<id, 4, repeatsize>(s, head, v);
}

#endif // TMWA_NET_PACKETS_HPP
