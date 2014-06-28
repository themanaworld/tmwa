#pragma once
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

#include "../sanity.hpp"

#include <cstdint>

namespace tmwa
{
template<uint16_t PACKET_ID> class Packet_Fixed;
template<uint16_t PACKET_ID> class NetPacket_Fixed;
template<uint16_t PACKET_ID> class Packet_Payload;
template<uint16_t PACKET_ID> class NetPacket_Payload;
template<uint16_t PACKET_ID> class Packet_Head;
template<uint16_t PACKET_ID> class NetPacket_Head;
template<uint16_t PACKET_ID> class Packet_Repeat;
template<uint16_t PACKET_ID> class NetPacket_Repeat;
template<uint16_t PACKET_ID> class Packet_Option;
template<uint16_t PACKET_ID> class NetPacket_Option;

template<>
struct Packet_Fixed<0x2709>;
template<>
struct NetPacket_Fixed<0x2709>;

template<>
struct Packet_Fixed<0x2710>;
template<>
struct NetPacket_Fixed<0x2710>;

template<>
struct Packet_Fixed<0x2711>;
template<>
struct NetPacket_Fixed<0x2711>;

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
struct Packet_Fixed<0x7918>;
template<>
struct NetPacket_Fixed<0x7918>;

template<>
struct Packet_Fixed<0x7919>;
template<>
struct NetPacket_Fixed<0x7919>;

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
struct Packet_Head<0x0063>;
template<>
struct NetPacket_Head<0x0063>;
template<>
struct Packet_Repeat<0x0063>;
template<>
struct NetPacket_Repeat<0x0063>;

template<>
struct Packet_Fixed<0x0064>;
template<>
struct NetPacket_Fixed<0x0064>;

template<>
struct Packet_Head<0x0069>;
template<>
struct NetPacket_Head<0x0069>;
template<>
struct Packet_Repeat<0x0069>;
template<>
struct NetPacket_Repeat<0x0069>;

template<>
struct Packet_Fixed<0x006a>;
template<>
struct NetPacket_Fixed<0x006a>;


template<>
struct Packet_Fixed<0x2af7>;
template<>
struct NetPacket_Fixed<0x2af7>;

template<>
struct Packet_Fixed<0x2af8>;
template<>
struct NetPacket_Fixed<0x2af8>;

template<>
struct Packet_Fixed<0x2af9>;
template<>
struct NetPacket_Fixed<0x2af9>;

template<>
struct Packet_Head<0x2afa>;
template<>
struct NetPacket_Head<0x2afa>;
template<>
struct Packet_Repeat<0x2afa>;
template<>
struct NetPacket_Repeat<0x2afa>;

template<>
struct Packet_Fixed<0x2afa>;
template<>
struct NetPacket_Fixed<0x2afa>;

template<>
struct Packet_Fixed<0x2afb>;
template<>
struct NetPacket_Fixed<0x2afb>;

template<>
struct Packet_Fixed<0x2afc>;
template<>
struct NetPacket_Fixed<0x2afc>;

template<>
struct Packet_Payload<0x2afd>;
template<>
struct NetPacket_Payload<0x2afd>;

template<>
struct Packet_Fixed<0x2afe>;
template<>
struct NetPacket_Fixed<0x2afe>;

template<>
struct Packet_Head<0x2aff>;
template<>
struct NetPacket_Head<0x2aff>;
template<>
struct Packet_Repeat<0x2aff>;
template<>
struct NetPacket_Repeat<0x2aff>;

template<>
struct Packet_Fixed<0x2b00>;
template<>
struct NetPacket_Fixed<0x2b00>;

template<>
struct Packet_Payload<0x2b01>;
template<>
struct NetPacket_Payload<0x2b01>;

template<>
struct Packet_Fixed<0x2b02>;
template<>
struct NetPacket_Fixed<0x2b02>;

template<>
struct Packet_Fixed<0x2b03>;
template<>
struct NetPacket_Fixed<0x2b03>;

template<>
struct Packet_Head<0x2b04>;
template<>
struct NetPacket_Head<0x2b04>;
template<>
struct Packet_Repeat<0x2b04>;
template<>
struct NetPacket_Repeat<0x2b04>;

template<>
struct Packet_Fixed<0x2b05>;
template<>
struct NetPacket_Fixed<0x2b05>;

template<>
struct Packet_Fixed<0x2b06>;
template<>
struct NetPacket_Fixed<0x2b06>;

template<>
struct Packet_Head<0x2b0a>;
template<>
struct NetPacket_Head<0x2b0a>;
template<>
struct Packet_Repeat<0x2b0a>;
template<>
struct NetPacket_Repeat<0x2b0a>;

template<>
struct Packet_Fixed<0x2b0b>;
template<>
struct NetPacket_Fixed<0x2b0b>;

template<>
struct Packet_Fixed<0x2b0c>;
template<>
struct NetPacket_Fixed<0x2b0c>;

template<>
struct Packet_Fixed<0x2b0d>;
template<>
struct NetPacket_Fixed<0x2b0d>;

template<>
struct Packet_Fixed<0x2b0e>;
template<>
struct NetPacket_Fixed<0x2b0e>;

template<>
struct Packet_Fixed<0x2b0f>;
template<>
struct NetPacket_Fixed<0x2b0f>;

template<>
struct Packet_Head<0x2b10>;
template<>
struct NetPacket_Head<0x2b10>;
template<>
struct Packet_Repeat<0x2b10>;
template<>
struct NetPacket_Repeat<0x2b10>;

template<>
struct Packet_Head<0x2b11>;
template<>
struct NetPacket_Head<0x2b11>;
template<>
struct Packet_Repeat<0x2b11>;
template<>
struct NetPacket_Repeat<0x2b11>;

template<>
struct Packet_Fixed<0x2b12>;
template<>
struct NetPacket_Fixed<0x2b12>;

template<>
struct Packet_Fixed<0x2b13>;
template<>
struct NetPacket_Fixed<0x2b13>;

template<>
struct Packet_Fixed<0x2b14>;
template<>
struct NetPacket_Fixed<0x2b14>;

template<>
struct Packet_Head<0x2b15>;
template<>
struct NetPacket_Head<0x2b15>;
template<>
struct Packet_Repeat<0x2b15>;
template<>
struct NetPacket_Repeat<0x2b15>;

template<>
struct Packet_Fixed<0x2b16>;
template<>
struct NetPacket_Fixed<0x2b16>;

template<>
struct Packet_Head<0x3000>;
template<>
struct NetPacket_Head<0x3000>;
template<>
struct Packet_Repeat<0x3000>;
template<>
struct NetPacket_Repeat<0x3000>;

template<>
struct Packet_Head<0x3001>;
template<>
struct NetPacket_Head<0x3001>;
template<>
struct Packet_Repeat<0x3001>;
template<>
struct NetPacket_Repeat<0x3001>;

template<>
struct Packet_Fixed<0x3002>;
template<>
struct NetPacket_Fixed<0x3002>;

template<>
struct Packet_Head<0x3003>;
template<>
struct NetPacket_Head<0x3003>;
template<>
struct Packet_Repeat<0x3003>;
template<>
struct NetPacket_Repeat<0x3003>;

template<>
struct Packet_Head<0x3004>;
template<>
struct NetPacket_Head<0x3004>;
template<>
struct Packet_Repeat<0x3004>;
template<>
struct NetPacket_Repeat<0x3004>;

template<>
struct Packet_Fixed<0x3005>;
template<>
struct NetPacket_Fixed<0x3005>;

template<>
struct Packet_Fixed<0x3010>;
template<>
struct NetPacket_Fixed<0x3010>;

template<>
struct Packet_Payload<0x3011>;
template<>
struct NetPacket_Payload<0x3011>;

template<>
struct Packet_Fixed<0x3020>;
template<>
struct NetPacket_Fixed<0x3020>;

template<>
struct Packet_Fixed<0x3021>;
template<>
struct NetPacket_Fixed<0x3021>;

template<>
struct Packet_Fixed<0x3022>;
template<>
struct NetPacket_Fixed<0x3022>;

template<>
struct Packet_Fixed<0x3023>;
template<>
struct NetPacket_Fixed<0x3023>;

template<>
struct Packet_Fixed<0x3024>;
template<>
struct NetPacket_Fixed<0x3024>;

template<>
struct Packet_Fixed<0x3025>;
template<>
struct NetPacket_Fixed<0x3025>;

template<>
struct Packet_Fixed<0x3026>;
template<>
struct NetPacket_Fixed<0x3026>;

template<>
struct Packet_Head<0x3027>;
template<>
struct NetPacket_Head<0x3027>;
template<>
struct Packet_Repeat<0x3027>;
template<>
struct NetPacket_Repeat<0x3027>;

template<>
struct Packet_Fixed<0x3028>;
template<>
struct NetPacket_Fixed<0x3028>;

template<>
struct Packet_Head<0x3800>;
template<>
struct NetPacket_Head<0x3800>;
template<>
struct Packet_Repeat<0x3800>;
template<>
struct NetPacket_Repeat<0x3800>;

template<>
struct Packet_Head<0x3801>;
template<>
struct NetPacket_Head<0x3801>;
template<>
struct Packet_Repeat<0x3801>;
template<>
struct NetPacket_Repeat<0x3801>;

template<>
struct Packet_Fixed<0x3802>;
template<>
struct NetPacket_Fixed<0x3802>;

template<>
struct Packet_Head<0x3803>;
template<>
struct NetPacket_Head<0x3803>;
template<>
struct Packet_Repeat<0x3803>;
template<>
struct NetPacket_Repeat<0x3803>;

template<>
struct Packet_Head<0x3804>;
template<>
struct NetPacket_Head<0x3804>;
template<>
struct Packet_Repeat<0x3804>;
template<>
struct NetPacket_Repeat<0x3804>;

template<>
struct Packet_Payload<0x3810>;
template<>
struct NetPacket_Payload<0x3810>;

template<>
struct Packet_Fixed<0x3811>;
template<>
struct NetPacket_Fixed<0x3811>;

template<>
struct Packet_Fixed<0x3820>;
template<>
struct NetPacket_Fixed<0x3820>;

template<>
struct Packet_Head<0x3821>;
template<>
struct NetPacket_Head<0x3821>;
template<>
struct Packet_Option<0x3821>;
template<>
struct NetPacket_Option<0x3821>;

template<>
struct Packet_Fixed<0x3822>;
template<>
struct NetPacket_Fixed<0x3822>;

template<>
struct Packet_Fixed<0x3823>;
template<>
struct NetPacket_Fixed<0x3823>;

template<>
struct Packet_Fixed<0x3824>;
template<>
struct NetPacket_Fixed<0x3824>;

template<>
struct Packet_Fixed<0x3825>;
template<>
struct NetPacket_Fixed<0x3825>;

template<>
struct Packet_Fixed<0x3826>;
template<>
struct NetPacket_Fixed<0x3826>;

template<>
struct Packet_Head<0x3827>;
template<>
struct NetPacket_Head<0x3827>;
template<>
struct Packet_Repeat<0x3827>;
template<>
struct NetPacket_Repeat<0x3827>;


template<>
struct Packet_Fixed<0x0061>;
template<>
struct NetPacket_Fixed<0x0061>;

template<>
struct Packet_Fixed<0x0062>;
template<>
struct NetPacket_Fixed<0x0062>;

template<>
struct Packet_Fixed<0x0065>;
template<>
struct NetPacket_Fixed<0x0065>;

template<>
struct Packet_Fixed<0x0066>;
template<>
struct NetPacket_Fixed<0x0066>;

template<>
struct Packet_Fixed<0x0067>;
template<>
struct NetPacket_Fixed<0x0067>;

template<>
struct Packet_Fixed<0x0068>;
template<>
struct NetPacket_Fixed<0x0068>;

template<>
struct Packet_Head<0x006b>;
template<>
struct NetPacket_Head<0x006b>;
template<>
struct Packet_Repeat<0x006b>;
template<>
struct NetPacket_Repeat<0x006b>;

template<>
struct Packet_Fixed<0x006c>;
template<>
struct NetPacket_Fixed<0x006c>;

template<>
struct Packet_Fixed<0x006d>;
template<>
struct NetPacket_Fixed<0x006d>;

template<>
struct Packet_Fixed<0x006e>;
template<>
struct NetPacket_Fixed<0x006e>;

template<>
struct Packet_Fixed<0x006f>;
template<>
struct NetPacket_Fixed<0x006f>;

template<>
struct Packet_Fixed<0x0070>;
template<>
struct NetPacket_Fixed<0x0070>;

template<>
struct Packet_Fixed<0x0071>;
template<>
struct NetPacket_Fixed<0x0071>;


template<>
struct Packet_Fixed<0x0072>;
template<>
struct NetPacket_Fixed<0x0072>;

template<>
struct Packet_Fixed<0x0073>;
template<>
struct NetPacket_Fixed<0x0073>;

template<>
struct Packet_Fixed<0x0078>;
template<>
struct NetPacket_Fixed<0x0078>;

template<>
struct Packet_Fixed<0x007b>;
template<>
struct NetPacket_Fixed<0x007b>;

template<>
struct Packet_Fixed<0x007c>;
template<>
struct NetPacket_Fixed<0x007c>;

template<>
struct Packet_Fixed<0x007d>;
template<>
struct NetPacket_Fixed<0x007d>;

template<>
struct Packet_Fixed<0x007e>;
template<>
struct NetPacket_Fixed<0x007e>;

template<>
struct Packet_Fixed<0x007f>;
template<>
struct NetPacket_Fixed<0x007f>;

template<>
struct Packet_Fixed<0x0080>;
template<>
struct NetPacket_Fixed<0x0080>;

template<>
struct Packet_Fixed<0x0085>;
template<>
struct NetPacket_Fixed<0x0085>;

template<>
struct Packet_Fixed<0x0087>;
template<>
struct NetPacket_Fixed<0x0087>;

template<>
struct Packet_Fixed<0x0088>;
template<>
struct NetPacket_Fixed<0x0088>;

template<>
struct Packet_Fixed<0x0089>;
template<>
struct NetPacket_Fixed<0x0089>;

template<>
struct Packet_Fixed<0x008a>;
template<>
struct NetPacket_Fixed<0x008a>;

template<>
struct Packet_Head<0x008c>;
template<>
struct NetPacket_Head<0x008c>;
template<>
struct Packet_Repeat<0x008c>;
template<>
struct NetPacket_Repeat<0x008c>;

template<>
struct Packet_Head<0x008d>;
template<>
struct NetPacket_Head<0x008d>;
template<>
struct Packet_Repeat<0x008d>;
template<>
struct NetPacket_Repeat<0x008d>;

template<>
struct Packet_Head<0x008e>;
template<>
struct NetPacket_Head<0x008e>;
template<>
struct Packet_Repeat<0x008e>;
template<>
struct NetPacket_Repeat<0x008e>;

template<>
struct Packet_Fixed<0x0090>;
template<>
struct NetPacket_Fixed<0x0090>;

template<>
struct Packet_Fixed<0x0091>;
template<>
struct NetPacket_Fixed<0x0091>;

template<>
struct Packet_Fixed<0x0092>;
template<>
struct NetPacket_Fixed<0x0092>;

template<>
struct Packet_Fixed<0x0094>;
template<>
struct NetPacket_Fixed<0x0094>;

template<>
struct Packet_Fixed<0x0095>;
template<>
struct NetPacket_Fixed<0x0095>;

template<>
struct Packet_Head<0x0096>;
template<>
struct NetPacket_Head<0x0096>;
template<>
struct Packet_Repeat<0x0096>;
template<>
struct NetPacket_Repeat<0x0096>;

template<>
struct Packet_Head<0x0097>;
template<>
struct NetPacket_Head<0x0097>;
template<>
struct Packet_Repeat<0x0097>;
template<>
struct NetPacket_Repeat<0x0097>;

template<>
struct Packet_Fixed<0x0098>;
template<>
struct NetPacket_Fixed<0x0098>;

template<>
struct Packet_Head<0x009a>;
template<>
struct NetPacket_Head<0x009a>;
template<>
struct Packet_Repeat<0x009a>;
template<>
struct NetPacket_Repeat<0x009a>;

template<>
struct Packet_Fixed<0x009b>;
template<>
struct NetPacket_Fixed<0x009b>;

template<>
struct Packet_Fixed<0x009c>;
template<>
struct NetPacket_Fixed<0x009c>;

template<>
struct Packet_Fixed<0x009d>;
template<>
struct NetPacket_Fixed<0x009d>;

template<>
struct Packet_Fixed<0x009e>;
template<>
struct NetPacket_Fixed<0x009e>;

template<>
struct Packet_Fixed<0x009f>;
template<>
struct NetPacket_Fixed<0x009f>;

template<>
struct Packet_Fixed<0x00a0>;
template<>
struct NetPacket_Fixed<0x00a0>;

template<>
struct Packet_Fixed<0x00a1>;
template<>
struct NetPacket_Fixed<0x00a1>;

template<>
struct Packet_Fixed<0x00a2>;
template<>
struct NetPacket_Fixed<0x00a2>;

template<>
struct Packet_Head<0x00a4>;
template<>
struct NetPacket_Head<0x00a4>;
template<>
struct Packet_Repeat<0x00a4>;
template<>
struct NetPacket_Repeat<0x00a4>;

template<>
struct Packet_Head<0x00a6>;
template<>
struct NetPacket_Head<0x00a6>;
template<>
struct Packet_Repeat<0x00a6>;
template<>
struct NetPacket_Repeat<0x00a6>;

template<>
struct Packet_Fixed<0x00a7>;
template<>
struct NetPacket_Fixed<0x00a7>;

template<>
struct Packet_Fixed<0x00a8>;
template<>
struct NetPacket_Fixed<0x00a8>;

template<>
struct Packet_Fixed<0x00a9>;
template<>
struct NetPacket_Fixed<0x00a9>;

template<>
struct Packet_Fixed<0x00aa>;
template<>
struct NetPacket_Fixed<0x00aa>;

template<>
struct Packet_Fixed<0x00ab>;
template<>
struct NetPacket_Fixed<0x00ab>;

template<>
struct Packet_Fixed<0x00ac>;
template<>
struct NetPacket_Fixed<0x00ac>;

template<>
struct Packet_Fixed<0x00af>;
template<>
struct NetPacket_Fixed<0x00af>;

template<>
struct Packet_Fixed<0x00b0>;
template<>
struct NetPacket_Fixed<0x00b0>;

template<>
struct Packet_Fixed<0x00b1>;
template<>
struct NetPacket_Fixed<0x00b1>;

template<>
struct Packet_Fixed<0x00b2>;
template<>
struct NetPacket_Fixed<0x00b2>;

template<>
struct Packet_Fixed<0x00b3>;
template<>
struct NetPacket_Fixed<0x00b3>;

template<>
struct Packet_Head<0x00b4>;
template<>
struct NetPacket_Head<0x00b4>;
template<>
struct Packet_Repeat<0x00b4>;
template<>
struct NetPacket_Repeat<0x00b4>;

template<>
struct Packet_Fixed<0x00b5>;
template<>
struct NetPacket_Fixed<0x00b5>;

template<>
struct Packet_Fixed<0x00b6>;
template<>
struct NetPacket_Fixed<0x00b6>;

template<>
struct Packet_Head<0x00b7>;
template<>
struct NetPacket_Head<0x00b7>;
template<>
struct Packet_Repeat<0x00b7>;
template<>
struct NetPacket_Repeat<0x00b7>;

template<>
struct Packet_Fixed<0x00b8>;
template<>
struct NetPacket_Fixed<0x00b8>;

template<>
struct Packet_Fixed<0x00b9>;
template<>
struct NetPacket_Fixed<0x00b9>;

template<>
struct Packet_Fixed<0x00bb>;
template<>
struct NetPacket_Fixed<0x00bb>;

template<>
struct Packet_Fixed<0x00bc>;
template<>
struct NetPacket_Fixed<0x00bc>;

template<>
struct Packet_Fixed<0x00bd>;
template<>
struct NetPacket_Fixed<0x00bd>;

template<>
struct Packet_Fixed<0x00be>;
template<>
struct NetPacket_Fixed<0x00be>;

template<>
struct Packet_Fixed<0x00bf>;
template<>
struct NetPacket_Fixed<0x00bf>;

template<>
struct Packet_Fixed<0x00c0>;
template<>
struct NetPacket_Fixed<0x00c0>;

template<>
struct Packet_Fixed<0x00c1>;
template<>
struct NetPacket_Fixed<0x00c1>;

template<>
struct Packet_Fixed<0x00c2>;
template<>
struct NetPacket_Fixed<0x00c2>;

template<>
struct Packet_Fixed<0x00c4>;
template<>
struct NetPacket_Fixed<0x00c4>;

template<>
struct Packet_Fixed<0x00c5>;
template<>
struct NetPacket_Fixed<0x00c5>;

template<>
struct Packet_Head<0x00c6>;
template<>
struct NetPacket_Head<0x00c6>;
template<>
struct Packet_Repeat<0x00c6>;
template<>
struct NetPacket_Repeat<0x00c6>;

template<>
struct Packet_Head<0x00c7>;
template<>
struct NetPacket_Head<0x00c7>;
template<>
struct Packet_Repeat<0x00c7>;
template<>
struct NetPacket_Repeat<0x00c7>;

template<>
struct Packet_Head<0x00c8>;
template<>
struct NetPacket_Head<0x00c8>;
template<>
struct Packet_Repeat<0x00c8>;
template<>
struct NetPacket_Repeat<0x00c8>;

template<>
struct Packet_Head<0x00c9>;
template<>
struct NetPacket_Head<0x00c9>;
template<>
struct Packet_Repeat<0x00c9>;
template<>
struct NetPacket_Repeat<0x00c9>;

template<>
struct Packet_Fixed<0x00ca>;
template<>
struct NetPacket_Fixed<0x00ca>;

template<>
struct Packet_Fixed<0x00cb>;
template<>
struct NetPacket_Fixed<0x00cb>;

template<>
struct Packet_Fixed<0x00cd>;
template<>
struct NetPacket_Fixed<0x00cd>;

template<>
struct Packet_Fixed<0x00e4>;
template<>
struct NetPacket_Fixed<0x00e4>;

template<>
struct Packet_Fixed<0x00e5>;
template<>
struct NetPacket_Fixed<0x00e5>;

template<>
struct Packet_Fixed<0x00e6>;
template<>
struct NetPacket_Fixed<0x00e6>;

template<>
struct Packet_Fixed<0x00e7>;
template<>
struct NetPacket_Fixed<0x00e7>;

template<>
struct Packet_Fixed<0x00e8>;
template<>
struct NetPacket_Fixed<0x00e8>;

template<>
struct Packet_Fixed<0x00e9>;
template<>
struct NetPacket_Fixed<0x00e9>;

template<>
struct Packet_Fixed<0x00eb>;
template<>
struct NetPacket_Fixed<0x00eb>;

template<>
struct Packet_Fixed<0x00ec>;
template<>
struct NetPacket_Fixed<0x00ec>;

template<>
struct Packet_Fixed<0x00ed>;
template<>
struct NetPacket_Fixed<0x00ed>;

template<>
struct Packet_Fixed<0x00ee>;
template<>
struct NetPacket_Fixed<0x00ee>;

template<>
struct Packet_Fixed<0x00ef>;
template<>
struct NetPacket_Fixed<0x00ef>;

template<>
struct Packet_Fixed<0x00f0>;
template<>
struct NetPacket_Fixed<0x00f0>;

template<>
struct Packet_Fixed<0x00f2>;
template<>
struct NetPacket_Fixed<0x00f2>;

template<>
struct Packet_Fixed<0x00f3>;
template<>
struct NetPacket_Fixed<0x00f3>;

template<>
struct Packet_Fixed<0x00f4>;
template<>
struct NetPacket_Fixed<0x00f4>;

template<>
struct Packet_Fixed<0x00f5>;
template<>
struct NetPacket_Fixed<0x00f5>;

template<>
struct Packet_Fixed<0x00f6>;
template<>
struct NetPacket_Fixed<0x00f6>;

template<>
struct Packet_Fixed<0x00f7>;
template<>
struct NetPacket_Fixed<0x00f7>;

template<>
struct Packet_Fixed<0x00f8>;
template<>
struct NetPacket_Fixed<0x00f8>;

template<>
struct Packet_Fixed<0x00f9>;
template<>
struct NetPacket_Fixed<0x00f9>;

template<>
struct Packet_Fixed<0x00fa>;
template<>
struct NetPacket_Fixed<0x00fa>;

template<>
struct Packet_Head<0x00fb>;
template<>
struct NetPacket_Head<0x00fb>;
template<>
struct Packet_Repeat<0x00fb>;
template<>
struct NetPacket_Repeat<0x00fb>;

template<>
struct Packet_Fixed<0x00fc>;
template<>
struct NetPacket_Fixed<0x00fc>;

template<>
struct Packet_Fixed<0x00fd>;
template<>
struct NetPacket_Fixed<0x00fd>;

template<>
struct Packet_Fixed<0x00fe>;
template<>
struct NetPacket_Fixed<0x00fe>;

template<>
struct Packet_Fixed<0x00ff>;
template<>
struct NetPacket_Fixed<0x00ff>;

template<>
struct Packet_Fixed<0x0100>;
template<>
struct NetPacket_Fixed<0x0100>;

template<>
struct Packet_Fixed<0x0101>;
template<>
struct NetPacket_Fixed<0x0101>;

template<>
struct Packet_Fixed<0x0102>;
template<>
struct NetPacket_Fixed<0x0102>;

template<>
struct Packet_Fixed<0x0103>;
template<>
struct NetPacket_Fixed<0x0103>;

template<>
struct Packet_Fixed<0x0105>;
template<>
struct NetPacket_Fixed<0x0105>;

template<>
struct Packet_Fixed<0x0106>;
template<>
struct NetPacket_Fixed<0x0106>;

template<>
struct Packet_Fixed<0x0107>;
template<>
struct NetPacket_Fixed<0x0107>;

template<>
struct Packet_Head<0x0108>;
template<>
struct NetPacket_Head<0x0108>;
template<>
struct Packet_Repeat<0x0108>;
template<>
struct NetPacket_Repeat<0x0108>;

template<>
struct Packet_Head<0x0109>;
template<>
struct NetPacket_Head<0x0109>;
template<>
struct Packet_Repeat<0x0109>;
template<>
struct NetPacket_Repeat<0x0109>;

template<>
struct Packet_Fixed<0x010c>;
template<>
struct NetPacket_Fixed<0x010c>;

template<>
struct Packet_Fixed<0x010e>;
template<>
struct NetPacket_Fixed<0x010e>;

template<>
struct Packet_Head<0x010f>;
template<>
struct NetPacket_Head<0x010f>;
template<>
struct Packet_Repeat<0x010f>;
template<>
struct NetPacket_Repeat<0x010f>;

template<>
struct Packet_Fixed<0x0110>;
template<>
struct NetPacket_Fixed<0x0110>;

template<>
struct Packet_Fixed<0x0112>;
template<>
struct NetPacket_Fixed<0x0112>;

template<>
struct Packet_Fixed<0x0118>;
template<>
struct NetPacket_Fixed<0x0118>;

template<>
struct Packet_Fixed<0x0119>;
template<>
struct NetPacket_Fixed<0x0119>;

template<>
struct Packet_Fixed<0x0139>;
template<>
struct NetPacket_Fixed<0x0139>;

template<>
struct Packet_Fixed<0x013a>;
template<>
struct NetPacket_Fixed<0x013a>;

template<>
struct Packet_Fixed<0x013b>;
template<>
struct NetPacket_Fixed<0x013b>;

template<>
struct Packet_Fixed<0x013c>;
template<>
struct NetPacket_Fixed<0x013c>;

template<>
struct Packet_Fixed<0x0141>;
template<>
struct NetPacket_Fixed<0x0141>;

template<>
struct Packet_Fixed<0x0142>;
template<>
struct NetPacket_Fixed<0x0142>;

template<>
struct Packet_Fixed<0x0143>;
template<>
struct NetPacket_Fixed<0x0143>;

template<>
struct Packet_Fixed<0x0146>;
template<>
struct NetPacket_Fixed<0x0146>;

template<>
struct Packet_Fixed<0x0147>;
template<>
struct NetPacket_Fixed<0x0147>;

template<>
struct Packet_Fixed<0x0148>;
template<>
struct NetPacket_Fixed<0x0148>;

template<>
struct Packet_Fixed<0x014d>;
template<>
struct NetPacket_Fixed<0x014d>;

template<>
struct Packet_Fixed<0x018a>;
template<>
struct NetPacket_Fixed<0x018a>;

template<>
struct Packet_Fixed<0x018b>;
template<>
struct NetPacket_Fixed<0x018b>;

template<>
struct Packet_Fixed<0x0195>;
template<>
struct NetPacket_Fixed<0x0195>;

template<>
struct Packet_Fixed<0x0196>;
template<>
struct NetPacket_Fixed<0x0196>;

template<>
struct Packet_Fixed<0x019b>;
template<>
struct NetPacket_Fixed<0x019b>;

template<>
struct Packet_Fixed<0x01b1>;
template<>
struct NetPacket_Fixed<0x01b1>;

template<>
struct Packet_Fixed<0x01c8>;
template<>
struct NetPacket_Fixed<0x01c8>;

template<>
struct Packet_Fixed<0x01d4>;
template<>
struct NetPacket_Fixed<0x01d4>;

template<>
struct Packet_Head<0x01d5>;
template<>
struct NetPacket_Head<0x01d5>;
template<>
struct Packet_Repeat<0x01d5>;
template<>
struct NetPacket_Repeat<0x01d5>;

template<>
struct Packet_Fixed<0x01d7>;
template<>
struct NetPacket_Fixed<0x01d7>;

template<>
struct Packet_Fixed<0x01d8>;
template<>
struct NetPacket_Fixed<0x01d8>;

template<>
struct Packet_Fixed<0x01d9>;
template<>
struct NetPacket_Fixed<0x01d9>;

template<>
struct Packet_Fixed<0x01da>;
template<>
struct NetPacket_Fixed<0x01da>;

template<>
struct Packet_Fixed<0x01de>;
template<>
struct NetPacket_Fixed<0x01de>;

template<>
struct Packet_Head<0x01ee>;
template<>
struct NetPacket_Head<0x01ee>;
template<>
struct Packet_Repeat<0x01ee>;
template<>
struct NetPacket_Repeat<0x01ee>;

template<>
struct Packet_Head<0x01f0>;
template<>
struct NetPacket_Head<0x01f0>;
template<>
struct Packet_Repeat<0x01f0>;
template<>
struct NetPacket_Repeat<0x01f0>;

template<>
struct Packet_Fixed<0x020c>;
template<>
struct NetPacket_Fixed<0x020c>;

template<>
struct Packet_Fixed<0x0212>;
template<>
struct NetPacket_Fixed<0x0212>;


template<>
struct Packet_Fixed<0x0081>;
template<>
struct NetPacket_Fixed<0x0081>;

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

template<>
struct Packet_Payload<0x8000>;
template<>
struct NetPacket_Payload<0x8000>;


} // namespace tmwa
