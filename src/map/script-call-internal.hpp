#pragma once
//    script-call-internal.hpp - EAthena script frontend, engine, and library.
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

#include "script-call.hpp"
#include "fwd.hpp"

#include "../mmo/ids.hpp"

#include "../strings/rstring.hpp"
#include "../generic/db.hpp"
#include "script-persist.hpp"


namespace tmwa
{
namespace map
{
enum class VariableCode : uint8_t
{
    PARAM,
    VARIABLE,
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
    int defsp, new_defsp, freeloop;
    int is_true = 0;

    // register keys are ints (interned)
    // Not anymore! Well, sort of.
    DMap<SIR, int> regm;
    // can't be DMap because we want predictable .c_str()s
    // TODO this can change now
    Map<SIR, RString> regstrm;
};

void run_func(ScriptState *st);

enum class ScriptEndState
{
    ZERO,
    STOP,
    END,
    RERUNLINE,
    GOTO,
    RETFUNC,
};

dumb_ptr<map_session_data> script_rid2sd(ScriptState *st);
void get_val(dumb_ptr<block_list> sd, struct script_data *data);
__attribute__((deprecated))
void get_val(ScriptState *st, struct script_data *data);
struct script_data get_val2(ScriptState *st, SIR reg);
void set_scope_reg(ScriptState *, SIR, struct script_data *);
void set_reg(dumb_ptr<block_list> sd, VariableCode type, SIR reg, struct script_data vd);
void set_reg(dumb_ptr<block_list> sd, VariableCode type, SIR reg, int id);
void set_reg(dumb_ptr<block_list> sd, VariableCode type, SIR reg, RString zd);
__attribute__((warn_unused_result))
RString conv_str(ScriptState *st, struct script_data *data);
__attribute__((warn_unused_result))
int conv_num(ScriptState *st, struct script_data *data);
__attribute__((warn_unused_result))
Borrowed<const ScriptBuffer> conv_script(ScriptState *st, struct script_data *data);

template<class T>
void push_int(struct script_stack *stack, int val);
template<class T>
void push_reg(struct script_stack *stack, SIR reg);
template<class T>
void push_script(struct script_stack *stack, Borrowed<const ScriptBuffer> code);
template<class T>
void push_str(struct script_stack *stack, RString str);

void push_copy(struct script_stack *stack, int pos_);
void pop_stack(struct script_stack *stack, int start, int end);
} // namespace map
} // namespace tmwa

#include "script-call-internal.tcc"
