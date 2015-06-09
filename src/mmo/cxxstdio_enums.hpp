#pragma once
//    cxxstdio_enums.hpp - Opt-in integer formatting support for enums.
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

#include <cstdint>

#include "../generic/enum.hpp"


namespace tmwa
{
namespace e
{
enum class EPOS : uint16_t;
enum class Opt0 : uint16_t;

inline
auto decay_for_printf(EPOS v) -> typename remove_enum<decltype(v)>::type { return typename remove_enum<decltype(v)>::type(v); }
inline
auto decay_for_printf(Opt0 v) -> typename remove_enum<decltype(v)>::type { return typename remove_enum<decltype(v)>::type(v); }
} // namespace e

enum class ItemLook : uint16_t;
enum class SP : uint16_t;
enum class SkillID : uint16_t;
enum class StatusChange : uint16_t;

inline
auto decay_for_printf(ItemLook v) -> typename remove_enum<decltype(v)>::type { return typename remove_enum<decltype(v)>::type(v); }
inline
auto decay_for_printf(SP v) -> typename remove_enum<decltype(v)>::type { return typename remove_enum<decltype(v)>::type(v); }
inline
auto decay_for_printf(SkillID v) -> typename remove_enum<decltype(v)>::type { return typename remove_enum<decltype(v)>::type(v); }
inline
auto decay_for_printf(StatusChange v) -> typename remove_enum<decltype(v)>::type { return typename remove_enum<decltype(v)>::type(v); }

namespace map
{
namespace e
{
enum class BF : uint16_t;
enum class MapCell : uint8_t;

inline
auto decay_for_printf(BF v) -> typename remove_enum<decltype(v)>::type { return typename remove_enum<decltype(v)>::type(v); }
inline
auto decay_for_printf(MapCell v) -> typename remove_enum<decltype(v)>::type { return typename remove_enum<decltype(v)>::type(v); }
} // namespace map::e

enum class BL : uint8_t;
enum class ByteCode : uint8_t;
enum class MS : uint8_t;

inline
auto decay_for_printf(BL v) -> typename remove_enum<decltype(v)>::type { return typename remove_enum<decltype(v)>::type(v); }
inline
auto decay_for_printf(ByteCode v) -> typename remove_enum<decltype(v)>::type { return typename remove_enum<decltype(v)>::type(v); }
inline
auto decay_for_printf(MS v) -> typename remove_enum<decltype(v)>::type { return typename remove_enum<decltype(v)>::type(v); }
} // namespace map
} // namespace tmwa
