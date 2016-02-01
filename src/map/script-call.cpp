#include "script-call-internal.hpp"
//    script-call.cpp - EAthena script frontend, engine, and library.
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

#include "../generic/intern-pool.hpp"

#include "../io/cxxstdio.hpp"

#include "../mmo/cxxstdio_enums.hpp"

#include "../high/core.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "globals.hpp"
#include "map.hpp"
#include "npc.hpp"
#include "pc.hpp"
#include "script-fun.hpp"
#include "script-parse-internal.hpp"
#include "script-persist.hpp"
#include "script-startup-internal.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
constexpr bool DEBUG_RUN = false;

static
struct ScriptConfigRun
{
    static const
    int check_cmdcount = 8192;
    static const
    int check_gotocount = 512;
} script_config;


/*==========================================
 * ridからsdへの解決
 *------------------------------------------
 */
dumb_ptr<map_session_data> script_rid2sd(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = map_id2sd(st->rid);
    if (!sd)
    {
        PRINTF("script_rid2sd: fatal error ! player not attached!\n"_fmt);
    }
    return sd;
}

/*==========================================
 * 変数の読み取り
 *------------------------------------------
 */
void get_val(dumb_ptr<map_session_data> sd, struct script_data *data)
{
    MATCH_BEGIN (*data)
    {
        MATCH_CASE (const ScriptDataParam&, u)
        {
            if (sd == nullptr)
                PRINTF("get_val error param SP::%d\n"_fmt, u.reg.sp());
            int numi = 0;
            if (sd)
                numi = pc_readparam(sd, u.reg.sp());
            *data = ScriptDataInt{numi};
        }
        MATCH_CASE (const ScriptDataVariable&, u)
        {
            ZString name_ = variable_names.outtern(u.reg.base());
            VarName name = stringish<VarName>(name_);
            char prefix = name.front();
            char postfix = name.back();

            if (prefix != '$')
            {
                if (sd == nullptr)
                    PRINTF("get_val error name?:%s\n"_fmt, name);
            }
            if (postfix == '$')
            {
                RString str;
                if (prefix == '@')
                {
                    if (sd)
                        str = pc_readregstr(sd, u.reg);
                }
                else if (prefix == '$')
                {
                    Option<P<RString>> s_ = mapregstr_db.search(u.reg);
                    OMATCH_BEGIN_SOME (s, s_)
                    {
                        str = *s;
                    }
                    OMATCH_END ();
                }
                else
                {
                    PRINTF("script: get_val: illegal scope string variable.\n"_fmt);
                    str = "!!ERROR!!"_s;
                }
                *data = ScriptDataStr{str};
            }
            else
            {
                int numi = 0;
                if (prefix == '@')
                {
                    if (sd)
                        numi = pc_readreg(sd, u.reg);
                }
                else if (prefix == '$')
                {
                    numi = mapreg_db.get(u.reg);
                }
                else if (prefix == '#')
                {
                    if (name[1] == '#')
                    {
                        if (sd)
                            numi = pc_readaccountreg2(sd, name);
                    }
                    else
                    {
                        if (sd)
                            numi = pc_readaccountreg(sd, name);
                    }
                }
                else
                {
                    if (sd)
                        numi = pc_readglobalreg(sd, name);
                }
                *data = ScriptDataInt{numi};
            }
        }
    }
    MATCH_END ();
}

void get_val(ScriptState *st, struct script_data *data)
{
    dumb_ptr<map_session_data> sd = st->rid ? map_id2sd(st->rid) : nullptr;
    get_val(sd, data);
}

/*==========================================
 * 変数の読み取り2
 *------------------------------------------
 */
struct script_data get_val2(ScriptState *st, SIR reg)
{
    struct script_data dat = ScriptDataVariable{reg};
    get_val(st, &dat);
    return dat;
}

/*==========================================
 * 変数設定用
 *------------------------------------------
 */
