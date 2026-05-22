#include "script-fun.hpp"
//    script-fun.cpp - EAthena script frontend, engine, and library.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011 Chuck Miller
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2013 wushin
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

#include "../compat/fun.hpp"
#include "../compat/nullpo.hpp"

#include "../generic/db.hpp"
#include "../generic/dumb_ptr.hpp"
#include "../generic/intern-pool.hpp"
#include "../generic/random.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"

#include "../net/timer.hpp"

#include "../proto2/net-HumanTimeDiff.hpp"

#include "../high/core.hpp"
#include "../high/extract_mmo.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "battle_conf.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "npc-parse.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "script-call-internal.hpp"
#include "script-parse-internal.hpp"
#include "script-persist.hpp"
#include "skill.hpp"
#include "storage.hpp"
#include "npc-internal.hpp"
#include "path.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{

#define AARG(n) (st->stack->stack_datav[st->start + 2 + (n)])
#define HARG(n) (st->end > st->start + 2 + (n))

#define BUILTIN_NAME() (builtin_functions[st->stack->stack_datav[st->start].get_if<ScriptDataFuncRef>()->numi].name)

#define script_nullpo_end(t, error)                                                             \
    if (nullpo_chk(__FILE__, __LINE__, __PRETTY_FUNCTION__, t)) {                               \
        if (st->oid) {                                                                          \
            dumb_ptr<npc_data> nullpo_nd = map_id_is_npc(st->oid);                              \
            if (nullpo_nd && nullpo_nd->name) {                                                 \
                PRINTF("script:%s: %s @ %s\n"_fmt, BUILTIN_NAME(), error, nullpo_nd->name);     \
            } else if (nullpo_nd) {                                                             \
                PRINTF("script:%s: %s (unnamed npc)\n"_fmt, BUILTIN_NAME(), error);             \
            } else {                                                                            \
                PRINTF("script:%s: %s (no npc)\n"_fmt, BUILTIN_NAME(), error);                  \
            }                                                                                   \
        } else {                                                                                \
            PRINTF("script:%s: %s (no npc)\n"_fmt, BUILTIN_NAME(), error);                      \
        }                                                                                       \
        st->state = ScriptEndState::END;                                                        \
        return;                                                                                 \
    }

enum class MonsterAttitude
{
    HOSTILE     = 0,
    FRIENDLY    = 1,
    SERVANT     = 2,
    FROZEN      = 3,
};
//
// 埋め込み関数 | Embedded functions
//
/*========================================
 * Print a line of dialogue in the NPC message box of the attached player;
 * with no argument an empty line is printed. The text stays on screen until
 * next, menu, close or clear is reached.
 *
 * @doc mes
 * @optarg text: str; the line of text to show.
 * @ret none
 ========================================*/
static
void builtin_mes(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    sd->state.npc_dialog_mes = 1;
    RString mes = HARG(0) ? conv_str(st, &AARG(0)) : ""_s;
    clif_scriptmes(sd, st->oid, mes);
}

/*========================================
 * Like mes, but the line is wrapped in double quotes before being sent; a
 * convenience for quoting spoken text.
 *
 * @doc mesq
 * @optarg text: str; the text to quote and show.
 * @ret none
 ========================================*/
static
void builtin_mesq(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    sd->state.npc_dialog_mes = 1;
    RString mes = HARG(0) ? conv_str(st, &AARG(0)) : ""_s;
    MString mesq;
    mesq += "\""_s;
    mesq += mes;
    mesq += "\""_s;
    clif_scriptmes(sd, st->oid, RString(mesq));
}

/*========================================
 * Like mes, but the line is wrapped in square brackets; with no argument
 * the NPC's own visible name (the part before any '#') is used, so it
 * prints a [Name] speaker header.
 *
 * @doc mesn
 * @optarg text: str; the name to bracket; defaults to the NPC name.
 * @ret none
 ========================================*/
static
void builtin_mesn(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    dumb_ptr<npc_data> nd;
    nd = map_id_is_npc(st->oid);
    script_nullpo_end(nd, "npc not found"_s);
    script_nullpo_end(sd, "player not found"_s);
    sd->state.npc_dialog_mes = 1;
    RString mes = HARG(0) ? conv_str(st, &AARG(0)) : RString(nd->name.xislice_h(std::find(nd->name.begin(), nd->name.end(), '#'))); // strnpcinf
    MString mesq;
    mesq += "["_s;
    mesq += mes;
    mesq += "]"_s;
    clif_scriptmes(sd, st->oid, RString(mesq));
}

/*========================================
 * Clear the NPC dialogue window of the attached player without ending the
 * script, so following mes lines start from a blank box.
 *
 * @doc clear
 * @ret none
 ========================================*/
static
void builtin_clear(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    clif_npc_action(sd, st->oid, 9, 0, 0, 0);
}

/*========================================
 * Jump unconditionally to a label in the current script; execution
 * continues there.
 *
 * @doc goto
 * @arg label: label; the label to jump to.
 * @ret none
 ========================================*/
static
void builtin_goto(ScriptState *st)
{
    if (!AARG(0).is<ScriptDataPos>())
    {
        PRINTF("fatal: script: goto: not label !\n"_fmt);
        st->state = ScriptEndState::END;
        abort();
    }

    st->scriptp.pos = conv_num(st, &AARG(0));
    st->state = ScriptEndState::GOTO;
}

/*========================================
 * Call the named user-defined function object; control transfers to the
 * function and return comes back here. This form passes no arguments; use
 * call to pass arguments.
 *
 * @doc callfunc
 * @arg func: func; name of the user-defined function to call.
 * @ret none
 ========================================*/
static
void builtin_callfunc(ScriptState *st)
{
    RString str = conv_str(st, &AARG(0));
    Option<P<const ScriptBuffer>> scr_ = userfunc_db.get(str);

    OMATCH_BEGIN (scr_)
    {
        OMATCH_CASE_SOME (scr)
        {
            int j = 0;
            assert (st->start + 3 == st->end);
#if 0
            for (int i = st->start + 3; i < st->end; i++, j++)
                push_copy(st->stack, i);
#endif
            push_int<ScriptDataInt>(st->stack, j); // 引数の数をプッシュ | Push the number of arguments
            push_int<ScriptDataInt>(st->stack, st->defsp); // 現在の基準スタックポインタをプッシュ | Push current reference stack pointer
            push_int<ScriptDataInt>(st->stack, st->scriptp.pos);   // 現在のスクリプト位置をプッシュ | Push Current Script Position
            push_script<ScriptDataRetInfo>(st->stack, TRY_UNWRAP(st->scriptp.code, abort()));  // 現在のスクリプトをプッシュ | Push Current Script

            st->scriptp = ScriptPointer(scr, 0);
            st->defsp = st->start + 4 + j;
            st->state = ScriptEndState::GOTO;
        }
        OMATCH_CASE_NONE ()
        {
            PRINTF("fatal: script: callfunc: function not found! [%s]\n"_fmt, str);
            st->state = ScriptEndState::END;
            abort();
        }
    }
    OMATCH_END ();
}

/*========================================
 * Call a user-defined function (when func is a string naming one) or a
 * script position, passing the remaining arguments. The arguments are
 * readable inside the callee with getarg. The value passed to return
 * becomes this expression's result.
 *
 * @doc call
 * @arg func: func; the function to call, or a script position.
 * @rest args: expr; arguments to pass to the callee.
 * @ret variant; the value the callee returned, if any.
 ========================================*/
static
void builtin_call(ScriptState *st)
{
    struct script_data *sdata = &AARG(0);
    get_val(st, sdata);
    RString str;
    if (sdata->is<ScriptDataStr>())
    {
        str = conv_str(st, sdata);
        Option<P<const ScriptBuffer>> scr_ = userfunc_db.get(str);
        OMATCH_BEGIN (scr_)
        {
            OMATCH_CASE_SOME (scr)
            {
                int j = 0;

                for (int i = st->start + 3; i < st->end; i++, j++)
                    push_copy(st->stack, i);

                push_int<ScriptDataInt>(st->stack, j); // 引数の数をプッシュ | Push the number of arguments
                push_int<ScriptDataInt>(st->stack, st->defsp); // 現在の基準スタックポインタをプッシュ | Push current reference stack pointer
                push_int<ScriptDataInt>(st->stack, st->scriptp.pos);   // 現在のスクリプト位置をプッシュ | Push Current Script Position
                push_script<ScriptDataRetInfo>(st->stack, TRY_UNWRAP(st->scriptp.code, abort()));  // 現在のスクリプトをプッシュ | Push Current Script

                st->scriptp = ScriptPointer(scr, 0);
                st->defsp = st->start + 4 + j;
                st->state = ScriptEndState::GOTO;
                return;
            }
            OMATCH_CASE_NONE ()
            {
                PRINTF("fatal: script: callfunc: function not found! [%s]\n"_fmt, str);
                st->state = ScriptEndState::END;
                abort();
            }
        }
        OMATCH_END ();
    }
    else
    {
        int pos_ = conv_num(st, &AARG(0));
        int j = 0;

        for (int i = st->start + 3; i < st->end; i++, j++)
            push_copy(st->stack, i);

        push_int<ScriptDataInt>(st->stack, j); // 引数の数をプッシュ | Push the number of arguments
        push_int<ScriptDataInt>(st->stack, st->defsp); // 現在の基準スタックポインタをプッシュ | Push current reference stack pointer
        push_int<ScriptDataInt>(st->stack, st->scriptp.pos);   // 現在のスクリプト位置をプッシュ | Push Current Script Position
        push_script<ScriptDataRetInfo>(st->stack, TRY_UNWRAP(st->scriptp.code, abort()));  // 現在のスクリプトをプッシュ | Push Current Script

        st->scriptp.pos = pos_;
        st->defsp = st->start + 4 + j;
        st->state = ScriptEndState::GOTO;
    }

}

/*========================================
 * Return an argument passed to the current function or subroutine by call
 * or callsub. If the index is out of range, the default is returned
 * instead, or an ETX (0x03) marker string when no default is given.
 *
 * @doc getarg
 * @arg index: int; zero-based index of the argument to fetch.
 * @optarg default: expr; value to return when index is out of range.
 * @ret variant; the requested argument, or the default.
 ========================================*/
static
void builtin_getarg(ScriptState *st)
{
    int arg = conv_num(st, &AARG(0));
    if(st->defsp < 1 || !(st->stack->stack_datav[st->defsp - 1].is<ScriptDataRetInfo>()))
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
        if(nd)
            PRINTF("builtin_getarg: no callfunc or callsub! @ %s\n"_fmt, nd->name);
        else
            PRINTF("builtin_getarg: no callfunc or callsub! (no npc)\n"_fmt);
        st->state = ScriptEndState::END;
        return;
    }

    int i = conv_num(st, &st->stack->stack_datav[st->defsp - 4]); // Number of arguments.
    if (arg > i || arg < 0 || i == 0)
    {
        const char a = 3; // ETX
        if (HARG(1))
            push_copy(st->stack, st->start + 3);
        else
            push_str<ScriptDataStr>(st->stack, VString<1>(a));
        return;
    }
    push_copy(st->stack, (st->defsp - 4 - i) + arg);
}

/*========================================
 * Evaluate and discard the arguments; used to call an expression-valued
 * builtin purely for its side effects.
 *
 * @doc void
 * @rest args: expr; expressions to evaluate and throw away.
 * @ret none
 ========================================*/
static
void builtin_void(ScriptState *)
{
    return;
}

/*========================================
 * Call a subroutine: transfer control to a label in the current script,
 * pushing a return frame so a later return resumes after this call.
 *
 * @doc callsub
 * @arg label: label; the subroutine label to call.
 * @ret none
 ========================================*/
static
void builtin_callsub(ScriptState *st)
{
    int pos_ = conv_num(st, &AARG(0));
    int j = 0;
    assert (st->start + 3 == st->end);
#if 0
    for (int i = st->start + 3; i < st->end; i++, j++)
        push_copy(st->stack, i);
#endif

    push_int<ScriptDataInt>(st->stack, j); // 引数の数をプッシュ | Push the number of arguments
    push_int<ScriptDataInt>(st->stack, st->defsp); // 現在の基準スタックポインタをプッシュ | Push current reference stack pointer
    push_int<ScriptDataInt>(st->stack, st->scriptp.pos);   // 現在のスクリプト位置をプッシュ | Push Current Script Position
    push_script<ScriptDataRetInfo>(st->stack, TRY_UNWRAP(st->scriptp.code, abort()));  // 現在のスクリプトをプッシュ | Push Current Script

    st->scriptp.pos = pos_;
    st->defsp = st->start + 4 + j;
    st->state = ScriptEndState::GOTO;
}

/*========================================
 * Return from the current callfunc, call or callsub. If a value is given it
 * becomes the call's result. Using return outside a call frame is
 * deprecated and logs a warning.
 *
 * @doc return
 * @optarg value: expr; the value to return to the caller.
 * @ret none
 ========================================*/
static
void builtin_return(ScriptState *st)
{
    if (st->defsp < 1 || !(st->stack->stack_datav[st->defsp - 1].is<ScriptDataRetInfo>()))
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
        if(nd)
            PRINTF("Deprecated: return outside of callfunc or callsub! @ %s\n"_fmt, nd->name);
        else
            PRINTF("Deprecated: return outside of callfunc or callsub! (no npc)\n"_fmt);
    }

    if (HARG(0))
    {                           // 戻り値有り | Return value available
        push_copy(st->stack, st->start + 2);
    }

    st->state = ScriptEndState::RETFUNC;
}

/*========================================
 * Suspend the script and show a "next" button in the player's dialogue
 * window; the script resumes when the player clicks it.
 *
 * @doc next
 * @ret none
 ========================================*/
static
void builtin_next(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    st->state = ScriptEndState::STOP;
    clif_scriptnext(sd, st->oid);
}

/*========================================
 * End the script and close the player's dialogue window. Using close inside
 * a callfunc or callsub frame is deprecated.
 *
 * @doc close
 * @ret none
 ========================================*/
static
void builtin_close(ScriptState *st)
{
    if (st->defsp >= 1 && st->stack->stack_datav[st->defsp - 1].is<ScriptDataRetInfo>())
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
        if(nd)
            PRINTF("Deprecated: close in a callfunc or callsub! @ %s\n"_fmt, nd->name);
        else
            PRINTF("Deprecated: close in a callfunc or callsub! (no npc)\n"_fmt);
    }
    st->state = ScriptEndState::END;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    if (sd->state.npc_dialog_mes)
        clif_scriptclose(sd, st->oid);
    else
        clif_npc_action(sd, st->oid, 5, 0, 0, 0);
}

/*========================================
 * Close the player's dialogue window but keep the script suspended rather
 * than ending it, so execution can continue after the player dismisses the
 * window.
 *
 * @doc close2
 * @ret none
 ========================================*/
static
void builtin_close2(ScriptState *st)
{
    st->state = ScriptEndState::STOP;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    if (sd->state.npc_dialog_mes)
        clif_scriptclose(sd, st->oid);
    else
        clif_npc_action(sd, st->oid, 5, 0, 0, 0);
}

/*========================================
 * Show a selection menu to the player. Arguments come in choice/label
 * pairs: a choice string and the label to jump to if it is chosen. An empty
 * choice string ends the displayed list. The script suspends; when the
 * player picks an entry it jumps to the matching label and the 1-based
 * choice index is stored in @menu. Cancelling ends the script.
 *
 * @doc menu
 * @arg choice: str; text of the first menu choice.
 * @rest label: label; the label paired with the preceding choice.
 * @ret none
 ========================================*/
static
void builtin_menu(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    if (sd->state.menu_or_input == 0)
    {
        // First half: show menu.
        st->state = ScriptEndState::RERUNLINE;
        sd->state.menu_or_input = 1;

        MString buf;
        for (int i = 0; i < (st->end - (st->start + 2)) / 2; i++)
        {
            RString choice_str = conv_str(st, &AARG(i * 2 + 0));
            if (!choice_str)
                break;
            buf += choice_str;
            buf += ':';
        }
        clif_scriptmenu(script_rid2sd(st), st->oid, AString(buf));
    }
    else
    {
        // Rerun: item is chosen from menu.
        if (sd->npc_menu == 0xff)
        {
            // cancel
            sd->state.menu_or_input = 0;
            st->state = ScriptEndState::END;
            return;
        }

        // Actually jump to the label.
        // Logic change: menu_choices is the *total* number of labels,
        // not just the displayed number that ends with the "".
        // (Would it be better to pop the stack before rerunning?)
        int menu_choices = (st->end - (st->start + 2)) / 2;
        pc_setreg(sd, SIR::from(variable_names.intern("@menu"_s)), sd->npc_menu);
        sd->state.menu_or_input = 0;
        if (sd->npc_menu > 0 && sd->npc_menu <= menu_choices)
        {
            int arg_index = (sd->npc_menu - 1) * 2 + 1;
            if (!AARG(arg_index).is<ScriptDataPos>())
            {
                PRINTF("fatal: script:menu: not a label\n"_fmt);
                st->state = ScriptEndState::END;
                abort();
            }
            st->scriptp.pos = AARG(arg_index).get_if<ScriptDataPos>()->numi;
            st->state = ScriptEndState::GOTO;
        }
    }
}

/*========================================
 * Return a random integer. With one argument, a value in 0..max-1 (0 if max
 * is not positive). With two arguments, a value between the bounds
 * inclusive (the bounds are swapped if given out of order).
 *
 * @doc rand
 * @arg max: int; exclusive upper bound, or the first bound.
 * @optarg upper: int; inclusive upper bound when given.
 * @ret int; the random number.
 ========================================*/
static
void builtin_rand(ScriptState *st)
{
    if (HARG(1))
    {
        int min = conv_num(st, &AARG(0));
        int max = conv_num(st, &AARG(1));
        if (min > max)
            std::swap(max, min);
        push_int<ScriptDataInt>(st->stack, random_::in(min, max));
    }
    else
    {
        int range = conv_num(st, &AARG(0));
        push_int<ScriptDataInt>(st->stack, range <= 0 ? 0 : random_::to(range));
    }
}

/*========================================
 * Return the largest of the integer arguments. Called with a single array
 * variable, it returns the largest element of that array.
 *
 * @doc max
 * @arg value: expr; an integer, or a single array variable.
 * @rest more: expr; further integers to compare.
 * @ret int; the largest value.
 ========================================*/
static
void builtin_max(ScriptState *st)
{
    int max=0, num;
    if (HARG(1))
    {
        max = conv_num(st, &AARG(0));
        for (int i = 1; HARG(i); i++)
        {
            num = conv_num(st, &AARG(i));
            if (num > max)
                max = num;
        }
    }
    else
    {
        SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;
        ZString name = variable_names.outtern(reg.base());
        char prefix = name.front();
        if (prefix != '$' && prefix != '@' && prefix != '.')
        {
            PRINTF("builtin_max: illegal scope!\n"_fmt);
            st->state = ScriptEndState::END;
            return;
        }
        for (int i = reg.index(); i < 256; i++)
        {
            struct script_data vd = get_val2(st, reg.iplus(i));
            MATCH_BEGIN (vd)
            {
                MATCH_CASE (const ScriptDataInt&, u)
                {
                    if (u.numi > max)
                        max = u.numi;
                    continue;
                }
            }
            MATCH_END ();
            abort();
        }
    }

    push_int<ScriptDataInt>(st->stack, max);
}

/*========================================
 * Return the smallest of the integer arguments. Called with a single array
 * variable, it returns the smallest element of that array.
 *
 * @doc min
 * @arg value: expr; an integer, or a single array variable.
 * @rest more: expr; further integers to compare.
 * @ret int; the smallest value.
 ========================================*/
static
void builtin_min(ScriptState *st)
{
    int min = 0xFFFFFFF6, num;

    if (HARG(1))
    {
        min = conv_num(st, &AARG(0));
        for (int i = 1; HARG(i); i++)
        {
            num = conv_num(st, &AARG(i));
            if (num < min)
                min = num;
        }
    }
    else
    {
        SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;
        ZString name = variable_names.outtern(reg.base());
        char prefix = name.front();
        if (prefix != '$' && prefix != '@' && prefix != '.')
        {
            PRINTF("builtin_min: illegal scope!\n"_fmt);
            st->state = ScriptEndState::END;
            return;
        }
        for (int i = reg.index(); i < 256; i++)
        {
            struct script_data vd = get_val2(st, reg.iplus(i));
            MATCH_BEGIN (vd)
            {
                MATCH_CASE (const ScriptDataInt&, u)
                {
                    if (u.numi < min)
                        min = u.numi;
                    continue;
                }
            }
            MATCH_END ();
            abort();
        }
    }

    push_int<ScriptDataInt>(st->stack, min);
}

/*========================================
 * Return the integer (truncated) average of the integer arguments.
 *
 * @doc average
 * @arg value: int; the first integer.
 * @rest more: int; further integers to average.
 * @ret int; the truncated average.
 ========================================*/
static
void builtin_average(ScriptState *st)
{
    int total, i;
    total = conv_num(st, &AARG(0));

    for (i = 1; HARG(i); i++)
        total += conv_num(st, &AARG(i));

    push_int<ScriptDataInt>(st->stack, (total / i));
}

/*========================================
 * Return the integer square root of a number.
 *
 * @doc sqrt
 * @arg value: int; the number to take the root of.
 * @ret int; the integer square root.
 ========================================*/
static
void builtin_sqrt(ScriptState *st)
{
    push_int<ScriptDataInt>(st->stack, static_cast<int>(sqrt(conv_num(st, &AARG(0)))));
}

/*========================================
 * Return the integer cube root of a number.
 *
 * @doc cbrt
 * @arg value: int; the number to take the root of.
 * @ret int; the integer cube root.
 ========================================*/
static
void builtin_cbrt(ScriptState *st)
{
    push_int<ScriptDataInt>(st->stack, static_cast<int>(cbrt(conv_num(st, &AARG(0)))));
}

/*========================================
 * Return one integer raised to the power of another, truncated to an
 * integer.
 *
 * @doc pow
 * @arg base: int; the base.
 * @arg exponent: int; the exponent.
 * @ret int; base raised to the exponent.
 ========================================*/
static
void builtin_pow(ScriptState *st)
{
    push_int<ScriptDataInt>(st->stack, static_cast<int>(pow(conv_num(st, &AARG(0)), conv_num(st, &AARG(1)))));
}

/*========================================
 * Test whether the attached player is standing on a given map cell.
 *
 * @doc isat
 * @arg map: map; the map name to test.
 * @arg x: coordinate; the x coordinate to test.
 * @arg y: coordinate; the y coordinate to test.
 * @ret int; 1 if the player is at that cell, 0 otherwise.
 ========================================*/
static
void builtin_isat(ScriptState *st)
{
    int x, y;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x = conv_num(st, &AARG(1));
    y = conv_num(st, &AARG(2));

    script_nullpo_end(sd, "player not found"_s);

    push_int<ScriptDataInt>(st->stack,
            (x == sd->bl_x) && (y == sd->bl_y)
            && (str == sd->bl_m->name_));
}

/*========================================
 * Teleport the attached player to a map cell. The warp is refused if the
 * source map has the nowarp flag or the destination has nowarpto, unless
 * the player is a GM of sufficient level.
 *
 * @doc warp
 * @arg map: map; the destination map name.
 * @arg x: coordinate; the destination x coordinate.
 * @arg y: coordinate; the destination y coordinate.
 * @ret none
 ========================================*/
static
void builtin_warp(ScriptState *st)
{
    int x, y;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    MapName map_name = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x = conv_num(st, &AARG(1));
    y = conv_num(st, &AARG(2));
    script_nullpo_end(sd, "player not found"_s);

    Option<P<map_local>> m = map_mapname2mapid(map_name);
    if (m.map([](P<map_local> m_){ return m_->flag.get(MapFlag::NOWARPTO); }).copy_or(false)
        && !(pc_isGM(sd).satisfies(battle_config.any_warp_GM_min_level)))
        return;
    if (sd->bl_m->flag.get(MapFlag::NOWARP)
        && !(pc_isGM(sd).satisfies(battle_config.any_warp_GM_min_level)))
        return;

    pc_setpos(sd, map_name, x, y, BeingRemoveWhy::GONE);
}

