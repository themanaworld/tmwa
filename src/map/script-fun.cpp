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
#include "magic-interpreter-base.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "script-call-internal.hpp"
#include "script-parse-internal.hpp"
#include "script-persist.hpp"
#include "skill.hpp"
#include "storage.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
static
Array<LString, 11> pos_str //=
{{
    "Head"_s,
    "Body"_s,
    "Left hand"_s,
    "Right hand"_s,
    "Robe"_s,
    "Shoes"_s,
    "Accessory 1"_s,
    "Accessory 2"_s,
    "Head 2"_s,
    "Head 3"_s,
    "Not Equipped"_s,
}};

#define AARG(n) (st->stack->stack_datav[st->start + 2 + (n)])
#define HARG(n) (st->end > st->start + 2 + (n))

//
// 埋め込み関数
//
/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_mes(ScriptState *st)
{
    RString mes = conv_str(st, &AARG(0));
    clif_scriptmes(script_rid2sd(st), st->oid, mes);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_goto(ScriptState *st)
{
    if (!AARG(0).is<ScriptDataPos>())
    {
        PRINTF("script: goto: not label !\n"_fmt);
        st->state = ScriptEndState::END;
        return;
    }

    st->scriptp.pos = conv_num(st, &AARG(0));
    st->state = ScriptEndState::GOTO;
}

/*==========================================
 * ユーザー定義関数の呼び出し
 *------------------------------------------
 */
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

            push_int<ScriptDataInt>(st->stack, j); // 引数の数をプッシュ
            push_int<ScriptDataInt>(st->stack, st->defsp); // 現在の基準スタックポインタをプッシュ
            push_int<ScriptDataInt>(st->stack, st->scriptp.pos);   // 現在のスクリプト位置をプッシュ
            push_script<ScriptDataRetInfo>(st->stack, TRY_UNWRAP(st->scriptp.code, abort()));  // 現在のスクリプトをプッシュ

            st->scriptp = ScriptPointer(scr, 0);
            st->defsp = st->start + 4 + j;
            st->state = ScriptEndState::GOTO;
        }
        OMATCH_CASE_NONE ()
        {
            PRINTF("script:callfunc: function not found! [%s]\n"_fmt, str);
            st->state = ScriptEndState::END;
        }
    }
    OMATCH_END ();
}

/*==========================================
 * サブルーティンの呼び出し
 *------------------------------------------
 */
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

    push_int<ScriptDataInt>(st->stack, j); // 引数の数をプッシュ
    push_int<ScriptDataInt>(st->stack, st->defsp); // 現在の基準スタックポインタをプッシュ
    push_int<ScriptDataInt>(st->stack, st->scriptp.pos);   // 現在のスクリプト位置をプッシュ
    push_script<ScriptDataRetInfo>(st->stack, TRY_UNWRAP(st->scriptp.code, abort()));  // 現在のスクリプトをプッシュ

    st->scriptp.pos = pos_;
    st->defsp = st->start + 4 + j;
    st->state = ScriptEndState::GOTO;
}

/*==========================================
 * サブルーチン/ユーザー定義関数の終了
 *------------------------------------------
 */
static
void builtin_return(ScriptState *st)
{
#if 0
    if (HARG(0))
    {                           // 戻り値有り
        push_copy(st->stack, st->start + 2);
    }
#endif
    st->state = ScriptEndState::RETFUNC;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_next(ScriptState *st)
{
    st->state = ScriptEndState::STOP;
    clif_scriptnext(script_rid2sd(st), st->oid);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_close(ScriptState *st)
{
    st->state = ScriptEndState::END;
    clif_scriptclose(script_rid2sd(st), st->oid);
}

static
void builtin_close2(ScriptState *st)
{
    st->state = ScriptEndState::STOP;
    clif_scriptclose(script_rid2sd(st), st->oid);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_menu(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

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
                st->state = ScriptEndState::END;
                return;
            }
            st->scriptp.pos = AARG(arg_index).get_if<ScriptDataPos>()->numi;
            st->state = ScriptEndState::GOTO;
        }
    }
}

/*==========================================
 *
 *------------------------------------------
 */
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

/*==========================================
 * Check whether the PC is at the specified location
 *------------------------------------------
 */