void set_reg(dumb_ptr<map_session_data> sd, VariableCode type, SIR reg, struct script_data vd)
{
    if (type == VariableCode::PARAM)
    {
        int val = vd.get_if<ScriptDataInt>()->numi;
        pc_setparam(sd, reg.sp(), val);
        return;
    }
    assert (type == VariableCode::VARIABLE);

    ZString name_ = variable_names.outtern(reg.base());
    VarName name = stringish<VarName>(name_);
    char prefix = name.front();
    char postfix = name.back();

    if (postfix == '$')
    {
        RString str = vd.get_if<ScriptDataStr>()->str;
        if (prefix == '@')
        {
            pc_setregstr(sd, reg, str);
        }
        else if (prefix == '$')
        {
            mapreg_setregstr(reg, str);
        }
        else
        {
            PRINTF("script: set_reg: illegal scope string variable !"_fmt);
        }
    }
    else
    {
        int val = vd.get_if<ScriptDataInt>()->numi;
        if (prefix == '@')
        {
            pc_setreg(sd, reg, val);
        }
        else if (prefix == '$')
        {
            mapreg_setreg(reg, val);
        }
        else if (prefix == '#')
        {
            if (name[1] == '#')
                pc_setaccountreg2(sd, name, val);
            else
                pc_setaccountreg(sd, name, val);
        }
        else
        {
            pc_setglobalreg(sd, name, val);
        }
    }
}

void set_reg(dumb_ptr<map_session_data> sd, VariableCode type, SIR reg, int id)
{
    struct script_data vd = ScriptDataInt{id};
    set_reg(sd, type, reg, vd);
}

void set_reg(dumb_ptr<map_session_data> sd, VariableCode type, SIR reg, RString zd)
{
    struct script_data vd = ScriptDataStr{zd};
    set_reg(sd, type, reg, vd);
}

/*==========================================
 * 文字列への変換
 *------------------------------------------
 */
RString conv_str(ScriptState *st, struct script_data *data)
{
    get_val(st, data);
    assert (!data->is<ScriptDataRetInfo>());
    if (auto *u = data->get_if<ScriptDataInt>())
    {
        AString buf = STRPRINTF("%d"_fmt, u->numi);
        *data = ScriptDataStr{buf};
    }
    return data->get_if<ScriptDataStr>()->str;
}

/*==========================================
 * 数値へ変換
 *------------------------------------------
 */
int conv_num(ScriptState *st, struct script_data *data)
{
    int rv = 0;
    get_val(st, data);
    assert (!data->is<ScriptDataRetInfo>());
    MATCH_BEGIN (*data)
    {
        MATCH_DEFAULT ()
        {
            abort();
        }
        MATCH_CASE (const ScriptDataStr&, u)
        {
            RString p = u.str;
            rv = atoi(p.c_str());
        }
        MATCH_CASE (const ScriptDataInt&, u)
        {
            return u.numi;
        }
        MATCH_CASE (const ScriptDataPos&, u)
        {
            return u.numi;
        }
    }
    MATCH_END ()
    *data = ScriptDataInt{rv};
    return rv;
}

Borrowed<const ScriptBuffer> conv_script(ScriptState *st, struct script_data *data)
{
    get_val(st, data);
    return data->get_if<ScriptDataRetInfo>()->script;
}

void push_copy(struct script_stack *stack, int pos_)
{
    script_data csd = stack->stack_datav[pos_];
    stack->stack_datav.push_back(csd);
}

void pop_stack(struct script_stack *stack, int start, int end)
{
    auto it = stack->stack_datav.begin();
    stack->stack_datav.erase(it + start, it + end);
}

//
// 実行部main
//
/*==========================================
 * コマンドの読み取り
 *------------------------------------------
 */
static
ByteCode get_com(ScriptPointer *script)
{
    if (static_cast<uint8_t>(script->peek()) >= 0x80)
    {
        // synthetic! Does not advance pos yet.
        return ByteCode::INT;
    }
    return script->pop();
}

/*==========================================
 * 数値の所得
 *------------------------------------------
 */
static
int get_num(ScriptPointer *scr)
{
    int i = 0;
    int j = 0;
    uint8_t val;
    do
    {
        val = static_cast<uint8_t>(scr->pop());
        i += (val & 0x7f) << j;
        j += 6;
    }
    while (val >= 0xc0);
    return i;
}

/*==========================================
 * スタックから値を取り出す
 *------------------------------------------
 */
static
int pop_val(ScriptState *st)
{
    if (st->stack->stack_datav.empty())
        return 0;
    script_data& back = st->stack->stack_datav.back();
    get_val(st, &back);
    int rv = 0;
    if (auto *u = back.get_if<ScriptDataInt>())
        rv = u->numi;
    st->stack->stack_datav.pop_back();
    return rv;
}