/*==========================================
 * エリア指定ワープ
 * Area Designation Warp
 *------------------------------------------
 */
static
void builtin_areawarp_sub(dumb_ptr<block_list> bl, MapName mapname, int x, int y)
{
    dumb_ptr<map_session_data> sd = bl->is_player();

    Option<P<map_local>> m = map_mapname2mapid(mapname);
    if (m.map([](P<map_local> m_){ return m_->flag.get(MapFlag::NOWARPTO); }).copy_or(false)
        && !(pc_isGM(sd).satisfies(battle_config.any_warp_GM_min_level)))
        return;
    if (sd->bl_m->flag.get(MapFlag::NOWARP)
        && !(pc_isGM(sd).satisfies(battle_config.any_warp_GM_min_level)))
        return;

    pc_setpos(sd, mapname, x, y, BeingRemoveWhy::GONE);
}

/*========================================
 * Warp every player inside a rectangle on one map to a cell on another. The
 * same nowarp/nowarpto GM-level restrictions as warp apply per player.
 *
 * @doc areawarp
 * @arg from_map: map; the map holding the source rectangle.
 * @arg x0: coordinate; x of one corner of the source rectangle.
 * @arg y0: coordinate; y of one corner of the source rectangle.
 * @arg x1: coordinate; x of the opposite corner.
 * @arg y1: coordinate; y of the opposite corner.
 * @arg to_map: map; the destination map name.
 * @arg x: coordinate; the destination x coordinate.
 * @arg y: coordinate; the destination y coordinate.
 * @ret none
 ========================================*/
static
void builtin_areawarp(ScriptState *st)
{
    int x, y;
    int x0, y0, x1, y1;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x0 = conv_num(st, &AARG(1));
    y0 = conv_num(st, &AARG(2));
    x1 = conv_num(st, &AARG(3));
    y1 = conv_num(st, &AARG(4));
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(5))));
    x = conv_num(st, &AARG(6));
    y = conv_num(st, &AARG(7));

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);

    map_foreachinarea(std::bind(builtin_areawarp_sub, ph::_1, str, x, y),
            m,
            x0, y0,
            x1, y1,
            BL::PC);
}

/*========================================
 * Restore hit points and spell points to the attached player (negative
 * values subtract). A dead player healed for positive HP is resurrected.
 *
 * @doc heal
 * @arg hp: amount; hit points to restore.
 * @arg sp: amount; spell points to restore.
 * @optarg item_heal: bool; nonzero to treat it as item healing, subject to
 *                          healing modifiers.
 * @ret none
 ========================================*/
static
void builtin_heal(ScriptState *st)
{
    int hp, sp;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    hp = conv_num(st, &AARG(0));
    sp = conv_num(st, &AARG(1));
    script_nullpo_end(sd, "player not found"_s);

    if(sd != nullptr && (sd->status.hp < 1 && hp > 0)){
        pc_setstand(sd);
        if (battle_config.player_invincible_time > interval_t::zero())
            pc_setinvincibletimer(sd, battle_config.player_invincible_time);
        clif_resurrection(sd, 1);
    }

    if(HARG(2) && bool(conv_num(st, &AARG(2))) && hp > 0)
        pc_itemheal(sd, hp, sp);
    else
        pc_heal(sd, hp, sp);
}

/*========================================
 * Return the straight-line (Pythagorean) distance between two beings.
 * Beings on different maps are reported as a very large distance.
 *
 * @doc distance
 * @arg a: GID; being id of the first being.
 * @arg b: GID; being id of the second being.
 * @optarg mode: int; distance mode; only mode 0 is implemented.
 * @ret int; the distance between the beings.
 ========================================*/
static
void builtin_distance(ScriptState *st)
{
    dumb_ptr<block_list> source = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(0))));
    dumb_ptr<block_list> target = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))));
    int distance = 0;
    int mode = HARG(2) ? conv_num(st, &AARG(2)) : 0;

    switch (mode)
    {
        // TODO implement case 1 (walk distance)
        case 0:
        default:
            if (source->bl_m != target->bl_m)
            {
                // FIXME make it work even if source and target are not in the same map
                distance = 0x7fffffff;
                break;
            }
            int dx = abs(source->bl_x - target->bl_x);
            int dy = abs(source->bl_y - target->bl_y);
            distance = sqrt((dx * dx) + (dy * dy)); // Pythagoras' theorem
    }

    push_int<ScriptDataInt>(st->stack, distance);
}

/*========================================
 * Test the relationship between two beings and return a bitmask of the
 * requested tests that passed.
 *
 * @doc target
 * @arg source: GID; being id of the source.
 * @arg target: GID; being id of the target.
 * @arg tests: int; which tests to run: 0x01 within view range, 0x02 within
 *                  attack range, 0x04 a clear walk path exists, 0x10
 *                  attackable by the source, 0x20 in line of sight.
 * @ret int; a bitmask of the tests that passed.
 ========================================*/
static
void builtin_target(ScriptState *st)
{
    // TODO maybe scrap all this and make it use battle_ functions? (add missing functions to battle)

    dumb_ptr<block_list> source = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(0))));
    dumb_ptr<block_list> target = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))));
    int flag = conv_num(st, &AARG(2));
    int val = 0;

    if (!source || !target)
    {
        push_int<ScriptDataInt>(st->stack, val);
        return;
    }

    if (flag & 0x01)
    {
        int x0 = source->bl_x - AREA_SIZE;
        int y0 = source->bl_y - AREA_SIZE;
        int x1 = source->bl_x + AREA_SIZE;
        int y1 = source->bl_y + AREA_SIZE;
        if (target->bl_x >= x0 && target->bl_x <= x1 && target->bl_y >= y0 && target->bl_y <= y1)
            val |= 0x01; // 0x01 target is in visible range
    }

    if (flag & 0x02)
    {
        int range = battle_get_range(source);
        int x2 = source->bl_x - range;
        int y2 = source->bl_y - range;
        int x3 = source->bl_x + range;
        int y3 = source->bl_y + range;
        if (target->bl_x >= x2 && target->bl_x <= x3 && target->bl_y >= y2 && target->bl_y <= y3)
            val |= 0x02; // 0x02 target is in attack range
    }

    if (flag & 0x04)
    {
        struct walkpath_data wpd;
        if (!path_search(&wpd, source->bl_m, source->bl_x, source->bl_y, target->bl_x, target->bl_y, 0))
            val |= 0x04; // 0x04 target is walkable (has clear path to target)
    }

    // TODO 0x08 target is visible (not behind collision)

    if (flag & 0x10)
    {
        if (target->bl_type != BL::PC || (target->bl_type == BL::PC &&
            (target->bl_m->flag.get(MapFlag::PVP) || pc_iskiller(source->is_player(), target->is_player()))))
            val |= 0x10; // 0x10 target can be attacked by source (killer, killable and so on)
    }

    if (flag & 0x20)
    {
        if (battle_check_range(source, target, 0))
            val |= 0x20; // 0x20 target is in line of sight
    }

    push_int<ScriptDataInt>(st->stack, val);
}

/*========================================
 * Deal damage to a being, attributed to another being. The damage is shown
 * to clients and may kill (and free) the target.
 *
 * @doc injure
 * @arg source: GID; being id credited with the damage.
 * @arg target: GID; being id of the victim.
 * @arg damage: amount; amount of damage to deal.
 * @ret none
 ========================================*/
static
void builtin_injure(ScriptState *st)
{
    dumb_ptr<block_list> source = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(0))));
    dumb_ptr<block_list> target = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))));
    int damage_caused = conv_num(st, &AARG(2));

    if (source != nullptr && source->bl_type == BL::PC)
        pc_setstand(source->is_player());

    // display damage first, because dealing damage may deallocate the target.
    clif_damage(source, target,
            gettick(), interval_t::zero(), interval_t::zero(),
            damage_caused, 0, DamageType::NORMAL);

    battle_damage(source, target, damage_caused, 0);

    return;
}

/*========================================
 * Prompt the attached player for input and store the result in a variable.
 * A string variable opens a text prompt; a numeric variable opens a number
 * prompt. The script suspends until the player answers; a negative numeric
 * answer cancels the dialogue.
 *
 * @doc input
 * @arg dest: var; the variable to store the answer in.
 * @ret none
 ========================================*/
static
void builtin_input(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = nullptr;
    script_data& scrd = AARG(0);
    assert (scrd.is<ScriptDataVariable>());

    SIR reg = scrd.get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());
//  char prefix = name.front();
    char postfix = name.back();

    sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    if (sd->state.menu_or_input)
    {
        // Second time (rerun)
        sd->state.menu_or_input = 0;
        if (postfix == '$')
        {
            set_reg(sd, VariableCode::VARIABLE, reg, sd->npc_str);
        }
        else
        {
            //commented by Lupus (check Value Number Input fix in clif.c)
            //** Fix by fritz :X keeps people from abusing old input bugs
            // wtf?
            if (sd->npc_amount < 0) //** If input amount is less then 0
            {
                clif_tradecancelled(sd);   // added "Deal has been cancelled" message by Valaris
                builtin_close(st); //** close
            }

            set_reg(sd, VariableCode::VARIABLE, reg, sd->npc_amount);
        }
    }
    else
    {
        // First time - send prompt to client, then wait
        st->state = ScriptEndState::RERUNLINE;
        if (postfix == '$')
            clif_scriptinputstr(sd, st->oid);
        else
            clif_scriptinput(sd, st->oid);
        sd->state.menu_or_input = 1;
    }
}

/*========================================
 * Prompt the attached player to choose items from their inventory and store
 * the chosen item ids (or names, if dest is a string array) into the array.
 * The script suspends until the player answers.
 *
 * @doc requestitem
 * @arg dest: var; array variable to receive the chosen items.
 * @optarg count: amount; how many items to request, 1 to 16; defaults to 1.
 * @ret none
 ========================================*/
static
void builtin_requestitem(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = nullptr;
    script_data& scrd = AARG(0);
    assert (scrd.is<ScriptDataVariable>());
    int amount = HARG(1) ? conv_num(st, &AARG(1)) : 1;
    if (amount < 1 || amount > 16)
        amount = 1;

    SIR reg = scrd.get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();
    char postfix = name.back();

    if (prefix != '$' && prefix != '@' && prefix != '.')
    {
        PRINTF("builtin_requestitem: illegal scope!\n"_fmt);
        abort();
    }

    sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    if (sd->state.menu_or_input)
    {
        // Second time (rerunline)
        sd->state.menu_or_input = 0;
        RString str = sd->npc_str;
        RString val;
        const char separator = ';';
        for (int j = 0; j < amount; j++)
        {
            auto find = std::find(str.begin(), str.end(), separator);
            if (find == str.end())
                val = str.xislice_h(std::find(str.begin(), str.end(), ','));
            else
            {
                val = str.xislice_h(find);
                val = val.xislice_h(std::find(val.begin(), val.end(), ','));
                str = str.xislice_t(find + 1);
            }

            // check that the item exists in the inventory
            int num = atoi(val.c_str());
            if (num < 1)
            {
                j--;
                if (find == str.end())
                    break;
                continue;
            }
            ItemNameId nameid = wrap<ItemNameId>(num);
            for (IOff0 i : IOff0::iter())
                if (sd->status.inventory[i].nameid == nameid)
                    goto pass;
        fail:
            j--;
            if (find == str.end())
                break;
            continue;

        pass:
            // push to array
            if (postfix == '$')
            {
                Option<P<struct item_data>> i_data = Some(itemdb_search(nameid));
                RString item_name = i_data.pmd_pget(&item_data::name).copy_or(stringish<ItemName>(""_s));
                if (item_name == ""_s)
                    goto fail;
                if (name.startswith(".@"_s))
                {
                    struct script_data vd = script_data(ScriptDataStr{item_name});
                    set_scope_reg(st, reg.iplus(j), &vd);
                }
                else
                    set_reg(sd, VariableCode::VARIABLE, reg.iplus(j), item_name);
            }
            else
            {
                if (name.startswith(".@"_s))
                {
                    struct script_data vd = script_data(ScriptDataInt{num});
                    set_scope_reg(st, reg.iplus(j), &vd);
                }
                else
                    set_reg(sd, VariableCode::VARIABLE, reg.iplus(j), num);
            }
            if (find == str.end())
                break;
        }
    }
    else
    {
        // First time - send prompt to client, then wait
        st->state = ScriptEndState::RERUNLINE;
        clif_scriptinputstr(sd, st->oid); // send string prompt
        clif_npc_action(sd, st->oid, 10, amount, 0, 0); // send item request
        sd->state.menu_or_input = 1;
    }
}

/*========================================
 * Prompt the attached player for their client language and store the answer
 * in a string variable. The script suspends until the client replies.
 *
 * @doc requestlang
 * @arg dest: var; string variable to receive the language.
 * @ret none
 ========================================*/
static
void builtin_requestlang(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_data& scrd = AARG(0);
    assert (scrd.is<ScriptDataVariable>());
    SIR reg = scrd.get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());
    char postfix = name.back();

    script_nullpo_end(sd, "player not found"_s);

    if (postfix != '$')
    {
        PRINTF("builtin_requestlang: illegal type (expects string)!\n"_fmt);
        abort();
    }

    if (sd->state.menu_or_input)
    {
        // Second time (rerunline)
        sd->state.menu_or_input = 0;
        if (name.startswith(".@"_s))
        {
            struct script_data vd = script_data(ScriptDataStr{sd->npc_str});
            set_scope_reg(st, reg, &vd);
        }
        else
            set_reg(sd, VariableCode::VARIABLE, reg, sd->npc_str);
    }
    else
    {
        // First time - send prompt to client, then wait
        st->state = ScriptEndState::RERUNLINE;
        clif_npc_action(sd, st->oid, 0, 0, 0, 0); // send lang request
        clif_scriptinputstr(sd, st->oid); // send string prompt
        sd->state.menu_or_input = 1;
    }
}

/*========================================
 * If the condition is nonzero, call the given function with the remaining
 * arguments. Records the test result so a following elif or else can react
 * to it.
 *
 * @doc if
 * @arg condition: int; the condition to test.
 * @rest func: func; function to call and its arguments.
 * @ret none
 ========================================*/
static
void builtin_if (ScriptState *st)
{
    int sel, i;

    sel = conv_num(st, &AARG(0));
    st->is_true = sel ? 2 : 1;
    if (!sel)
        return;

    // 関数名をコピー | Copy function name
    push_copy(st->stack, st->start + 3);
    // 間に引数マーカを入れて | Put argument markers in between
    push_int<ScriptDataArg>(st->stack, 0);
    // 残りの引数をコピー | Copy the remaining arguments
    for (i = st->start + 4; i < st->end; i++)
    {
        push_copy(st->stack, i);
    }
    run_func(st);
}

/*========================================
 * Return one of two values depending on a condition. Both branches are
 * evaluated.
 *
 * @doc if_then_else
 * @arg condition: int; the condition to test.
 * @arg if_true: int; the value to return when the condition is nonzero.
 * @arg if_false: int; the value to return otherwise.
 * @ret variant; the chosen value.
 ========================================*/
static
void builtin_if_then_else (ScriptState *st)
{
    int condition = conv_num(st, &AARG(0));
    push_copy(st->stack, st->start + (condition ? 3 : 4));
}

/*========================================
 * Continuation of an if chain: if no earlier if or elif branch was taken,
 * call the function with the remaining arguments.
 *
 * @doc else
 * @rest func: func; function to call and its arguments.
 * @ret none
 ========================================*/
static
void builtin_else (ScriptState *st)
{
    int i;

    if (st->is_true < 1)
    {
        PRINTF("builtin_else: no if statement!\n"_fmt);
        abort();
    }

    if (st->is_true > 1)
        return;

    st->is_true = 0;
    // 関数名をコピー | Copy function name
    push_copy(st->stack, st->start + 2);
    // 間に引数マーカを入れて | Put argument markers in between
    push_int<ScriptDataArg>(st->stack, 0);
    // 残りの引数をコピー | Copy the remaining arguments
    for (i = st->start + 3; i < st->end; i++)
    {
        push_copy(st->stack, i);
    }
    run_func(st);
}

/*========================================
 * Continuation of an if chain: if no earlier branch was taken and the
 * condition is nonzero, call the function with the remaining arguments.
 *
 * @doc elif
 * @arg condition: int; the condition to test.
 * @rest func: func; function to call and its arguments.
 * @ret none
 ========================================*/
static
void builtin_elif (ScriptState *st)
{
    int sel, i;

    if (st->is_true < 1)
    {
        PRINTF("builtin_elif: no if statement!\n"_fmt);
        abort();
    }
    if (st->is_true > 1)
        return;

    sel = conv_num(st, &AARG(0));
    st->is_true = sel ? 2 : 1;
    if (!sel)
        return;

    // 関数名をコピー | Copy function name
    push_copy(st->stack, st->start + 3);
    // 間に引数マーカを入れて | Put argument markers in between
    push_int<ScriptDataArg>(st->stack, 0);
    // 残りの引数をコピー | Copy the remaining arguments
    for (i = st->start + 4; i < st->end; i++)
    {
        push_copy(st->stack, i);
    }
    run_func(st);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_foreach_sub(dumb_ptr<block_list> bl, NpcEvent event, BlockId caster)
{
    // call_spell_event_script
    argrec_t arg[1] =
    {
        {"@target_id"_s, static_cast<int32_t>(unwrap<BlockId>(bl->bl_id))},
    };
    npc_event_do_l(event, caster, arg);
}

/*========================================
 * Run an NPC event once for every being inside a rectangle on a map. Each
 * invocation gets the matched being's id in @target_id.
 *
 * @doc foreach
 * @arg type: int; being type: 0 players, 1 NPCs, 2 monsters, 3 everything.
 * @arg map: map; the map holding the rectangle.
 * @arg x0: coordinate; x of one corner of the rectangle.
 * @arg y0: coordinate; y of one corner of the rectangle.
 * @arg x1: coordinate; x of the opposite corner.
 * @arg y1: coordinate; y of the opposite corner.
 * @arg event: event; the NPC event to run.
 * @optarg caster: GID; being id of the caster; defaults to the attached
 *                      player.
 * @ret none
 ========================================*/
static
void builtin_foreach(ScriptState *st)
{
    int x0, y0, x1, y1, bl_num;

    dumb_ptr<block_list> sd;
    bl_num = conv_num(st, &AARG(0));
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(1))));
    x0 = conv_num(st, &AARG(2));
    y0 = conv_num(st, &AARG(3));
    x1 = conv_num(st, &AARG(4));
    y1 = conv_num(st, &AARG(5));
    ZString event_ = ZString(conv_str(st, &AARG(6)));
    BL block_type;
    NpcEvent event;
    extract(event_, &event);

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);

    switch (bl_num)
    {
        case 0:
            block_type = BL::PC;
            break;
        case 1:
            block_type = BL::NPC;
            break;
        case 2:
            block_type = BL::MOB;
            break;
        case 3:
            block_type = BL::NUL;
            break;
        default:
            return;
    }
    if (HARG(7))
        sd = map_id_is_player(wrap<BlockId>(conv_num(st, &AARG(7))));
    else if (st->rid)
        sd = script_rid2sd(st);

    script_nullpo_end(sd, "player not found"_s);

    map_foreachinarea(std::bind(builtin_foreach_sub, ph::_1, event, sd->bl_id),
            m,
            x0, y0,
            x1, y1,
            block_type);
}

/*========================================
 * Destroy a temporary NPC. With no argument the script's own NPC is
 * destroyed and the script ends; with a being id that NPC is destroyed
 * instead. Only script-subtype NPCs can be destroyed.
 *
 * @doc destroy
 * @optarg gid: GID; being id of the NPC to destroy.
 * @ret none
 ========================================*/
static
void builtin_destroy(ScriptState *st)
{
    BlockId id;
    dumb_ptr<map_session_data> sd;

    if (HARG(0))
        id = wrap<BlockId>(conv_num(st, &AARG(0)));
    else
        id = st->oid;

    dumb_ptr<npc_data> nd = map_id_is_npc(id);
    if(!nd || nd->npc_subtype != NpcSubtype::SCRIPT)
        return;

    /* If we have a player attached, make sure it is cleared. */
    /* Not safe to call destroy if others may also be paused on this NPC! */
    if (st->rid) {
        sd = script_rid2sd(st);
        script_nullpo_end(sd, "player not found"_s);
        npc_event_dequeue(sd);
    }

    // Cancel all existing timers on the NPC.
    // They "would" never fire, and we don't want race conditions here.
    for (int i = 0; i < MAX_EVENTTIMER; i++)
    {
        nd->eventtimer[i].cancel();
    }
    // Schedule the NPC to be freed on the next available tick.
    // Scripts can be invoked under iteration of the ev_db global event
    // database, and we don't want to invalidate active iterators.
    nd->deletion_pending = npc_data::DELETION_QUEUED;
    nd->eventtimer[0] = Timer(gettick(), std::bind(npc_free, nd));

    nd = nd->is_script();
    st->oid = BlockId();

    if (!HARG(0))
        st->state = ScriptEndState::END;
}
/*========================================
 * Create a temporary NPC (a "puppet") with a sprite. It inherits the labels
 * of the calling NPC.
 *
 * @doc puppet
 * @arg sprite: mob; the puppet's sprite species id.
 * @arg x: coordinate; the puppet's x coordinate.
 * @arg y: coordinate; the puppet's y coordinate.
 * @arg name: str; the puppet's NPC name.
 * @arg map: int; the map name for the puppet.
 * @optarg half_width: int; touch-area half-width.
 * @optarg half_height: int; touch-area half-height.
 * @ret int; the new NPC's being id, or 0 if the name is already in use.
 ========================================*/
static
void builtin_puppet(ScriptState *st)
{
    int x, y;

    NpcName npc = stringish<NpcName>(ZString(conv_str(st, &AARG(3))));
    if (npc_name2id(npc) != nullptr)
    {
        push_int<ScriptDataInt>(st->stack, 0);
        return;
    }

    dumb_ptr<block_list> bl = map_id2bl(st->oid);
    dumb_ptr<npc_data_script> parent_nd = bl->is_npc()->is_script();
    dumb_ptr<npc_data_script> nd;
    nd.new_();

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x = conv_num(st, &AARG(1));
    y = conv_num(st, &AARG(2));
    Species sprite = wrap<Species>(static_cast<uint16_t>(conv_num(st, &AARG(4))));

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);

    nd->bl_prev = nd->bl_next = nullptr;
    nd->scr.event_needs_map = false;

    // PlayerName::SpellName
    nd->name = npc;
    nd->sex = SEX::UNSPECIFIED;

    // Dynamically set location
    nd->bl_m = m;
    nd->bl_x = x;
    nd->bl_y = y;
    if (HARG(5) && HARG(6))
    {
        nd->scr.xs = ((conv_num(st, &AARG(5)) * 2) + 1); // do the same equation as in AST
        nd->scr.ys = ((conv_num(st, &AARG(6)) * 2) + 1);
    }
    nd->bl_id = npc_get_new_npc_id();
    nd->scr.parent = parent_nd->bl_id;
    nd->dir = DIR::S;
    nd->flag = 0;
    nd->sit = DamageType::STAND;
    nd->npc_class = sprite;
    nd->speed = 200_ms;
    nd->option = Opt0::ZERO;
    nd->opt1 = Opt1::ZERO;
    nd->opt2 = Opt2::ZERO;
    nd->opt3 = Opt3::ZERO;
    nd->scr.label_listv = parent_nd->scr.label_listv;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::SCRIPT;
    npc_script++;

    nd->deletion_pending = npc_data::NOT_DELETING;

    nd->n = map_addnpc(nd->bl_m, nd);

    map_addblock(nd);
    clif_spawnnpc(nd);

    register_npc_name(nd);

    for (npc_label_list& el : parent_nd->scr.label_listv)
    {
        ScriptLabel lname = el.name;
        int pos = el.pos;

        if (lname.startswith("On"_s))
        {
            struct event_data ev {};
            ev.nd = nd;
            ev.pos = pos;
            NpcEvent buf;
            buf.npc = nd->name;
            buf.label = lname;
            ev_db.insert(buf, ev);
        }
    }

    for (npc_label_list& el : parent_nd->scr.label_listv)
    {
        int t_ = 0;
        ScriptLabel lname = el.name;
        int pos = el.pos;
        if (lname.startswith("OnTimer"_s) && extract(lname.xslice_t(7), &t_) && t_ > 0)
        {
            interval_t t = static_cast<interval_t>(t_);

            npc_timerevent_list tel {};
            tel.timer = t;
            tel.pos = pos;

            auto it = std::lower_bound(nd->scr.timer_eventv.begin(), nd->scr.timer_eventv.end(), tel,
                    [](const npc_timerevent_list& l, const npc_timerevent_list& r)
                    {
                        return l.timer < r.timer;
                    }
            );
            assert (it == nd->scr.timer_eventv.end() || it->timer != tel.timer);

            nd->scr.timer_eventv.insert(it, std::move(tel));
        }
    }

    nd->scr.timer = interval_t::zero();
    nd->scr.next_event = nd->scr.timer_eventv.begin();

    push_int<ScriptDataInt>(st->stack, unwrap<BlockId>(nd->bl_id));
}

