#pragma once
//    strings/fwd.hpp - Forward declarations for all the string classes.
//
//    Copyright © 2013 Ben Longbons <b.r.longbons@gmail.com>
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

#include "../sanity.hpp"

#include <cstddef>
#include <cstdint>


namespace tmwa
{
// It is a common mistake to assume that one string class for everything.
// Because C++ and TMWA have a C legacy, there are a few more here
// than would probably be necessary in an ideal language.
namespace strings
{
    // owning
    class MString;
    class RString;
    class AString;
    class TString; // C legacy version of SString
    class SString; // is this one really worth it?

    // non-owning
    class ZString; // C legacy version of XString
    class XString;

    // semi-owning
    class LString;
    class FormatString;
    template<uint8_t len>
    class VString;

    // TODO refactor this into a function?
    enum _type_that_just_has_a_name_to_fix_linkage
    { really_construct_from_a_pointer };

    LString operator "" _s(const char *, size_t);
    constexpr
    FormatString operator "" _fmt(const char *, size_t);
} // namespace strings

using strings::MString;
using strings::RString;
using strings::AString;
using strings::TString;
using strings::SString;

using strings::ZString;
using strings::XString;

using strings::LString;
using strings::FormatString;
using strings::VString;

using strings::operator "" _s;
using strings::operator "" _fmt;
} // namespace tmwa