static
bool isstr(struct script_data& c)
{
    return c.is<ScriptDataStr>();
}

/*==========================================
 * 加算演算子
 *------------------------------------------
 */
static
void op_add(ScriptState *st)
{
    get_val(st, &st->stack->stack_datav.back());
    script_data back = st->stack->stack_datav.back();
    st->stack->stack_datav.pop_back();

    script_data& back1 = st->stack->stack_datav.back();
    get_val(st, &back1);

    if (!(isstr(back) || isstr(back1)))
    {
        back1.get_if<ScriptDataInt>()->numi += back.get_if<ScriptDataInt>()->numi;
    }
    else
    {
        RString sb = conv_str(st, &back);
        RString sb1 = conv_str(st, &back1);
        MString buf;
        buf += sb1;
        buf += sb;
        back1 = ScriptDataStr{.str= AString(buf)};
    }
}

/*==========================================
 * 二項演算子(文字列)
 *------------------------------------------
 */
static
void op_2str(ScriptState *st, ByteCode op, ZString s1, ZString s2)
{
    int a = 0;

    switch (op)
    {
        case ByteCode::EQ:
            a = s1 == s2;
            break;
        case ByteCode::NE:
            a = s1 != s2;
            break;
        case ByteCode::GT:
            a = s1 > s2;
            break;
        case ByteCode::GE:
            a = s1 >= s2;
            break;
        case ByteCode::LT:
            a = s1 < s2;
            break;
        case ByteCode::LE:
            a = s1 <= s2;
            break;
        default:
            PRINTF("illegal string operater\n"_fmt);
            break;
    }

    push_int<ScriptDataInt>(st->stack, a);
}

/*==========================================
 * 二項演算子(数値)
 *------------------------------------------
 */
static
void op_2num(ScriptState *st, ByteCode op, int i1, int i2)
{
    switch (op)
    {
        case ByteCode::SUB:
            i1 -= i2;
            break;
        case ByteCode::MUL:
            i1 *= i2;
            break;
        case ByteCode::DIV:
            i1 /= i2;
            break;
        case ByteCode::MOD:
            i1 %= i2;
            break;
        case ByteCode::AND:
            i1 &= i2;
            break;
        case ByteCode::OR:
            i1 |= i2;
            break;
        case ByteCode::XOR:
            i1 ^= i2;
            break;
        case ByteCode::LAND:
            i1 = i1 && i2;
            break;
        case ByteCode::LOR:
            i1 = i1 || i2;
            break;
        case ByteCode::EQ:
            i1 = i1 == i2;
            break;
        case ByteCode::NE:
            i1 = i1 != i2;
            break;
        case ByteCode::GT:
            i1 = i1 > i2;
            break;
        case ByteCode::GE:
            i1 = i1 >= i2;
            break;
        case ByteCode::LT:
            i1 = i1 < i2;
            break;
        case ByteCode::LE:
            i1 = i1 <= i2;
            break;
        case ByteCode::R_SHIFT:
            i1 = i1 >> i2;
            break;
        case ByteCode::L_SHIFT:
            i1 = i1 << i2;
            break;
    }
    push_int<ScriptDataInt>(st->stack, i1);
}

/*==========================================
 * 二項演算子
 *------------------------------------------
 */
static
void op_2(ScriptState *st, ByteCode op)
{
    // pop_val has unfortunate implications here
    script_data d2 = st->stack->stack_datav.back();
    st->stack->stack_datav.pop_back();
    get_val(st, &d2);
    script_data d1 = st->stack->stack_datav.back();
    st->stack->stack_datav.pop_back();
    get_val(st, &d1);

    if (isstr(d1) && isstr(d2))
    {
        // ss => op_2str
        op_2str(st, op, d1.get_if<ScriptDataStr>()->str, d2.get_if<ScriptDataStr>()->str);
    }
    else if (!(isstr(d1) || isstr(d2)))
    {
        // ii => op_2num
        op_2num(st, op, d1.get_if<ScriptDataInt>()->numi, d2.get_if<ScriptDataInt>()->numi);
    }
    else
    {
        // si,is => error
        PRINTF("script: op_2: int&str, str&int not allow.\n"_fmt);
        push_int<ScriptDataInt>(st->stack, 0);
    }
}

/*==========================================
 * 単項演算子
 *------------------------------------------
 */