/*========================================
 * Assign a value to a variable or player parameter. The optional third
 * argument selects whose copy to write: a character name or id for player-
 * scope variables, an NPC name or id for NPC-scope variables.
 *
 * @doc set
 * @arg dest: var; the variable or parameter to write.
 * @arg value: expr; the value to assign.
 * @optarg owner: expr; the character or NPC that owns the variable.
 * @ret none
 ========================================*/
static
void builtin_set(ScriptState *st)
{
    BlockId id;
    dumb_ptr<block_list> bl = nullptr;
    if (auto *u = AARG(0).get_if<ScriptDataParam>())
    {
        SIR reg = u->reg;
        if(HARG(2))
        {
            struct script_data *sdata = &AARG(2);
            get_val(st, sdata);
            CharName name;
            if (sdata->is<ScriptDataStr>())
            {
                name = stringish<CharName>(ZString(conv_str(st, sdata)));
                if (name.to__actual())
                    bl = map_nick2sd(name);
            }
            else
            {
                int num = conv_num(st, sdata);
                if (num >= 2000000)
                    id = wrap<BlockId>(num);
                else if (num >= 150000)
                {
                    dumb_ptr<map_session_data> p_sd = nullptr;
                    if ((p_sd = map_nick2sd(map_charid2nick(wrap<CharId>(num)))) != nullptr)
                        id = p_sd->bl_id;
                    else
                        return;
                }
                else
                    return;
                bl = map_id2bl(id);
            }
        }

        else
        {
            bl = script_rid2sd(st);
            script_nullpo_end(bl, "player not found"_s);
        }

        int val = conv_num(st, &AARG(1));
        set_reg(bl, VariableCode::PARAM, reg, val);
        return;
    }

    SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;

    ZString name = variable_names.outtern(reg.base());
    VarName name_ = stringish<VarName>(name);
    char prefix = name_.front();
    char postfix = name_.back();

    if (prefix != '$')
    {
        if(HARG(2))
        {
            struct script_data *sdata = &AARG(2);
            get_val(st, sdata);
            if(prefix == '.')
            {
                if (name_.startswith(".@"_s))
                {
                    PRINTF("builtin_set: illegal scope!\n"_fmt);
                    return;
                }
                NpcName n_name;
                if (sdata->is<ScriptDataStr>())
                {
                    n_name = stringish<NpcName>(ZString(conv_str(st, sdata)));
                    bl = npc_name2id(n_name);
                }
                else
                {
                    id = wrap<BlockId>(conv_num(st, sdata));
                    bl = map_id2bl(id);
                }
            }
            else
            {
                CharName c_name;
                if (sdata->is<ScriptDataStr>())
                {
                    c_name = stringish<CharName>(ZString(conv_str(st, sdata)));
                    if (c_name.to__actual())
                        bl = map_nick2sd(c_name);
                }
                else
                {
                    id = wrap<BlockId>(conv_num(st, sdata));
                    bl = map_id2bl(id);
                }
            }
        }
        else
        {
            if(prefix == '.')
            {
                if (name_.startswith(".@"_s))
                {
                        set_scope_reg(st, reg, &AARG(1));
                    return;
                }
                bl = map_id_is_npc(st->oid);
            }
            else
                bl = map_id_is_player(st->rid);
        }
        if (bl == nullptr)
            return;
    }

    if (postfix == '$')
    {
        // 文字列 | string
        RString str = conv_str(st, &AARG(1));
        set_reg(bl, VariableCode::VARIABLE, reg, str);
    }
    else
    {
        // 数値 | numeric value
        int val = conv_num(st, &AARG(1));
        set_reg(bl, VariableCode::VARIABLE, reg, val);
    }

}

/*==========================================
 * this is a special function that returns array index for a variable stored in another being
 *------------------------------------------
 */
static
int getarraysize2(SIR reg, dumb_ptr<block_list> bl)
{
    int i = reg.index(), c = i;
    bool zero = true; // index zero is empty
    for (; i < 256; i++)
    {
        struct script_data vd = ScriptDataVariable{reg.iplus(i)};
        get_val(bl, &vd);
        MATCH_BEGIN (vd)
        {
            MATCH_CASE (const ScriptDataStr&, u)
            {
                if (u.str[0])
                {
                    if (i == 0)
                        zero = false; // index zero is not empty
                    c = i;
                }
                continue;
            }
            MATCH_CASE (const ScriptDataInt&, u)
            {
                if (u.numi)
                {
                    if (i == 0)
                        zero = false; // index zero is not empty
                    c = i;
                }
                continue;
            }
        }
        MATCH_END ();
        abort();
    }
    return (c == 0 && zero) ? c : (c + 1);
}

/*========================================
 * Fill an array starting at its given index with the listed values. For an
 * NPC-scope array the second argument is instead the NPC to target and the
 * values follow.
 *
 * @doc setarray
 * @arg dest: var; the array variable to fill.
 * @rest value: expr; the values to store, or an NPC then values.
 * @ret none
 ========================================*/
static
void builtin_setarray(ScriptState *st)
{
    dumb_ptr<block_list> bl = nullptr;
    SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();
    char postfix = name.back();
    int i = 1, j = 0;

    if (prefix != '$' && prefix != '@' && prefix != '.')
    {
        PRINTF("builtin_setarray: illegal scope!\n"_fmt);
        return;
    }
    if (prefix == '.' && !name.startswith(".@"_s))
    {
        struct script_data *sdata = &AARG(1);
        get_val(st, sdata);
        i++; // 2nd argument is npc, not an array element
        if (sdata->is<ScriptDataStr>())
        {
            ZString tn = conv_str(st, sdata);
            if (tn == "this"_s || tn == "oid"_s)
                bl = map_id_is_npc(st->oid);
            else
            {
                NpcName name_ = stringish<NpcName>(tn);
                bl = npc_name2id(name_);
            }
        }
        else
        {
            int tid = conv_num(st, sdata);
            if (tid == 0)
                bl = map_id_is_npc(st->oid);
            else
               bl = map_id_is_npc(wrap<BlockId>(tid));
        }
        script_nullpo_end(bl, "npc not found"_s);
        if (st->oid && bl->bl_id != st->oid)
            j = getarraysize2(reg, bl);
    }
    else if (prefix != '$' && !name.startswith(".@"_s))
    {
        bl = map_id_is_player(st->rid);
        script_nullpo_end(bl, "player not found"_s);
    }

    for (; i < st->end - st->start - 2 && j < 256; i++, j++)
    {
        if (name.startswith(".@"_s))
            set_scope_reg(st, reg.iplus(j), &AARG(i));
        else if (postfix == '$')
            set_reg(bl, VariableCode::VARIABLE, reg.iplus(j), conv_str(st, &AARG(i)));
        else
            set_reg(bl, VariableCode::VARIABLE, reg.iplus(j), conv_num(st, &AARG(i)));
    }
}

/*========================================
 * Set the first count elements of an array to a single value.
 *
 * @doc cleararray
 * @arg dest: var; the array variable to clear.
 * @arg value: expr; the value to store in every element.
 * @arg count: amount; how many elements to set.
 * @ret none
 ========================================*/
static
void builtin_cleararray(ScriptState *st)
{
    dumb_ptr<block_list> bl = nullptr;
    SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();
    char postfix = name.back();
    int sz = conv_num(st, &AARG(2));

    if (prefix != '$' && prefix != '@' && prefix != '.')
    {
        PRINTF("builtin_cleararray: illegal scope!\n"_fmt);
        return;
    }
    if (prefix == '.' && !name.startswith(".@"_s))
        bl = map_id_is_npc(st->oid);
    else if (prefix != '$' && !name.startswith(".@"_s))
    {
        bl = map_id_is_player(st->rid);
        script_nullpo_end(bl, "player not found"_s);
    }

    for (int i = 0; i < sz; i++)
    {
        if (name.startswith(".@"_s))
            set_scope_reg(st, reg.iplus(i), &AARG(i));
        else if (postfix == '$')
            set_reg(bl, VariableCode::VARIABLE, reg.iplus(i), conv_str(st, &AARG(1)));
        else
            set_reg(bl, VariableCode::VARIABLE, reg.iplus(i), conv_num(st, &AARG(1)));
    }

}

/*==========================================
 * 配列変数のサイズ所得
 * Size of the array variable income
 *------------------------------------------
 */
static
int getarraysize(ScriptState *st, SIR reg)
{
    int i = reg.index(), c = i;
    for (; i < 256; i++)
    {
        struct script_data vd = get_val2(st, reg.iplus(i));
        MATCH_BEGIN (vd)
        {
            MATCH_CASE (const ScriptDataStr&, u)
            {
                if (u.str[0])
                    c = i;
                continue;
            }
            MATCH_CASE (const ScriptDataInt&, u)
            {
                if (u.numi)
                    c = i;
                continue;
            }
        }
        MATCH_END ();
        abort();
    }
    return c + 1;
}

/*========================================
 * Return the logical size of an array: one past the index of its last
 * nonzero or non-empty element.
 *
 * @doc getarraysize
 * @arg name: var; the array variable to measure.
 * @ret int; the array size.
 ========================================*/
static
void builtin_getarraysize(ScriptState *st)
{
    SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();

    if (prefix != '$' && prefix != '@' && prefix != '.')
    {
        PRINTF("builtin_copyarray: illegal scope!\n"_fmt);
        return;
    }

    push_int<ScriptDataInt>(st->stack, getarraysize(st, reg));
}

/*========================================
 * Return one element of an array as a variable reference, the runtime form
 * of the var[index] subscript syntax. Indices outside 0..255 yield 0.
 *
 * @doc getelementofarray
 * @arg name: var; the array variable.
 * @arg index: int; the element index.
 * @ret variant; a reference to the requested element.
 ========================================*/
static
void builtin_getelementofarray(ScriptState *st)
{
    if (auto *u = AARG(0).get_if<ScriptDataVariable>())
    {
        int i = conv_num(st, &AARG(1));
        if (i > 255 || i < 0)
        {
            PRINTF("script: getelementofarray (operator[]): param2 illegal number: %d\n"_fmt,
                    i);
            push_int<ScriptDataInt>(st->stack, 0);
        }
        else
        {
            push_reg<ScriptDataVariable>(st->stack,
                    u->reg.iplus(i));
        }
    }
    else
    {
        PRINTF("script: getelementofarray (operator[]): param1 not named!\n"_fmt);
        push_int<ScriptDataInt>(st->stack, 0);
    }
}

/*========================================
 * Search an array for a value and return the index of the first match, or
 * -1 if the value is not present.
 *
 * @doc array_search
 * @arg value: expr; the value to look for.
 * @arg haystack: var; the array variable to search.
 * @ret int; the index of the first match, or -1.
 ========================================*/
static
void builtin_array_search(ScriptState *st)
{
    ZString needle_str = ZString(conv_str(st, &AARG(0)));
    int needle_int = conv_num(st, &AARG(0));
    SIR reg = AARG(1).get_if<ScriptDataVariable>()->reg; // haystack
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();
    int i, c;

    if (prefix != '$' && prefix != '@' && prefix != '.')
    {
        PRINTF("builtin_array_search: illegal scope!\n"_fmt);
        return;
    }

    i = reg.index(); c = -1;
    for (; i < 256; i++)
    {
        struct script_data vd = get_val2(st, reg.iplus(i));
        MATCH_BEGIN (vd)
        {
            MATCH_CASE (const ScriptDataStr&, u)
            {
                if (u.str == needle_str)
                {
                    c = i;
                    goto Out;
                }
                continue;
            }
            MATCH_CASE (const ScriptDataInt&, u)
            {
                if (u.numi == needle_int)
                {
                    c = i;
                    goto Out;
                }
                continue;
            }
        }
        MATCH_END ();
        abort();
    }
    Out:
    push_int<ScriptDataInt>(st->stack, c);
}

/*========================================
 * Send a message, prefixed with [GM], as a whisper to all online GMs of at
 * least the configured hack-info GM level.
 *
 * @doc wgm
 * @arg message: str; the message to send.
 * @ret none
 ========================================*/
static
void builtin_wgm(ScriptState *st)
{
    ZString message = ZString(conv_str(st, &AARG(0)));

    intif_wis_message_to_gm(WISP_SERVER_NAME,
            battle_config.hack_info_GM_level,
            STRPRINTF("[GM] %s"_fmt, message));
}

/*========================================
 * Write a message, prefixed with {SCRIPT}, to the GM/atcommand audit log,
 * attributed to the attached player.
 *
 * @doc gmlog
 * @arg message: str; the message to log.
 * @ret none
 ========================================*/
static
void builtin_gmlog(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    ZString message = ZString(conv_str(st, &AARG(0)));
    script_nullpo_end(sd, "player not found"_s);
    log_atcommand(sd, STRPRINTF("{SCRIPT} %s"_fmt, message));
}

/*========================================
 * Change a cosmetic look value of the attached player.
 *
 * @doc setlook
 * @arg type: int; a LOOK type: hair, weapon sprite, hair colour, and so on.
 * @arg value: int; the new value for that look.
 * @ret none
 ========================================*/
static
void builtin_setlook(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    LOOK type = LOOK(conv_num(st, &AARG(0)));
    int val = conv_num(st, &AARG(1));
    script_nullpo_end(sd, "player not found"_s);

    pc_changelook(sd, type, val);

}

/*==========================================
 * Get item parameter, supports both item ID and item name
 *------------------------------------------
 */
static ItemNameId get_item_id(ScriptState *st, struct script_data *data)
{
    ItemNameId nameid;

    get_val(st, data);
    if (data->is<ScriptDataStr>())
    {
        ZString name = ZString(conv_str(st, data));
        Option<P<struct item_data>> item_data_ = itemdb_searchname(name);
        OMATCH_BEGIN_SOME (item_data, item_data_)
        {
            nameid = item_data->nameid;
        }
        OMATCH_END ();
    }
    else
        nameid = wrap<ItemNameId>(conv_num(st, data));

    return nameid;
}

/*========================================
 * Return how many of an item the attached player carries in their
 * inventory.
 *
 * @doc countitem
 * @arg item: item; the item, by numeric id or name.
 * @ret int; the quantity carried.
 ========================================*/
static
void builtin_countitem(ScriptState *st)
{
    int count = 0;
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    ItemNameId nameid = get_item_id(st, &AARG(0));
    if (nameid)
    {
        for (IOff0 i : IOff0::iter())
        {
            if (sd->status.inventory[i].nameid == nameid)
                count += sd->status.inventory[i].amount;
        }
    }
    else
    {
        if (battle_config.error_log)
            PRINTF("builtin_countitem: no item ID\n"_fmt);
    }
    push_int<ScriptDataInt>(st->stack, count);

}

/*========================================
 * Test whether the attached player could carry more of an item without
 * exceeding their weight limit. Also 0 for an invalid item or a non-
 * positive amount.
 *
 * @doc checkweight
 * @arg item: item; the item, by numeric id or name.
 * @arg amount: amount; how many more units to test for.
 * @ret int; 1 if the items would fit, 0 otherwise.
 ========================================*/
static
void builtin_checkweight(ScriptState *st)
{
    int amount;
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    ItemNameId nameid = get_item_id(st, &AARG(0));

    amount = conv_num(st, &AARG(1));
    if (amount <= 0 || !nameid)
    {
        //If it gets the wrong item ID or the amount<=0, don't count its weight (assume it's a non-existent item)
        push_int<ScriptDataInt>(st->stack, 0);
        return;
    }

    if (itemdb_weight(nameid) * amount + sd->weight > sd->max_weight)
    {
        push_int<ScriptDataInt>(st->stack, 0);
    }
    else
    {
        push_int<ScriptDataInt>(st->stack, 1);
    }
}

/*========================================
 * Give units of an item to a player. By default the recipient is the
 * attached player; an optional being id gives the item to someone else.
 * Items that do not fit in the inventory are dropped at the recipient's
 * feet.
 *
 * @doc getitem
 * @arg item: item; the item, by numeric id or name.
 * @arg amount: amount; how many units to give.
 * @optarg unused: expr; unused; kept for argument-order compatibility.
 * @optarg gid: GID; being id of the recipient.
 * @ret none
 ========================================*/
static
void builtin_getitem(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    ItemNameId nameid = get_item_id(st, &AARG(0));
    int amount = conv_num(st, &AARG(1));

    if (!nameid || amount <= 0)
    {
        return;               //return if amount <=0, skip the useles iteration
    }

    Item item_tmp {};
    item_tmp.nameid = nameid;
    if (HARG(3))    //アイテムを指定したIDに渡す | Pass an item to the specified ID
        sd = map_id2sd(wrap<BlockId>(conv_num(st, &AARG(3))));
    if (sd == nullptr)         //アイテムを渡す相手がいなかったらお帰り | If you don't have anyone to give the item to, go home
        return;
    PickupFail flag;
    if ((flag = pc_additem(sd, &item_tmp, amount)) != PickupFail::OKAY)
    {
        clif_additem(sd, IOff0::from(0), 0, flag);
        map_addflooritem(&item_tmp, amount,
                sd->bl_m, sd->bl_x, sd->bl_y,
                nullptr, nullptr, nullptr);
    }
}

/*========================================
 * Create units of an item on the ground as a floor item. The map name
 * "this" places it on the attached player's current map.
 *
 * @doc makeitem
 * @arg item: item; the item, by numeric id or name.
 * @arg amount: amount; how many units to create.
 * @arg map: map; the map to drop the item on.
 * @arg x: coordinate; the x coordinate of the drop.
 * @arg y: coordinate; the y coordinate of the drop.
 * @ret none
 ========================================*/
static
void builtin_makeitem(ScriptState *st)
{
    int x, y;
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);

    ItemNameId nameid = get_item_id(st, &AARG(0));
    int amount = conv_num(st, &AARG(1));
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(2))));
    x = conv_num(st, &AARG(3));
    y = conv_num(st, &AARG(4));

    P<map_local> m = ((sd && mapname == MOB_THIS_MAP)
            ? sd->bl_m
            : TRY_UNWRAP(map_mapname2mapid(mapname), return));

    if (nameid)
    {
        Item item_tmp {};
        item_tmp.nameid = nameid;

        map_addflooritem(&item_tmp, amount, m, x, y, nullptr, nullptr, nullptr);
    }
}

/*========================================
 * Remove units of an item from the attached player's inventory.
 *
 * @doc delitem
 * @arg item: item; the item, by numeric id or name.
 * @arg amount: amount; how many units to remove.
 * @ret none
 ========================================*/
static
void builtin_delitem(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    ItemNameId nameid = get_item_id(st, &AARG(0));
    int amount = conv_num(st, &AARG(1));

    if (!nameid || amount <= 0)
    {
        //By Lupus. Don't run FOR if you've got the wrong item ID or amount<=0
        return;
    }

    for (IOff0 i : IOff0::iter())
    {
        if (sd->status.inventory[i].nameid == nameid)
        {
            if (sd->status.inventory[i].amount >= amount)
            {
                pc_delitem(sd, i, amount, 0);
                break;
            }
            else
            {
                amount -= sd->status.inventory[i].amount;
                if (amount == 0)
                    amount = sd->status.inventory[i].amount;
                pc_delitem(sd, i, amount, 0);
                break;
            }
        }
    }

}

/*========================================
 * Return the client-protocol version reported by the attached player's
 * client.
 *
 * @doc getversion
 * @ret int; the client protocol version number.
 ========================================*/
static
void builtin_getversion(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    push_int<ScriptDataInt>(st->stack, unwrap<ClientVersion>(sd->client_version));
}

/*========================================
 * Return an id of a character. The optional name picks the character;
 * without it the attached player is used.
 *
 * @doc getcharid
 * @arg type: int; which id: 0 character id, 1 party id, 2 always 0 (guilds
 *                 are unimplemented), 3 account id.
 * @optarg name: str; name of the character to query.
 * @ret int; the requested id, or -1 if no such player.
 ========================================*/
static
void builtin_getcharid(ScriptState *st)
{
    int num;
    dumb_ptr<map_session_data> sd;

    num = conv_num(st, &AARG(0));
    if (HARG(1))
        sd = map_nick2sd(stringish<CharName>(ZString(conv_str(st, &AARG(1)))));
    else
        sd = script_rid2sd(st);

    if (sd == nullptr) {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    }

    if (num == 0)
        push_int<ScriptDataInt>(st->stack, unwrap<CharId>(sd->status_key.char_id));
    if (num == 1)
        push_int<ScriptDataInt>(st->stack, unwrap<PartyId>(sd->status.party_id));
    if (num == 2)
        push_int<ScriptDataInt>(st->stack, 0/*guild_id*/);
    if (num == 3)
        push_int<ScriptDataInt>(st->stack, unwrap<AccountId>(sd->status_key.account_id));
}

/*========================================
 * Return the block-list id of an NPC. With a name argument that NPC is
 * looked up; otherwise the NPC running the script is used.
 *
 * @doc getnpcid
 * @optarg name: str; name of the NPC to look up.
 * @ret int; the NPC's block-list id, or -1 if not found.
 ========================================*/
static
void builtin_getnpcid(ScriptState *st)
{
    dumb_ptr<npc_data> nd;

    if (HARG(0))
        nd = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(0)))));
    else
        nd = map_id_is_npc(st->oid);
    if (nd == nullptr)
    {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    }

    push_int<ScriptDataInt>(st->stack, unwrap<BlockId>(nd->bl_id));
}

/*==========================================
 * 指定IDのPT名取得
 * Obtaining PT name of specified ID
 *------------------------------------------
 */
static
RString builtin_getpartyname_sub(PartyId party_id)
{
    Option<PartyPair> p = party_search(party_id);

    return p.pmd_pget(&PartyMost::name).copy_or(PartyName());
}

/*========================================
 * Return a string about a character. The optional being id picks the
 * character; without it the attached player is used.
 *
 * @doc strcharinfo
 * @arg type: int; which string: 0 character name, 1 party name, 2 empty
 *                 (guilds are unimplemented).
 * @optarg gid: GID; being id of the character to query.
 * @ret str; the requested string.
 ========================================*/
static
void builtin_strcharinfo(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;
    int num;

    if (HARG(1))    //指定したキャラを状態異常にする | Make the specified character abnormal
        sd = map_id_is_player(wrap<BlockId>(conv_num(st, &AARG(1))));
    else
        sd = script_rid2sd(st);

    script_nullpo_end(sd, "player not found"_s);

    num = conv_num(st, &AARG(0));
    if (num == 0)
    {
        RString buf = sd->status_key.name.to__actual();
        push_str<ScriptDataStr>(st->stack, buf);
    }
    if (num == 1)
    {
        RString buf = builtin_getpartyname_sub(sd->status.party_id);
        if (buf)
            push_str<ScriptDataStr>(st->stack, buf);
        else
            push_str<ScriptDataStr>(st->stack, ""_s);
    }
    if (num == 2)
    {
        // was: guild name
        push_str<ScriptDataStr>(st->stack, ""_s);
    }

}

