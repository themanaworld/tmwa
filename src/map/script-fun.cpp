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

enum class MonsterAttitude
{
    HOSTILE     = 0,
    FRIENDLY    = 1,
    SERVANT     = 2,
    FROZEN      = 3,
};
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
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    sd->state.npc_dialog_mes = 1;
    RString mes = HARG(0) ? conv_str(st, &AARG(0)) : ""_s;
    clif_scriptmes(sd, st->oid, mes);
}

static
void builtin_clear(ScriptState *st)
{
    clif_npc_action(script_rid2sd(st), st->oid, 9, 0, 0, 0);
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
        PRINTF("fatal: script: goto: not label !\n"_fmt);
        st->state = ScriptEndState::END;
        abort();
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
            PRINTF("fatal: script: callfunc: function not found! [%s]\n"_fmt, str);
            st->state = ScriptEndState::END;
            abort();
        }
    }
    OMATCH_END ();
}

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

                push_int<ScriptDataInt>(st->stack, j); // 引数の数をプッシュ
                push_int<ScriptDataInt>(st->stack, st->defsp); // 現在の基準スタックポインタをプッシュ
                push_int<ScriptDataInt>(st->stack, st->scriptp.pos);   // 現在のスクリプト位置をプッシュ
                push_script<ScriptDataRetInfo>(st->stack, TRY_UNWRAP(st->scriptp.code, abort()));  // 現在のスクリプトをプッシュ

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

        push_int<ScriptDataInt>(st->stack, j); // 引数の数をプッシュ
        push_int<ScriptDataInt>(st->stack, st->defsp); // 現在の基準スタックポインタをプッシュ
        push_int<ScriptDataInt>(st->stack, st->scriptp.pos);   // 現在のスクリプト位置をプッシュ
        push_script<ScriptDataRetInfo>(st->stack, TRY_UNWRAP(st->scriptp.code, abort()));  // 現在のスクリプトをプッシュ

        st->scriptp.pos = pos_;
        st->defsp = st->start + 4 + j;
        st->state = ScriptEndState::GOTO;
    }

}

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

static
void builtin_void(ScriptState *)
{
    return;
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
    if (!(st->stack->stack_datav[st->defsp - 1].is<ScriptDataRetInfo>()))
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
        if(nd)
            PRINTF("Deprecated: return outside of callfunc or callsub! @ %s\n"_fmt, nd->name);
        else
            PRINTF("Deprecated: return outside of callfunc or callsub! (no npc)\n"_fmt);
    }

    if (HARG(0))
    {                           // 戻り値有り
        push_copy(st->stack, st->start + 2);
    }

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
    if (st->stack->stack_datav[st->defsp - 1].is<ScriptDataRetInfo>())
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
        if(nd)
            PRINTF("Deprecated: close in a callfunc or callsub! @ %s\n"_fmt, nd->name);
        else
            PRINTF("Deprecated: close in a callfunc or callsub! (no npc)\n"_fmt);
    }
    st->state = ScriptEndState::END;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    if (sd->state.npc_dialog_mes)
        clif_scriptclose(sd, st->oid);
    else
        clif_npc_action(sd, st->oid, 5, 0, 0, 0);
}