static
void builtin_isat(ScriptState *st)
{
    int x, y;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x = conv_num(st, &AARG(1));
    y = conv_num(st, &AARG(2));

    if (!sd)
        return;

    push_int<ScriptDataInt>(st->stack,
            (x == sd->bl_x) && (y == sd->bl_y)
            && (str == sd->bl_m->name_));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_warp(ScriptState *st)
{
    int x, y;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x = conv_num(st, &AARG(1));
    y = conv_num(st, &AARG(2));
    if (str == "Random"_s)
        pc_randomwarp(sd, BeingRemoveWhy::WARPED);
    else if (str == "SavePoint"_s or str == "Save"_s)
    {
        if (sd->bl_m->flag.get(MapFlag::NORETURN))
            return;

        pc_setpos(sd, sd->status.save_point.map_, sd->status.save_point.x, sd->status.save_point.y,
                BeingRemoveWhy::WARPED);
    }
    else
        pc_setpos(sd, str, x, y, BeingRemoveWhy::GONE);
}

/*==========================================
 * エリア指定ワープ
 *------------------------------------------
 */
static
void builtin_areawarp_sub(dumb_ptr<block_list> bl, MapName mapname, int x, int y)
{
    dumb_ptr<map_session_data> sd = bl->is_player();
    if (mapname == "Random"_s)
        pc_randomwarp(sd, BeingRemoveWhy::WARPED);
    else
        pc_setpos(sd, mapname, x, y, BeingRemoveWhy::GONE);
}

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

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_heal(ScriptState *st)
{
    int hp, sp;

    hp = conv_num(st, &AARG(0));
    sp = conv_num(st, &AARG(1));
    pc_heal(script_rid2sd(st), hp, sp);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_itemheal(ScriptState *st)
{
    int hp, sp;

    hp = conv_num(st, &AARG(0));
    sp = conv_num(st, &AARG(1));
    pc_itemheal(script_rid2sd(st), hp, sp);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_percentheal(ScriptState *st)
{
    int hp, sp;

    hp = conv_num(st, &AARG(0));
    sp = conv_num(st, &AARG(1));
    pc_percentheal(script_rid2sd(st), hp, sp);
}

/*==========================================
 *
 *------------------------------------------
 */
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

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_if (ScriptState *st)
{
    int sel, i;

    sel = conv_num(st, &AARG(0));
    if (!sel)
        return;

    // 関数名をコピー
    push_copy(st->stack, st->start + 3);
    // 間に引数マーカを入れて
    push_int<ScriptDataArg>(st->stack, 0);
    // 残りの引数をコピー
    for (i = st->start + 4; i < st->end; i++)
    {
        push_copy(st->stack, i);
    }
    run_func(st);
}

/*==========================================
 * 変数設定
 *------------------------------------------
 */
static
void builtin_set(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = nullptr;
    if (auto *u = AARG(0).get_if<ScriptDataParam>())
    {
        SIR reg = u->reg;
        sd = script_rid2sd(st);

        int val = conv_num(st, &AARG(1));
        set_reg(sd, VariableCode::PARAM, reg, val);
        return;
    }

    SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;

    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();
    char postfix = name.back();

    if (prefix != '$')
        sd = script_rid2sd(st);

    if (postfix == '$')
    {
        // 文字列
        RString str = conv_str(st, &AARG(1));
        set_reg(sd, VariableCode::VARIABLE, reg, str);
    }
    else
    {
        // 数値
        int val = conv_num(st, &AARG(1));
        set_reg(sd, VariableCode::VARIABLE, reg, val);
    }

}

/*==========================================
 * 配列変数設定
 *------------------------------------------
 */
static
void builtin_setarray(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = nullptr;
    SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();
    char postfix = name.back();

    if (prefix != '$' && prefix != '@')
    {
        PRINTF("builtin_setarray: illegal scope !\n"_fmt);
        return;
    }
    if (prefix != '$')
        sd = script_rid2sd(st);

    for (int j = 0, i = 1; i < st->end - st->start - 2 && j < 256; i++, j++)
    {
        if (postfix == '$')
            set_reg(sd, VariableCode::VARIABLE, reg.iplus(j), conv_str(st, &AARG(i)));
        else
            set_reg(sd, VariableCode::VARIABLE, reg.iplus(j), conv_num(st, &AARG(i)));
    }
}

/*==========================================
 * 配列変数クリア
 *------------------------------------------
 */
static
void builtin_cleararray(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = nullptr;
    SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();
    char postfix = name.back();
    int sz = conv_num(st, &AARG(2));

    if (prefix != '$' && prefix != '@')
    {
        PRINTF("builtin_cleararray: illegal scope !\n"_fmt);
        return;
    }
    if (prefix != '$')
        sd = script_rid2sd(st);

    for (int i = 0; i < sz; i++)
    {
        if (postfix == '$')
            set_reg(sd, VariableCode::VARIABLE, reg.iplus(i), conv_str(st, &AARG(1)));
        else
            set_reg(sd, VariableCode::VARIABLE, reg.iplus(i), conv_num(st, &AARG(1)));
    }

}

/*==========================================
 * 配列変数のサイズ所得
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

static
void builtin_getarraysize(ScriptState *st)
{
    SIR reg = AARG(0).get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();

    if (prefix != '$' && prefix != '@')
    {
        PRINTF("builtin_copyarray: illegal scope !\n"_fmt);
        return;
    }

    push_int<ScriptDataInt>(st->stack, getarraysize(st, reg));
}

/*==========================================
 * 指定要素を表す値(キー)を所得する
 *------------------------------------------
 */
static
void builtin_getelementofarray(ScriptState *st)
{
    if (auto *u = AARG(0).get_if<ScriptDataVariable>())
    {
        int i = conv_num(st, &AARG(1));
        if (i > 255 || i < 0)
        {
            PRINTF("script: getelementofarray (operator[]): param2 illegal number %d\n"_fmt,
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
        PRINTF("script: getelementofarray (operator[]): param1 not name !\n"_fmt);
        push_int<ScriptDataInt>(st->stack, 0);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_setlook(ScriptState *st)
{
    LOOK type = LOOK(conv_num(st, &AARG(0)));
    int val = conv_num(st, &AARG(1));

    pc_changelook(script_rid2sd(st), type, val);

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_countitem(ScriptState *st)
{
    ItemNameId nameid;
    int count = 0;
    dumb_ptr<map_session_data> sd;

    struct script_data *data;

    sd = script_rid2sd(st);

    data = &AARG(0);
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
            PRINTF("wrong item ID : countitem (%i)\n"_fmt, nameid);
    }
    push_int<ScriptDataInt>(st->stack, count);

}

/*==========================================
 * 重量チェック
 *------------------------------------------
 */
static
void builtin_checkweight(ScriptState *st)
{
    ItemNameId nameid;
    int amount;
    dumb_ptr<map_session_data> sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &AARG(0);
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

    amount = conv_num(st, &AARG(1));
    if (amount <= 0 || !nameid)
    {
        //if get wrong item ID or amount<=0, don't count weight of non existing items
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

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_getitem(ScriptState *st)
{
    ItemNameId nameid;
    int amount;
    dumb_ptr<map_session_data> sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &AARG(0);
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

    if ((amount =
         conv_num(st, &AARG(1))) <= 0)
    {
        return;               //return if amount <=0, skip the useles iteration
    }

    if (nameid)
    {
        Item item_tmp {};
        item_tmp.nameid = nameid;
        if (HARG(3))    //アイテムを指定したIDに渡す
            sd = map_id2sd(wrap<BlockId>(conv_num(st, &AARG(3))));
        if (sd == nullptr)         //アイテムを渡す相手がいなかったらお帰り
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

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_makeitem(ScriptState *st)
{
    ItemNameId nameid;
    int amount;
    int x, y;
    dumb_ptr<map_session_data> sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &AARG(0);
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

    amount = conv_num(st, &AARG(1));
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

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_delitem(ScriptState *st)
{
    ItemNameId nameid;
    int amount;
    dumb_ptr<map_session_data> sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &AARG(0);
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

    amount = conv_num(st, &AARG(1));

    if (!nameid || amount <= 0)
    {
        //by Lupus. Don't run FOR if u got wrong item ID or amount<=0
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

/*==========================================
 *キャラ関係のパラメータ取得
 *------------------------------------------
 */
static
void builtin_readparam(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    SP type = SP(conv_num(st, &AARG(0)));
    if (HARG(1))
        sd = map_nick2sd(stringish<CharName>(ZString(conv_str(st, &AARG(1)))));
    else
        sd = script_rid2sd(st);

    if (sd == nullptr)
    {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    }

    push_int<ScriptDataInt>(st->stack, pc_readparam(sd, type));

}

/*==========================================
 *キャラ関係のID取得
 *------------------------------------------
 */
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
    if (sd == nullptr)
    {
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

/*==========================================
 *指定IDのPT名取得
 *------------------------------------------
 */
static
RString builtin_getpartyname_sub(PartyId party_id)
{
    Option<PartyPair> p = party_search(party_id);

    return p.pmd_pget(&PartyMost::name).copy_or(PartyName());
}

/*==========================================
 * キャラクタの名前
 *------------------------------------------
 */
static
void builtin_strcharinfo(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;
    int num;

    sd = script_rid2sd(st);
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

/*==========================================
 * GetEquipID(Pos);     Pos: 1-10
 *------------------------------------------
 */
static
void builtin_getequipid(ScriptState *st)
{
    int num;
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);
    if (sd == nullptr)
    {
        PRINTF("getequipid: sd == nullptr\n"_fmt);
        return;
    }
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

/*==========================================
 * freeloop
 *------------------------------------------
 */
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

/*==========================================
 * 装備名文字列（精錬メニュー用）
 *------------------------------------------
 */
static
void builtin_getequipname(ScriptState *st)
{
    int num;
    dumb_ptr<map_session_data> sd;

    AString buf;

    sd = script_rid2sd(st);
    num = conv_num(st, &AARG(0));
    IOff0 i = pc_checkequip(sd, equip[num - 1]);
    if (i.ok())
    {
        Option<P<struct item_data>> item_ = sd->inventory_data[i];
        OMATCH_BEGIN (item_)
        {
            OMATCH_CASE_SOME (item)
            {
                buf = STRPRINTF("%s-[%s]"_fmt, pos_str[num - 1], item->jname);
            }
            OMATCH_CASE_NONE ()
            {
                buf = STRPRINTF("%s-[%s]"_fmt, pos_str[num - 1], pos_str[10]);
            }
        }
        OMATCH_END ();
    }
    else
    {
        buf = STRPRINTF("%s-[%s]"_fmt, pos_str[num - 1], pos_str[10]);
    }
    push_str<ScriptDataStr>(st->stack, buf);

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_statusup2(ScriptState *st)
{
    SP type = SP(conv_num(st, &AARG(0)));
    int val = conv_num(st, &AARG(1));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    pc_statusup2(sd, type, val);

}

/*==========================================
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
static
void builtin_bonus(ScriptState *st)
{
    SP type = SP(conv_num(st, &AARG(0)));
    int val = conv_num(st, &AARG(1));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    pc_bonus(sd, type, val);

}

/*==========================================
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
static
void builtin_bonus2(ScriptState *st)
{
    SP type = SP(conv_num(st, &AARG(0)));
    int type2 = conv_num(st, &AARG(1));
    int val = conv_num(st, &AARG(2));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    pc_bonus2(sd, type, type2, val);

}

/*==========================================
 * スキル所得
 *------------------------------------------
 */
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
    pc_skill(sd, id, level, flag);
    clif_skillinfoblock(sd);

}

/*==========================================
 * [Fate] Sets the skill level permanently
 *------------------------------------------
 */
static
void builtin_setskill(ScriptState *st)
{
    int level;
    dumb_ptr<map_session_data> sd;

    SkillID id = static_cast<SkillID>(conv_num(st, &AARG(0)));
    level = conv_num(st, &AARG(1));
    sd = script_rid2sd(st);

    level = std::min(level, MAX_SKILL_LEVEL);
    level = std::max(level, 0);
    sd->status.skill[id].lv = level;
    clif_skillinfoblock(sd);
}

/*==========================================
 * スキルレベル所得
 *------------------------------------------
 */
static
void builtin_getskilllv(ScriptState *st)
{
    SkillID id = SkillID(conv_num(st, &AARG(0)));
    push_int<ScriptDataInt>(st->stack, pc_checkskill(script_rid2sd(st), id));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_getgmlevel(ScriptState *st)
{
    push_int<ScriptDataInt>(st->stack, pc_isGM(script_rid2sd(st)).get_all_bits());
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_end(ScriptState *st)
{
    st->state = ScriptEndState::END;
}

/*==========================================
 * [Freeyorp] Return the current opt2
 *------------------------------------------
 */

static
void builtin_getopt2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);

    push_int<ScriptDataInt>(st->stack, static_cast<uint16_t>(sd->opt2));

}

/*==========================================
 * [Freeyorp] Sets opt2
 *------------------------------------------
 */

static
void builtin_setopt2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    Opt2 new_opt2 = Opt2(conv_num(st, &AARG(0)));
    sd = script_rid2sd(st);
    if (new_opt2 == sd->opt2)
        return;
    sd->opt2 = new_opt2;
    clif_changeoption(sd);
    pc_calcstatus(sd, 0);

}

/*==========================================
 *      セーブポイントの保存
 *------------------------------------------
 */
static
void builtin_savepoint(ScriptState *st)
{
    int x, y;

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x = conv_num(st, &AARG(1));
    y = conv_num(st, &AARG(2));
    pc_setsavepoint(script_rid2sd(st), str, x, y);
}

/*==========================================
 * gettimetick(type)
 *
 * type The type of time measurement.
 *  Specify 0 for the system tick, 1 for
 *  seconds elapsed today, or 2 for seconds
 *  since Unix epoch. Defaults to 0 for any
 *  other value.
 *------------------------------------------
 */
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

/*==========================================
 * GetTime(Type);
 * 1: Sec     2: Min     3: Hour
 * 4: WeekDay     5: MonthDay     6: Month
 * 7: Year
 *------------------------------------------
 */
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

/*==========================================
 * カプラ倉庫を開く
 *------------------------------------------
 */
static
void builtin_openstorage(ScriptState *st)
{
//  int sync = 0;
//  if (st->end >= 3) sync = conv_num(st,& (st->stack->stack_data[st->start+2]));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

//  if (sync) {
    st->state = ScriptEndState::STOP;
    sd->npc_flags.storage = 1;
//  } else st->state = ScriptEndState::END;

    storage_storageopen(sd);
}

/*==========================================
 * NPCで経験値上げる
 *------------------------------------------
 */
static
void builtin_getexp(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int base = 0, job = 0;

    base = conv_num(st, &AARG(0));
    job = conv_num(st, &AARG(1));
    if (base < 0 || job < 0)
        return;
    if (sd)
        pc_gainexp_reason(sd, base, job, PC_GAINEXP_REASON::SCRIPT);

}

/*==========================================
 * モンスター発生
 *------------------------------------------
 */
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

/*==========================================
 * モンスター発生
 *------------------------------------------
 */
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

static
void builtin_killmonsterall_sub(dumb_ptr<block_list> bl)
{
    mob_delete(bl->is_mob());
}

static
void builtin_killmonsterall(ScriptState *st)
{
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);
    map_foreachinarea(builtin_killmonsterall_sub,
            m,
            0, 0,
            m->xs, m->ys,
            BL::MOB);
}

/*==========================================
 * NPC主体イベント実行
 *------------------------------------------
 */
static
void builtin_donpcevent(ScriptState *st)
{
    ZString event_ = ZString(conv_str(st, &AARG(0)));
    NpcEvent event;
    extract(event_, &event);
    npc_event_do(event);
}

/*==========================================
 * イベントタイマー追加
 *------------------------------------------
 */
static
void builtin_addtimer(ScriptState *st)
{
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARG(0)));
    ZString event_ = ZString(conv_str(st, &AARG(1)));
    NpcEvent event;
    extract(event_, &event);
    pc_addeventtimer(script_rid2sd(st), tick, event);
}

/*==========================================
 * NPCタイマー初期化
 *------------------------------------------
 */
static
void builtin_initnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    if (HARG(0))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(0)))));
    else
        nd_ = map_id_is_npc(st->oid);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_settimerevent_tick(nd, interval_t::zero());
    npc_timerevent_start(nd);
}

/*==========================================
 * NPCタイマー開始
 *------------------------------------------
 */
static
void builtin_startnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    if (HARG(0))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(0)))));
    else
        nd_ = map_id_is_npc(st->oid);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_timerevent_start(nd);
}

/*==========================================
 * NPCタイマー停止
 *------------------------------------------
 */
static
void builtin_stopnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    if (HARG(0))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(0)))));
    else
        nd_ = map_id_is_npc(st->oid);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_timerevent_stop(nd);
}