// indexed by the equip_* in db/const.txt
// TODO change to use EQUIP
static
Array<EPOS, 11> equip //=
{{
    EPOS::HAT,
    EPOS::MISC1,
    EPOS::SHIELD,
    EPOS::WEAPON,
    EPOS::GLOVES,
    EPOS::SHOES,
    EPOS::CAPE,
    EPOS::MISC2,
    EPOS::TORSO,
    EPOS::LEGS,
    EPOS::ARROW,
}};

/*========================================
 * Return the item id worn in an equipment slot of a player. The optional
 * name picks the character; otherwise the attached player is used.
 *
 * @doc getequipid
 * @arg slot: int; equipment slot 1 to 11, indexing the equip_* constants.
 * @optarg name: str; name of the character to query.
 * @ret int; the worn item id, or -1 if the slot is empty.
 ========================================*/
static
void builtin_getequipid(ScriptState *st)
{
    int num;
    dumb_ptr<map_session_data> sd;

    if (HARG(1))
        sd = map_nick2sd(stringish<CharName>(ZString(conv_str(st, &AARG(1)))));
    else
        sd = script_rid2sd(st);

    script_nullpo_end(sd, "player not found"_s);
    num = conv_num(st, &AARG(0));
    IOff0 i = pc_checkequip(sd, equip[num - 1]);
    if (i.ok())
    {
        Option<P<struct item_data>> item_ = sd->inventory_data[i];
        OMATCH_BEGIN (item_)
        {
            OMATCH_CASE_SOME (item)
            {
                push_int<ScriptDataInt>(st->stack, unwrap<ItemNameId>(item->nameid));
            }
            OMATCH_CASE_NONE ()
            {
                push_int<ScriptDataInt>(st->stack, 0);
            }
        }
        OMATCH_END ();
    }
    else
    {
        push_int<ScriptDataInt>(st->stack, -1);
    }
}

/*========================================
 * Enable or disable free-looping for the current script, lifting the normal
 * instruction-count limit that guards against runaway loops.
 *
 * @doc freeloop
 * @arg enable: bool; 1 to enable free-looping, any other value to disable
 *                    it.
 * @ret none
 ========================================*/
static
void builtin_freeloop(ScriptState *st)
{
    int num;
    num = conv_num(st, &AARG(0));
    if(num == 1)
    {
        st->freeloop = 1;
    }
    else
    {
        st->freeloop = 0;
    }
}

/*========================================
 * Add an equipment-style stat bonus to the attached player. Intended for
 * item equip_script fragments; the effect lasts only until stats are
 * recalculated.
 *
 * @doc bonus
 * @arg type: int; an SP bonus type.
 * @arg value: int; the bonus value.
 * @ret none
 ========================================*/
static
void builtin_bonus(ScriptState *st)
{
    SP type;
    if (auto *u = AARG(0).get_if<ScriptDataParam>())
        type = u->reg.sp();
    else
        type = SP(conv_num(st, &AARG(0)));
    int val = conv_num(st, &AARG(1));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    pc_bonus(sd, type, val);

}

/*========================================
 * Like bonus, but for bonus types that take an extra parameter.
 *
 * @doc bonus2
 * @arg type: int; an SP bonus type.
 * @arg extra: int; the extra parameter for the bonus type.
 * @arg value: int; the bonus value.
 * @ret none
 ========================================*/
static
void builtin_bonus2(ScriptState *st)
{
    SP type;
    if (auto *u = AARG(0).get_if<ScriptDataParam>())
        type = u->reg.sp();
    else
        type = SP(conv_num(st, &AARG(0)));
    int type2 = conv_num(st, &AARG(1));
    int val = conv_num(st, &AARG(2));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    pc_bonus2(sd, type, type2, val);

}

/*========================================
 * Grant a skill at a given level to the attached player.
 *
 * @doc skill
 * @arg skill: int; the skill id to grant.
 * @arg level: int; the level to grant the skill at.
 * @optarg flag: int; flag passed to the skill-granting code; defaults to 1.
 * @ret none
 ========================================*/
static
void builtin_skill(ScriptState *st)
{
    int level, flag = 1;
    dumb_ptr<map_session_data> sd;

    SkillID id = SkillID(conv_num(st, &AARG(0)));
    level = conv_num(st, &AARG(1));
    if (HARG(2))
        flag = conv_num(st, &AARG(2));
    sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    pc_skill(sd, id, level, flag);
    clif_skillinfoblock(sd);

}

/*========================================
 * Set the attached player's level in a skill permanently, clamped to
 * 0..MAX_SKILL_LEVEL.
 *
 * @doc setskill
 * @arg skill: int; the skill id to set.
 * @arg level: int; the new skill level.
 * @ret none
 ========================================*/
static
void builtin_setskill(ScriptState *st)
{
    int level;
    dumb_ptr<map_session_data> sd;

    SkillID id = static_cast<SkillID>(conv_num(st, &AARG(0)));
    level = conv_num(st, &AARG(1));
    sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    level = std::min(level, MAX_SKILL_LEVEL);
    level = std::max(level, 0);
    sd->status.skill[id].lv = level;
    clif_skillinfoblock(sd);
}

/*========================================
 * Return the attached player's current level in a skill (0 if the player
 * does not have it).
 *
 * @doc getskilllv
 * @arg skill: int; the skill id to query.
 * @ret int; the player's level in that skill.
 ========================================*/
static
void builtin_getskilllv(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    SkillID id = SkillID(conv_num(st, &AARG(0)));
    script_nullpo_end(sd, "player not found"_s);
    push_int<ScriptDataInt>(st->stack, pc_checkskill(sd, id));
}

/*========================================
 * Override the attached player's normal attack with a spell. Called with no
 * arguments it removes the override and restores the normal attack.
 *
 * @doc overrideattack
 * @optarg delay: timer; attack delay in milliseconds.
 * @optarg range: int; attack range.
 * @optarg icon: status; a status-change icon id.
 * @optarg weapon: item; a weapon-look item id.
 * @optarg event: event; NPC event to run on attack.
 * @optarg charges: amount; number of charges; defaults to 1.
 * @ret none
 ========================================*/
static
void builtin_overrideattack(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    if (HARG(0))
    {
        interval_t attack_delay = static_cast<interval_t>(conv_num(st, &AARG(0)));
        int attack_range = conv_num(st, &AARG(1));
        StatusChange icon = StatusChange(conv_num(st, &AARG(2)));
        ItemNameId look = wrap<ItemNameId>(static_cast<uint16_t>(conv_num(st, &AARG(3))));
        ZString event_ = ZString(conv_str(st, &AARG(4)));

        NpcEvent event;
        extract(event_, &event);

        sd->attack_spell_override = st->oid;
        sd->attack_spell_charges = HARG(5) ? conv_num(st, &AARG(5)) : 1;
        sd->magic_attack = event;
        pc_set_weapon_icon(sd, 1, icon, look);
        pc_set_attack_info(sd, attack_delay, attack_range);
    }
    else
    {
        // explicit discharge
        sd->attack_spell_override = BlockId();
        pc_set_weapon_icon(sd, 0, StatusChange::ZERO, ItemNameId());
        pc_set_attack_info(sd, interval_t::zero(), 0);
        pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);
    }
}

/*========================================
 * Return the GM privilege bitmask of the attached player (0 for an ordinary
 * player).
 *
 * @doc getgmlevel
 * @ret int; the GM privilege bitmask.
 ========================================*/
static
void builtin_getgmlevel(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    push_int<ScriptDataInt>(st->stack, pc_isGM(sd).get_all_bits());
}

/*========================================
 * End the current script run. Using end inside a callfunc or callsub frame
 * is deprecated.
 *
 * @doc end
 * @ret none
 ========================================*/
static
void builtin_end(ScriptState *st)
{
    if (st->defsp >= 1 && st->stack->stack_datav[st->defsp - 1].is<ScriptDataRetInfo>())
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
        if(nd)
            PRINTF("Deprecated: end in a callfunc or callsub! @ %s\n"_fmt, nd->name);
        else
            PRINTF("Deprecated: end in a callfunc or callsub! (no npc)\n"_fmt);
    }
    st->state = ScriptEndState::END;
}

/*========================================
 * Return the attached player's opt2 status-option bitmask.
 *
 * @doc getopt2
 * @ret int; the opt2 bitmask.
 ========================================*/
static
void builtin_getopt2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    push_int<ScriptDataInt>(st->stack, static_cast<uint16_t>(sd->opt2));

}

/*========================================
 * Set the attached player's opt2 status-option bitmask and refresh the look
 * and stats.
 *
 * @doc setopt2
 * @arg opt2: int; the new opt2 bitmask.
 * @ret none
 ========================================*/
static
void builtin_setopt2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    Opt2 new_opt2 = Opt2(conv_num(st, &AARG(0)));
    sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    if (new_opt2 == sd->opt2)
        return;
    sd->opt2 = new_opt2;
    clif_changeoption(sd);
    pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);

}

/*========================================
 * Set the attached player's respawn point.
 *
 * @doc savepoint
 * @arg map: map; the respawn map name.
 * @arg x: coordinate; the respawn x coordinate.
 * @arg y: coordinate; the respawn y coordinate.
 * @ret none
 ========================================*/
static
void builtin_savepoint(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int x, y;
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    script_nullpo_end(sd, "player not found"_s);

    x = conv_num(st, &AARG(1));
    y = conv_num(st, &AARG(2));

    pc_setsavepoint(script_rid2sd(st), str, x, y);
}

/*========================================
 * Return a time value. Type 0 is the server tick (an unsigned counter that
 * wraps), type 1 is seconds elapsed since midnight, type 2 is the Unix
 * timestamp. Any other value behaves as type 0.
 *
 * @doc gettimetick
 * @arg type: int; which time value to return.
 * @ret int; the requested time value.
 ========================================*/
static
void builtin_gettimetick(ScriptState *st)   /* Asgard Version */
{
    int type;
    type = conv_num(st, &AARG(0));

    switch (type)
    {
        /* Number of seconds elapsed today(0-86399, 00:00:00-23:59:59). */
        case 1:
        {
            struct tm t = TimeT::now();
            push_int<ScriptDataInt>(st->stack,
                    t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec);
            break;
        }
        /* Seconds since Unix epoch. */
        case 2:
            push_int<ScriptDataInt>(st->stack, static_cast<time_t>(TimeT::now()));
            break;
        /* System tick(unsigned int, and yes, it will wrap). */
        case 0:
        default:
            push_int<ScriptDataInt>(st->stack, gettick().time_since_epoch().count());
            break;
    }
}

/*========================================
 * Return a component of the current local time.
 *
 * @doc gettime
 * @arg type: int; which component: 1 second, 2 minute, 3 hour, 4 weekday
 *                 (0..6), 5 day of month, 6 month (1..12), 7 full year.
 * @ret int; the requested time component, or -1 for an unknown type.
 ========================================*/
static
void builtin_gettime(ScriptState *st)   /* Asgard Version */
{
    int type = conv_num(st, &AARG(0));

    struct tm t = TimeT::now();

    switch (type)
    {
        case 1:                //Sec(0~59)
            push_int<ScriptDataInt>(st->stack, t.tm_sec);
            break;
        case 2:                //Min(0~59)
            push_int<ScriptDataInt>(st->stack, t.tm_min);
            break;
        case 3:                //Hour(0~23)
            push_int<ScriptDataInt>(st->stack, t.tm_hour);
            break;
        case 4:                //WeekDay(0~6)
            push_int<ScriptDataInt>(st->stack, t.tm_wday);
            break;
        case 5:                //MonthDay(01~31)
            push_int<ScriptDataInt>(st->stack, t.tm_mday);
            break;
        case 6:                //Month(01~12)
            push_int<ScriptDataInt>(st->stack, t.tm_mon + 1);
            break;
        case 7:                //Year(20xx)
            push_int<ScriptDataInt>(st->stack, t.tm_year + 1900);
            break;
        default:               //(format error)
            push_int<ScriptDataInt>(st->stack, -1);
            break;
    }
}

/*========================================
 * Open the attached player's Kafra storage window and suspend the script.
 *
 * @doc openstorage
 * @ret none
 ========================================*/
static
void builtin_openstorage(ScriptState *st)
{
//  int sync = 0;
//  if (st->end >= 3) sync = conv_num(st,& (st->stack->stack_data[st->start+2]));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

//  if (sync) {
    st->state = ScriptEndState::STOP;
    sd->npc_flags.storage = 1;
//  } else st->state = ScriptEndState::END;

    storage_storageopen(sd);
}

/*========================================
 * Grant base and job experience to the attached player. Negative amounts
 * are ignored.
 *
 * @doc getexp
 * @arg base: amount; base experience to grant.
 * @arg job: amount; job experience to grant.
 * @ret none
 ========================================*/
static
void builtin_getexp(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int base = 0, job = 0;
    script_nullpo_end(sd, "player not found"_s);

    base = conv_num(st, &AARG(0));
    job = conv_num(st, &AARG(1));
    if (base < 0 || job < 0)
        return;
    if (sd)
        pc_gainexp_reason(sd, base, job, PC_GAINEXP_REASON::SCRIPT);

}

/*==========================================
 * Returns attributes of a monster
 * return value -1 = mob not found
 *------------------------------------------
 */
static
int get_mob_drop_nameid(Species mob_id, int index)
{
    return unwrap<ItemNameId>(get_mob_db(mob_id).dropitem[index].nameid);
}
static
int get_mob_drop_percent(Species mob_id, int index)
{
    return get_mob_db(mob_id).dropitem[index].p.num;
}
static
AString get_mob_drop_name(Species mob_id, int index)
{
    Option<P<struct item_data>> i_data = Some(itemdb_search(get_mob_db(mob_id).dropitem[index].nameid));
    return i_data.pmd_pget(&item_data::name).copy_or(stringish<ItemName>(""_s));
}
/*========================================
 * Return a field of the monster-database entry for a species.
 *
 * @doc mobinfo
 * @arg species: mob; the monster species id.
 * @arg field: int; a MobInfo selector: id, names, level, HP/SP, experience,
 *                  attack and defence stats, attributes, element, mode,
 *                  speeds, and drop id/name/percent for drop slots 0..9.
 * @ret variant; the requested field, or -1 for an unknown species or
 *               selector.
 ========================================*/
static
void builtin_mobinfo(ScriptState *st)
{
    Species mob_id = wrap<Species>(conv_num(st, &AARG(0)));
    MobInfo request = MobInfo(conv_num(st, &AARG(1)));
    int info = 0;
    AString info_str;
    char mode = 0; // 0 = int, 1 = str

    if (mobdb_checkid(mob_id) == Species())
    {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    }

#define CASE_MobInfo_DROPID(index) \
        MobInfo::DROPID##index: info = get_mob_drop_nameid(mob_id, index)

#define CASE_MobInfo_DROPPERCENT(index) \
        MobInfo::DROPPERCENT##index: info = get_mob_drop_percent(mob_id, index)

#define CASE_MobInfo_DROPNAME(index) \
        MobInfo::DROPNAME##index: \
            { \
                info_str = get_mob_drop_name(mob_id, index); \
                mode = 1; \
            }

    switch (request)
    {
        case MobInfo::ID:
            info = unwrap<Species>(mob_id);
            break;
        case MobInfo::ENG_NAME:
            info_str = get_mob_db(mob_id).name;
            mode = 1;
            break;
        case MobInfo::JAP_NAME:
            info_str = get_mob_db(mob_id).jname;
            mode = 1;
            break;
        case MobInfo::LVL:
            info = get_mob_db(mob_id).lv;
            break;
        case MobInfo::HP:
            info = get_mob_db(mob_id).max_hp;
            break;
        case MobInfo::SP:
            info = get_mob_db(mob_id).max_sp;
            break;
        case MobInfo::BASE_EXP:
            info = get_mob_db(mob_id).base_exp;
            break;
        case MobInfo::JOB_EXP:
            info = get_mob_db(mob_id).job_exp;
            break;
        case MobInfo::RANGE1:
            info = get_mob_db(mob_id).range;
            break;
        case MobInfo::ATK1:
            info = get_mob_db(mob_id).atk1;
            break;
        case MobInfo::ATK2:
            info = get_mob_db(mob_id).atk2;
            break;
        case MobInfo::DEF:
            info = get_mob_db(mob_id).def;
            break;
        case MobInfo::MDEF:
            info = get_mob_db(mob_id).mdef;
            break;
        case MobInfo::CRITICAL_DEF:
            info = get_mob_db(mob_id).critical_def;
            break;
        case MobInfo::STR:
            info = get_mob_db(mob_id).attrs[ATTR::STR];
            break;
        case MobInfo::AGI:
            info = get_mob_db(mob_id).attrs[ATTR::AGI];
            break;
        case MobInfo::VIT:
            info = get_mob_db(mob_id).attrs[ATTR::VIT];
            break;
        case MobInfo::INT:
            info = get_mob_db(mob_id).attrs[ATTR::INT];
            break;
        case MobInfo::DEX:
            info = get_mob_db(mob_id).attrs[ATTR::DEX];
            break;
        case MobInfo::LUK:
            info = get_mob_db(mob_id).attrs[ATTR::LUK];
            break;
        case MobInfo::RANGE2:
            info = get_mob_db(mob_id).range2;
            break;
        case MobInfo::RANGE3:
            info = get_mob_db(mob_id).range3;
            break;
        case MobInfo::SCALE:
            info = get_mob_db(mob_id).size;
            break;
        case MobInfo::RACE:
            info = int(get_mob_db(mob_id).race);
            break;
        case MobInfo::ELEMENT:
            info = int(get_mob_db(mob_id).element.element);
            break;
        case MobInfo::ELEMENT_LVL:
            info = get_mob_db(mob_id).element.level;
            break;
        case MobInfo::MODE:
            info = int(get_mob_db(mob_id).mode);
            break;
        case MobInfo::SPEED:
            info = get_mob_db(mob_id).speed.count();
            break;
        case MobInfo::ADELAY:
            info = get_mob_db(mob_id).adelay.count();
            break;
        case MobInfo::AMOTION:
            info = get_mob_db(mob_id).amotion.count();
            break;
        case MobInfo::DMOTION:
            info = get_mob_db(mob_id).dmotion.count();
            break;
        case MobInfo::MUTATION_NUM:
            info = get_mob_db(mob_id).mutations_nr;
            break;
        case MobInfo::MUTATION_POWER:
            info = get_mob_db(mob_id).mutation_power;
            break;

        case CASE_MobInfo_DROPID(0); break;
        case CASE_MobInfo_DROPNAME(0); break;
        case CASE_MobInfo_DROPPERCENT(0); break;

        case CASE_MobInfo_DROPID(1); break;
        case CASE_MobInfo_DROPNAME(1); break;
        case CASE_MobInfo_DROPPERCENT(1); break;

        case CASE_MobInfo_DROPID(2); break;
        case CASE_MobInfo_DROPNAME(2); break;
        case CASE_MobInfo_DROPPERCENT(2); break;

        case CASE_MobInfo_DROPID(3); break;
        case CASE_MobInfo_DROPNAME(3); break;
        case CASE_MobInfo_DROPPERCENT(3); break;

        case CASE_MobInfo_DROPID(4); break;
        case CASE_MobInfo_DROPNAME(4); break;
        case CASE_MobInfo_DROPPERCENT(4); break;

        case CASE_MobInfo_DROPID(5); break;
        case CASE_MobInfo_DROPNAME(5); break;
        case CASE_MobInfo_DROPPERCENT(5); break;

        case CASE_MobInfo_DROPID(6); break;
        case CASE_MobInfo_DROPNAME(6); break;
        case CASE_MobInfo_DROPPERCENT(6); break;

        case CASE_MobInfo_DROPID(7); break;
        case CASE_MobInfo_DROPNAME(7); break;
        case CASE_MobInfo_DROPPERCENT(7); break;

        case CASE_MobInfo_DROPID(8); break;
        case CASE_MobInfo_DROPNAME(8); break;
        case CASE_MobInfo_DROPPERCENT(8); break;

        case CASE_MobInfo_DROPID(9); break;
        case CASE_MobInfo_DROPNAME(9); break;
        case CASE_MobInfo_DROPPERCENT(9); break;

        default:
            PRINTF("builtin_mobinfo: unknown request\n"_fmt);
            push_int<ScriptDataInt>(st->stack, -1);
            return;
            break;
    }
#undef CASE_MobInfo_DROPID
#undef CASE_MobInfo_DROPPERCENT
#undef CASE_MobInfo_DROPNAME

    if (!mode)
        push_int<ScriptDataInt>(st->stack, info);
    else
        push_str<ScriptDataStr>(st->stack, info_str);
}

/*========================================
 * Fill an array with the drops of a monster species.
 *
 * @doc mobinfo_droparrays
 * @arg species: mob; the monster species id.
 * @arg field: int; what to store: 0 item ids, 1 item names (string array),
 *                  2 drop percents.
 * @arg dest: var; the array variable to fill.
 * @ret int; 0 on error, 1 if the monster has drops, 2 if it has none.
 ========================================*/
static
void builtin_mobinfo_droparrays(ScriptState *st)
{
    dumb_ptr<block_list> bl = nullptr;

    Species mob_id = wrap<Species>(conv_num(st, &AARG(0)));

    MobInfo_DropArrays request = MobInfo_DropArrays(conv_num(st, &AARG(1)));

    SIR reg = AARG(2).get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());

    char prefix = name.front();
    char postfix = name.back();
    
    int status = 0; // 0 = mob not found or error, 1 = mob found and has drops, 2 = mob found and has no drops

    if (prefix != '$' && prefix != '@' && prefix != '.')
    {
        PRINTF("builtin_mobinfo_droparrays: illegal scope!\n"_fmt);
        push_int<ScriptDataInt>(st->stack, 0);
        return;
    }

    if (prefix == '.' && !name.startswith(".@"_s))
        bl = map_id_is_npc(st->oid);
    else if (prefix != '$' && !name.startswith(".@"_s))
    {
        bl = map_id_is_player(st->rid);
        script_nullpo_end(bl, "player not found"_s);
    }

    switch (request)
    {
        case MobInfo_DropArrays::IDS:
            if (postfix == '$')
            {
                PRINTF("builtin_mobinfo_droparrays: wrong array type for ID's (Int expected but String found)!\n"_fmt);
                push_int<ScriptDataInt>(st->stack, 0);
                return;
            }
            break;
        case MobInfo_DropArrays::NAMES:
            if (postfix != '$')
            {
                PRINTF("builtin_mobinfo_droparrays: wrong array type for Names (String expected but Int found)!\n"_fmt);
                push_int<ScriptDataInt>(st->stack, 0);
                return;
            }
            break;
        case MobInfo_DropArrays::PERCENTS:
            if (postfix == '$')
            {
                PRINTF("builtin_mobinfo_droparrays: wrong array type for Percents (Int expected but String found)!\n"_fmt);
                push_int<ScriptDataInt>(st->stack, 0);
                return;
            }
            break;
        default:
            PRINTF("builtin_mobinfo_droparrays: unknown request\n"_fmt);
            push_int<ScriptDataInt>(st->stack, 0);
            return;
            break;
    }

    if (mobdb_checkid(mob_id) == Species())
    {
        push_int<ScriptDataInt>(st->stack, status);
        return;
    }
    
    for (int i = 0; i < MaxDrops; ++i)
    {
        auto& dropitem = get_mob_db(mob_id).dropitem[i];
        if (dropitem.nameid)
        {
            status = 1;
            switch (request)
            {
                case MobInfo_DropArrays::IDS:
                    if (name.startswith(".@"_s))
                        {
                            struct script_data vd = script_data(ScriptDataInt{unwrap<ItemNameId>(dropitem.nameid)});
                            set_scope_reg(st, reg.iplus(i), &vd);
                        }
                    else
                        set_reg(bl, VariableCode::VARIABLE, reg.iplus(i), unwrap<ItemNameId>(dropitem.nameid));
                    break;
                case MobInfo_DropArrays::NAMES:
                    {
                        Option<P<struct item_data>> i_data = Some(itemdb_search(dropitem.nameid));
                        RString item_name = i_data.pmd_pget(&item_data::name).copy_or(stringish<ItemName>(""_s));

                        if (name.startswith(".@"_s))
                        {
                            struct script_data vd = script_data(ScriptDataStr{item_name});
                            set_scope_reg(st, reg.iplus(i), &vd);
                        }
                        else
                            set_reg(bl, VariableCode::VARIABLE, reg.iplus(i), item_name);
                        break;
                    }
                case MobInfo_DropArrays::PERCENTS:
                    if (name.startswith(".@"_s))
                        {
                            struct script_data vd = script_data(ScriptDataInt{dropitem.p.num});
                            set_scope_reg(st, reg.iplus(i), &vd);
                        }
                    else
                        set_reg(bl, VariableCode::VARIABLE, reg.iplus(i), dropitem.p.num);
                    break;
            }
        }
        else
        {
            if (i == 0)
                status = 2;
            break;
        }
    }
    push_int<ScriptDataInt>(st->stack, status);
}