static
void builtin_close2(ScriptState *st)
{
    st->state = ScriptEndState::STOP;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    if (sd->state.npc_dialog_mes)
        clif_scriptclose(sd, st->oid);
    else
        clif_npc_action(sd, st->oid, 5, 0, 0, 0);
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
                PRINTF("fatal: script:menu: not a label\n"_fmt);
                st->state = ScriptEndState::END;
                abort();
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
            PRINTF("builtin_max: illegal scope!\n"_fmt);
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

static
void builtin_average(ScriptState *st)
{
    int total, i;
    total = conv_num(st, &AARG(0));

    for (i = 1; HARG(i); i++)
        total += conv_num(st, &AARG(i));

    push_int<ScriptDataInt>(st->stack, (total / i));
}

static
void builtin_sqrt(ScriptState *st)
{
    push_int<ScriptDataInt>(st->stack, static_cast<int>(sqrt(conv_num(st, &AARG(0)))));
}

static
void builtin_cbrt(ScriptState *st)
{
    push_int<ScriptDataInt>(st->stack, static_cast<int>(cbrt(conv_num(st, &AARG(0)))));
}

static
void builtin_pow(ScriptState *st)
{
    push_int<ScriptDataInt>(st->stack, static_cast<int>(pow(conv_num(st, &AARG(0)), conv_num(st, &AARG(1)))));
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
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    hp = conv_num(st, &AARG(0));
    sp = conv_num(st, &AARG(1));

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

/*==========================================
 *
 *------------------------------------------
 */
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

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_target(ScriptState *st)
{
    // TODO maybe scrap all this and make it use battle_ functions? (add missing functions to battle)

    dumb_ptr<block_list> source = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(0))));
    dumb_ptr<block_list> target = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))));
    int flag = conv_num(st, &AARG(2));
    int val = 0;

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

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_injure(ScriptState *st)
{
    dumb_ptr<block_list> source = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(0))));
    dumb_ptr<block_list> target = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))));
    int damage_caused = conv_num(st, &AARG(2));

    // display damage first, because dealing damage may deallocate the target.
    clif_damage(source, target,
            gettick(), interval_t::zero(), interval_t::zero(),
            damage_caused, 0, DamageType::NORMAL);

    battle_damage(source, target, damage_caused, 0);

    return;
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

static
void builtin_requestlang(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    script_data& scrd = AARG(0);
    assert (scrd.is<ScriptDataVariable>());
    SIR reg = scrd.get_if<ScriptDataVariable>()->reg;
    ZString name = variable_names.outtern(reg.base());
    char postfix = name.back();

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

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_if (ScriptState *st)
{
    int sel, i;

    sel = conv_num(st, &AARG(0));
    st->is_true = sel ? 2 : 1;
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

static
void builtin_if_then_else (ScriptState *st)
{
    int condition = conv_num(st, &AARG(0));
    push_copy(st->stack, st->start + (condition ? 3 : 4));
}

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
    // 関数名をコピー
    push_copy(st->stack, st->start + 2);
    // 間に引数マーカを入れて
    push_int<ScriptDataArg>(st->stack, 0);
    // 残りの引数をコピー
    for (i = st->start + 3; i < st->end; i++)
    {
        push_copy(st->stack, i);
    }
    run_func(st);
}

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
static
void builtin_foreach(ScriptState *st)
{
    int x0, y0, x1, y1, bl_num;

    dumb_ptr<block_list> caster = map_id2bl(st->rid);
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

    map_foreachinarea(std::bind(builtin_foreach_sub, ph::_1, event, caster->bl_id),
            m,
            x0, y0,
            x1, y1,
            block_type);
}
/*========================================
 * Destructs a temp NPC
 *----------------------------------------
 */
static
void builtin_destroy(ScriptState *st)
{
    BlockId id;
    if (HARG(0))
        id = wrap<BlockId>(conv_num(st, &AARG(0)));
    else
        id = st->oid;

    dumb_ptr<npc_data_script> nd = map_id2bl(id)->is_npc()->is_script();
    if(!nd)
        return;
    //assert(nd->disposable == true); we don't care about it anymore
    npc_free(nd);
    if (!HARG(0))
        st->state = ScriptEndState::END;
}
/*========================================
 * Creates a temp NPC
 *----------------------------------------
 */

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

/*==========================================
 * 変数設定
 *------------------------------------------
 */
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
            bl = script_rid2sd(st)->is_player();

        if (bl == nullptr)
            return;
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
                bl = map_id2bl(st->oid)->is_npc();
            }
            else
                bl = map_id2bl(st->rid)->is_player();
        }
        if (bl == nullptr)
            return;
    }

    if (postfix == '$')
    {
        // 文字列
        RString str = conv_str(st, &AARG(1));
        set_reg(bl, VariableCode::VARIABLE, reg, str);
    }
    else
    {
        // 数値
        int val = conv_num(st, &AARG(1));
        set_reg(bl, VariableCode::VARIABLE, reg, val);
    }

}