/*==========================================
 * NPCタイマー情報所得
 *------------------------------------------
 */
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

/*==========================================
 * NPCタイマー値設定
 *------------------------------------------
 */
static
void builtin_setnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARG(0)));
    if (HARG(1))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(1)))));
    else
        nd_ = map_id_is_npc(st->oid);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_settimerevent_tick(nd, tick);
}

/*==========================================
 * 天の声アナウンス
 *------------------------------------------
 */
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
        clif_GMmessage(bl, str, flag);
    }
    else
        intif_GMmessage(str);
}

/*==========================================
 * 天の声アナウンス（特定マップ）
 *------------------------------------------
 */
static
void builtin_mapannounce_sub(dumb_ptr<block_list> bl, XString str, int flag)
{
    clif_GMmessage(bl, str, flag | 3);
}

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

/*==========================================
 * ユーザー数所得
 *------------------------------------------
 */
static
void builtin_getusers(ScriptState *st)
{
    int flag = conv_num(st, &AARG(0));
    dumb_ptr<block_list> bl = map_id2bl((flag & 0x08) ? st->oid : st->rid);
    int val = 0;
    switch (flag & 0x07)
    {
        case 0:
            val = bl->bl_m->users;
            break;
        case 1:
            val = map_getusers();
            break;
    }
    push_int<ScriptDataInt>(st->stack, val);
}