static
void op_1num(ScriptState *st, ByteCode op)
{
    int i1;
    i1 = pop_val(st);
    switch (op)
    {
        case ByteCode::NEG:
            i1 = -i1;
            break;
        case ByteCode::NOT:
            i1 = ~i1;
            break;
        case ByteCode::LNOT:
            i1 = !i1;
            break;
    }
    push_int<ScriptDataInt>(st->stack, i1);
}

/*==========================================
 * 関数の実行
 *------------------------------------------
 */
void run_func(ScriptState *st)
{
    size_t end_sp = st->stack->stack_datav.size();
    size_t start_sp = end_sp - 1;
    while (!st->stack->stack_datav[start_sp].is<ScriptDataArg>())
    {
        start_sp--;
        if (start_sp == 0)
        {
            dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
            PRINTF("run_func: function not found! @ %s\n"_fmt, nd ? nd->name : NpcName());
            abort();
        }
    }
    // the func is before the arg
    start_sp--;
    st->start = start_sp;
    st->end = end_sp;

    if (!st->stack->stack_datav[st->start].is<ScriptDataFuncRef>())
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
        PRINTF("run_func: not a function or statement! @ %s\n"_fmt,
                nd ? nd->name : NpcName());
        abort();
    }
    size_t func = st->stack->stack_datav[st->start].get_if<ScriptDataFuncRef>()->numi;

    if (DEBUG_RUN && battle_config.etc_log)
    {
        PRINTF("run_func : %s\n"_fmt,
                builtin_functions[func].name);
        PRINTF("stack dump :"_fmt);
        for (script_data& d : st->stack->stack_datav)
        {
            MATCH_BEGIN (d)
            {
                MATCH_CASE (const ScriptDataInt&, u)
                {
                    PRINTF(" int(%d)"_fmt, u.numi);
                }
                MATCH_CASE (const ScriptDataRetInfo&, u)
                {
                    PRINTF(" retinfo(%p)"_fmt, static_cast<const void *>(&*u.script));
                }
                MATCH_CASE (const ScriptDataParam&, u)
                {
                    PRINTF(" param(%d)"_fmt, u.reg.sp());
                }
                MATCH_CASE (const ScriptDataVariable&, u)
                {
                    PRINTF(" name(%s)[%d]"_fmt, variable_names.outtern(u.reg.base()), u.reg.index());
                }
                MATCH_CASE (const ScriptDataArg&, u)
                {
                    (void)u;
                    PRINTF(" arg"_fmt);
                }
                MATCH_CASE (const ScriptDataPos&, u)
                {
                    (void)u;
                    PRINTF(" pos(%d)"_fmt, u.numi);
                }
                MATCH_CASE (const ScriptDataStr&, u)
                {
                    (void)u;
                    PRINTF(" str(%s)"_fmt, u.str);
                }
                MATCH_CASE (const ScriptDataFuncRef&, u)
                {
                    (void)u;
                    PRINTF(" func(%s)"_fmt, builtin_functions[u.numi].name);
                }
            }
            MATCH_END ();
        }
        PRINTF("\n"_fmt);
    }
    builtin_functions[func].func(st);

    pop_stack(st->stack, start_sp, end_sp);

    if (st->state == ScriptEndState::RETFUNC)
    {
        // ユーザー定義関数からの復帰
        int olddefsp = st->defsp;

        pop_stack(st->stack, st->defsp, start_sp); // 復帰に邪魔なスタック削除
        if (st->defsp < 4
            || !st->stack->stack_datav[st->defsp - 1].is<ScriptDataRetInfo>())
        {
            dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
            PRINTF("run_func: return without callfunc or callsub! @ %s\n"_fmt,
                    nd ? nd->name : NpcName());
            abort();
        }
        assert (olddefsp == st->defsp); // pretty sure it hasn't changed yet
        st->scriptp.code = Some(conv_script(st, &st->stack->stack_datav[olddefsp - 1]));   // スクリプトを復元
        st->scriptp.pos = conv_num(st, &st->stack->stack_datav[olddefsp - 2]);   // スクリプト位置の復元
        st->defsp = conv_num(st, &st->stack->stack_datav[olddefsp - 3]); // 基準スタックポインタを復元
        // Number of arguments.
        int i = conv_num(st, &st->stack->stack_datav[olddefsp - 4]); // 引数の数所得
        assert (i == 0);

        pop_stack(st->stack, olddefsp - 4 - i, olddefsp);  // 要らなくなったスタック(引数と復帰用データ)削除

        st->state = ScriptEndState::GOTO;
    }
}