// this is a special function that returns array index for a variable stored in another being
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

/*==========================================
 * 配列変数設定
 *------------------------------------------
 */
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
                bl = map_id2bl(st->oid)->is_npc();
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
                bl = map_id2bl(st->oid)->is_npc();
            else
               bl = map_id2bl(wrap<BlockId>(tid))->is_npc();
        }
        if (!bl)
        {
            PRINTF("builtin_setarray: npc not found\n"_fmt);
            return;
        }
        if (st->oid && bl->bl_id != st->oid)
            j = getarraysize2(reg, bl);
    }
    else if (prefix != '$' && !name.startswith(".@"_s))
        bl = map_id2bl(st->rid)->is_player();

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

/*==========================================
 * 配列変数クリア
 *------------------------------------------
 */
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
        bl = map_id2bl(st->oid)->is_npc();
    else if (prefix != '$' && !name.startswith(".@"_s))
        bl = map_id2bl(st->rid)->is_player();

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

    if (prefix != '$' && prefix != '@' && prefix != '.')
    {
        PRINTF("builtin_copyarray: illegal scope!\n"_fmt);
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

static
void builtin_wgm(ScriptState *st)
{
    ZString message = ZString(conv_str(st, &AARG(0)));

    intif_wis_message_to_gm(WISP_SERVER_NAME,
            battle_config.hack_info_GM_level,
            STRPRINTF("[GM] %s"_fmt, message));
}

static
void builtin_gmlog(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    ZString message = ZString(conv_str(st, &AARG(0)));
    log_atcommand(sd, STRPRINTF("{SCRIPT} %s"_fmt, message));
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
            PRINTF("wrong item ID: countitem (%i)\n"_fmt, nameid);
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

static
void builtin_getversion(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);;
    push_int<ScriptDataInt>(st->stack, unwrap<ClientVersion>(sd->client_version));
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
 *
 *------------------------------------------
 */
static
void builtin_getnpcid(ScriptState *st)
{
    dumb_ptr<npc_data> nd;

    if (HARG(0))
        nd = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(0)))));
    else
        nd = map_id2bl(st->oid)->is_npc();
    if (nd == nullptr)
    {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    }

    push_int<ScriptDataInt>(st->stack, unwrap<BlockId>(nd->bl_id));
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

    if (HARG(1))    //指定したキャラを状態異常にする
        sd = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))))->is_player();
    else
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

    if (HARG(1))
        sd = map_nick2sd(stringish<CharName>(ZString(conv_str(st, &AARG(1)))));
    else
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
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
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
    pc_bonus(sd, type, val);

}