/*==========================================
 * マップ指定ユーザー数所得
 *------------------------------------------
 */
static
void builtin_getmapusers(ScriptState *st)
{
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(str),
    {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    });
    push_int<ScriptDataInt>(st->stack, m->users);
}

/*==========================================
 * エリア指定ユーザー数所得
 *------------------------------------------
 */
static
void builtin_getareausers_sub(dumb_ptr<block_list> bl, int *users)
{
    if (bool(bl->is_player()->status.option & Opt0::HIDE))
        return;
    (*users)++;
}

static
void builtin_getareausers_living_sub(dumb_ptr<block_list> bl, int *users)
{
    if (bool(bl->is_player()->status.option & Opt0::HIDE))
        return;
    if (!pc_isdead(bl->is_player()))
        (*users)++;
}

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
 *------------------------------------------
 */
static
void builtin_getareadropitem_sub(dumb_ptr<block_list> bl, ItemNameId item, int *amount)
{
    dumb_ptr<flooritem_data> drop = bl->is_item();

    if (drop->item_data.nameid == item)
        (*amount) += drop->item_data.amount;

}

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

static
void builtin_getareadropitem(ScriptState *st)
{
    ItemNameId item;
    int x0, y0, x1, y1, amount = 0, delitems = 0;
    struct script_data *data;

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x0 = conv_num(st, &AARG(1));
    y0 = conv_num(st, &AARG(2));
    x1 = conv_num(st, &AARG(3));
    y1 = conv_num(st, &AARG(4));

    data = &AARG(5);
    get_val(st, data);
    if (data->is<ScriptDataStr>())
    {
        ZString name = ZString(conv_str(st, data));
        Option<P<struct item_data>> item_data_ = itemdb_searchname(name);
        OMATCH_BEGIN_SOME (item_data, item_data_)
        {
            item = item_data->nameid;
        }
        OMATCH_END ();
    }
    else
        item = wrap<ItemNameId>(conv_num(st, data));

    if (HARG(6))
        delitems = conv_num(st, &AARG(6));

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(str),
    {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    });
    if (delitems)
        map_foreachinarea(std::bind(builtin_getareadropitem_sub_anddelete, ph::_1, item, &amount),
                m,
                x0, y0,
                x1, y1,
                BL::ITEM);
    else
        map_foreachinarea(std::bind(builtin_getareadropitem_sub, ph::_1, item, &amount),
                m,
                x0, y0,
                x1, y1,
                BL::ITEM);

    push_int<ScriptDataInt>(st->stack, amount);
}