/*========================================
 * Store the drops of a monster species into the global arrays
 * $@MobDrop_item, $@MobDrop_name$ and $@MobDrop_rate, with the count in
 * $@MobDrop_count.
 *
 * @doc getmobdrops
 * @arg species: mob; the monster species id.
 * @ret int; 0 if the species is unknown, 1 if it has drops, 2 if it has
 *           none.
 ========================================*/
static
void builtin_getmobdrops(ScriptState *st)
{
    dumb_ptr<block_list> bl = nullptr;

    Species mob_id = wrap<Species>(conv_num(st, &AARG(0)));

    int status = 0; // 0 = mob not found, 1 = mob found and has drops, 2 = mob found and has no drops
    int i = 0;

    if (mobdb_checkid(mob_id) == Species())
    {
        push_int<ScriptDataInt>(st->stack, status);
        return;
    }

    status = 1;

    const mob_db_& mob_info = get_mob_db(mob_id);
    for (; i < MaxDrops; ++i)
    {
        auto& dropitem = mob_info.dropitem[i];
        if (dropitem.nameid)
        {
            set_reg(bl, VariableCode::VARIABLE, SIR::from(variable_names.intern("$@MobDrop_item"_s), i), dropitem.p.num);

            Option<P<struct item_data>> i_data = Some(itemdb_search(dropitem.nameid));
            RString item_name = i_data.pmd_pget(&item_data::name).copy_or(stringish<ItemName>(""_s));
            set_reg(bl, VariableCode::VARIABLE, SIR::from(variable_names.intern("$@MobDrop_name$"_s), i), item_name);

            set_reg(bl, VariableCode::VARIABLE, SIR::from(variable_names.intern("$@MobDrop_rate"_s), i), dropitem.p.num);
        }
        else
        {
            if (i == 0)
                status = 2;
            break;
        }
    }

    if (status == 1)
        set_reg(bl, VariableCode::VARIABLE, SIR::from(variable_names.intern("$@MobDrop_count"_s), 0), i);
    else
        set_reg(bl, VariableCode::VARIABLE, SIR::from(variable_names.intern("$@MobDrop_count"_s), 0), 0);

    push_int<ScriptDataInt>(st->stack, status);
}

/*========================================
 * Spawn a single monster owned by another being.
 *
 * @doc summon
 * @arg map: map; the map to spawn on.
 * @arg x: coordinate; the spawn x coordinate.
 * @arg y: coordinate; the spawn y coordinate.
 * @arg owner: str; the owner's being id, passed as a string.
 * @arg name: str; the monster's display name.
 * @arg species: mob; the monster species id.
 * @arg attitude: int; an attitude code: servant, friendly, hostile or
 *                     frozen.
 * @arg lifespan: int; how long the monster lives, in milliseconds.
 * @optarg event: event; NPC event to run on the monster's death.
 * @ret none
 ========================================*/
static
void builtin_summon(ScriptState *st)
{
    NpcEvent event;
    MapName map = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    int x = conv_num(st, &AARG(1));
    int y = conv_num(st, &AARG(2));
    dumb_ptr<block_list> owner_e = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(3))));
    dumb_ptr<map_session_data> owner = nullptr;
    MobName str = stringish<MobName>(ZString(conv_str(st, &AARG(4))));
    Species monster_id = wrap<Species>(conv_num(st, &AARG(5)));
    MonsterAttitude monster_attitude = static_cast<MonsterAttitude>(conv_num(st, &AARG(6)));
    interval_t lifespan = static_cast<interval_t>(conv_num(st, &AARG(7)));
    if (HARG(8))
        extract(ZString(conv_str(st, &AARG(8))), &event);

    if (!owner_e)
    {
        PRINTF("builtin_summon: bad owner\n"_fmt);
        return;
    }

    if (monster_attitude == MonsterAttitude::SERVANT
        && owner_e->bl_type == BL::PC)
        owner = owner_e->is_player(); // XXX in the future this should also work with mobs as owner

    if (!str)
        str = MobName();
    BlockId mob_id = mob_once_spawn(owner, map, x, y, str, monster_id, 1, event);
    dumb_ptr<mob_data> mob = map_id_is_mob(mob_id);

    if (mob)
    {
        mob->mode = get_mob_db(monster_id).mode;

        switch (monster_attitude)
        {
            case MonsterAttitude::SERVANT:
                mob->state.special_mob_ai = 1;
                mob->mode |= MobMode::AGGRESSIVE;
                break;

            case MonsterAttitude::FRIENDLY:
                mob->mode = MobMode::CAN_ATTACK | (mob->mode & MobMode::CAN_MOVE);
                break;

            case MonsterAttitude::HOSTILE:
                mob->mode = MobMode::CAN_ATTACK | MobMode::AGGRESSIVE | (mob->mode & MobMode::CAN_MOVE);
                if (owner)
                {
                    mob->target_id = owner->bl_id;
                    mob->attacked_id = owner->bl_id;
                }
                break;

            case MonsterAttitude::FROZEN:
                mob->mode = MobMode::ZERO;
                break;
        }

        mob->mode |=
            MobMode::SUMMONED;  // | MobMode::TURNS_AGAINST_BAD_MASTER; <- its fun but bugged.
                                //   This flag identified to be source of AFK PK city exploits, etc.
        mob->deletetimer = Timer(gettick() + lifespan,
                std::bind(mob_timer_delete, ph::_1, ph::_2,
                    mob_id));

        if (owner)
        {
            mob->master_id = owner->bl_id;
            mob->master_dist = 6;
        }
    }
}

/*========================================
 * Spawn one or more monsters of a species at a fixed cell.
 *
 * @doc monster
 * @arg map: map; the map to spawn on.
 * @arg x: coordinate; the spawn x coordinate.
 * @arg y: coordinate; the spawn y coordinate.
 * @arg name: str; the monster's display name.
 * @arg species: mob; the monster species id.
 * @arg amount: amount; how many monsters to spawn.
 * @optarg event: event; NPC event run when each monster dies.
 * @ret none
 ========================================*/
static
void builtin_monster(ScriptState *st)
{
    Species mob_class;
    int amount, x, y;
    NpcEvent event;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x = conv_num(st, &AARG(1));
    y = conv_num(st, &AARG(2));
    MobName str = stringish<MobName>(ZString(conv_str(st, &AARG(3))));
    mob_class = wrap<Species>(conv_num(st, &AARG(4)));
    amount = conv_num(st, &AARG(5));
    if (HARG(6))
        extract(ZString(conv_str(st, &AARG(6))), &event);

    mob_once_spawn(map_id2sd(st->rid), mapname, x, y, str, mob_class, amount,
            event);
}

/*========================================
 * Spawn one or more monsters of a species scattered within a rectangle.
 *
 * @doc areamonster
 * @arg map: map; the map to spawn on.
 * @arg x0: coordinate; x of one corner of the rectangle.
 * @arg y0: coordinate; y of one corner of the rectangle.
 * @arg x1: coordinate; x of the opposite corner.
 * @arg y1: coordinate; y of the opposite corner.
 * @arg name: str; the monster's display name.
 * @arg species: mob; the monster species id.
 * @arg amount: amount; how many monsters to spawn.
 * @optarg event: event; NPC event run when each monster dies.
 * @ret none
 ========================================*/
static
void builtin_areamonster(ScriptState *st)
{
    Species mob_class;
    int amount, x0, y0, x1, y1;
    NpcEvent event;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x0 = conv_num(st, &AARG(1));
    y0 = conv_num(st, &AARG(2));
    x1 = conv_num(st, &AARG(3));
    y1 = conv_num(st, &AARG(4));
    MobName str = stringish<MobName>(ZString(conv_str(st, &AARG(5))));
    mob_class = wrap<Species>(conv_num(st, &AARG(6)));
    amount = conv_num(st, &AARG(7));
    if (HARG(8))
        extract(ZString(conv_str(st, &AARG(8))), &event);

    mob_once_spawn_area(map_id2sd(st->rid), mapname, x0, y0, x1, y1, str, mob_class,
            amount, event);
}

/*==========================================
 * モンスター削除
 * Monster Removal
 *------------------------------------------
 */
static
void builtin_killmonster_sub(dumb_ptr<block_list> bl, NpcEvent event)
{
    dumb_ptr<mob_data> md = bl->is_mob();
    if (event)
    {
        if (event == md->npc_event)
            mob_delete(md);
        return;
    }
    else if (!event)
    {
        if (md->spawn.delay1 == static_cast<interval_t>(-1)
            && md->spawn.delay2 == static_cast<interval_t>(-1))
            mob_delete(md);
        return;
    }
}

/*========================================
 * Remove monsters on a map. With the special event string "All", all non-
 * permanently-spawned monsters are removed; otherwise only monsters whose
 * death event matches.
 *
 * @doc killmonster
 * @arg map: map; the map to clear.
 * @arg event: event; the death event to match, or "All".
 * @ret none
 ========================================*/
static
void builtin_killmonster(ScriptState *st)
{
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    ZString event_ = ZString(conv_str(st, &AARG(1)));
    NpcEvent event;
    if (event_ != "All"_s)
        extract(event_, &event);

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);
    map_foreachinarea(std::bind(builtin_killmonster_sub, ph::_1, event),
            m,
            0, 0,
            m->xs, m->ys,
            BL::MOB);
}

/*========================================
 * Trigger an NPC event immediately, running it as an event-label script.
 *
 * @doc donpcevent
 * @arg event: event; the NPC event to run.
 * @ret none
 ========================================*/
static
void builtin_donpcevent(ScriptState *st)
{
    ZString event_ = ZString(conv_str(st, &AARG(0)));
    NpcEvent event;
    extract(event_, &event);
    npc_event_do(event);
}

/*========================================
 * Schedule an NPC event to run for a player after a delay. The optional
 * being id selects the player; otherwise the attached player is used.
 *
 * @doc addtimer
 * @arg delay: timer; delay in milliseconds before the event runs.
 * @arg event: event; the NPC event to schedule.
 * @optarg gid: GID; being id of the player to run it for.
 * @ret none
 ========================================*/
static
void builtin_addtimer(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARG(0)));
    ZString event_ = ZString(conv_str(st, &AARG(1)));
    NpcEvent event;
    extract(event_, &event);

    if (HARG(2))
        sd = map_id_is_player(wrap<BlockId>(conv_num(st, &AARG(2))));
    else if (st->rid)
        sd = script_rid2sd(st);

    script_nullpo_end(sd, "player not found"_s);

    pc_addeventtimer(sd, tick, event);
}

/*========================================
 * Schedule an NPC event to run after a delay, attached to the NPC named in
 * the event.
 *
 * @doc addnpctimer
 * @arg delay: timer; delay in milliseconds before the event runs.
 * @arg event: event; the NPC event to schedule.
 * @ret none
 ========================================*/
static
void builtin_addnpctimer(ScriptState *st)
{
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARG(0)));
    ZString event_ = ZString(conv_str(st, &AARG(1)));
    NpcEvent event;
    extract(event_, &event);
    npc_addeventtimer(npc_name2id(event.npc), tick, event);
}

/*========================================
 * Reset the NPC's timer to zero and start it running, which drives that
 * NPC's OnTimer<ms> labels. The optional argument names another NPC.
 *
 * @doc initnpctimer
 * @optarg npc: str; name of the NPC to act on; defaults to the script's own
 *                   NPC.
 * @ret none
 ========================================*/
static
void builtin_initnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    if (HARG(0))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(0)))));
    else
        nd_ = map_id_is_npc(st->oid);
    script_nullpo_end(nd_, "no npc"_s);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_settimerevent_tick(nd, interval_t::zero());
    npc_timerevent_start(nd);
}

/*========================================
 * Start or resume the NPC timer without resetting it. The optional argument
 * names another NPC.
 *
 * @doc startnpctimer
 * @optarg npc: str; name of the NPC to act on; defaults to the script's own
 *                   NPC.
 * @ret none
 ========================================*/
static
void builtin_startnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    if (HARG(0))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(0)))));
    else
        nd_ = map_id_is_npc(st->oid);
    script_nullpo_end(nd_, "no npc"_s);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_timerevent_start(nd);
}

/*========================================
 * Stop the NPC timer. The optional argument names another NPC.
 *
 * @doc stopnpctimer
 * @optarg npc: str; name of the NPC to act on; defaults to the script's own
 *                   NPC.
 * @ret none
 ========================================*/
static
void builtin_stopnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    if (HARG(0))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(0)))));
    else
        nd_ = map_id_is_npc(st->oid);
    script_nullpo_end(nd_, "no npc"_s);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_timerevent_stop(nd);
}

/*========================================
 * Return information about an NPC timer. The optional name picks another
 * NPC; otherwise the script's own NPC is used.
 *
 * @doc getnpctimer
 * @arg type: int; which value: 0 current tick in milliseconds, 1 whether
 *                 the timer is active, 2 number of OnTimer events defined.
 * @optarg npc: str; name of the NPC to query.
 * @ret int; the requested timer value.
 ========================================*/
static
void builtin_getnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    int type = conv_num(st, &AARG(0));
    int val = 0;
    if (HARG(1))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(1)))));
    else
        nd_ = map_id_is_npc(st->oid);
    script_nullpo_end(nd_, "no npc"_s);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    switch (type)
    {
        case 0:
            val = npc_gettimerevent_tick(nd).count();
            break;
        case 1:
            val = nd->scr.timer_active;
            break;
        case 2:
            val = nd->scr.timer_eventv.size();
            break;
    }
    push_int<ScriptDataInt>(st->stack, val);
}

/*========================================
 * Set the NPC timer to a number of milliseconds. The optional name picks
 * another NPC; otherwise the script's own NPC is used.
 *
 * @doc setnpctimer
 * @arg tick: timer; the new timer value, in milliseconds.
 * @optarg npc: str; name of the NPC to act on.
 * @ret none
 ========================================*/
static
void builtin_setnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARG(0)));
    if (HARG(1))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(1)))));
    else
        nd_ = map_id_is_npc(st->oid);
    script_nullpo_end(nd_, "no npc"_s);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_settimerevent_tick(nd, tick);
}

/*========================================
 * Send a raw NPC client-action command to the attached player.
 *
 * @doc npcaction
 * @arg command: int; the client-action command code.
 * @optarg target: expr; a target being id, or an NPC name when the command
 *                       is 2.
 * @optarg x: coordinate; an x coordinate for the command.
 * @optarg y: coordinate; a y coordinate for the command.
 * @ret none
 ========================================*/
static
void builtin_npcaction(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    short command = conv_num(st, &AARG(0));
    int id = 0;
    short x = HARG(2) ? conv_num(st, &AARG(2)) : 0;
    short y = HARG(3) ? conv_num(st, &AARG(3)) : 0;
    script_nullpo_end(sd, "player not found"_s);

    if(HARG(1))
    {
        if(command == 2)
        {
            dumb_ptr<npc_data> nd_;
            nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(1)))));
            id = unwrap<BlockId>(nd_->bl_id);
        }
        else
            id = conv_num(st, &AARG(1));
    }

    clif_npc_action(sd, st->oid, command, id, x, y);
}

/*========================================
 * Control the attached player's camera. With no arguments it returns the
 * camera to the player. Two arguments move it to absolute coordinates. One
 * or three arguments centre it on an actor.
 *
 * @doc camera
 * @optarg actor: expr; a being id, or one of rid/player, oid/npc, relative,
 *                      or an NPC name; or an x coordinate when moving to
 *                      absolute coordinates.
 * @optarg x: coordinate; x offset from the actor, or a y coordinate.
 * @optarg y: coordinate; y offset from the actor.
 * @ret none
 ========================================*/
static
void builtin_camera(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    if (!HARG(0))
    {
        // no arguments: return camera
        clif_npc_action(sd, st->oid, 3, 0, 0, 0);
        return;
    }

    if (HARG(1) && !HARG(2))
    {
        // two arguments: camera to x, y
        clif_npc_action(sd, st->oid, 2, 0, conv_num(st, &AARG(0)), conv_num(st, &AARG(1)));
        return;
    }

    // one or three arguments: camera relative to actor by x, y
    dumb_ptr<block_list> bl;
    short x = 0, y = 0;

    if (HARG(1) && HARG(2))
    {
        x = conv_num(st, &AARG(1));
        y = conv_num(st, &AARG(2));
    }

    if (auto *u = AARG(0).get_if<ScriptDataInt>())
    {
        // interpret as block id (generally player or npc)
        bl = map_id2bl(wrap<BlockId>(u->numi));
    }
    else if (auto *g = AARG(0).get_if<ScriptDataStr>())
    {
        if (g->str == "relative"_s)
        {
            // camera relative from current camera
            clif_npc_action(sd, st->oid, 4, 0, x, y);
            return;
        }

        if (g->str == "rid"_s || g->str == "player"_s)
            bl = sd;  // player
        else if (g->str == "oid"_s || g->str == "npc"_s)
            bl = map_id2bl(st->oid);  // script NPC
        else
            bl = npc_name2id(stringish<NpcName>(g->str));  // NPC name
    }

    // camera relative to actor
    clif_npc_action(sd, st->oid, 2, unwrap<BlockId>(bl->bl_id), x, y);
}

/*========================================
 * Set the facing direction and sitting state shown for an NPC. The optional
 * name picks another NPC; otherwise the script's own NPC is used.
 *
 * @doc setnpcdirection
 * @arg direction: int; the facing direction.
 * @arg sit: bool; nonzero to make the NPC sit.
 * @arg persistent: bool; nonzero to make the change persistent rather than
 *                        per-viewer.
 * @optarg npc: str; name of the NPC to act on.
 * @ret none
 ========================================*/
static
void builtin_setnpcdirection(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    DIR dir = static_cast<DIR>(conv_num(st, &AARG(0)));
    bool save = bool(conv_num(st, &AARG(2)));
    DamageType action;

    if (HARG(3))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(3)))));
    else
        nd_ = map_id_is_npc(st->oid);

    script_nullpo_end(nd_, "no npc"_s);

    if (bool(conv_num(st, &AARG(1))))
        action = DamageType::SIT;
    else
        action = DamageType::STAND;

    if (save)
    {
        nd_->dir = dir;
        nd_->sit = action;
    }

    if (st->rid)
    {
        dumb_ptr<map_session_data> sd = script_rid2sd(st);
        script_nullpo_end(sd, "player not found"_s);
        clif_sitnpc_towards(sd, nd_, action);
        clif_setnpcdirection_towards(sd, nd_, dir);
    }
    else
    {
        clif_sitnpc(nd_, action);
        clif_setnpcdirection(nd_, dir);
    }
}

/*========================================
 * Broadcast a message. The flag controls scope and styling: with the low
 * bits clear it is a server-wide announcement; with bit 0x08 set it is
 * limited to the running NPC's surroundings, otherwise to the player's.
 *
 * @doc announce
 * @arg message: str; the message text.
 * @arg flag: int; scope and styling flags.
 * @ret none
 ========================================*/
static
void builtin_announce(ScriptState *st)
{
    int flag;
    ZString str = ZString(conv_str(st, &AARG(0)));
    flag = conv_num(st, &AARG(1));

    if (flag & 0x0f)
    {
        dumb_ptr<block_list> bl;
        if (flag & 0x08)
            bl = map_id2bl(st->oid);
        else
            bl = script_rid2sd(st);
        script_nullpo_end(bl, "player not found"_s);
        clif_GMmessage(bl, str, flag);
    }
    else
        intif_GMmessage(str);
}

/*==========================================
 * 天の声アナウンス（特定マップ）
 * Voice of Heaven Announcement (Specific Map)
 *------------------------------------------
 */
static
void builtin_mapannounce_sub(dumb_ptr<block_list> bl, XString str, int flag)
{
    clif_GMmessage(bl, str, flag | 3);
}

/*========================================
 * Broadcast a message to every player on a map.
 *
 * @doc mapannounce
 * @arg map: map; the map to broadcast on.
 * @arg message: str; the message text.
 * @arg flag: int; styling flags.
 * @ret none
 ========================================*/
static
void builtin_mapannounce(ScriptState *st)
{
    int flag;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    ZString str = ZString(conv_str(st, &AARG(1)));
    flag = conv_num(st, &AARG(2));

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);
    map_foreachinarea(std::bind(builtin_mapannounce_sub, ph::_1, str, flag & 0x10),
            m,
            0, 0,
            m->xs, m->ys,
            BL::PC);
}

/*========================================
 * Return a user count. Flag 1 returns the total players on the server; flag
 * 0 is disabled (use getmapusers). Bit 0x08 selects the NPC rather than the
 * player as the reference being.
 *
 * @doc getusers
 * @arg flag: int; which count and reference being.
 * @ret int; the requested user count.
 ========================================*/
static
void builtin_getusers(ScriptState *st)
{
    int flag = conv_num(st, &AARG(0));
    dumb_ptr<block_list> bl = map_id2bl((flag & 0x08) ? st->oid : st->rid);
    int val = 0;
    switch (flag & 0x07)
    {
        case 0:
            // FIXME: SIGSEGV error
            //val = bl->bl_m->users;
            PRINTF("Sorry, but getusers(0) is disabled. Please use getmapusers() instead.\n"_fmt);
            break;
        case 1:
            val = map_getusers();
            break;
    }
    push_int<ScriptDataInt>(st->stack, val);
}

/*==========================================
 * マップ指定ユーザー数所得
 * maps Designated User Count
 *------------------------------------------
 */
 static
 void builtin_getareausers_sub(dumb_ptr<block_list> bl, int *users)
 {
     if (bool(bl->is_player()->status.option & Opt0::HIDE))
         return;
     (*users)++;
 }

/*========================================
 * Return the number of players currently on a map.
 *
 * @doc getmapusers
 * @arg map: map; the map to count players on.
 * @ret int; the player count, or -1 if the map does not exist.
 ========================================*/
static
void builtin_getmapusers(ScriptState *st)
{
    int users = 0;
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(str),
    {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    });
    map_foreachinarea(std::bind(builtin_getareausers_sub, ph::_1, &users),
            m,
            0, 0,
            m->xs, m->ys,
            BL::PC);
    push_int<ScriptDataInt>(st->stack, users);
}

/*========================================
 * Make a monster become aggressive towards a target. By default the target
 * is the attached player.
 *
 * @doc aggravate
 * @arg gid: GID; being id of the monster.
 * @optarg target: GID; being id of the target to aggravate against.
 * @ret none
 ========================================*/
static
void builtin_aggravate(ScriptState *st)
{
    dumb_ptr<mob_data> md = map_id_is_mob(wrap<BlockId>(conv_num(st, &AARG(0))));
    if (md)
    {
        dumb_ptr<block_list> target = script_rid2sd(st);
        if (HARG(1))
            target = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))));

        nullpo_retv(target);
        mob_aggravate(md, target);
    }
}

/*========================================
 * Test whether a monster is a summoned creature.
 *
 * @doc issummon
 * @arg gid: GID; being id of the monster.
 * @ret int; 1 if the monster is summoned, 0 otherwise.
 ========================================*/