/*==========================================
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
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
void builtin_overrideattack(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int charges = conv_num(st, &AARG(0));
    interval_t attack_delay = static_cast<interval_t>(conv_num(st, &AARG(1)));
    int attack_range = conv_num(st, &AARG(2));
    StatusChange icon = StatusChange(conv_num(st, &AARG(3)));
    ItemNameId look = wrap<ItemNameId>(static_cast<uint16_t>(conv_num(st, &AARG(4))));
    ZString event_ = ZString(conv_str(st, &AARG(5)));

    NpcEvent event;
    extract(event_, &event);

    sd->attack_spell_override = st->oid;
    sd->attack_spell_charges = charges;
    sd->magic_attack = event;
    pc_set_weapon_icon(sd, charges, icon, look);
    pc_set_attack_info(sd, attack_delay, attack_range);
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
    if (st->stack->stack_datav[st->defsp - 1].is<ScriptDataRetInfo>())
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
        if(nd)
            PRINTF("Deprecated: close in a callfunc or callsub! @ %s\n"_fmt, nd->name);
        else
            PRINTF("Deprecated: close in a callfunc or callsub! (no npc)\n"_fmt);
    }
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

static
void builtin_summon(ScriptState *st)
{
    NpcEvent event;
    MapName map = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    int x = conv_num(st, &AARG(1));
    int y = conv_num(st, &AARG(2));
    dumb_ptr<block_list> owner_e = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(3))));
    dumb_ptr<map_session_data> owner = nullptr;
    Species monster_id = wrap<Species>(conv_num(st, &AARG(4)));
    MonsterAttitude monster_attitude = static_cast<MonsterAttitude>(conv_num(st, &AARG(5)));
    interval_t lifespan = static_cast<interval_t>(conv_num(st, &AARG(6)));
    if (HARG(7))
        extract(ZString(conv_str(st, &AARG(7))), &event);

    if (monster_attitude == MonsterAttitude::SERVANT
        && owner_e->bl_type == BL::PC)
        owner = owner_e->is_player(); // XXX in the future this should also work with mobs as owner

    BlockId mob_id = mob_once_spawn(owner, map, x, y, MobName(), monster_id, 1, event);
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
            MobMode::SUMMONED | MobMode::TURNS_AGAINST_BAD_MASTER;

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
 * NPCイベントタイマー追加
 *------------------------------------------
 */
static
void builtin_addnpctimer(ScriptState *st)
{
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARG(0)));
    ZString event_ = ZString(conv_str(st, &AARG(1)));
    NpcEvent event;
    extract(event_, &event);
    npc_addeventtimer(npc_name2id(event.npc), tick, event);
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

static
void builtin_npcaction(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    short command = conv_num(st, &AARG(0));
    int id = 0;
    short x = HARG(2) ? conv_num(st, &AARG(2)) : 0;
    short y = HARG(3) ? conv_num(st, &AARG(3)) : 0;

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

static
void builtin_camera(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    if (HARG(0))
    {
        if (HARG(1) && !HARG(2))
            clif_npc_action(sd, st->oid, 2, 0, conv_num(st, &AARG(0)), conv_num(st, &AARG(1))); // camera to x, y
        else
        {
            dumb_ptr<block_list> bl;
            short x = 0, y = 0;
            bool rel = false;
            if (auto *u = AARG(0).get_if<ScriptDataInt>())
                bl = map_id2bl(wrap<BlockId>(u->numi));
            if (auto *g = AARG(0).get_if<ScriptDataStr>())
            {
                if (g->str == "rid"_s || g->str == "player"_s)
                    bl = sd;
                if (g->str == "relative"_s)
                    rel = true;
                else if (g->str == "oid"_s || g->str == "npc"_s)
                    bl = map_id2bl(st->oid);
                else
                    bl = npc_name2id(stringish<NpcName>(g->str));
            }
            if (HARG(1) && HARG(2))
            {
                x = conv_num(st, &AARG(1));
                y = conv_num(st, &AARG(2));
            }
            if (rel)
                clif_npc_action(sd, st->oid, 4, 0, x, y); // camera relative from current camera
            else
                clif_npc_action(sd, st->oid, 2, unwrap<BlockId>(bl->bl_id), x, y); // camera to actor
        }
    }

    else
        clif_npc_action(sd, st->oid, 3, 0, 0, 0); // return camera
}

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
        clif_sitnpc_towards(sd, nd_, action);
        clif_setnpcdirection_towards(sd, nd_, dir);
    }
    else
    {
        clif_sitnpc(nd_, action);
        clif_setnpcdirection(nd_, dir);
    }
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
 void builtin_getareausers_sub(dumb_ptr<block_list> bl, int *users)
 {
     if (bool(bl->is_player()->status.option & Opt0::HIDE))
         return;
     (*users)++;
 }

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

static
void builtin_aggravate_sub(dumb_ptr<block_list> bl, dumb_ptr<block_list> target, int effect)
{
    dumb_ptr<mob_data> md = bl->is_mob();

    if (mob_aggravate(md, target))
        clif_misceffect(bl, effect);
}

