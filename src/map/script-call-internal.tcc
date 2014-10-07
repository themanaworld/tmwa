//    script-call-internal.tcc - EAthena script frontend, engine, and library.
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

#include "script-persist.hpp"


namespace tmwa
{
template<class D>
bool first_type_is_any()
{
    return false;
}

template<class D, class F, class... R>
constexpr
bool first_type_is_any()
{
    return std::is_same<D, F>::value || first_type_is_any<D, R...>();
}


template<class T>
void push_int(struct script_stack *stack, int val)
{
    static_assert(first_type_is_any<T, ScriptDataPos, ScriptDataInt, ScriptDataArg, ScriptDataFuncRef>(), "not int type");

    script_data nsd = T{.numi= val};
    stack->stack_datav.push_back(nsd);
}

template<class T>
void push_reg(struct script_stack *stack, SIR reg)
{
    static_assert(first_type_is_any<T, ScriptDataParam, ScriptDataVariable>(), "not reg type");

    script_data nsd = T{.reg= reg};
    stack->stack_datav.push_back(nsd);
}

template<class T>
void push_script(struct script_stack *stack, const ScriptBuffer *code)
{
    static_assert(first_type_is_any<T, ScriptDataRetInfo>(), "not scriptbuf type");

    script_data nsd = T{.script= code};
    stack->stack_datav.push_back(nsd);
}

template<class T>
void push_str(struct script_stack *stack, RString str)
{
    static_assert(first_type_is_any<T, ScriptDataStr>(), "not str type");

    script_data nsd = T{.str= str};
    stack->stack_datav.push_back(nsd);
}
} // namespace tmwa
