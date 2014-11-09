#pragma once
//    script-persist.hpp - EAthena script frontend, engine, and library.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "../compat/borrow.hpp"

#include "../strings/rstring.hpp"

#include "../sexpr/variant.hpp"

#include "script-buffer.hpp"


namespace tmwa
{
class SIR
{
    uint32_t impl;
    SIR(SP v)
    : impl(static_cast<uint32_t>(v))
    {}
    SIR(unsigned v, uint8_t i)
    : impl((i << 24) | v)
    {}
public:
    SIR() : impl() {}

    unsigned base() const { return impl & 0x00ffffff; }
    uint8_t index() const { return impl >> 24; }
    SIR iplus(uint8_t i) const { return SIR(base(), index() + i); }
    static SIR from(unsigned v, uint8_t i=0) { return SIR(v, i); }

    SP sp() const { return static_cast<SP>(impl); }
    static SIR from(SP v) { return SIR(v); }

    friend bool operator == (SIR l, SIR r) { return l.impl == r.impl; }
    friend bool operator < (SIR l, SIR r) { return l.impl < r.impl; }
};

struct ScriptDataPos
{
    int numi;
};
struct ScriptDataInt
{
    int numi;
};
struct ScriptDataParam
{
    SIR reg;
};
struct ScriptDataStr
{
    RString str;
};
struct ScriptDataArg
{
    int numi;
};
struct ScriptDataVariable
{
    SIR reg;
};
struct ScriptDataRetInfo
{
    // Not a ScriptPointer - pos is stored in a separate slot,
    // to avoid exploding the struct for everyone.
    Borrowed<const ScriptBuffer> script;
};
struct ScriptDataFuncRef
{
    int numi;
};

using ScriptDataVariantBase = Variant<
    ScriptDataPos,
    ScriptDataInt,
    ScriptDataParam,
    ScriptDataStr,
    ScriptDataArg,
    ScriptDataVariable,
    ScriptDataRetInfo,
    ScriptDataFuncRef
>;
struct script_data : ScriptDataVariantBase
{
    script_data() = delete;
    // TODO see if I can delete the copy ctor/assign instead of defaulting
    script_data(script_data&&) = default;
    script_data(const script_data&) = default /*delete*/;
    script_data& operator = (script_data&&) = default;
    script_data& operator = (const script_data&) = default /*delete*/;

    script_data(ScriptDataPos v) : ScriptDataVariantBase(std::move(v)) {}
    script_data(ScriptDataInt v) : ScriptDataVariantBase(std::move(v)) {}
    script_data(ScriptDataParam v) : ScriptDataVariantBase(std::move(v)) {}
    script_data(ScriptDataStr v) : ScriptDataVariantBase(std::move(v)) {}
    script_data(ScriptDataArg v) : ScriptDataVariantBase(std::move(v)) {}
    script_data(ScriptDataVariable v) : ScriptDataVariantBase(std::move(v)) {}
    script_data(ScriptDataRetInfo v) : ScriptDataVariantBase(std::move(v)) {}
    script_data(ScriptDataFuncRef v) : ScriptDataVariantBase(std::move(v)) {}
};
} // namespace tmwa