static
void builtin_issummon(ScriptState *st)
{
    dumb_ptr<mob_data> md = map_id_is_mob(wrap<BlockId>(conv_num(st, &AARG(0))));
    int val = 0;
    if (md)
    {
        val = bool(md->mode & MobMode::SUMMONED); 
    }
    
    push_int<ScriptDataInt>(st->stack, val);
}

/*==========================================
 * エリア指定ユーザー数所得
 * Area Designated User Income
 *------------------------------------------
 */

static
void builtin_getareausers_living_sub(dumb_ptr<block_list> bl, int *users)
{
    if (bool(bl->is_player()->status.option & Opt0::HIDE))
        return;
    if (!pc_isdead(bl->is_player()))
        (*users)++;
}

/*========================================
 * Return the number of players inside a rectangle on a map. Hidden players
 * are never counted.
 *
 * @doc getareausers
 * @arg map: map; the map holding the rectangle.
 * @arg x0: coordinate; x of one corner of the rectangle.
 * @arg y0: coordinate; y of one corner of the rectangle.
 * @arg x1: coordinate; x of the opposite corner.
 * @arg y1: coordinate; y of the opposite corner.
 * @optarg living_only: bool; nonzero to also exclude dead players.
 * @ret int; the player count, or -1 if the map does not exist.
 ========================================*/
static
void builtin_getareausers(ScriptState *st)
{
    int x0, y0, x1, y1, users = 0;
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x0 = conv_num(st, &AARG(1));
    y0 = conv_num(st, &AARG(2));
    x1 = conv_num(st, &AARG(3));
    y1 = conv_num(st, &AARG(4));

    int living = 0;
    if (HARG(5))
    {
        living = conv_num(st, &AARG(5));
    }
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(str),
    {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    });
    map_foreachinarea(std::bind(living ? builtin_getareausers_living_sub: builtin_getareausers_sub, ph::_1, &users),
            m,
            x0, y0,
            x1, y1,
            BL::PC);
    push_int<ScriptDataInt>(st->stack, users);
}

/*==========================================
 * エリア指定ドロップアイテム数所得
 * Area Designated Drop Item Number Income
 *------------------------------------------
 */
static
void builtin_getareadropitem_sub(dumb_ptr<block_list> bl, ItemNameId item, int *amount)
{
    dumb_ptr<flooritem_data> drop = bl->is_item();

    if (drop->item_data.nameid == item)
        (*amount) += drop->item_data.amount;

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_getareadropitem_sub_anddelete(dumb_ptr<block_list> bl, ItemNameId item, int *amount)
{
    dumb_ptr<flooritem_data> drop = bl->is_item();

    if (drop->item_data.nameid == item)
    {
        (*amount) += drop->item_data.amount;
        clif_clearflooritem(drop, nullptr);
        map_delobject(drop->bl_id, drop->bl_type);
    }
}

/*========================================
 * Return the total quantity of floor items of a type lying inside a
 * rectangle on a map.
 *
 * @doc getareadropitem
 * @arg map: map; the map holding the rectangle.
 * @arg x0: coordinate; x of one corner of the rectangle.
 * @arg y0: coordinate; y of one corner of the rectangle.
 * @arg x1: coordinate; x of the opposite corner.
 * @arg y1: coordinate; y of the opposite corner.
 * @arg item: item; the item to count, by id or name.
 * @optarg remove: bool; nonzero to also remove the matched floor items.
 * @ret int; the total quantity, or -1 if the map does not exist.
 ========================================*/
static
void builtin_getareadropitem(ScriptState *st)
{
    int amount = 0, delitems = 0;
    struct script_data *data;

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    int x0 = conv_num(st, &AARG(1));
    int y0 = conv_num(st, &AARG(2));
    int x1 = conv_num(st, &AARG(3));
    int y1 = conv_num(st, &AARG(4));

    ItemNameId nameid = get_item_id(st, &AARG(5));

    if (HARG(6))
        delitems = conv_num(st, &AARG(6));

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(str),
    {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    });
    if (delitems)
        map_foreachinarea(std::bind(builtin_getareadropitem_sub_anddelete, ph::_1, nameid, &amount),
                m,
                x0, y0,
                x1, y1,
                BL::ITEM);
    else
        map_foreachinarea(std::bind(builtin_getareadropitem_sub, ph::_1, nameid, &amount),
                m,
                x0, y0,
                x1, y1,
                BL::ITEM);

    push_int<ScriptDataInt>(st->stack, amount);
}

/*========================================
 * Make a named NPC visible and interactive.
 *
 * @doc enablenpc
 * @arg npc: str; name of the NPC to enable.
 * @ret none
 ========================================*/
static
void builtin_enablenpc(ScriptState *st)
{
    NpcName str = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
    npc_enable(str, 1);
}

/*========================================
 * Make a named NPC invisible and non-interactive.
 *
 * @doc disablenpc
 * @arg npc: str; name of the NPC to disable.
 * @ret none
 ========================================*/
static
void builtin_disablenpc(ScriptState *st)
{
    NpcName str = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
    npc_enable(str, 0);
}

/*========================================
 * Apply a status condition to a being for a duration. By default the
 * attached player is affected. For a few legacy potion-style conditions a
 * duration under one second is reinterpreted as seconds.
 *
 * @doc sc_start
 * @arg duration: int; duration in milliseconds.
 * @arg status: status; the status-condition id to apply.
 * @arg value: int; the value parameter for the condition.
 * @optarg gid: GID; being id to affect instead of the player.
 * @ret none
 ========================================*/
static
void builtin_sc_start(ScriptState *st)
{
    dumb_ptr<block_list> bl;
    int val1;
    StatusChange type = static_cast<StatusChange>(conv_num(st, &AARG(0)));
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARG(1)));
    if (tick < 1_s)
        switch (type)
        {
            // all those use ms so this checks for < 1s are not needed on those
            // and it would break the cooldown symbol since many spells have cooldowns less than 1s
            case StatusChange::SC_PHYS_SHIELD:
            case StatusChange::SC_PHYS_SHIELD_ITEM:
            case StatusChange::SC_MBARRIER:
            case StatusChange::SC_COOLDOWN:
            case StatusChange::SC_COOLDOWN_MG:
            case StatusChange::SC_COOLDOWN_MT:
            case StatusChange::SC_COOLDOWN_R:
            case StatusChange::SC_COOLDOWN_AR:
            case StatusChange::SC_COOLDOWN_ENCH:
            case StatusChange::SC_COOLDOWN_KOY:
            case StatusChange::SC_COOLDOWN_UPMARMU:
            case StatusChange::SC_COOLDOWN_SG:
            case StatusChange::SC_COOLDOWN_CG:
            case StatusChange::SC_SLOWMOVE:
            case StatusChange::SC_CANTMOVE:
           break;

            default:
            // work around old behaviour of:
            // speed potion
            // atk potion
            // matk potion
            //
            // which used to use seconds
            // all others used milliseconds
            tick *= 1000;
        }
    val1 = conv_num(st, &AARG(2));
    if (HARG(3))    //指定したキャラを状態異常にする | Make the specified character abnormal
        bl = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(3))));
    else
        bl = map_id2bl(st->rid);
    skill_status_change_start(bl, type, val1, tick);
}

/*========================================
 * Remove a status condition from a being. By default the attached player is
 * affected.
 *
 * @doc sc_end
 * @arg status: int; the status-condition id to remove.
 * @optarg gid: GID; being id to affect instead of the player.
 * @ret none
 ========================================*/
static
void builtin_sc_end(ScriptState *st)
{
    dumb_ptr<block_list> bl;
    StatusChange type = StatusChange(conv_num(st, &AARG(0)));
    if (HARG(1))    //指定したキャラを状態異常にする | Make the specified character abnormal
        bl = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))));
    else
        bl = map_id2bl(st->rid);

    skill_status_change_end(bl, type, nullptr);
}

/*========================================
 * Test whether a status condition is currently active on a being. By
 * default the attached player is checked.
 *
 * @doc sc_check
 * @arg status: int; the status-condition id to test.
 * @optarg gid: GID; being id to check instead of the player.
 * @ret int; nonzero if the condition is active.
 ========================================*/
static
void builtin_sc_check(ScriptState *st)
{
    dumb_ptr<block_list> bl;
    StatusChange type = StatusChange(conv_num(st, &AARG(0)));
    if (HARG(1))    //指定したキャラを状態異常にする | Make the specified character abnormal
        bl = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))));
    else
        bl = map_id2bl(st->rid);

    push_int<ScriptDataInt>(st->stack, skill_status_change_active(bl, type));

}

/*========================================
 * Print a message, with the current RID and OID, to the server log. For
 * script debugging; nothing is shown to players.
 *
 * @doc debugmes
 * @arg text: str; the text to log.
 * @ret none
 ========================================*/
static
void builtin_debugmes(ScriptState *st)
{
    RString mes = conv_str(st, &AARG(0));
    PRINTF("script debug: %d %d: '%s'\n"_fmt,
            st->rid, st->oid, mes);
}

/*========================================
 * Reset the attached player's stat-point allocation, refunding spent
 * points.
 *
 * @doc resetstatus
 * @ret none
 ========================================*/
static
void builtin_resetstatus(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;
    sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    pc_resetstate(sd);
}

/*========================================
 * Attach a player to the script as the RID.
 *
 * @doc attachrid
 * @arg gid: GID; being id of the player to attach.
 * @ret int; 1 if such a player exists and is now attached, 0 otherwise.
 ========================================*/
static
void builtin_attachrid(ScriptState *st)
{
    st->rid = wrap<BlockId>(conv_num(st, &AARG(0)));
    push_int<ScriptDataInt>(st->stack, (map_id2sd(st->rid) != nullptr));
}

/*========================================
 * Detach the current player (RID) from the script, so following player-
 * specific builtins have no player to act on.
 *
 * @doc detachrid
 * @ret none
 ========================================*/
static
void builtin_detachrid(ScriptState *st)
{
    st->rid = BlockId();
}

/*========================================
 * Test whether a player is currently online.
 *
 * @doc isloggedin
 * @arg gid: GID; being id of the player to check.
 * @ret int; 1 if the player is online, 0 otherwise.
 ========================================*/
static
void builtin_isloggedin(ScriptState *st)
{
    push_int<ScriptDataInt>(st->stack,
              map_id2sd(wrap<BlockId>(conv_num(st, &AARG(0)))) != nullptr);
}

/*========================================
 * Set a map flag on a map.
 *
 * @doc setmapflag
 * @arg map: map; the map to change.
 * @arg flag: int; the map-flag number to set.
 * @ret none
 ========================================*/
static
void builtin_setmapflag(ScriptState *st)
{
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    int i = conv_num(st, &AARG(1));
    MapFlag mf = map_flag_from_int(i);
    Option<P<map_local>> m_ = map_mapname2mapid(str);
    OMATCH_BEGIN_SOME (m, m_)
    {
        m->flag.set(mf, 1);
    }
    OMATCH_END ();
}

/*========================================
 * Clear a map flag on a map.
 *
 * @doc removemapflag
 * @arg map: map; the map to change.
 * @arg flag: int; the map-flag number to clear.
 * @ret none
 ========================================*/
static
void builtin_removemapflag(ScriptState *st)
{
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    int i = conv_num(st, &AARG(1));
    MapFlag mf = map_flag_from_int(i);
    Option<P<map_local>> m_ = map_mapname2mapid(str);
    OMATCH_BEGIN_SOME (m, m_)
    {
        m->flag.set(mf, 0);
    }
    OMATCH_END ();
}

/*========================================
 * Return the state of a map flag on a map.
 *
 * @doc getmapflag
 * @arg map: map; the map to query.
 * @arg flag: int; the map-flag number to read.
 * @ret int; the flag state (0 or 1), or -1 if the map does not exist.
 ========================================*/
static
void builtin_getmapflag(ScriptState *st)
{
    int r = -1;

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    int i = conv_num(st, &AARG(1));
    MapFlag mf = map_flag_from_int(i);
    Option<P<map_local>> m_ = map_mapname2mapid(str);
    OMATCH_BEGIN_SOME (m, m_)
    {
        r = m->flag.get(mf);
    }
    OMATCH_END ();

    push_int<ScriptDataInt>(st->stack, r);
}

/*========================================
 * Enable PvP on a map (unless it has the nopvp flag). Players already on
 * the map gain a PvP rank, unless server-wide pk-mode is on.
 *
 * @doc pvpon
 * @arg map: map; the map to enable PvP on.
 * @ret none
 ========================================*/
static
void builtin_pvpon(ScriptState *st)
{
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(str), return);
    if (!m->flag.get(MapFlag::PVP) && !m->flag.get(MapFlag::NOPVP))
    {
        m->flag.set(MapFlag::PVP, 1);

        if (battle_config.pk_mode)  // disable ranking functions if pk_mode is on [Valaris]
            return;

        for (io::FD i : iter_fds())
        {
            Session *s = get_session(i);
            if (!s)
                continue;
            dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
            if (pl_sd && pl_sd->state.auth)
            {
                if (m == pl_sd->bl_m && !pl_sd->pvp_timer)
                {
                    pl_sd->pvp_timer = Timer(gettick() + 200_ms,
                            std::bind(pc_calc_pvprank_timer, ph::_1, ph::_2,
                                pl_sd->bl_id));
                    //pl_sd->pvp_rank = 0;
                    //pl_sd->pvp_point = 5;
                    clif_map_pvp(pl_sd);
                }
            }
        }
    }
}

/*========================================
 * Disable PvP on a map, cancelling the PvP ranking of players on it.
 *
 * @doc pvpoff
 * @arg map: map; the map to disable PvP on.
 * @ret none
 ========================================*/
static
void builtin_pvpoff(ScriptState *st)
{
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(str), return);
    if (m->flag.get(MapFlag::PVP))
    {
        m->flag.set(MapFlag::PVP, 0);

        if (battle_config.pk_mode)  // disable ranking options if pk_mode is on [Valaris]
            return;

        for (io::FD i : iter_fds())
        {
            Session *s = get_session(i);
            if (!s)
                continue;
            dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
            if (pl_sd && pl_sd->state.auth)
            {
                if (m == pl_sd->bl_m)
                {
                    pl_sd->pvp_timer.cancel();
                    clif_map_pvp(pl_sd);
                }
            }
        }
    }
}

/*========================================
 * Set the attached player's PvP channel (values below 1 mean channel 0).
 *
 * @doc setpvpchannel
 * @arg channel: int; the new PvP channel.
 * @ret none
 ========================================*/
static
void builtin_setpvpchannel(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int flag;
    flag = conv_num(st, &AARG(0));
    if (flag < 1)
        flag = 0;

    script_nullpo_end(sd, "player not found"_s);
    sd->state.pvpchannel = flag;
}

/*========================================
 * Return a PvP-related flag for a player. The optional being id picks the
 * player; otherwise the attached player is used.
 *
 * @doc getpvpflag
 * @arg type: int; which flag: 0 the PvP channel, 1 whether the player is
 *                 hidden.
 * @optarg gid: GID; being id of the player to query.
 * @ret int; the requested PvP flag.
 ========================================*/
static
void builtin_getpvpflag(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;
    if (HARG(1))    //指定したキャラを状態異常にする | Make the specified character abnormal
        sd = map_id_is_player(wrap<BlockId>(conv_num(st, &AARG(1))));
    else
        sd = script_rid2sd(st);

    script_nullpo_end(sd, "player not found"_s);

    int num = conv_num(st, &AARG(0));
    int flag = 0;

    switch (num){
        case 0:
            flag = sd->state.pvpchannel;
            break;
        case 1:
            flag = bool(sd->status.option & Opt0::HIDE);
            break;
    }

    push_int<ScriptDataInt>(st->stack, flag);
}

/*========================================
 * Show an emote. With a second argument naming a player, the emote is shown
 * by the script's NPC towards that player; with the literal "self" it is
 * shown by the attached player; otherwise by the NPC.
 *
 * @doc emotion
 * @arg emote: int; the emote id, 0 to 200.
 * @optarg target: str; a player name, or "self".
 * @ret none
 ========================================*/
static
void builtin_emotion(ScriptState *st)
{
    ZString str;
    dumb_ptr<map_session_data> pl_sd = nullptr;
    int type = conv_num(st, &AARG(0));
    if (HARG(1)) {
        str = ZString(conv_str(st, &AARG(1)));
        CharName player = stringish<CharName>(str);
        pl_sd = map_nick2sd(player);
    }
    if (type < 0 || type > 200)
        return;
    if (pl_sd != nullptr)
        clif_emotion_towards(map_id2bl(st->oid), pl_sd, type);
    else if (st->rid && str == "self"_s)
        clif_emotion(map_id2sd(st->rid), type);
    else
        clif_emotion(map_id2bl(st->oid), type);
}

/*========================================
 * Warp every player on one map to a cell on another map.
 *
 * @doc mapwarp
 * @arg from_map: map; the map to warp players from.
 * @arg to_map: map; the destination map name.
 * @arg x: coordinate; the destination x coordinate.
 * @arg y: coordinate; the destination y coordinate.
 * @ret none
 ========================================*/
static
void builtin_mapwarp(ScriptState *st)   // Added by RoVeRT
{
    int x, y;
    int x0, y0, x1, y1;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x0 = 0;
    y0 = 0;
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);
    x1 = m->xs;
    y1 = m->ys;
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(1))));
    x = conv_num(st, &AARG(2));
    y = conv_num(st, &AARG(3));

    map_foreachinarea(std::bind(builtin_areawarp_sub, ph::_1, str, x, y),
            m,
            x0, y0,
            x1, y1,
            BL::PC);
}

/*==========================================
 * 
 *------------------------------------------
 */
static
void builtin_mobcount_sub(dumb_ptr<block_list> bl, NpcEvent event, int *c)
{
    if (event == bl->is_mob()->npc_event)
        (*c)++;
}

/*========================================
 * Return the number of monsters on a map whose death event matches. The
 * count is reported one lower than the raw total.
 *
 * @doc mobcount
 * @arg map: map; the map to count monsters on.
 * @arg event: event; the death event to match.
 * @ret int; the monster count, or -1 if the map does not exist.
 ========================================*/
static
void builtin_mobcount(ScriptState *st)  // Added by RoVeRT
{
    int c = 0;
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    ZString event_ = ZString(conv_str(st, &AARG(1)));
    NpcEvent event;
    extract(event_, &event);

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
    {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    });
    map_foreachinarea(std::bind(builtin_mobcount_sub, ph::_1, event, &c),
            m,
            0, 0,
            m->xs, m->ys,
            BL::MOB);

    push_int<ScriptDataInt>(st->stack, (c - 1));

}

/*========================================
 * Marry the attached player to another player.
 *
 * @doc marriage
 * @arg partner: player; name of the player to marry.
 * @ret int; 1 on success, 0 if a player is missing or the marriage fails.
 ========================================*/
static
void builtin_marriage(ScriptState *st)
{
    CharName partner = stringish<CharName>(ZString(conv_str(st, &AARG(0))));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    dumb_ptr<map_session_data> p_sd = map_nick2sd(partner);

    if (sd == nullptr || p_sd == nullptr || pc_marriage(sd, p_sd) < 0)
    {
        push_int<ScriptDataInt>(st->stack, 0);
        return;
    }
    push_int<ScriptDataInt>(st->stack, 1);
}

/*========================================
 * Divorce the attached player from their partner.
 *
 * @doc divorce
 * @ret int; 1 on success, 0 on failure.
 ========================================*/
static
void builtin_divorce(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    if (sd == nullptr || pc_divorce(sd) < 0)
    {
        push_int<ScriptDataInt>(st->stack, 0);
        return;
    }

    push_int<ScriptDataInt>(st->stack, 1);
}

/*========================================
 * Return a clickable item-link string (the @@id|@@ form) for an item.
 *
 * @doc getitemlink
 * @arg item: item; the item, by numeric id or name.
 * @ret str; the item-link string, or "Unknown Item" if it does not exist.
 ========================================*/
static
void builtin_getitemlink(ScriptState *st)
{
    AString buf;
    ItemNameId nameid = get_item_id(st, &AARG(0));

    if (nameid)
        buf = STRPRINTF("@@%d|@@"_fmt, nameid);
    else
        buf = "Unknown Item"_s;

    push_str<ScriptDataStr>(st->stack, buf);
}

/*========================================
 * Return the character id of the attached player's marriage partner (0 if
 * unmarried).
 *
 * @doc getpartnerid2
 * @ret int; the partner's character id.
 ========================================*/
static
void builtin_getpartnerid2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    push_int<ScriptDataInt>(st->stack, unwrap<CharId>(sd->status.partner_id));
}

/*========================================
 * Return a translatable string. Currently a pass-through: the translation
 * lookup and argument substitution are not implemented in this server, so
 * it returns its argument unchanged.
 *
 * @doc l
 * @rest text: str; the translatable string and substitution arguments.
 * @ret str; the (untranslated) string.
 ========================================*/
static
void builtin_l(ScriptState *st)
{
    RString mes = conv_str(st, &AARG(0));
    // TODO: Format: src/map/script.c:5720
    // TODO: Translation Table Lookup: src/emap/lang.c:161
    push_str<ScriptDataStr>(st->stack, mes);
}

/*========================================
 * Return a one-character string whose single byte has a given ASCII code.
 *
 * @doc chr
 * @arg code: int; the ASCII code.
 * @ret str; the one-character string.
 ========================================*/
static
void builtin_chr(ScriptState *st)
{
    const char ascii = conv_num(st, &AARG(0));
    push_str<ScriptDataStr>(st->stack, VString<1>(ascii));
}

/*========================================
 * Return the ASCII code of the first character of a string.
 *
 * @doc ord
 * @arg text: str; the string to inspect.
 * @ret int; the ASCII code of the first character.
 ========================================*/
static
void builtin_ord(ScriptState *st)
{
    const char ascii = conv_str(st, &AARG(0)).front();
    push_int<ScriptDataInt>(st->stack, static_cast<int>(ascii));
}

/*========================================
 * Split a string on a single-character separator and store the pieces into
 * an array. Pieces are stored as numbers unless dest is a string array.
 *
 * @doc explode
 * @arg dest: var; the array variable to fill.
 * @arg text: str; the string to split.
 * @arg separator: str; the single-character separator.
 * @ret none
 ========================================*/
static
void builtin_explode(ScriptState *st)
{
    dumb_ptr<block_list> bl = nullptr;
    SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());
    const char separator = conv_str(st, &AARG(2)).front();
    RString str = conv_str(st, &AARG(1));
    RString val;
    char prefix = name.front();
    char postfix = name.back();

    if (prefix != '$' && prefix != '@' && prefix != '.')
    {
        PRINTF("builtin_explode: illegal scope!\n"_fmt);
        return;
    }
    if (prefix == '.' && !name.startswith(".@"_s))
        bl = map_id2bl(st->oid)->is_npc();
    else if (prefix != '$' && prefix != '.')
    {
        bl = map_id2bl(st->rid)->is_player();
        script_nullpo_end(bl, "target player not found"_s);
    }


    for (int j = 0; j < 256; j++)
    {
        auto find = std::find(str.begin(), str.end(), separator);
        if (find == str.end())
        {
            if (name.startswith(".@"_s))
            {
                struct script_data vd = script_data(ScriptDataInt{atoi(str.c_str())});
                if (postfix == '$')
                    vd = script_data(ScriptDataStr{str});
                set_scope_reg(st, reg.iplus(j), &vd);
            }
            else if (postfix == '$')
                set_reg(bl, VariableCode::VARIABLE, reg.iplus(j), str);
            else
                set_reg(bl, VariableCode::VARIABLE, reg.iplus(j), atoi(str.c_str()));
            break;
        }
        {
            val = str.xislice_h(find);
            str = str.xislice_t(find + 1);

            if (name.startswith(".@"_s))
            {
                struct script_data vd = script_data(ScriptDataInt{atoi(val.c_str())});
                if (postfix == '$')
                    vd = script_data(ScriptDataStr{val});
                set_scope_reg(st, reg.iplus(j), &vd);
            }
            else if (postfix == '$')
                set_reg(bl, VariableCode::VARIABLE, reg.iplus(j), val);
            else
                set_reg(bl, VariableCode::VARIABLE, reg.iplus(j), atoi(val.c_str()));
        }
    }
}