/*==========================================
 * NPCの有効化
 *------------------------------------------
 */
static
void builtin_enablenpc(ScriptState *st)
{
    NpcName str = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
    npc_enable(str, 1);
}

/*==========================================
 * NPCの無効化
 *------------------------------------------
 */
static
void builtin_disablenpc(ScriptState *st)
{
    NpcName str = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
    npc_enable(str, 0);
}

/*==========================================
 * 状態異常にかかる
 *------------------------------------------
 */
static
void builtin_sc_start(ScriptState *st)
{
    dumb_ptr<block_list> bl;
    int val1;
    StatusChange type = static_cast<StatusChange>(conv_num(st, &AARG(0)));
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARG(1)));
    if (tick < 1_s)
        // work around old behaviour of:
        // speed potion
        // atk potion
        // matk potion
        //
        // which used to use seconds
        // all others used milliseconds
        tick *= 1000;
    val1 = conv_num(st, &AARG(2));
    if (HARG(3))    //指定したキャラを状態異常にする
        bl = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(3))));
    else
        bl = map_id2bl(st->rid);
    skill_status_change_start(bl, type, val1, tick);
}

/*==========================================
 * 状態異常が直る
 *------------------------------------------
 */
static
void builtin_sc_end(ScriptState *st)
{
    dumb_ptr<block_list> bl;
    StatusChange type = StatusChange(conv_num(st, &AARG(0)));
    bl = map_id2bl(st->rid);
    skill_status_change_end(bl, type, nullptr);
}

static
void builtin_sc_check(ScriptState *st)
{
    dumb_ptr<block_list> bl;
    StatusChange type = StatusChange(conv_num(st, &AARG(0)));
    bl = map_id2bl(st->rid);

    push_int<ScriptDataInt>(st->stack, skill_status_change_active(bl, type));

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_debugmes(ScriptState *st)
{
    RString mes = conv_str(st, &AARG(0));
    PRINTF("script debug : %d %d : %s\n"_fmt,
            st->rid, st->oid, mes);
}

/*==========================================
 * ステータスリセット
 *------------------------------------------
 */
static
void builtin_resetstatus(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;
    sd = script_rid2sd(st);
    pc_resetstate(sd);
}

/*==========================================
 * 性別変換
 *------------------------------------------
 */
static
void builtin_changesex(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = nullptr;
    sd = script_rid2sd(st);

    chrif_char_ask_name(AccountId(), sd->status_key.name, 5, HumanTimeDiff()); // type: 5 - changesex
    chrif_save(sd);
}

/*==========================================
 * RIDのアタッチ
 *------------------------------------------
 */
static
void builtin_attachrid(ScriptState *st)
{
    st->rid = wrap<BlockId>(conv_num(st, &AARG(0)));
    push_int<ScriptDataInt>(st->stack, (map_id2sd(st->rid) != nullptr));
}

/*==========================================
 * RIDのデタッチ
 *------------------------------------------
 */
static
void builtin_detachrid(ScriptState *st)
{
    st->rid = BlockId();
}

/*==========================================
 * 存在チェック
 *------------------------------------------
 */
static
void builtin_isloggedin(ScriptState *st)
{
    push_int<ScriptDataInt>(st->stack,
              map_id2sd(wrap<BlockId>(conv_num(st, &AARG(0)))) != nullptr);
}

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
            map_session_data *pl_sd = static_cast<map_session_data *>(s->session_data.get());
            if (pl_sd && pl_sd->state.auth)
            {
                if (m == pl_sd->bl_m && !pl_sd->pvp_timer)
                {
                    pl_sd->pvp_timer = Timer(gettick() + 200_ms,
                            std::bind(pc_calc_pvprank_timer, ph::_1, ph::_2,
                                pl_sd->bl_id));
                    pl_sd->pvp_rank = 0;
                    pl_sd->pvp_point = 5;
                }
            }
        }
    }
}

static
void builtin_pvpoff(ScriptState *st)
{
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(str), return);
    if (m->flag.get(MapFlag::PVP) && m->flag.get(MapFlag::NOPVP))
    {
        m->flag.set(MapFlag::PVP, 0);

        if (battle_config.pk_mode)  // disable ranking options if pk_mode is on [Valaris]
            return;

        for (io::FD i : iter_fds())
        {
            Session *s = get_session(i);
            if (!s)
                continue;
            map_session_data *pl_sd = static_cast<map_session_data *>(s->session_data.get());
            if (pl_sd && pl_sd->state.auth)
            {
                if (m == pl_sd->bl_m)
                {
                    pl_sd->pvp_timer.cancel();
                }
            }
        }
    }
}