static
void builtin_aggravate(ScriptState *st)
{
    dumb_ptr<block_list> target = map_id2bl(st->rid);
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARG(0))));
    int x0 = conv_num(st, &AARG(1));
    int y0 = conv_num(st, &AARG(2));
    int x1 = conv_num(st, &AARG(3));
    int y1 = conv_num(st, &AARG(4));
    int effect = conv_num(st, &AARG(5));
    P<map_local> m = TRY_UNWRAP(map_mapname2mapid(str),
    {
        push_int<ScriptDataInt>(st->stack, -1);
        return;
    });

    map_foreachinarea(std::bind(builtin_aggravate_sub, ph::_1, target, effect),
            m,
            x0, y0,
            x1, y1,
            BL::MOB);
}

/*==========================================
 * エリア指定ユーザー数所得
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
    if (HARG(1))    //指定したキャラを状態異常にする
        bl = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))));
    else
        bl = map_id2bl(st->rid);

    skill_status_change_end(bl, type, nullptr);
}

static
void builtin_sc_check(ScriptState *st)
{
    dumb_ptr<block_list> bl;
    StatusChange type = StatusChange(conv_num(st, &AARG(0)));
    if (HARG(1))    //指定したキャラを状態異常にする
        bl = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))));
    else
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
    PRINTF("script debug: %d %d: '%s'\n"_fmt,
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
 * RIDのアタッチ
 *------------------------------------------
 */
static
void builtin_attachrid(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = map_id2sd(st->rid);
    BlockId newid = wrap<BlockId>(conv_num(st, &AARG(0)));

    if (sd && newid != st->rid)
        sd->npc_id = BlockId();

    st->rid = newid;
    push_int<ScriptDataInt>(st->stack, (map_id2sd(st->rid) != nullptr));
}


/*==========================================
 * RIDのデタッチ
 *------------------------------------------
 */
static
void builtin_detachrid(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = map_id2sd(st->rid);
    if (sd)
        sd->npc_id = BlockId();
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
    dumb_ptr<map_session_data> sd;
    if (HARG(1))    //指定したキャラを状態異常にする
        sd = map_id2bl(wrap<BlockId>(conv_num(st, &AARG(1))))->is_player();
    else
        sd = script_rid2sd(st);

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

static
void builtin_getitemlink(ScriptState *st)
{
    struct script_data *data;
    AString buf;
    data = &AARG(0);
    ZString name = conv_str(st, data);

    ItemNameId item_id;
    Option<P<struct item_data>> item_data_ = itemdb_searchname(name);
    OMATCH_BEGIN (item_data_)
    {
        OMATCH_CASE_SOME (item_data)
        {
            buf = STRPRINTF("@@%d|@@"_fmt, item_data->nameid);
        }
        OMATCH_CASE_NONE ()
        {
            buf = "Unknown Item"_s;
        }
    }
    OMATCH_END ();
    push_str<ScriptDataStr>(st->stack, buf);
}

static
void builtin_getpartnerid2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    push_int<ScriptDataInt>(st->stack, unwrap<CharId>(sd->status.partner_id));
}

static
void builtin_chr(ScriptState *st)
{
    const char ascii = conv_num(st, &AARG(0));
    push_str<ScriptDataStr>(st->stack, VString<1>(ascii));
}

static
void builtin_ord(ScriptState *st)
{
    const char ascii = conv_str(st, &AARG(0)).front();
    push_int<ScriptDataInt>(st->stack, static_cast<int>(ascii));
}

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
        bl = map_id2bl(st->rid)->is_player();

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
                    return;
            }
            else
                return;
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
        int var;
        if (prefix == '#' && bl)
        {
            if (name_.startswith("##"_s))
                var = pc_readaccountreg2(bl->is_player(), stringish<VarName>(name_));
            else
                var = pc_readaccountreg(bl->is_player(), stringish<VarName>(name_));
        }
        else
            var = pc_readreg(bl, reg);

        push_int<ScriptDataInt>(st->stack, var);
    }
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
        PRINTF("builtin_npcwarp: no such npc: '%s'\n"_fmt, npc);
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