/*========================================
 * Copy the attached player's inventory into the temporary arrays
 * @inventorylist_id, @inventorylist_amount and @inventorylist_equip, with
 * the entry count in @inventorylist_count.
 *
 * @doc getinventorylist
 * @ret none
 ========================================*/
static
void builtin_getinventorylist(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int j = 0;
    script_nullpo_end(sd, "player not found"_s);

    for (IOff0 i : IOff0::iter())
    {
        if (sd->status.inventory[i].nameid
            && sd->status.inventory[i].amount > 0)
        {
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_id"_s), j),
                       unwrap<ItemNameId>(sd->status.inventory[i].nameid));
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_amount"_s), j),
                       sd->status.inventory[i].amount);
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_equip"_s), j),
                    static_cast<uint16_t>(sd->status.inventory[i].equip));
            j++;
        }
    }
    pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_count"_s)), j);
}

/*========================================
 * Copy the attached player's activated pool skills into the temporary
 * arrays @skilllist_id, @skilllist_lv, @skilllist_flag and
 * @skilllist_name$, with the count in @skilllist_count.
 *
 * @doc getactivatedpoolskilllist
 * @ret none
 ========================================*/
static
void builtin_getactivatedpoolskilllist(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    SkillID pool_skills[MAX_SKILL_POOL];
    int skill_pool_size = skill_pool(sd, pool_skills);
    int i, count = 0;

    script_nullpo_end(sd, "player not found"_s);

    for (i = 0; i < skill_pool_size; i++)
    {
        SkillID skill_id = pool_skills[i];

        if (sd->status.skill[skill_id].lv)
        {
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_id"_s), count),
                    static_cast<uint16_t>(skill_id));
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_lv"_s), count),
                    sd->status.skill[skill_id].lv);
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_flag"_s), count),
                    static_cast<uint16_t>(sd->status.skill[skill_id].flags));
            pc_setregstr(sd, SIR::from(variable_names.intern("@skilllist_name$"_s), count),
                    skill_name(skill_id));
            ++count;
        }
    }
    pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_count"_s)), count);

}

/*========================================
 * Copy the attached player's known-but-not-activated pool skills into the
 * temporary arrays @skilllist_id, @skilllist_lv, @skilllist_flag and
 * @skilllist_name$, with the count in @skilllist_count.
 *
 * @doc getunactivatedpoolskilllist
 * @ret none
 ========================================*/
static
void builtin_getunactivatedpoolskilllist(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int i, count = 0;

    script_nullpo_end(sd, "player not found"_s);

    for (i = 0; i < skill_pool_skills.size(); i++)
    {
        SkillID skill_id = skill_pool_skills[i];

        if (sd->status.skill[skill_id].lv
            && !bool(sd->status.skill[skill_id].flags & SkillFlags::POOL_ACTIVATED))
        {
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_id"_s), count),
                    static_cast<uint16_t>(skill_id));
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_lv"_s), count),
                    sd->status.skill[skill_id].lv);
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_flag"_s), count),
                    static_cast<uint16_t>(sd->status.skill[skill_id].flags));
            pc_setregstr(sd, SIR::from(variable_names.intern("@skilllist_name$"_s), count),
                    skill_name(skill_id));
            ++count;
        }
    }
    pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_count"_s)), count);
}

/*========================================
 * Activate a pool skill for the attached player.
 *
 * @doc poolskill
 * @arg skill: int; the pool-skill id to activate.
 * @ret none
 ========================================*/
static
void builtin_poolskill(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    SkillID skill_id = SkillID(conv_num(st, &AARG(0)));
    script_nullpo_end(sd, "player not found"_s);
    skill_pool_activate(sd, skill_id);
    clif_skillinfoblock(sd);

}

/*========================================
 * Deactivate a pool skill for the attached player.
 *
 * @doc unpoolskill
 * @arg skill: int; the pool-skill id to deactivate.
 * @ret none
 ========================================*/
static
void builtin_unpoolskill(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    SkillID skill_id = SkillID(conv_num(st, &AARG(0)));
    script_nullpo_end(sd, "player not found"_s);
    skill_pool_deactivate(sd, skill_id);
    clif_skillinfoblock(sd);

}

/*========================================
 * Display a miscellaneous visual effect. The optional target chooses where;
 * with no target it falls back to the script's NPC, then the attached
 * player.
 *
 * @doc misceffect
 * @arg effect: int; the effect type id.
 * @optarg target: expr; a player name or a being id to show it on.
 * @ret none
 ========================================*/
static
void builtin_misceffect(ScriptState *st)
{
    int type;
    BlockId id;
    CharName name;
    dumb_ptr<block_list> bl = nullptr;

    type = conv_num(st, &AARG(0));

    if (HARG(1))
    {
        struct script_data *sdata = &AARG(1);

        get_val(st, sdata);

        if (sdata->is<ScriptDataStr>())
            name = stringish<CharName>(ZString(conv_str(st, sdata)));
        else
            id = wrap<BlockId>(conv_num(st, sdata));
    }

    if (name.to__actual())
    {
        dumb_ptr<map_session_data> sd = map_nick2sd(name);
        if (sd)
            bl = sd;
    }
    else if (id)
        bl = map_id2bl(id);
    else if (st->oid)
        bl = map_id2bl(st->oid);
    else
    {
        dumb_ptr<map_session_data> sd = script_rid2sd(st);
        if (sd)
            bl = sd;
    }

    if (bl)
        clif_misceffect(bl, type);

}

/*========================================
 * Display a special effect on the script's NPC.
 *
 * @doc specialeffect
 * @arg effect: int; the special-effect id.
 * @ret none
 ========================================*/
static
void builtin_specialeffect(ScriptState *st)
{
    dumb_ptr<block_list> bl = map_id2bl(st->oid);

    if (bl == nullptr)
        return;

    clif_specialeffect(bl,
                        conv_num(st,
                                  &AARG(0)),
                        0);

}

/*========================================
 * Display a special effect on the attached player.
 *
 * @doc specialeffect2
 * @arg effect: int; the special-effect id.
 * @ret none
 ========================================*/
static
void builtin_specialeffect2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    nullpo_retv(sd);

    clif_specialeffect(sd,
                        conv_num(st,
                                  &AARG(0)),
                        0);

}

/*========================================
 * Read a variable or player parameter belonging to another being and return
 * its value. Returns 0 or an empty string when the target cannot be found.
 *
 * @doc get
 * @arg name: var; the variable or parameter to read.
 * @arg owner: expr; the character or NPC that owns the variable.
 * @ret variant; the value of the variable.
 ========================================*/
static
void builtin_get(ScriptState *st)
{
    BlockId id;
    dumb_ptr<block_list> bl = nullptr;
    if (auto *u = AARG(0).get_if<ScriptDataParam>())
    {
        SIR reg = u->reg;
        struct script_data *sdata = &AARG(1);
        get_val(st, sdata);
        CharName name;
        if (sdata->is<ScriptDataStr>())
        {
            name = stringish<CharName>(ZString(conv_str(st, sdata)));
            if (name.to__actual())
                bl = map_nick2sd(name);
        }
        else
        {
            int num = conv_num(st, sdata);
            if (num >= 2000000)
                id = wrap<BlockId>(num);
            else if (num >= 150000)
            {
                dumb_ptr<map_session_data> p_sd = nullptr;
                if ((p_sd = map_nick2sd(map_charid2nick(wrap<CharId>(num)))) != nullptr)
                    id = p_sd->bl_id;
                else
                {
                    push_int<ScriptDataInt>(st->stack, -1);
                    return;
                }
            }
            else
            {
                push_int<ScriptDataInt>(st->stack, -1);
                return;
            }
            bl = map_id2bl(id);
        }

        if (bl == nullptr)
        {
            push_int<ScriptDataInt>(st->stack, -1);
            return;
        }
        int var = pc_readparam(bl, reg.sp());
        push_int<ScriptDataInt>(st->stack, var);
        return;
    }

    struct script_data *sdata = &AARG(1);
    get_val(st, sdata);

    SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;
    ZString name_ = variable_names.outtern(reg.base());
    char prefix = name_.front();
    char postfix = name_.back();

    if(prefix == '.')
    {
        if (name_.startswith(".@"_s))
        {
            PRINTF("builtin_get: illegal scope!\n"_fmt);
            push_int<ScriptDataInt>(st->stack, 0);
            return;
        }
        NpcName name;
        if (sdata->is<ScriptDataStr>())
        {
            name = stringish<NpcName>(ZString(conv_str(st, sdata)));
            bl = npc_name2id(name);
        }
        else
        {
            id = wrap<BlockId>(conv_num(st, sdata));
            bl = map_id2bl(id);
        }
    }
    else if(prefix != '$')
    {
        CharName name;
        if (sdata->is<ScriptDataStr>())
        {
            name = stringish<CharName>(ZString(conv_str(st, sdata)));
            if (name.to__actual())
                bl = map_nick2sd(name);
        }
        else
        {
            id = wrap<BlockId>(conv_num(st, sdata));
            bl = map_id2bl(id);
        }
    }
    else
    {
        PRINTF("builtin_get: illegal scope !\n"_fmt);
        push_int<ScriptDataInt>(st->stack, 0);
        return;
    }

    if (!bl)
    {
        if (postfix == '$')
            push_str<ScriptDataStr>(st->stack, ""_s);
        else
            push_int<ScriptDataInt>(st->stack, 0);
        return;
    }

    if (postfix == '$')
    {
        ZString var = pc_readregstr(bl, reg);
        push_str<ScriptDataStr>(st->stack, var);
    }
    else
    {
        int var = 0;
        if (prefix == '#' && bl)
        {
            if (name_.startswith("##"_s))
                var = pc_readaccountreg2(bl->is_player(), stringish<VarName>(name_));
            else
                var = pc_readaccountreg(bl->is_player(), stringish<VarName>(name_));
        }
        else if (bl)
            var = pc_readreg(bl, reg);

        push_int<ScriptDataInt>(st->stack, var);
    }
}

/*========================================
 * Unequip every item the attached player is wearing.
 *
 * @doc nude
 * @ret none
 ========================================*/
static
void builtin_nude(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    script_nullpo_end(sd, "player not found"_s);

    for (EQUIP i : EQUIPs)
    {
        IOff0 idx = sd->equip_index_maybe[i];
        if (idx.ok())
            pc_unequipitem(sd, idx, CalcStatus::LATER);
    }
    pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);

}

/*========================================
 * Unequip whatever the attached player has in an equipment slot.
 *
 * @doc unequipbyid
 * @arg slot: int; an EQUIP slot id.
 * @ret none
 ========================================*/
static
void builtin_unequipbyid(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    EQUIP slot_id = EQUIP(conv_num(st, &AARG(0)));

    if (slot_id >= EQUIP() && slot_id < EQUIP::COUNT)
    {
        IOff0 idx = sd->equip_index_maybe[slot_id];
        if (idx.ok())
            pc_unequipitem(sd, idx, CalcStatus::LATER);
    }

    pc_calcstatus(sd, (int)CalcStatusKind::NORMAL_RECALC);

}

/*========================================
 * Move a named NPC to a new cell on its current map. Out-of-bounds
 * coordinates are ignored.
 *
 * @doc npcwarp
 * @arg x: coordinate; the destination x coordinate.
 * @arg y: coordinate; the destination y coordinate.
 * @arg npc: str; name of the NPC to move.
 * @ret none
 ========================================*/
static
void builtin_npcwarp(ScriptState *st)
{
    int x, y;
    dumb_ptr<npc_data> nd = nullptr;

    x = conv_num(st, &AARG(0));
    y = conv_num(st, &AARG(1));
    NpcName npc = stringish<NpcName>(ZString(conv_str(st, &AARG(2))));
    nd = npc_name2id(npc);

    script_nullpo_end(nd, STRPRINTF("no such npc: '%s'"_fmt, npc));

    P<map_local> m = nd->bl_m;

    /* Crude sanity checks. */
    if (!nd->bl_prev
            || x < 0 || x > m->xs -1
            || y < 0 || y > m->ys - 1)
        return;

    npc_enable(npc, 0);
    map_delblock(nd); /* [Freeyorp] */
    nd->bl_x = x;
    nd->bl_y = y;
    map_addblock(nd);
    npc_enable(npc, 1);

}

/*========================================
 * Move a named NPC to a random cell inside a rectangle on its current map.
 *
 * @doc npcareawarp
 * @arg x0: coordinate; x of one corner of the rectangle.
 * @arg y0: coordinate; y of one corner of the rectangle.
 * @arg x1: coordinate; x of the opposite corner.
 * @arg y1: coordinate; y of the opposite corner.
 * @arg walkable: bool; nonzero to require the chosen cell to be walkable.
 * @arg npc: str; name of the NPC to move.
 * @ret none
 ========================================*/
static
void builtin_npcareawarp(ScriptState *st)
{
    int x0, y0, x1, y1, x, y, max, cb, lx = -1, ly = -1, j = 0;
    dumb_ptr<npc_data> nd = nullptr;

    NpcName npc = stringish<NpcName>(ZString(conv_str(st, &AARG(5))));
    nd = npc_name2id(npc);

    x0 = conv_num(st, &AARG(0));
    y0 = conv_num(st, &AARG(1));
    x1 = conv_num(st, &AARG(2));
    y1 = conv_num(st, &AARG(3));
    cb = conv_num(st, &AARG(4));

    if (!nd)
    {
        PRINTF("builtin_npcareawarp: no such npc: '%s'\n"_fmt, npc);
        return;
    }

    max = (y1 - y0 + 1) * (x1 - x0 + 1) * 3;
    if (max > 1000)
        max = 1000;

    P<map_local> m = nd->bl_m;
    if (cb) {
        do
        {
            x = random_::in(x0, x1);
            y = random_::in(y0, y1);
        }
        while (bool(map_getcell(m, x, y) & MapCell::UNWALKABLE)
             && (++j) < max);
        if (j >= max)
        {
            if (lx >= 0)
            {                   // Since reference went wrong, the place which boiled before is used.
                x = lx;
                y = ly;
            }
            else
                return;       // Since reference of the place which boils first went wrong, it stops.
        }
    }
    else
    {
        x = random_::in(x0, x1);
        y = random_::in(y0, y1);
    }

    npc_enable(npc, 0);
    map_delblock(nd); /* [Freeyorp] */
    nd->bl_x = x;
    nd->bl_y = y;
    map_addblock(nd);
    npc_enable(npc, 1);

}

/*========================================
 * Send a plain chat message to a named player.
 *
 * @doc message
 * @arg player: player; name of the recipient.
 * @arg text: str; the message text.
 * @ret none
 ========================================*/
static
void builtin_message(ScriptState *st)
{
    CharName player = stringish<CharName>(ZString(conv_str(st, &AARG(0))));
    ZString msg = ZString(conv_str(st, &AARG(1)));

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(player);
    if (pl_sd == nullptr)
        return;
    clif_displaymessage(pl_sd->sess, msg);

}

/*========================================
 * Set the title-bar text of the script's NPC dialogue window for the
 * attached player.
 *
 * @doc title
 * @arg text: str; the title-bar text.
 * @ret none
 ========================================*/
static
void builtin_title(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    ZString msg = ZString(conv_str(st, &AARG(0)));
    script_nullpo_end(sd, "player not found"_s);
    clif_npc_send_title(sd->sess, st->oid, msg);
}

/*========================================
 * Send a localized server message to a player. With one argument it is the
 * message text; with two, the first is a numeric message type and the
 * second the text.
 *
 * @doc smsg
 * @arg type_or_text: expr; the message type, or the text when called with
 *                          one argument.
 * @optarg text: expr; the message text.
 * @optarg player: expr; name of the recipient player.
 * @ret none
 ========================================*/
static
void builtin_smsg(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    if (HARG(2))
    {
        CharName player = stringish<CharName>(ZString(conv_str(st, &AARG(2))));
        sd = map_nick2sd(player);
    }

    int type = HARG(1) ? conv_num(st, &AARG(0)) : 0;
    ZString msg = ZString(conv_str(st, (HARG(1) ? &AARG(1) : &AARG(0))));
    script_nullpo_end(sd, "player not found"_s);
    if (type < 0 || type > 0xFF)
        type = 0;

    clif_server_message(sd, type, msg);
}

/*========================================
 * Send a remote command string to a player's client. The optional name
 * picks the recipient; otherwise the attached player is used.
 *
 * @doc remotecmd
 * @arg command: str; the remote command string.
 * @optarg player: expr; name of the recipient player.
 * @ret none
 ========================================*/
static
void builtin_remotecmd(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    if (HARG(1))
    {
        CharName player = stringish<CharName>(ZString(conv_str(st, &AARG(1))));
        sd = map_nick2sd(player);
    }

    ZString msg = ZString(conv_str(st, &AARG(0)));
    if (sd == nullptr)
        return;
    clif_remote_command(sd, msg);
}

/*========================================
 * Send a client-side collision update for a map. The required x, y give one
 * corner; an optional second x, y pair gives the opposite corner of a
 * rectangle.
 *
 * @doc sendcollision
 * @arg map: map; the map to update.
 * @arg mask: int; the collision bitmask.
 * @arg x: coordinate; x of one corner.
 * @arg y: coordinate; y of one corner.
 * @optarg x2: coordinate; x of the opposite corner.
 * @optarg y2: coordinate; y of the opposite corner.
 * @optarg player: expr; name of the player to send to.
 * @ret none
 ========================================*/
static
void builtin_sendcollision(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    MapName map_name = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    int mask = conv_num(st, &AARG(1));
    short x1, y1, x2, y2;
    x1 = x2 = conv_num(st, &AARG(2));
    y1 = y2 = conv_num(st, &AARG(3));
    script_nullpo_end(sd, "player not found"_s);

    if (HARG(5))
    {
        x2 = conv_num(st, &AARG(4));
        y2 = conv_num(st, &AARG(5));
        if (HARG(6))
        {
            CharName player = stringish<CharName>(ZString(conv_str(st, &AARG(6))));
            sd = map_nick2sd(player);
        }
    }

    else if (HARG(4))
    {
        CharName player = stringish<CharName>(ZString(conv_str(st, &AARG(4))));
        sd = map_nick2sd(player);
    }

    if (sd == nullptr)
        return;
    clif_update_collision(sd, x1, y1, x2, y2, map_name, mask);
}

/*========================================
 * Change the background music for the attached player.
 *
 * @doc music
 * @arg track: str; the music track to play.
 * @ret none
 ========================================*/
static
void builtin_music(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    ZString msg = ZString(conv_str(st, &AARG(0)));
    script_nullpo_end(sd, "player not found"_s);
    clif_change_music(sd, msg);
}

/*========================================
 * Set the rendering mask for the attached player and send it to the client.
 * With a second argument the mask is also stored on the current map.
 *
 * @doc mapmask
 * @arg mask: int; the rendering mask.
 * @optarg store: expr; present to also store the mask on the player's or
 *                      NPC's map.
 * @ret none
 ========================================*/
static
void builtin_mapmask(ScriptState *st)
{
    dumb_ptr<npc_data> nd;
    dumb_ptr<map_session_data> sd;
    int map_mask = conv_num(st, &AARG(0));

    if(st->oid)
        nd = map_id_is_npc(st->oid);
    if(st->rid)
        sd = script_rid2sd(st);

    if(HARG(1) && sd != nullptr)
        sd->bl_m->mask = map_mask;
    else if(HARG(1) && nd)
        nd->bl_m->mask = map_mask;

    script_nullpo_end(sd, "player not found"_s);
    clif_send_mask(sd, map_mask);
}

/*========================================
 * Return the rendering mask of the current map, taken from the attached
 * player's map, or the NPC's.
 *
 * @doc getmask
 * @ret int; the rendering mask, or -1 if neither is available.
 ========================================*/
static
void builtin_getmask(ScriptState *st)
{
    dumb_ptr<npc_data> nd;
    dumb_ptr<map_session_data> sd;
    int map_mask;

    if(st->oid)
        nd = map_id_is_npc(st->oid);
    if(st->rid)
        sd = script_rid2sd(st);

    if(sd != nullptr)
        map_mask = sd->bl_m->mask;
    else if(nd)
        map_mask = nd->bl_m->mask;
    else
        map_mask = -1;

    push_int<ScriptDataInt>(st->stack, map_mask);
}

/*========================================
 * Make a named NPC say a line in local chat, shown to players around it.
 * With a third argument naming a player, the line is shown only to that
 * player.
 *
 * @doc npctalk
 * @arg npc: str; name of the NPC that speaks.
 * @arg text: str; the line to say.
 * @optarg player: str; name of the only player to show it to.
 * @ret none
 ========================================*/
static
void builtin_npctalk(ScriptState *st)
{
    dumb_ptr<npc_data> nd;
    RString str = conv_str(st, &AARG(1));

    nd = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(0)))));
    if (nd == nullptr)
        return;


    if(HARG(2)){
        CharName player = stringish<CharName>(ZString(conv_str(st, &AARG(2))));
        dumb_ptr<map_session_data> pl_sd = map_nick2sd(player);
        if (pl_sd == nullptr)
            return;
        clif_message_towards(pl_sd, nd, str);
    }

    else
        clif_message(nd, str);
}

/*========================================
 * Register a spell invocation so that when a player types it as a chat
 * message the named NPC event runs. This is how the magic system binds chat
 * phrases to scripts.
 *
 * @doc registercmd
 * @arg invocation: str; the chat phrase to register.
 * @arg event: str; the NPC event to run when it is typed.
 * @ret none
 ========================================*/
static
void builtin_registercmd(ScriptState *st)
{
    RString evoke = conv_str(st, &AARG(0));
    ZString event_ = conv_str(st, &AARG(1));
    NpcEvent event;
    extract(event_, &event);
    spells_by_events.put(evoke, event);
}

/*========================================
 * Return a cosmetic look value of the attached player.
 *
 * @doc getlook
 * @arg type: int; a LOOK type: hair, weapon, headgear, hair colour, clothes
 *                 colour, shield.
 * @ret int; the look value, or -1 for an unsupported type.
 ========================================*/
static
void builtin_getlook(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    LOOK type = LOOK(conv_num(st, &AARG(0)));
    int val = -1;
    script_nullpo_end(sd, "player not found"_s);

    switch (type)
    {
        case LOOK::HAIR:        //1
            val = sd->status.hair;
            break;
        case LOOK::WEAPON:      //2
            val = static_cast<uint16_t>(sd->status.weapon);
            break;
        case LOOK::HEAD_BOTTOM: //3
            val = unwrap<ItemNameId>(sd->status.head_bottom);
            break;
        case LOOK::HEAD_TOP:    //4
            val = unwrap<ItemNameId>(sd->status.head_top);
            break;
        case LOOK::HEAD_MID:    //5
            val = unwrap<ItemNameId>(sd->status.head_mid);
            break;
        case LOOK::HAIR_COLOR:  //6
            val = sd->status.hair_color;
            break;
        case LOOK::CLOTHES_COLOR:   //7
            val = sd->status.clothes_color;
            break;
        case LOOK::SHIELD:      //8
            val = unwrap<ItemNameId>(sd->status.shield);
            break;
        case LOOK::SHOES:       //9
            break;
    }

    push_int<ScriptDataInt>(st->stack, val);
}

/*========================================
 * Return part of the attached player's save (respawn) point.
 *
 * @doc getsavepoint
 * @arg type: int; which part: 0 map name, 1 x coordinate, 2 y coordinate.
 * @ret variant; the requested part of the save point.
 ========================================*/
static
void builtin_getsavepoint(ScriptState *st)
{
    int x, y, type;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    type = conv_num(st, &AARG(0));

    x = sd->status.save_point.x;
    y = sd->status.save_point.y;
    switch (type)
    {
        case 0:
        {
            RString mapname = sd->status.save_point.map_;
            push_str<ScriptDataStr>(st->stack, mapname);
        }
            break;
        case 1:
            push_int<ScriptDataInt>(st->stack, x);
            break;
        case 2:
            push_int<ScriptDataInt>(st->stack, y);
            break;
    }
}

/*==========================================
 * areatimer
 *------------------------------------------
 */