static
void builtin_setpvpchannel(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int flag;
    flag = conv_num(st, &AARG(0));
    if (flag < 1)
        flag = 0;

    sd->state.pvpchannel = flag;
}

static
void builtin_getpvpflag(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
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

/*==========================================
 *      NPCエモーション
 *------------------------------------------
 */

static
void builtin_emotion(ScriptState *st)
{
    int type;
    type = conv_num(st, &AARG(0));
    if (type < 0 || type > 100)
        return;
    clif_emotion(map_id2bl(st->oid), type);
}

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

static
void builtin_cmdothernpc(ScriptState *st)   // Added by RoVeRT
{
    NpcName npc = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
    ZString command = ZString(conv_str(st, &AARG(1)));

    npc_command(map_id2sd(st->rid), npc, command);
}

static
void builtin_mobcount_sub(dumb_ptr<block_list> bl, NpcEvent event, int *c)
{
    if (event == bl->is_mob()->npc_event)
        (*c)++;
}

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

/*==========================================
 * IDからItem名
 *------------------------------------------
 */
static
void builtin_getitemname(ScriptState *st)
{
    Option<P<struct item_data>> i_data = None;
    struct script_data *data;

    data = &AARG(0);
    get_val(st, data);
    if (data->is<ScriptDataStr>())
    {
        ZString name = ZString(conv_str(st, data));
        i_data = itemdb_searchname(name);
    }
    else
    {
        ItemNameId item_id = wrap<ItemNameId>(conv_num(st, data));
        i_data = Some(itemdb_search(item_id));
    }

    RString item_name = i_data.pmd_pget(&item_data::jname).copy_or(stringish<ItemName>("Unknown Item"_s));

    push_str<ScriptDataStr>(st->stack, item_name);
}

static
void builtin_getspellinvocation(ScriptState *st)
{
    RString name = conv_str(st, &AARG(0));

    AString invocation = magic::magic_find_invocation(name);
    if (!invocation)
        invocation = "..."_s;

    push_str<ScriptDataStr>(st->stack, invocation);
}

static
void builtin_getpartnerid2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    push_int<ScriptDataInt>(st->stack, unwrap<CharId>(sd->status.partner_id));
}

/*==========================================
 * PCの所持品情報読み取り
 *------------------------------------------
 */
static
void builtin_getinventorylist(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int j = 0;
    if (!sd)
        return;
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

static
void builtin_getactivatedpoolskilllist(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    SkillID pool_skills[MAX_SKILL_POOL];
    int skill_pool_size = skill_pool(sd, pool_skills);
    int i, count = 0;

    if (!sd)
        return;

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

static
void builtin_getunactivatedpoolskilllist(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int i, count = 0;

    if (!sd)
        return;

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

static
void builtin_poolskill(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    SkillID skill_id = SkillID(conv_num(st, &AARG(0)));

    skill_pool_activate(sd, skill_id);
    clif_skillinfoblock(sd);

}

static
void builtin_unpoolskill(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    SkillID skill_id = SkillID(conv_num(st, &AARG(0)));

    skill_pool_deactivate(sd, skill_id);
    clif_skillinfoblock(sd);

}

/*==========================================
 * NPCから発生するエフェクト
 * misceffect(effect, [target])
 *
 * effect The effect type/ID.
 * target The player name or being ID on
 *  which to display the effect. If not
 *  specified, it attempts to default to
 *  the current NPC or invoking PC.
 *------------------------------------------
 */
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

/*==========================================
 * Special effects [Valaris]
 *------------------------------------------
 */
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

static
void builtin_specialeffect2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    if (sd == nullptr)
        return;

    clif_specialeffect(sd,
                        conv_num(st,
                                  &AARG(0)),
                        0);

}

/*==========================================
 * Nude [Valaris]
 *------------------------------------------
 */

static
void builtin_nude(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    if (sd == nullptr)
        return;

    for (EQUIP i : EQUIPs)
    {
        IOff0 idx = sd->equip_index_maybe[i];
        if (idx.ok())
            pc_unequipitem(sd, idx, CalcStatus::LATER);
    }
    pc_calcstatus(sd, 0);

}

/*==========================================
 * UnequipById [Freeyorp]
 *------------------------------------------
 */

static
void builtin_unequipbyid(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    if (sd == nullptr)
        return;

    EQUIP slot_id = EQUIP(conv_num(st, &AARG(0)));

    if (slot_id >= EQUIP() && slot_id < EQUIP::COUNT)
    {
        IOff0 idx = sd->equip_index_maybe[slot_id];
        if (idx.ok())
            pc_unequipitem(sd, idx, CalcStatus::LATER);
    }

    pc_calcstatus(sd, 0);

}

/*==========================================
 * gmcommand [MouseJstr]
 *
 * suggested on the forums...
 *------------------------------------------
 */

static
void builtin_gmcommand(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);
    RString cmd = conv_str(st, &AARG(0));

    is_atcommand(sd->sess, sd, cmd, GmLevel::from(-1U));

}

/*==========================================
 * npcwarp [remoitnane]
 * Move NPC to a new position on the same map.
 *------------------------------------------
 */
static
void builtin_npcwarp(ScriptState *st)
{
    int x, y;
    dumb_ptr<npc_data> nd = nullptr;

    x = conv_num(st, &AARG(0));
    y = conv_num(st, &AARG(1));
    NpcName npc = stringish<NpcName>(ZString(conv_str(st, &AARG(2))));
    nd = npc_name2id(npc);

    if (!nd)
    {
        PRINTF("builtin_npcwarp: no such npc: %s\n"_fmt, npc);
        return;
    }

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

/*==========================================
 * npcareawarp [remoitnane] [wushin]
 * Move NPC to a new area on the same map.
 *------------------------------------------
 */
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
        PRINTF("builtin_npcareawarp: no such npc: %s\n"_fmt, npc);
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
        x = random_::in(x0, x1);
        y = random_::in(y0, y1);

    npc_enable(npc, 0);
    map_delblock(nd); /* [Freeyorp] */
    nd->bl_x = x;
    nd->bl_y = y;
    map_addblock(nd);
    npc_enable(npc, 1);

}

/*==========================================
 * message [MouseJstr]
 *------------------------------------------
 */

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

/*==========================================
 * npctalk (sends message to surrounding
 * area) [Valaris]
 *------------------------------------------
 */

static
void builtin_npctalk(ScriptState *st)
{
    dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
    RString str = conv_str(st, &AARG(0));

    if (nd)
    {
        clif_message(nd, str);
    }
}

/*==========================================
  * getlook char info. getlook(arg)
  *------------------------------------------
  */
static
void builtin_getlook(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    LOOK type = LOOK(conv_num(st, &AARG(0)));
    int val = -1;
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

/*==========================================
  *     get char save point. argument: 0- map name, 1- x, 2- y
  *------------------------------------------
*/
static
void builtin_getsavepoint(ScriptState *st)
{
    int x, y, type;
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);

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
 *     areatimer
 *------------------------------------------
 */
static
void builtin_areatimer_sub(dumb_ptr<block_list> bl, interval_t tick, NpcEvent event)
{
    pc_addeventtimer(bl->is_player(), tick, event);
}

static
void builtin_areatimer(ScriptState *st)
{
    int x0, y0, x1, y1;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    x0 = conv_num(st, &AARG(1));
    y0 = conv_num(st, &AARG(2));
    x1 = conv_num(st, &AARG(3));
    y1 = conv_num(st, &AARG(4));
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARG(5)));
    ZString event_ = ZString(conv_str(st, &AARG(6)));
    NpcEvent event;
    extract(event_, &event);

    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname), return);

    map_foreachinarea(std::bind(builtin_areatimer_sub, ph::_1, tick, event),
            m,
            x0, y0,
            x1, y1,
            BL::PC);
}