/*==========================================
 * スクリプトの実行メイン部分
 *------------------------------------------
 */
static
void run_script_main(ScriptState *st, Borrowed<const ScriptBuffer> rootscript)
{
    int cmdcount = script_config.check_cmdcount;
    int gotocount = script_config.check_gotocount;
    struct script_stack *stack = st->stack;

    st->defsp = stack->stack_datav.size();

    int rerun_pos = st->scriptp.pos;
    st->state = ScriptEndState::ZERO;
    while (st->state == ScriptEndState::ZERO)
    {
        switch (ByteCode c = get_com(&st->scriptp))
        {
            case ByteCode::EOL:
                if (stack->stack_datav.size() != st->defsp)
                {
                    if (true)
                        PRINTF("stack.sp (%zu) != default (%d)\n"_fmt,
                                stack->stack_datav.size(),
                                st->defsp);
                    abort();
                }
                rerun_pos = st->scriptp.pos;
                break;
            case ByteCode::INT:
                // synthesized!
                push_int<ScriptDataInt>(stack, get_num(&st->scriptp));
                break;

            case ByteCode::POS:
            case ByteCode::VARIABLE:
            case ByteCode::FUNC_REF:
            case ByteCode::PARAM:
                // Note that these 3 have *very* different meanings,
                // despite being encoded similarly.
            {
                int arg = 0;
                arg |= static_cast<uint8_t>(st->scriptp.pop()) << 0;
                arg |= static_cast<uint8_t>(st->scriptp.pop()) << 8;
                arg |= static_cast<uint8_t>(st->scriptp.pop()) << 16;
                switch(c)
                {
                case ByteCode::POS:
                    push_int<ScriptDataPos>(stack, arg);
                    break;
                case ByteCode::VARIABLE:
                    push_reg<ScriptDataVariable>(stack, SIR::from(arg));
                    break;
                case ByteCode::FUNC_REF:
                    push_int<ScriptDataFuncRef>(stack, arg);
                    break;
                case ByteCode::PARAM:
                    SP arg_sp = static_cast<SP>(arg);
                    push_reg<ScriptDataParam>(stack, SIR::from(arg_sp));
                    break;
                }
            }
                break;
            case ByteCode::ARG:
                push_int<ScriptDataArg>(stack, 0);
                break;
            case ByteCode::STR:
                push_str<ScriptDataStr>(stack, st->scriptp.pops());
                break;
            case ByteCode::FUNC:
                run_func(st);
                if (st->state == ScriptEndState::GOTO)
                {
                    rerun_pos = st->scriptp.pos;
                    st->state = ScriptEndState::ZERO;
                    if (st->freeloop != 1 && gotocount > 0 && (--gotocount) <= 0)
                    {
                        dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
                        PRINTF("run_script: infinity loop! @ %s\n"_fmt,
                                nd ? nd->name : NpcName());
                        abort();
                    }
                }
                break;

            case ByteCode::ADD:
                op_add(st);
                break;

            case ByteCode::SUB:
            case ByteCode::MUL:
            case ByteCode::DIV:
            case ByteCode::MOD:
            case ByteCode::EQ:
            case ByteCode::NE:
            case ByteCode::GT:
            case ByteCode::GE:
            case ByteCode::LT:
            case ByteCode::LE:
            case ByteCode::AND:
            case ByteCode::OR:
            case ByteCode::XOR:
            case ByteCode::LAND:
            case ByteCode::LOR:
            case ByteCode::R_SHIFT:
            case ByteCode::L_SHIFT:
                op_2(st, c);
                break;

            case ByteCode::NEG:
            case ByteCode::NOT:
            case ByteCode::LNOT:
                op_1num(st, c);
                break;

            case ByteCode::NOP:
                st->state = ScriptEndState::END;
                break;

            default:
                if (battle_config.error_log)
                {
                    PRINTF("unknown command : %d @ %zu\n"_fmt,
                            c, st->scriptp.pos);
                    if (st->oid)
                    {
                        dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
                        PRINTF("NPC => %s\n"_fmt, nd->name);
                    }
                    if (st->rid)
                    {
                        dumb_ptr<map_session_data> sd = script_rid2sd(st);
                        PRINTF("PC => %s\n"_fmt, sd->status_key.name.to__actual());
                    }
                }
                abort();
        }
        if (st->freeloop != 1 && cmdcount > 0 && (--cmdcount) <= 0)
        {
            dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
            PRINTF("run_script: infinity loop! @ %s\n"_fmt,
                    nd ? nd->name : NpcName());
            abort();
        }
    }
    switch (st->state)
    {
        case ScriptEndState::STOP:
            break;
        case ScriptEndState::END:
        {
            dumb_ptr<map_session_data> sd = map_id2sd(st->rid);
            st->scriptp.code = None;
            st->scriptp.pos = -1;
            if (sd && sd->npc_id == st->oid)
                npc_event_dequeue(sd);
        }
            break;
        case ScriptEndState::RERUNLINE:
            st->scriptp.pos = rerun_pos;
            break;
    }

    if (st->state != ScriptEndState::END)
    {
        // 再開するためにスタック情報を保存
        dumb_ptr<map_session_data> sd = map_id2sd(st->rid);
        if (sd)
        {
            sd->npc_stackbuf = stack->stack_datav;
            sd->npc_script = st->scriptp.code;
            // sd->npc_pos is set later ... ???
            sd->npc_scriptroot = Some(rootscript);
        }
    }
}

