#ifndef TMWA_PROTO2_FWD_HPP
#define TMWA_PROTO2_FWD_HPP
//    proto2/fwd.hpp - Forward declarations of network packets
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

# include "../sanity.hpp"

# include <cstdint>

template<uint16_t PACKET_ID> class Packet_Fixed;
template<uint16_t PACKET_ID> class NetPacket_Fixed;
template<uint16_t PACKET_ID> class Packet_Head;
template<uint16_t PACKET_ID> class NetPacket_Head;
template<uint16_t PACKET_ID> class Packet_Repeat;
template<uint16_t PACKET_ID> class NetPacket_Repeat;

template<>
struct Packet_Fixed<0x2709>;
template<>
struct NetPacket_Fixed<0x2709>;
template<>
struct Packet_Fixed<0x2712>;
template<>
struct NetPacket_Fixed<0x2712>;
template<>
struct Packet_Fixed<0x2713>;
template<>
struct NetPacket_Fixed<0x2713>;
template<>
struct Packet_Fixed<0x2714>;
template<>
struct NetPacket_Fixed<0x2714>;
template<>
struct Packet_Fixed<0x2715>;
template<>
struct NetPacket_Fixed<0x2715>;
template<>
struct Packet_Fixed<0x2716>;
template<>
struct NetPacket_Fixed<0x2716>;
template<>
struct Packet_Fixed<0x2717>;
template<>
struct NetPacket_Fixed<0x2717>;
template<>
struct Packet_Head<0x2720>;
template<>
struct NetPacket_Head<0x2720>;
template<>
struct Packet_Repeat<0x2720>;
template<>
struct NetPacket_Repeat<0x2720>;
template<>
struct Packet_Fixed<0x2721>;
template<>
struct NetPacket_Fixed<0x2721>;
template<>
struct Packet_Fixed<0x2722>;
template<>
struct NetPacket_Fixed<0x2722>;
template<>
struct Packet_Fixed<0x2723>;
template<>
struct NetPacket_Fixed<0x2723>;
template<>
struct Packet_Fixed<0x2724>;
template<>
struct NetPacket_Fixed<0x2724>;
template<>
struct Packet_Fixed<0x2725>;
template<>
struct NetPacket_Fixed<0x2725>;
template<>
struct Packet_Fixed<0x2727>;
template<>
struct NetPacket_Fixed<0x2727>;
template<>
struct Packet_Head<0x2728>;
template<>
struct NetPacket_Head<0x2728>;
template<>
struct Packet_Repeat<0x2728>;
template<>
struct NetPacket_Repeat<0x2728>;
template<>
struct Packet_Head<0x2729>;
template<>
struct NetPacket_Head<0x2729>;
template<>
struct Packet_Repeat<0x2729>;
template<>
struct NetPacket_Repeat<0x2729>;
template<>
struct Packet_Fixed<0x272a>;
template<>
struct NetPacket_Fixed<0x272a>;
template<>
struct Packet_Fixed<0x2730>;
template<>
struct NetPacket_Fixed<0x2730>;
template<>
struct Packet_Fixed<0x2731>;
template<>
struct NetPacket_Fixed<0x2731>;
template<>
struct Packet_Head<0x2732>;
template<>
struct NetPacket_Head<0x2732>;
template<>
struct Packet_Repeat<0x2732>;
template<>
struct NetPacket_Repeat<0x2732>;
template<>
struct Packet_Fixed<0x2740>;
template<>
struct NetPacket_Fixed<0x2740>;
template<>
struct Packet_Fixed<0x2741>;
template<>
struct NetPacket_Fixed<0x2741>;

