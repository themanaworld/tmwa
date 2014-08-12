#pragma once
//    script.hpp - EAthena script frontend, engine, and library.
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

#include <cstdint>

#include <vector>

#include "../range/slice.hpp"

#include "../strings/zstring.hpp"

#include "../generic/db.hpp"
#include "../generic/dumb_ptr.hpp"

#include "../sexpr/variant.hpp"

#include "../mmo/ids.hpp"

#include "clif.t.hpp"
#include "map.t.hpp"


namespace tmwa
{
enum class VariableCode : uint8_t
{
    PARAM,
    VARIABLE,
};

enum class StringCode : uint8_t
{
    NOP, POS, INT, PARAM, FUNC,
    VARIABLE,
};

enum class ByteCode : uint8_t
{
    // types and specials
    // Note that 'INT' is synthetic, and does not occur in the data stream
    NOP, POS, INT, PARAM, FUNC, STR, ARG,
    VARIABLE, EOL,

    // unary and binary operators
    LOR, LAND, LE, LT, GE, GT, EQ, NE,
    XOR, OR, AND, ADD, SUB, MUL, DIV, MOD,
    NEG, LNOT, NOT, R_SHIFT, L_SHIFT,

    // additions
    // needed because FUNC is used for the actual call
    FUNC_REF,
};

struct str_data_t;

class ScriptBuffer
{
    typedef ZString::iterator ZSit;

    std::vector<ByteCode> script_buf;
public:
    // construction methods used only by script.cpp
    void add_scriptc(ByteCode a);
    void add_scriptb(uint8_t a);
    void add_scripti(uint32_t a);
    void add_scriptl(str_data_t *a);
    void set_label(str_data_t *ld, int pos_);
    ZSit parse_simpleexpr(ZSit p);
    ZSit parse_subexpr(ZSit p, int limit);
    ZSit parse_expr(ZSit p);
    ZSit parse_line(ZSit p, bool *canstep);
    void parse_script(ZString src, int line, bool implicit_end);

    // consumption methods used only by script.cpp
    ByteCode operator[](size_t i) const { return script_buf[i]; }
    ZString get_str(size_t i) const
    {
        return ZString(strings::really_construct_from_a_pointer, reinterpret_cast<const char *>(&script_buf[i]), nullptr);
    }

    // method used elsewhere
};

struct ScriptPointer
{
    const ScriptBuffer *code;
    size_t pos;

    ScriptPointer()
    : code()
    , pos()
    {}

    ScriptPointer(const ScriptBuffer *c, size_t p)
    : code(c)
    , pos(p)
    {}

    ByteCode peek() const { return (*code)[pos]; }
    ByteCode pop() { return (*code)[pos++]; }
    ZString pops()
    {
        ZString rv = code->get_str(pos);
        pos += rv.size();
        ++pos;
        return rv;
    }
};

// internal
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
    dumb_string str;
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
    const ScriptBuffer *script;
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

struct script_stack
{
    std::vector<struct script_data> stack_datav;
};

enum class ScriptEndState;
// future improvements coming!
class ScriptState
{
public:
    struct script_stack *stack;
    int start, end;
    ScriptEndState state;
    BlockId rid, oid;
    ScriptPointer scriptp, new_scriptp;
    int defsp, new_defsp;
};

std::unique_ptr<const ScriptBuffer> parse_script(ZString, int, bool implicit_end);

struct argrec_t
{
    ZString name;
    union _aru
    {
        int i;
        ZString s;

        _aru(int n) : i(n) {}
        _aru(ZString z) : s(z) {}
    } v;

    argrec_t(ZString n, int i) : name(n), v(i) {}
    argrec_t(ZString n, ZString z) : name(n), v(z) {}
};
int run_script_l(ScriptPointer, BlockId, BlockId, Slice<argrec_t> args);
int run_script(ScriptPointer, BlockId, BlockId);

extern
Map<ScriptLabel, int> scriptlabel_db;
extern
UPMap<RString, const ScriptBuffer> userfunc_db;

void do_init_script(void);
void do_final_script(void);

extern AString mapreg_txt;

extern int script_errors;

bool read_constdb(ZString filename);

void set_script_var_i(dumb_ptr<map_session_data> sd, VarName var, int e, int val);
void set_script_var_s(dumb_ptr<map_session_data> sd, VarName var, int e, XString val);

int get_script_var_i(dumb_ptr<map_session_data> sd, VarName var, int e);
ZString get_script_var_s(dumb_ptr<map_session_data> sd, VarName var, int e);
} // namespace tmwa