static
void builtin_title(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    ZString msg = ZString(conv_str(st, &AARG(0)));
    if (sd == nullptr)
        return;
    clif_npc_send_title(sd->sess, st->oid, msg);
}

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
    if (sd == nullptr)
        return;
    if (type < 0 || type > 0xFF)
        type = 0;

    clif_server_message(sd, type, msg);
}

static
void builtin_remotecmd(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
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

static
void builtin_music(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    ZString msg = ZString(conv_str(st, &AARG(0)));
    if (sd == nullptr)
        return;
    clif_change_music(sd, msg);
}

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

    if (sd == nullptr)
        return;
    clif_send_mask(sd, map_mask);
}

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

/*==========================================
 * npctalk (sends message to surrounding
 * area) [Valaris]
 *------------------------------------------
 */

static
void builtin_npctalk(ScriptState *st)
{
    dumb_ptr<npc_data> nd;
    RString str = conv_str(st, &AARG(1));

    dumb_ptr<npc_data> nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARG(0)))));
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    nd = nd_->is_script();


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

/*==========================================
  * register cmd
  *------------------------------------------
  */
static
void builtin_registercmd(ScriptState *st)
{
    RString evoke = conv_str(st, &AARG(0));
    ZString event_ = conv_str(st, &AARG(1));
    NpcEvent event;
    extract(event_, &event);
    spells_by_events.put(evoke, event);
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
    if (bl->bl_type == BL::PC)
    {
        dumb_ptr<map_session_data> sd = map_id_is_player(bl->bl_id);
        pc_addeventtimer(sd, tick, event);
    }
    else
    {
        npc_addeventtimer(bl, tick, event);
    }
}

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
        case 1:
            block_type = BL::NPC;
            break;
        case 2:
            block_type = BL::MOB;
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
        PRINTF("builtin_shop: no such npc: '%s'\n"_fmt, name);
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
        PRINTF("builtin_fakenpcname: no such npc: '%s'\n"_fmt, name);
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

/*============================
 * Gets the PC's direction
 *----------------------------
 */
static
void builtin_getdir(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    push_int<ScriptDataInt>(st->stack, static_cast<uint8_t>(sd->dir));
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

/*
 * Get the NPC's info
 */
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
            nd = map_id2bl(id)->is_npc();
        }

        if (!nd)
        {
            PRINTF("builtin_strnpcinfo: npc not found\n"_fmt);
            return;
        }
    } else {
        nd = map_id_is_npc(st->oid);
    }

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

/*============================
 * Gets the NPC's x pos
 *----------------------------
 */
static
void builtin_getnpcx(ScriptState *st)
{
    dumb_ptr<npc_data> nd;

    if(HARG(0)){
        NpcName name = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
        nd = npc_name2id(name);
        if (!nd)
        {
            PRINTF("builtin_getnpcx: no such npc: '%s'\n"_fmt, name);
            return;
        }
    } else {
        nd = map_id_is_npc(st->oid);
    }

    push_int<ScriptDataInt>(st->stack, nd->bl_x);
}

/*============================
 * Gets the NPC's y pos
 *----------------------------
 */