/*==========================================
 * Check whether the PC is in the specified rectangle
 *------------------------------------------
 */
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

    if (!sd)
        return;

    push_int<ScriptDataInt>(st->stack,
              (sd->bl_x >= x1 && sd->bl_x <= x2)
              && (sd->bl_y >= y1 && sd->bl_y <= y2)
              && (str == sd->bl_m->name_));
}

/*==========================================
 * Check whether the coords are collision
 *------------------------------------------
 */
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

// Trigger the shop on a (hopefully) nearby shop NPC
static
void builtin_shop(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    dumb_ptr<npc_data> nd;

    if (!sd)
        return;

    NpcName name = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
    nd = npc_name2id(name);
    if (!nd)
    {
        PRINTF("builtin_shop: no such npc: %s\n"_fmt, name);
        return;
    }

    builtin_close(st);
    clif_npcbuysell(sd, nd->bl_id);
}

/*==========================================
 * Check whether the PC is dead
 *------------------------------------------
 */
static
void builtin_isdead(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    push_int<ScriptDataInt>(st->stack, pc_isdead(sd));
}

/*========================================
 * Changes a NPC name, and sprite
 *----------------------------------------
 */
static
void builtin_fakenpcname(ScriptState *st)
{
    NpcName name = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
    NpcName newname = stringish<NpcName>(ZString(conv_str(st, &AARG(1))));
    Species newsprite = wrap<Species>(static_cast<uint16_t>(conv_num(st, &AARG(2))));
    dumb_ptr<npc_data> nd = npc_name2id(name);
    if (!nd)
    {
        PRINTF("builtin_fakenpcname: no such npc: %s\n"_fmt, name);
        return;
    }
    nd->name = newname;
    nd->npc_class = newsprite;

    // Refresh this npc
    npc_enable(name, 0);
    npc_enable(name, 1);

}

/*============================
 * Gets the PC's x pos
 *----------------------------
 */
static
void builtin_getx(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    push_int<ScriptDataInt>(st->stack, sd->bl_x);
}

/*============================
 * Gets the PC's y pos
 *----------------------------
 */
static
void builtin_gety(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    push_int<ScriptDataInt>(st->stack, sd->bl_y);
}

/*
 * Get the PC's current map's name
 */
static
void builtin_getmap(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    push_str<ScriptDataStr>(st->stack, sd->bl_m->name_);
}

static
void builtin_mapexit(ScriptState *)
{
    runflag = 0;
}