template<>
struct Packet_Head<0x2726>;
template<>
struct NetPacket_Head<0x2726>;
template<>
struct Packet_Repeat<0x2726>;
template<>
struct NetPacket_Repeat<0x2726>;
template<>
struct Packet_Fixed<0x7920>;
template<>
struct NetPacket_Fixed<0x7920>;
template<>
struct Packet_Head<0x7921>;
template<>
struct NetPacket_Head<0x7921>;
template<>
struct Packet_Repeat<0x7921>;
template<>
struct NetPacket_Repeat<0x7921>;
template<>
struct Packet_Fixed<0x7924>;
template<>
struct NetPacket_Fixed<0x7924>;
template<>
struct Packet_Fixed<0x7925>;
template<>
struct NetPacket_Fixed<0x7925>;
template<>
struct Packet_Fixed<0x7930>;
template<>
struct NetPacket_Fixed<0x7930>;
template<>
struct Packet_Fixed<0x7931>;
template<>
struct NetPacket_Fixed<0x7931>;
template<>
struct Packet_Fixed<0x7932>;
template<>
struct NetPacket_Fixed<0x7932>;
template<>
struct Packet_Fixed<0x7933>;
template<>
struct NetPacket_Fixed<0x7933>;
template<>
struct Packet_Fixed<0x7934>;
template<>
struct NetPacket_Fixed<0x7934>;
template<>
struct Packet_Fixed<0x7935>;
template<>
struct NetPacket_Fixed<0x7935>;
template<>
struct Packet_Fixed<0x7936>;
template<>
struct NetPacket_Fixed<0x7936>;
template<>
struct Packet_Fixed<0x7937>;
template<>
struct NetPacket_Fixed<0x7937>;
template<>
struct Packet_Fixed<0x7938>;
template<>
struct NetPacket_Fixed<0x7938>;
template<>
struct Packet_Head<0x7939>;
template<>
struct NetPacket_Head<0x7939>;
template<>
struct Packet_Repeat<0x7939>;
template<>
struct NetPacket_Repeat<0x7939>;
template<>
struct Packet_Fixed<0x793a>;
template<>
struct NetPacket_Fixed<0x793a>;
template<>
struct Packet_Fixed<0x793b>;
template<>
struct NetPacket_Fixed<0x793b>;
template<>
struct Packet_Fixed<0x793c>;
template<>
struct NetPacket_Fixed<0x793c>;
template<>
struct Packet_Fixed<0x793d>;
template<>
struct NetPacket_Fixed<0x793d>;
template<>
struct Packet_Fixed<0x793e>;
template<>
struct NetPacket_Fixed<0x793e>;
template<>
struct Packet_Fixed<0x793f>;
template<>
struct NetPacket_Fixed<0x793f>;
template<>
struct Packet_Fixed<0x7940>;
template<>
struct NetPacket_Fixed<0x7940>;
template<>
struct Packet_Fixed<0x7941>;
template<>
struct NetPacket_Fixed<0x7941>;
template<>
struct Packet_Head<0x7942>;
template<>
struct NetPacket_Head<0x7942>;
template<>
struct Packet_Repeat<0x7942>;
template<>
struct NetPacket_Repeat<0x7942>;
template<>
struct Packet_Fixed<0x7943>;
template<>
struct NetPacket_Fixed<0x7943>;
template<>
struct Packet_Fixed<0x7944>;
template<>
struct NetPacket_Fixed<0x7944>;
template<>
struct Packet_Fixed<0x7945>;
template<>
struct NetPacket_Fixed<0x7945>;
template<>
struct Packet_Fixed<0x7946>;
template<>
struct NetPacket_Fixed<0x7946>;
template<>
struct Packet_Fixed<0x7947>;
template<>
struct NetPacket_Fixed<0x7947>;
template<>
struct Packet_Fixed<0x7948>;
template<>
struct NetPacket_Fixed<0x7948>;
template<>
struct Packet_Fixed<0x7949>;
template<>
struct NetPacket_Fixed<0x7949>;
template<>
struct Packet_Fixed<0x794a>;
template<>
struct NetPacket_Fixed<0x794a>;
template<>
struct Packet_Fixed<0x794b>;
template<>
struct NetPacket_Fixed<0x794b>;
template<>
struct Packet_Fixed<0x794c>;
template<>
struct NetPacket_Fixed<0x794c>;
template<>
struct Packet_Fixed<0x794d>;
template<>
struct NetPacket_Fixed<0x794d>;
template<>
struct Packet_Head<0x794e>;
template<>
struct NetPacket_Head<0x794e>;
template<>
struct Packet_Repeat<0x794e>;
template<>
struct NetPacket_Repeat<0x794e>;
template<>
struct Packet_Fixed<0x794f>;
template<>
struct NetPacket_Fixed<0x794f>;
template<>
struct Packet_Fixed<0x7950>;
template<>
struct NetPacket_Fixed<0x7950>;
template<>
struct Packet_Fixed<0x7951>;
template<>
struct NetPacket_Fixed<0x7951>;
template<>
struct Packet_Fixed<0x7952>;
template<>
struct NetPacket_Fixed<0x7952>;
template<>
struct Packet_Head<0x7953>;
template<>
struct NetPacket_Head<0x7953>;
template<>
struct Packet_Repeat<0x7953>;
template<>
struct NetPacket_Repeat<0x7953>;
template<>
struct Packet_Fixed<0x7954>;
template<>
struct NetPacket_Fixed<0x7954>;
template<>
struct Packet_Fixed<0x7955>;
template<>
struct NetPacket_Fixed<0x7955>;




template<>
struct Packet_Fixed<0x0212>;
template<>
struct NetPacket_Fixed<0x0212>;

template<>
struct Packet_Fixed<0x7530>;
template<>
struct NetPacket_Fixed<0x7530>;
template<>
struct Packet_Fixed<0x7531>;
template<>
struct NetPacket_Fixed<0x7531>;
template<>
struct Packet_Fixed<0x7532>;
template<>
struct NetPacket_Fixed<0x7532>;


#endif // TMWA_PROTO2_FWD_HPP