static
void builtin_getnpcy(ScriptState *st)
{
    dumb_ptr<npc_data> nd;

    if(HARG(0)){
        NpcName name = stringish<NpcName>(ZString(conv_str(st, &AARG(0))));
        nd = npc_name2id(name);
        if (!nd)
        {
            PRINTF("builtin_getnpcy: no such npc: '%s'\n"_fmt, name);
            return;
        }
    } else {
        nd = map_id_is_npc(st->oid);
    }

    push_int<ScriptDataInt>(st->stack, nd->bl_y);
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
    BUILTIN(mes, "?"_s, '\0'),
    BUILTIN(clear, ""_s, '\0'),
    BUILTIN(goto, "L"_s, '\0'),
    BUILTIN(callfunc, "F"_s, '\0'),
    BUILTIN(call, "F?*"_s, '.'),
    BUILTIN(callsub, "L"_s, '\0'),
    BUILTIN(getarg, "i?"_s, '.'),
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
    BUILTIN(get, "Ne"_s, '.'),
    BUILTIN(setarray, "Ne*"_s, '\0'),
    BUILTIN(cleararray, "Nei"_s, '\0'),
    BUILTIN(getarraysize, "N"_s, 'i'),
    BUILTIN(getelementofarray, "Ni"_s, '.'),
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
    BUILTIN(overrideattack, "iiiiiE"_s, '\0'),
    BUILTIN(getgmlevel, ""_s, 'i'),
    BUILTIN(end, ""_s, '\0'),
    BUILTIN(getopt2, ""_s, 'i'),
    BUILTIN(setopt2, "i"_s, '\0'),
    BUILTIN(savepoint, "Mxy"_s, '\0'),
    BUILTIN(gettimetick, "i"_s, 'i'),
    BUILTIN(gettime, "i"_s, 'i'),
    BUILTIN(openstorage, ""_s, '\0'),
    BUILTIN(getexp, "ii"_s, '\0'),
    BUILTIN(summon, "Mxysmii?"_s, '\0'),
    BUILTIN(monster, "Mxysmi?"_s, '\0'),
    BUILTIN(areamonster, "Mxyxysmi?"_s, '\0'),
    BUILTIN(killmonster, "ME"_s, '\0'),
    BUILTIN(donpcevent, "E"_s, '\0'),
    BUILTIN(addtimer, "tE"_s, '\0'),
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
    BUILTIN(music, "s"_s, '\0'),
    BUILTIN(mapmask, "i?"_s, '\0'),
    BUILTIN(getmask, ""_s, 'i'),
    BUILTIN(getlook, "i"_s, 'i'),
    BUILTIN(getsavepoint, "i"_s, '.'),
    BUILTIN(areatimer, "MxyxytEi"_s, '\0'),
    BUILTIN(foreach, "iMxyxyE"_s, '\0'),
    BUILTIN(isin, "Mxyxy"_s, 'i'),
    BUILTIN(iscollision, "Mxy"_s, 'i'),
    BUILTIN(shop, "s"_s, '\0'),
    BUILTIN(isdead, ""_s, 'i'),
    BUILTIN(aggravate, "Mxyxyi"_s, '\0'),
    BUILTIN(fakenpcname, "ssi"_s, '\0'),
    BUILTIN(puppet, "mxysi??"_s, 'i'),
    BUILTIN(destroy, "?"_s, '\0'),
    BUILTIN(getx, ""_s, 'i'),
    BUILTIN(gety, ""_s, 'i'),
    BUILTIN(getdir, ""_s, 'i'),
    BUILTIN(getnpcx, "?"_s, 'i'),
    BUILTIN(getnpcy, "?"_s, 'i'),
    BUILTIN(strnpcinfo, "i?"_s, 's'),
    BUILTIN(getmap, ""_s, 's'),
    BUILTIN(mapexit, ""_s, '\0'),
    BUILTIN(freeloop, "i"_s, '\0'),
    BUILTIN(if_then_else, "iii"_s, '.'),
    BUILTIN(max, "e?*"_s, 'i'),
    BUILTIN(min, "e?*"_s, 'i'),
    BUILTIN(average, "ii*"_s, 'i'),
    BUILTIN(sqrt, "i"_s, 'i'),
    BUILTIN(cbrt, "i"_s, 'i'),
    BUILTIN(pow, "ii"_s, 'i'),
    BUILTIN(target, "iii"_s, 'i'),
    BUILTIN(distance, "ii?"_s, 'i'),
    BUILTIN(chr, "i"_s, 'i'),
    BUILTIN(ord, "s"_s, 'i'),
    {nullptr, ""_s, ""_s, '\0'},
};
} // namespace map
} // namespace tmwa