#define BUILTIN(func, args, ret)    \
{builtin_##func, #func ## _s, args, ret}

BuiltinFunction builtin_functions[] =
{
    BUILTIN(mes, "s"_s, '\0'),
    BUILTIN(goto, "L"_s, '\0'),
    BUILTIN(callfunc, "F"_s, '\0'),
    BUILTIN(callsub, "L"_s, '\0'),
    BUILTIN(return, ""_s, '\0'),
    BUILTIN(next, ""_s, '\0'),
    BUILTIN(close, ""_s, '\0'),
    BUILTIN(close2, ""_s, '\0'),
    BUILTIN(menu, "sL**"_s, '\0'),
    BUILTIN(rand, "i?"_s, 'i'),
    BUILTIN(isat, "Mxy"_s, 'i'),
    BUILTIN(warp, "Mxy"_s, '\0'),
    BUILTIN(areawarp, "MxyxyMxy"_s, '\0'),
    BUILTIN(heal, "ii"_s, '\0'),
    BUILTIN(itemheal, "ii"_s, '\0'),
    BUILTIN(percentheal, "ii"_s, '\0'),
    BUILTIN(input, "N"_s, '\0'),
    BUILTIN(if, "iF*"_s, '\0'),
    BUILTIN(set, "Ne"_s, '\0'),
    BUILTIN(setarray, "Ne*"_s, '\0'),
    BUILTIN(cleararray, "Nei"_s, '\0'),
    BUILTIN(getarraysize, "N"_s, 'i'),
    BUILTIN(getelementofarray, "Ni"_s, '.'),
    BUILTIN(setlook, "ii"_s, '\0'),
    BUILTIN(countitem, "I"_s, 'i'),
    BUILTIN(checkweight, "Ii"_s, 'i'),
    BUILTIN(getitem, "Ii??"_s, '\0'),
    BUILTIN(makeitem, "IiMxy"_s, '\0'),
    BUILTIN(delitem, "Ii"_s, '\0'),
    BUILTIN(readparam, "i?"_s, 'i'),
    BUILTIN(getcharid, "i?"_s, 'i'),
    BUILTIN(strcharinfo, "i"_s, 's'),
    BUILTIN(getequipid, "i"_s, 'i'),
    BUILTIN(getequipname, "i"_s, 's'),
    BUILTIN(statusup2, "ii"_s, '\0'),
    BUILTIN(bonus, "ii"_s, '\0'),
    BUILTIN(bonus2, "iii"_s, '\0'),
    BUILTIN(skill, "ii?"_s, '\0'),
    BUILTIN(setskill, "ii"_s, '\0'),
    BUILTIN(getskilllv, "i"_s, 'i'),
    BUILTIN(getgmlevel, ""_s, 'i'),
    BUILTIN(end, ""_s, '\0'),
    BUILTIN(getopt2, ""_s, 'i'),
    BUILTIN(setopt2, "i"_s, '\0'),
    BUILTIN(savepoint, "Mxy"_s, '\0'),
    BUILTIN(gettimetick, "i"_s, 'i'),
    BUILTIN(gettime, "i"_s, 'i'),
    BUILTIN(openstorage, ""_s, '\0'),
    BUILTIN(getexp, "ii"_s, '\0'),
    BUILTIN(monster, "Mxysmi?"_s, '\0'),
    BUILTIN(areamonster, "Mxyxysmi?"_s, '\0'),
    BUILTIN(killmonster, "ME"_s, '\0'),
    BUILTIN(killmonsterall, "M"_s, '\0'),
    BUILTIN(donpcevent, "E"_s, '\0'),
    BUILTIN(addtimer, "tE"_s, '\0'),
    BUILTIN(initnpctimer, ""_s, '\0'),
    BUILTIN(startnpctimer, "?"_s, '\0'),
    BUILTIN(stopnpctimer, ""_s, '\0'),
    BUILTIN(getnpctimer, "i"_s, 'i'),
    BUILTIN(setnpctimer, "i"_s, '\0'),
    BUILTIN(announce, "si"_s, '\0'),
    BUILTIN(mapannounce, "Msi"_s, '\0'),
    BUILTIN(getusers, "i"_s, 'i'),
    BUILTIN(getmapusers, "M"_s, 'i'),
    BUILTIN(getareausers, "Mxyxy?"_s, 'i'),
    BUILTIN(getareadropitem, "Mxyxyi?"_s, 'i'),
    BUILTIN(enablenpc, "s"_s, '\0'),
    BUILTIN(disablenpc, "s"_s, '\0'),
    BUILTIN(sc_start, "iTi?"_s, '\0'),
    BUILTIN(sc_end, "i"_s, '\0'),
    BUILTIN(sc_check, "i"_s, 'i'),
    BUILTIN(debugmes, "s"_s, '\0'),
    BUILTIN(resetstatus, ""_s, '\0'),
    BUILTIN(changesex, ""_s, '\0'),
    BUILTIN(attachrid, "i"_s, 'i'),
    BUILTIN(detachrid, ""_s, '\0'),
    BUILTIN(isloggedin, "i"_s, 'i'),
    BUILTIN(setmapflag, "Mi"_s, '\0'),
    BUILTIN(removemapflag, "Mi"_s, '\0'),
    BUILTIN(getmapflag, "Mi"_s, 'i'),
    BUILTIN(pvpon, "M"_s, '\0'),
    BUILTIN(pvpoff, "M"_s, '\0'),
    BUILTIN(setpvpchannel, "i"_s, '\0'),
    BUILTIN(getpvpflag, "i"_s, 'i'),
    BUILTIN(emotion, "i"_s, '\0'),
    BUILTIN(mapwarp, "MMxy"_s, '\0'),
    BUILTIN(cmdothernpc, "ss"_s, '\0'),
    BUILTIN(mobcount, "ME"_s, 'i'),
    BUILTIN(marriage, "P"_s, 'i'),
    BUILTIN(divorce, ""_s, 'i'),
    BUILTIN(getitemname, "I"_s, 's'),
    BUILTIN(getspellinvocation, "s"_s, 's'),
    BUILTIN(getpartnerid2, ""_s, 'i'),
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
    BUILTIN(gmcommand, "s"_s, '\0'),
    BUILTIN(npcwarp, "xys"_s, '\0'),
    BUILTIN(npcareawarp, "xyxyis"_s, '\0'),
    BUILTIN(message, "Ps"_s, '\0'),
    BUILTIN(npctalk, "s"_s, '\0'),
    BUILTIN(getlook, "i"_s, 'i'),
    BUILTIN(getsavepoint, "i"_s, '.'),
    BUILTIN(areatimer, "MxyxytE"_s, '\0'),
    BUILTIN(isin, "Mxyxy"_s, 'i'),
    BUILTIN(iscollision, "Mxy"_s, 'i'),
    BUILTIN(shop, "s"_s, '\0'),
    BUILTIN(isdead, ""_s, 'i'),
    BUILTIN(fakenpcname, "ssi"_s, '\0'),
    BUILTIN(getx, ""_s, 'i'),
    BUILTIN(gety, ""_s, 'i'),
    BUILTIN(getmap, ""_s, 's'),
    BUILTIN(mapexit, ""_s, '\0'),
    BUILTIN(freeloop, "i"_s, '\0'),
    {nullptr, ""_s, ""_s, '\0'},
};
} // namespace map
} // namespace tmwa