static
void builtin_areatimer_sub(dumb_ptr<block_list> bl, interval_t tick, NpcEvent event)
{
    if (!bl)
        return;
    if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = map_id_is_player(bl->bl_id);
        pc_addeventtimer(sd, tick, event);
    }
}

/*========================================
 * Schedule an NPC event to run after a delay for every player inside a
 * rectangle on a map.
 *
 * @doc areatimer
 * @arg type: int; being-type selector; must be 0 (players).
 * @arg map: map; the map holding the rectangle.
 * @arg x0: coordinate; x of one corner of the rectangle.
 * @arg y0: coordinate; y of one corner of the rectangle.
 * @arg x1: coordinate; x of the opposite corner.
 * @arg y1: coordinate; y of the opposite corner.
 * @arg delay: timer; delay in milliseconds before the event runs.
 * @arg event: event; the NPC event to schedule.
 * @ret none
 ========================================*/
static
void builtin_areatimer(ScriptState *st)
{
    int x0, y0, x1, y1, bl_num;

    bl_num = conv_num(st, &AARG(0));
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(1))));
    x0 = conv_num(st, &AARG(2));
    y0 = conv_num(st, &AARG(3));
    x1 = conv_num(st, &AARG(4));
    y1 = conv_num(st, &AARG(5));
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARG(6)));
    ZString event_ = conv_str(st, &AARG(7));
    BL block_type;
    NpcEvent event;
    extract(event_, &event);

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);

    switch (bl_num)
    {
        case 0:
            block_type = BL::PC;
            break;
        default:
            return;
    }

    map_foreachinarea(std::bind(builtin_areatimer_sub, ph::_1, tick, event),
            m,
            x0, y0,
            x1, y1,
            block_type);
}

/*========================================
 * Test whether the attached player is inside a rectangle on a map.
 *
 * @doc isin
 * @arg map: map; the map holding the rectangle.
 * @arg x0: coordinate; x of one corner of the rectangle.
 * @arg y0: coordinate; y of one corner of the rectangle.
 * @arg x1: coordinate; x of the opposite corner.
 * @arg y1: coordinate; y of the opposite corner.
 * @ret int; 1 if the player is inside the rectangle, 0 otherwise.
 ========================================*/
static
void builtin_isin(ScriptState *st)
{
    int x1, y1, x2, y2;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x1 = conv_num(st, &AARG(1));
    y1 = conv_num(st, &AARG(2));
    x2 = conv_num(st, &AARG(3));
    y2 = conv_num(st, &AARG(4));

    script_nullpo_end(sd, "player not found"_s);

    push_int<ScriptDataInt>(st->stack,
              (sd->bl_x >= x1 && sd->bl_x <= x2)
              && (sd->bl_y >= y1 && sd->bl_y <= y2)
              && (str == sd->bl_m->name_));
}

/*========================================
 * Test whether a map cell is unwalkable (a collision cell).
 *
 * @doc iscollision
 * @arg map: map; the map to test.
 * @arg x: coordinate; the x coordinate to test.
 * @arg y: coordinate; the y coordinate to test.
 * @ret int; 1 if the cell is unwalkable, 0 otherwise.
 ========================================*/
static
void builtin_iscollision(ScriptState *st)
{
    int x, y;
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);

    x = conv_num(st, &AARG(1));
    y = conv_num(st, &AARG(2));

    push_int<ScriptDataInt>(st->stack,
        bool(map_getcell(m, x, y) & MapCell::UNWALKABLE));
}

/*========================================
 * Close the current dialogue and open the buy/sell window of a shop NPC for
 * the attached player.
 *
 * @doc shop
 * @arg npc: str; name of the shop NPC.
 * @ret none
 ========================================*/
static
void builtin_shop(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    dumb_ptr<npc_data> nd;

    script_nullpo_end(sd, "player not found"_s);

    NpcName name = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
    nd = npc_name2id(name);
    script_nullpo_end(nd, STRPRINTF("no such npc: '%s'"_fmt, name));

    builtin_close(st);
    clif_npcbuysell(sd, nd->bl_id);
}

/*========================================
 * Test whether the attached player is dead.
 *
 * @doc isdead
 * @ret int; 1 if the player is dead, 0 otherwise.
 ========================================*/
static
void builtin_isdead(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);

    push_int<ScriptDataInt>(st->stack, pc_isdead(sd));
}

/*========================================
 * Rename a named NPC and change its sprite, then refresh it.
 *
 * @doc fakenpcname
 * @arg npc: str; name of the NPC to change.
 * @arg newname: str; the new NPC name.
 * @arg sprite: int; the new sprite species id.
 * @ret none
 ========================================*/
static
void builtin_fakenpcname(ScriptState *st)
{
    NpcName name = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
    NpcName newname = stringish<NpcName>(ZString(conv_str(st, &AARG(1))));
    Species newsprite = wrap<Species>(static_cast<uint16_t>(conv_num(st, &AARG(2))));
    dumb_ptr<npc_data> nd = npc_name2id(name);
    script_nullpo_end(nd, STRPRINTF("no such npc: '%s'"_fmt, name));
    nd->name = newname;
    nd->npc_class = newsprite;

    // Refresh this npc
    npc_enable(name, 0);
    npc_enable(name, 1);

}

/*========================================
 * Return the x coordinate of the attached player.
 *
 * @doc getx
 * @ret int; the player's x coordinate.
 ========================================*/
static
void builtin_getx(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    push_int<ScriptDataInt>(st->stack, sd->bl_x);
}

/*========================================
 * Return the y coordinate of the attached player.
 *
 * @doc gety
 * @ret int; the player's y coordinate.
 ========================================*/
static
void builtin_gety(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    push_int<ScriptDataInt>(st->stack, sd->bl_y);
}

/*========================================
 * Return the facing direction of the attached player as a numeric DIR.
 *
 * @doc getdir
 * @ret int; the player's facing direction.
 ========================================*/
static
void builtin_getdir(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_nullpo_end(sd, "player not found"_s);
    push_int<ScriptDataInt>(st->stack, static_cast<uint8_t>(sd->dir));
}

/*========================================
 * Return the name of the map a player is on. The optional being id picks
 * the player; otherwise the attached player is used.
 *
 * @doc getmap
 * @optarg gid: GID; being id of the player to query.
 * @ret str; the map name, or an empty string if unavailable.
 ========================================*/
static
void builtin_getmap(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    if (HARG(0))    //指定したキャラを状態異常にする | Make the specified character abnormal
        sd = map_id_is_player(wrap<BlockId>(conv_num(st, &AARG(0))));
    else
        sd = script_rid2sd(st);

    if (!sd || !as_raw_pointer(Some(sd->bl_m)) || sd->bl_m == borrow(undefined_gat)) {
        push_str<ScriptDataStr>(st->stack, ""_s);
        return;
    }

    push_str<ScriptDataStr>(st->stack, sd->bl_m->name_);
}

/*========================================
 * Return the highest valid x coordinate of a map (its width minus one).
 *
 * @doc getmapmaxx
 * @arg map: map; the map to query.
 * @ret int; the maximum x coordinate.
 ========================================*/
static
void builtin_getmapmaxx(ScriptState *st)
{
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);
    push_int<ScriptDataInt>(st->stack, m->xs-1);
}

/*========================================
 * Return the highest valid y coordinate of a map (its height minus one).
 *
 * @doc getmapmaxy
 * @arg map: map; the map to query.
 * @ret int; the maximum y coordinate.
 ========================================*/
static
void builtin_getmapmaxy(ScriptState *st)
{
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);
    push_int<ScriptDataInt>(st->stack, m->ys-1);
}

/*========================================
 * Return the numeric hash identifying a map.
 *
 * @doc getmaphash
 * @arg map: map; the map to query.
 * @ret int; the map's hash.
 ========================================*/
static
void builtin_getmaphash(ScriptState *st)
{
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);
    push_int<ScriptDataInt>(st->stack, m->hash);
}

/*========================================
 * Return the name of the map with a given hash.
 *
 * @doc getmapnamefromhash
 * @arg hash: int; the map hash to look up.
 * @ret str; the map name, or an empty string if none matches.
 ========================================*/
static
void builtin_getmapnamefromhash(ScriptState *st)
{
    int hash = conv_num(st, &AARG(0));
    MapName mapname;
    for (auto& mit : maps_db)
    {
        map_local *ml = static_cast<map_local *>(mit.second.get());
        if (ml->hash == hash)
        {
            mapname = ml->name_;
            break;
        }
    }
    push_str<ScriptDataStr>(st->stack, mapname);
}

/*========================================
 * Test whether a map is loaded.
 *
 * @doc mapexists
 * @arg map: map; the map name to test.
 * @ret int; 1 if the map is loaded, 0 otherwise.
 ========================================*/
static
void builtin_mapexists(ScriptState *st)
{
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    push_int<ScriptDataInt>(st->stack, map_mapname2mapid(mapname).is_some());
}

/*========================================
 * Return the number of maps currently loaded on the server.
 *
 * @doc numberofmaps
 * @ret int; the number of loaded maps.
 ========================================*/
static
void builtin_numberofmaps(ScriptState *st)
{
    push_int<ScriptDataInt>(st->stack, maps_db.size());
}

/*========================================
 * Return the name of the map at a position in the server's map list.
 *
 * @doc getmapnamebyindex
 * @arg index: int; the index into the map list.
 * @ret str; the map name, or an empty string if the index is out of range.
 ========================================*/
static
void builtin_getmapnamebyindex(ScriptState *st)
{
    int index = conv_num(st, &AARG(0));
    int count = 0;

    for (auto& mit : maps_db)
    {
        if (count == index)
        {
            push_str<ScriptDataStr>(st->stack, mit.second->name_);
            return;
        }
        ++count;
    }

    push_str<ScriptDataStr>(st->stack, ""_s);
}

/*========================================
 * Return a string about an NPC. The optional argument picks another NPC;
 * otherwise the script's own NPC is used.
 *
 * @doc strnpcinfo
 * @arg type: int; which string: 0 full name, 1 visible name (before any
 *                 '#'), 2 hidden part (from the '#' on), 3 map name.
 * @optarg npc: expr; an NPC name or being id to query.
 * @ret str; the requested string.
 ========================================*/
static
void builtin_strnpcinfo(ScriptState *st)
{
    int num = conv_num(st, &AARG(0));
    RString name;
    dumb_ptr<npc_data> nd;

    if(HARG(1)){
        struct script_data *sdata = &AARG(1);
        get_val(st, sdata);

        if (sdata->is<ScriptDataStr>())
        {
            NpcName name_ = stringish<NpcName>(ZString(conv_str(st, sdata)));
            nd = npc_name2id(name_);
        }
        else
        {
            BlockId id = wrap<BlockId>(conv_num(st, sdata));
            nd = map_id_is_npc(id);
        }
    } else {
        nd = map_id_is_npc(st->oid);
    }

    script_nullpo_end(nd, "npc not found"_s);

    switch(num)
    {
        case 0:
            name = nd->name;
            break;
        case 1:
            name = nd->name.xislice_h(std::find(nd->name.begin(), nd->name.end(), '#'));
            break;
        case 2:
            name = nd->name.xislice_t(std::find(nd->name.begin(), nd->name.end(), '#'));
            break;
        case 3:
            name = nd->bl_m->name_;
            break;
    }

    push_str<ScriptDataStr>(st->stack, name);
}

/*========================================
 * Return the x coordinate of an NPC.
 *
 * @doc getnpcx
 * @optarg npc: str; name of the NPC; defaults to the script's own NPC.
 * @ret int; the NPC's x coordinate.
 ========================================*/
static
void builtin_getnpcx(ScriptState *st)
{
    dumb_ptr<npc_data> nd;

    if(HARG(0)){
        NpcName name = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
        nd = npc_name2id(name);
        script_nullpo_end(nd, STRPRINTF("no such npc: '%s'"_fmt, name));
    } else {
        nd = map_id_is_npc(st->oid);
    }

    script_nullpo_end(nd, "no npc"_s);

    push_int<ScriptDataInt>(st->stack, nd->bl_x);
}

/*========================================
 * Return the y coordinate of an NPC.
 *
 * @doc getnpcy
 * @optarg npc: str; name of the NPC; defaults to the script's own NPC.
 * @ret int; the NPC's y coordinate.
 ========================================*/
static
void builtin_getnpcy(ScriptState *st)
{
    dumb_ptr<npc_data> nd;

    if(HARG(0)){
        NpcName name = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
        nd = npc_name2id(name);
        script_nullpo_end(nd, STRPRINTF("no such npc: '%s'"_fmt, name));
    } else {
        nd = map_id_is_npc(st->oid);
    }

    script_nullpo_end(nd, "no npc"_s);

    push_int<ScriptDataInt>(st->stack, nd->bl_y);
}

/*========================================
 * Shut the map server down, clearing the run flag and ending the main loop.
 *
 * @doc mapexit
 * @ret none
 ========================================*/
static
void builtin_mapexit(ScriptState *)
{
    runflag = 0;
}

#define BUILTIN(func, args, ret)    \
{builtin_##func, #func ## _s, args, ret}

BuiltinFunction builtin_functions[] =
{
    BUILTIN(mes, "?"_s, '\0'),
    BUILTIN(mesq, "?"_s, '\0'),
    BUILTIN(mesn, "?"_s, '\0'),
    BUILTIN(clear, ""_s, '\0'),
    BUILTIN(goto, "L"_s, '\0'),
    BUILTIN(callfunc, "F"_s, '\0'),
    BUILTIN(call, "F?*"_s, '.'),
    BUILTIN(callsub, "L"_s, '\0'),
    BUILTIN(getarg, "i?"_s, 'v'),
    BUILTIN(return, "?"_s, '\0'),
    BUILTIN(void, "?*"_s, '\0'),
    BUILTIN(next, ""_s, '\0'),
    BUILTIN(close, ""_s, '\0'),
    BUILTIN(close2, ""_s, '\0'),
    BUILTIN(menu, "sL**"_s, '\0'),
    BUILTIN(rand, "i?"_s, 'i'),
    BUILTIN(isat, "Mxy"_s, 'i'),
    BUILTIN(warp, "Mxy"_s, '\0'),
    BUILTIN(areawarp, "MxyxyMxy"_s, '\0'),
    BUILTIN(heal, "ii?"_s, '\0'),
    BUILTIN(injure, "iii"_s, '\0'),
    BUILTIN(input, "N"_s, '\0'),
    BUILTIN(requestitem, "N?"_s, '\0'),
    BUILTIN(requestlang, "N"_s, '\0'),
    BUILTIN(if, "iF*"_s, '\0'),
    BUILTIN(elif, "iF*"_s, '\0'),
    BUILTIN(else, "F*"_s, '\0'),
    BUILTIN(set, "Ne?"_s, '\0'),
    BUILTIN(get, "Ne"_s, 'v'),
    BUILTIN(setarray, "Ne*"_s, '\0'),
    BUILTIN(cleararray, "Nei"_s, '\0'),
    BUILTIN(getarraysize, "N"_s, 'i'),
    BUILTIN(getelementofarray, "Ni"_s, 'v'),
    BUILTIN(array_search, "eN"_s, 'i'),
    BUILTIN(setlook, "ii"_s, '\0'),
    BUILTIN(countitem, "I"_s, 'i'),
    BUILTIN(checkweight, "Ii"_s, 'i'),
    BUILTIN(getitem, "Ii??"_s, '\0'),
    BUILTIN(makeitem, "IiMxy"_s, '\0'),
    BUILTIN(delitem, "Ii"_s, '\0'),
    BUILTIN(getcharid, "i?"_s, 'i'),
    BUILTIN(getnpcid, "?"_s, 'i'),
    BUILTIN(getversion, ""_s, 'i'),
    BUILTIN(strcharinfo, "i?"_s, 's'),
    BUILTIN(getequipid, "i?"_s, 'i'),
    BUILTIN(bonus, "ii"_s, '\0'),
    BUILTIN(bonus2, "iii"_s, '\0'),
    BUILTIN(skill, "ii?"_s, '\0'),
    BUILTIN(setskill, "ii"_s, '\0'),
    BUILTIN(getskilllv, "i"_s, 'i'),
    BUILTIN(overrideattack, "??????"_s, '\0'),
    BUILTIN(getgmlevel, ""_s, 'i'),
    BUILTIN(end, ""_s, '\0'),
    BUILTIN(getopt2, ""_s, 'i'),
    BUILTIN(setopt2, "i"_s, '\0'),
    BUILTIN(savepoint, "Mxy"_s, '\0'),
    BUILTIN(gettimetick, "i"_s, 'i'),
    BUILTIN(gettime, "i"_s, 'i'),
    BUILTIN(openstorage, ""_s, '\0'),
    BUILTIN(getexp, "ii"_s, '\0'),
    BUILTIN(mobinfo, "ii"_s, 'v'),
    BUILTIN(mobinfo_droparrays, "iiN"_s, 'i'),
    BUILTIN(getmobdrops, "i"_s, 'i'),
    BUILTIN(summon, "Mxyssmii?"_s, '\0'),
    BUILTIN(monster, "Mxysmi?"_s, '\0'),
    BUILTIN(areamonster, "Mxyxysmi?"_s, '\0'),
    BUILTIN(killmonster, "ME"_s, '\0'),
    BUILTIN(donpcevent, "E"_s, '\0'),
    BUILTIN(addtimer, "tE?"_s, '\0'),
    BUILTIN(addnpctimer, "tE"_s, '\0'),
    BUILTIN(initnpctimer, "?"_s, '\0'),
    BUILTIN(startnpctimer, "?"_s, '\0'),
    BUILTIN(stopnpctimer, "?"_s, '\0'),
    BUILTIN(getnpctimer, "i?"_s, 'i'),
    BUILTIN(setnpctimer, "i?"_s, '\0'),
    BUILTIN(setnpcdirection, "iii?"_s, '\0'),
    BUILTIN(npcaction, "i???"_s, '\0'),
    BUILTIN(camera, "???"_s, '\0'),
    BUILTIN(announce, "si"_s, '\0'),
    BUILTIN(mapannounce, "Msi"_s, '\0'),
    BUILTIN(getusers, "i"_s, 'i'),
    BUILTIN(getmapusers, "M"_s, 'i'),
    BUILTIN(getareausers, "Mxyxy?"_s, 'i'),
    BUILTIN(getareadropitem, "Mxyxyi?"_s, 'i'),
    BUILTIN(enablenpc, "s"_s, '\0'),
    BUILTIN(disablenpc, "s"_s, '\0'),
    BUILTIN(sc_start, "iTi?"_s, '\0'),
    BUILTIN(sc_end, "i?"_s, '\0'),
    BUILTIN(sc_check, "i?"_s, 'i'),
    BUILTIN(debugmes, "s"_s, '\0'),
    BUILTIN(wgm, "s"_s, '\0'),
    BUILTIN(gmlog, "s"_s, '\0'),
    BUILTIN(resetstatus, ""_s, '\0'),
    BUILTIN(attachrid, "i"_s, 'i'),
    BUILTIN(detachrid, ""_s, '\0'),
    BUILTIN(isloggedin, "i"_s, 'i'),
    BUILTIN(setmapflag, "Mi"_s, '\0'),
    BUILTIN(removemapflag, "Mi"_s, '\0'),
    BUILTIN(getmapflag, "Mi"_s, 'i'),
    BUILTIN(pvpon, "M"_s, '\0'),
    BUILTIN(pvpoff, "M"_s, '\0'),
    BUILTIN(setpvpchannel, "i"_s, '\0'),
    BUILTIN(getpvpflag, "i?"_s, 'i'),
    BUILTIN(emotion, "i?"_s, '\0'),
    BUILTIN(mapwarp, "MMxy"_s, '\0'),
    BUILTIN(mobcount, "ME"_s, 'i'),
    BUILTIN(marriage, "P"_s, 'i'),
    BUILTIN(divorce, ""_s, 'i'),
    BUILTIN(getitemlink, "I"_s, 's'),
    BUILTIN(getpartnerid2, ""_s, 'i'),
    BUILTIN(explode, "Nss"_s, '\0'),
    BUILTIN(getinventorylist, ""_s, '\0'),
    BUILTIN(getactivatedpoolskilllist, ""_s, '\0'),
    BUILTIN(getunactivatedpoolskilllist, ""_s, '\0'),
    BUILTIN(poolskill, "i"_s, '\0'),
    BUILTIN(unpoolskill, "i"_s, '\0'),
    BUILTIN(misceffect, "i?"_s, '\0'),
    BUILTIN(specialeffect, "i"_s, '\0'),
    BUILTIN(specialeffect2, "i"_s, '\0'),
    BUILTIN(nude, ""_s, '\0'),
    BUILTIN(unequipbyid, "i"_s, '\0'),
    BUILTIN(npcwarp, "xys"_s, '\0'),
    BUILTIN(npcareawarp, "xyxyis"_s, '\0'),
    BUILTIN(message, "Ps"_s, '\0'),
    BUILTIN(npctalk, "ss?"_s, '\0'),
    BUILTIN(registercmd, "ss"_s, '\0'),
    BUILTIN(title, "s"_s, '\0'),
    BUILTIN(smsg, "e??"_s, '\0'),
    BUILTIN(remotecmd, "s?"_s, '\0'),
    BUILTIN(sendcollision, "Mixy???"_s, '\0'),
    BUILTIN(music, "s"_s, '\0'),
    BUILTIN(mapmask, "i?"_s, '\0'),
    BUILTIN(getmask, ""_s, 'i'),
    BUILTIN(getlook, "i"_s, 'i'),
    BUILTIN(getsavepoint, "i"_s, 'v'),
    BUILTIN(areatimer, "MxyxytEi"_s, '\0'),
    BUILTIN(foreach, "iMxyxyE?"_s, '\0'),
    BUILTIN(isin, "Mxyxy"_s, 'i'),
    BUILTIN(iscollision, "Mxy"_s, 'i'),
    BUILTIN(shop, "s"_s, '\0'),
    BUILTIN(isdead, ""_s, 'i'),
    BUILTIN(aggravate, "i?"_s, '\0'),
    BUILTIN(issummon, "i"_s, 'i'),
    BUILTIN(fakenpcname, "ssi"_s, '\0'),
    BUILTIN(puppet, "mxysi??"_s, 'i'),
    BUILTIN(destroy, "?"_s, '\0'),
    BUILTIN(getx, ""_s, 'i'),
    BUILTIN(gety, ""_s, 'i'),
    BUILTIN(getdir, ""_s, 'i'),
    BUILTIN(getnpcx, "?"_s, 'i'),
    BUILTIN(getnpcy, "?"_s, 'i'),
    BUILTIN(strnpcinfo, "i?"_s, 's'),
    BUILTIN(getmap, "?"_s, 's'),
    BUILTIN(getmapmaxx, "M"_s, 'i'),
    BUILTIN(getmapmaxy, "M"_s, 'i'),
    BUILTIN(getmaphash, "M"_s, 'i'),
    BUILTIN(getmapnamefromhash, "i"_s, 's'),
    BUILTIN(mapexists, "M"_s, 'i'),
    BUILTIN(numberofmaps, ""_s, 'i'),
    BUILTIN(getmapnamebyindex, "i"_s, 's'),
    BUILTIN(mapexit, ""_s, '\0'),
    BUILTIN(freeloop, "i"_s, '\0'),
    BUILTIN(if_then_else, "iii"_s, 'v'),
    BUILTIN(max, "e?*"_s, 'i'),
    BUILTIN(min, "e?*"_s, 'i'),
    BUILTIN(average, "ii*"_s, 'i'),
    BUILTIN(sqrt, "i"_s, 'i'),
    BUILTIN(cbrt, "i"_s, 'i'),
    BUILTIN(pow, "ii"_s, 'i'),
    BUILTIN(target, "iii"_s, 'i'),
    BUILTIN(distance, "ii?"_s, 'i'),
    BUILTIN(chr, "i"_s, 's'),
    BUILTIN(ord, "s"_s, 'i'),
    BUILTIN(l, "s*"_s, 's'),
    {nullptr, ""_s, ""_s, '\0'},
};
} // namespace map
} // namespace tmwa
