#pragma once
//    extract_enums.hpp - Opt-in integer extraction support for enums.
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

#include "../io/extract.hpp"

#include "clif.t.hpp"


namespace tmwa
{
namespace e
{
enum class EPOS : uint16_t;
enum class MobMode : uint16_t;
enum class Opt1 : uint16_t;
enum class Opt2 : uint16_t;
enum class Opt0 : uint16_t;

inline
bool extract(XString str, EPOS *iv) { return extract_as_int(str, iv); }
inline
bool extract(XString str, MobMode *iv) { return extract_as_int(str, iv); }
inline
bool extract(XString str, Opt1 *iv) { return extract_as_int(str, iv); }
inline
bool extract(XString str, Opt2 *iv) { return extract_as_int(str, iv); }
inline
bool extract(XString str, Opt0 *iv) { return extract_as_int(str, iv); }
}

enum class ItemLook : uint16_t;
enum class ItemType : uint8_t;
enum class Race : uint8_t;
enum class SEX : uint8_t;
enum class SkillID : uint16_t;
enum class StatusChange : uint16_t;

inline
bool extract(XString str, ItemLook *iv) { return extract_as_int(str, iv); }
inline
bool extract(XString str, ItemType *iv) { return extract_as_int(str, iv); }
inline
bool extract(XString str, Race *iv) { return extract_as_int(str, iv); }
inline
bool extract(XString str, SEX *iv) { return extract_as_int(str, iv); }
inline
bool extract(XString str, SkillID *iv) { return extract_as_int(str, iv); }
inline
bool extract(XString str, StatusChange *iv) { return extract_as_int(str, iv); }

bool extract(XString, DIR *);
} // namespace tmwa