/*==========================================
 * スクリプトの実行
 *------------------------------------------
 */
int run_script(ScriptPointer sp, BlockId rid, BlockId oid)
{
    return run_script_l(sp, rid, oid, nullptr);
}

int run_script_l(ScriptPointer sp, BlockId rid, BlockId oid,
        Slice<argrec_t> args)
{
    struct script_stack stack;
    ScriptState st;
    dumb_ptr<map_session_data> sd = map_id2sd(rid);
    P<const ScriptBuffer> rootscript = TRY_UNWRAP(sp.code, return -1);
    int i;
    if (sp.pos >> 24)
        return -1;

    if (sd && !sd->npc_stackbuf.empty() && sd->npc_scriptroot == Some(rootscript))
    {
        // 前回のスタックを復帰
        sp.code = sd->npc_script;
        stack.stack_datav = std::move(sd->npc_stackbuf);
    }
    st.stack = &stack;
    st.scriptp = sp;
    st.rid = rid;
    st.oid = oid;
    for (i = 0; i < args.size(); i++)
    {
        if (args[i].name.back() == '$')
            pc_setregstr(sd, SIR::from(variable_names.intern(args[i].name)), args[i].v.s);
        else
            pc_setreg(sd, SIR::from(variable_names.intern(args[i].name)), args[i].v.i);
    }
    run_script_main(&st, rootscript);

    stack.stack_datav.clear();
    return st.scriptp.pos;
}

void set_script_var_i(dumb_ptr<map_session_data> sd, VarName var, int e, int val)
{
    size_t k = variable_names.intern(var);
    SIR reg = SIR::from(k, e);
    set_reg(sd, VariableCode::VARIABLE, reg, val);
}
void set_script_var_s(dumb_ptr<map_session_data> sd, VarName var, int e, XString val)
{
    size_t k = variable_names.intern(var);
    SIR reg = SIR::from(k, e);
    set_reg(sd, VariableCode::VARIABLE, reg, val);
}
int get_script_var_i(dumb_ptr<map_session_data> sd, VarName var, int e)
{
    size_t k = variable_names.intern(var);
    SIR reg = SIR::from(k, e);
    struct script_data dat = ScriptDataVariable{.reg= reg};
    get_val(sd, &dat);
    if (auto *u = dat.get_if<ScriptDataInt>())
        return u->numi;
    PRINTF("Warning: you lied about the type and I'm too lazy to fix it!"_fmt);
    return 0;
}
ZString get_script_var_s(dumb_ptr<map_session_data> sd, VarName var, int e)
{
    size_t k = variable_names.intern(var);
    SIR reg = SIR::from(k, e);
    struct script_data dat = ScriptDataVariable{.reg= reg};
    get_val(sd, &dat);
    if (auto *u = dat.get_if<ScriptDataStr>())
        // this is almost certainly a memory leak after CONSTSTR removal
        return u->str;
    PRINTF("Warning: you lied about the type and I can't fix it!"_fmt);
    return ZString();
}
} // namespace map
} // namespace tmwa
