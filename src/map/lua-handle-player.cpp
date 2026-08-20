#include "lua-handles.hpp"
//    lua-handle-player.cpp - the player handle metatable and methods.
//
//    Copyright © 2026 The Mana World Development Team
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

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/vstring.hpp"

#include "../compat/borrow.hpp"
#include "../compat/option.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/timer.t.hpp"

#include "../mmo/clif.t.hpp"
#include "../mmo/enums.hpp"
#include "../mmo/skill.t.hpp"
#include "../mmo/strs.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "skill.hpp"
#include "lua-callback.hpp"
#include "lua-dialog.hpp"
#include "lua-engine.hpp"
#include "lua-internal.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


// Player handles follow the layout documented at the top of
// lua-handle-being.cpp: a table with raw fields "__id" and "__serial", the
// raw sub-tables tmp/tmpstr (typed-default temporaries) and the vars/acc/
// acc2 proxies, all created once per login session and cached in the
// session's handle_ref plus the PLAYERS registry table. Dispatch order for
// reads (doc/lua-engine.md 7.1): methods, convenience properties, the
// data-driven params.txt set; unknown names read nil, unknown writes raise.

namespace tmwa
{
namespace map
{
// defined in lua-handle-being.cpp (internal cross-TU helpers between the
// two handle modules; not part of any public header)
int lua_check_int32_wrap(lua_State* L, int idx);

// The LuaCallback overload of the player event-timer entry, implemented in
// lua-timers.cpp per doc/lua-engine.md section 6. The old NpcEvent overload
// in pc.cpp stays untouched until host integration.
int pc_addeventtimer(dumb_ptr<map_session_data> sd, interval_t tick,
        LuaCallback cb);

static int g_player_mt_ref = lua_noref;
// index/newindex metatables of the p.vars / p.acc / p.acc2 proxies
static int g_scope_mt_ref[3] = {lua_noref, lua_noref, lua_noref};

// ------------------------------------------------------------------------
// helpers

// A player write goes through pc_setparam only for the SPs its switch
// actually handles; everything else is read-only and raises
// (doc/lua-api.md 5.1).
static
bool sp_is_writable(SP sp)
{
    switch (sp)
    {
        case SP::BASELEVEL:
        case SP::JOBLEVEL:
        case SP::CLASS:
        case SP::SKILLPOINT:
        case SP::STATUSPOINT:
        case SP::ZENY:
        case SP::BASEEXP:
        case SP::JOBEXP:
        case SP::SEX:
        case SP::WEIGHT:
        case SP::MAXWEIGHT:
        case SP::MAXWEIGHT_OVERRIDE:
        case SP::HP:
        case SP::MAXHP:
        case SP::SP:
        case SP::MAXSP:
        case SP::STR:
        case SP::AGI:
        case SP::VIT:
        case SP::INT:
        case SP::DEX:
        case SP::LUK:
        case SP::PARTNER:
        case SP::INVISIBLE:
        case SP::GM:
        case SP::HIDDEN:
        case SP::MUTE_GLOBAL:
        case SP::MUTE_PARTY:
        case SP::MUTE_WHISPER:
        case SP::MUTE_GUILD:
        case SP::KILLS:
        case SP::CASTS:
        case SP::ITEMS_USED:
        case SP::TILES_WALKED:
        case SP::ATTACKS:
        case SP::AUTOMOD:
            return true;
        default:
            return false;
    }
}

// resolve the player behind a handle or proxy table (nullptr when stale)
static
dumb_ptr<map_session_data> table_player(lua_State* L, int idx)
{
    BlockId id = lua_handle_id(L, idx);
    if (!id)
        return nullptr;
    return map_id_is_player(id);
}

// display text: a string or an integer (formatted %d), per doc/lua-api.md
// 1.3. Raises for other types; the AString is built by the caller's worker
// scope via text_to_string AFTER all checks passed.
static
void check_text(lua_State* L, int idx)
{
    if (lua_type(L, idx) != LUA_TSTRING && !luac::is_integer_value(L, idx))
        luaL_argerror(L, idx, "string expected");
}

static
AString text_to_string(lua_State* L, int idx)
{
    if (lua_type(L, idx) == LUA_TSTRING)
    {
        size_t len;
        return AString(XString(luac::to_string(L, idx, &len)));
    }
    return STRPRINTF("%d"_fmt, static_cast<int>(lua_tointeger(L, idx)));
}

// permanent-scope variable name: [A-Za-z0-9_]{1,31}, with the '#'/'##'
// prefix added here so wire and disk formats stay byte-identical
// (doc/lua-engine.md 7.1). scope: 0 = char, 1 = account, 2 = account2.
static
VarName check_reg_name(lua_State* L, int idx, int scope)
{
    if (lua_type(L, idx) != LUA_TSTRING)
        luaL_argerror(L, idx, "variable name must be a string");
    size_t len;
    ZString name = luac::to_string(L, idx, &len);
    if (len == 0)
        luaL_argerror(L, idx, "empty variable name");
    if (name[len - 1] == '$')
        luaL_argerror(L, idx,
                "permanent variables are int only (use p.tmpstr for strings)");
    for (char c : name)
    {
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                    || (c >= '0' && c <= '9') || c == '_'))
            luaL_argerror(L, idx,
                    "variable name may only use [A-Za-z0-9_]");
    }
    // the stored name (including the prefix) must fit VarName's 31 chars,
    // exactly the limit the old '#'/'##' spellings had
    size_t max = scope == 0 ? 31 : scope == 1 ? 30 : 29;
    if (len > max)
        luaL_argerror(L, idx, "variable name too long");
    if (scope == 1)
        return stringish<VarName>(STRPRINTF("#%s"_fmt, name));
    if (scope == 2)
        return stringish<VarName>(STRPRINTF("##%s"_fmt, name));
    return stringish<VarName>(name);
}

// ------------------------------------------------------------------------
// p.vars / p.acc / p.acc2 proxies (upvalue 1: the scope tag)

static
int vars_names(lua_State* L)
{
    int scope = static_cast<int>(lua_tointeger(L, lua_upvalueindex(1)));
    dumb_ptr<map_session_data> sd = table_player(L, 1);
    if (sd == nullptr)
    {
        lua_warn("stale player handle (vars names)"_s);
        return 0;
    }
    lua_newtable(L);
    int n = 0;
    if (scope == 0)
    {
        for (int i = 0; i < sd->status.global_reg_num; ++i)
        {
            push_string(L, sd->status.global_reg[i].str);
            lua_rawseti(L, -2, ++n);
        }
    }
    else if (scope == 1)
    {
        for (int i = 0; i < sd->status.account_reg_num; ++i)
        {
            XString full = sd->status.account_reg[i].str;
            push_string(L, full.xislice_t(full.begin() + 1));
            lua_rawseti(L, -2, ++n);
        }
    }
    else
    {
        for (int i = 0; i < sd->status.account_reg2_num; ++i)
        {
            XString full = sd->status.account_reg2[i].str;
            push_string(L, full.xislice_t(full.begin() + 2));
            lua_rawseti(L, -2, ++n);
        }
    }
    return 1;
}

static
int vars_index(lua_State* L)
{
    // stack: proxy, key
    int scope = static_cast<int>(lua_tointeger(L, lua_upvalueindex(1)));
    if (lua_type(L, 2) == LUA_TSTRING)
    {
        size_t klen;
        ZString key = luac::to_string(L, 2, &klen);
        if (key == "names"_s)
        {
            // p.vars:names(); note this shadows a variable literally
            // named "names" (none exists in serverdata)
            lua_pushinteger(L, scope);
            luac::push_cclosure(L, vars_names, "vars.names", 1);
            return 1;
        }
    }
    VarName vn = check_reg_name(L, 2, scope);
    dumb_ptr<map_session_data> sd = table_player(L, 1);
    if (sd == nullptr)
    {
        lua_warn("stale player handle (variable read)"_s);
        lua_pushnil(L);
        return 1;
    }
    int v;
    {
        v = scope == 0 ? pc_readglobalreg(sd, vn)
            : scope == 1 ? pc_readaccountreg(sd, vn)
            : pc_readaccountreg2(sd, vn);
    }
    return luac::push_int(L, v);
}

static
int vars_newindex(lua_State* L)
{
    // stack: proxy, key, value
    int scope = static_cast<int>(lua_tointeger(L, lua_upvalueindex(1)));
    VarName vn = check_reg_name(L, 2, scope);
    // 0 or nil deletes; other values are int32 (wrapping write boundary)
    int v = lua_isnil(L, 3) ? 0 : lua_check_int32_wrap(L, 3);
    dumb_ptr<map_session_data> sd = table_player(L, 1);
    if (sd == nullptr)
    {
        lua_warn("stale player handle (variable write ignored)"_s);
        return 0;
    }
    {
        // cap overflow logs inside the pc_ function (as the old engine did)
        // and the write returns normally
        if (scope == 0)
            pc_setglobalreg(sd, vn, v);
        else if (scope == 1)
            pc_setaccountreg(sd, vn, v);
        else
            pc_setaccountreg2(sd, vn, v);
    }
    return 0;
}

// ------------------------------------------------------------------------
// player methods: state, stats, status (doc/lua-api.md 5.4)

static
int lp_p_heal(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int hp = check_int(L, 2);
    int sp = check_int(L, 3);
    bool itemheal = !lua_isnoneornil(L, 4) && lua_toboolean(L, 4);
    if (sd == nullptr)
        return 0;
    {
        if (sd->status.hp < 1 && hp > 0)
        {
            pc_setstand(sd);
            if (battle_config.player_invincible_time > interval_t::zero())
                pc_setinvincibletimer(sd,
                        battle_config.player_invincible_time);
            clif_resurrection(sd, 1);
        }
        if (itemheal && hp > 0)
            pc_itemheal(sd, hp, sp);
        else
            pc_heal(sd, hp, sp);
    }
    return 0;
}

static
int lp_p_getexp(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int base = check_int(L, 2);
    int job = check_int(L, 3);
    if (sd == nullptr)
        return 0;
    {
        pc_gainexp_reason(sd, base, job, PC_GAINEXP_REASON::SCRIPT);
    }
    return 0;
}

static
int lp_p_setlook(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int type = check_int(L, 2);
    int val = check_int(L, 3);
    if (sd == nullptr)
        return 0;
    {
        pc_changelook(sd, static_cast<LOOK>(type), val);
    }
    return 0;
}

static
int lp_p_getlook(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int type = check_int(L, 2);
    if (sd == nullptr)
        return 0;
    int val = -1;
    {
        switch (static_cast<LOOK>(type))
        {
            case LOOK::HAIR:
                val = sd->status.hair;
                break;
            case LOOK::WEAPON:
                val = static_cast<uint16_t>(sd->status.weapon);
                break;
            case LOOK::HEAD_BOTTOM:
                val = unwrap<ItemNameId>(sd->status.head_bottom);
                break;
            case LOOK::HEAD_TOP:
                val = unwrap<ItemNameId>(sd->status.head_top);
                break;
            case LOOK::HEAD_MID:
                val = unwrap<ItemNameId>(sd->status.head_mid);
                break;
            case LOOK::HAIR_COLOR:
                val = sd->status.hair_color;
                break;
            case LOOK::CLOTHES_COLOR:
                val = sd->status.clothes_color;
                break;
            case LOOK::SHIELD:
                val = unwrap<ItemNameId>(sd->status.shield);
                break;
            default:
                break;   // -1, incl. LOOK::SHOES (old behaviour)
        }
    }
    return luac::push_int(L, val);
}

static
int lp_p_savepoint(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    MapName m = check_mapname(L, 2);
    int x = check_int(L, 3);
    int y = check_int(L, 4);
    if (sd == nullptr)
        return 0;
    {
        pc_setsavepoint(sd, m, x, y);
    }
    return 0;
}

static
int lp_p_getsavepoint(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr)
        return 0;
    push_string(L, sd->status.save_point.map_);
    luac::push_int(L, sd->status.save_point.x);
    luac::push_int(L, sd->status.save_point.y);
    return 3;
}

static
int lp_p_resetstatus(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr)
        return 0;
    {
        pc_resetstate(sd);
    }
    return 0;
}

static
int lp_p_bonus(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int type = check_int(L, 2);
    int val = check_int(L, 3);
    if (sd == nullptr)
        return 0;
    if (!sd->lua.in_calcstatus)
        lua_warn("p:bonus outside an equip script (the bonus only lasts until the next status recalc)"_s);
    {
        pc_bonus(sd, static_cast<SP>(type), val);
    }
    return 0;
}

static
int lp_p_bonus2(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int type = check_int(L, 2);
    int type2 = check_int(L, 3);
    int val = check_int(L, 4);
    if (sd == nullptr)
        return 0;
    if (!sd->lua.in_calcstatus)
        lua_warn("p:bonus2 outside an equip script (the bonus only lasts until the next status recalc)"_s);
    {
        pc_bonus2(sd, static_cast<SP>(type), type2, val);
    }
    return 0;
}

static
int lp_p_param(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int sp = check_int(L, 2);
    if (sd == nullptr)
        return 0;
    int v;
    {
        v = pc_readparam(sd, static_cast<SP>(sp));
    }
    return luac::push_int(L, v);
}

static
int lp_p_setparam(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int sp = check_int(L, 2);
    int v = lua_check_int32_wrap(L, 3);
    if (!sp_is_writable(static_cast<SP>(sp)))
        return luaL_error(L, "param %d is read-only", sp);
    if (sd == nullptr)
        return 0;
    {
        pc_setparam(sd, static_cast<SP>(sp), v);
    }
    return 0;
}

static
int lp_p_marriage(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    ZString partner = check_string(L, 2);
    bool ok;
    {
        dumb_ptr<map_session_data> p_sd =
            map_nick2sd(stringish<CharName>(partner));
        ok = sd != nullptr && p_sd != nullptr && pc_marriage(sd, p_sd) >= 0;
    }
    lua_pushboolean(L, ok);
    return 1;
}

static
int lp_p_divorce(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    bool ok;
    {
        ok = sd != nullptr && pc_divorce(sd) >= 0;
    }
    lua_pushboolean(L, ok);
    return 1;
}

static
int lp_p_isat(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    MapName m = check_mapname(L, 2);
    int x = check_int(L, 3);
    int y = check_int(L, 4);
    bool at = sd != nullptr
        && x == sd->bl_x && y == sd->bl_y && m == sd->bl_m->name_;
    lua_pushboolean(L, at);
    return 1;
}

static
int lp_p_isin(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    MapName m = check_mapname(L, 2);
    int x0 = check_int(L, 3);
    int y0 = check_int(L, 4);
    int x1 = check_int(L, 5);
    int y1 = check_int(L, 6);
    bool in = sd != nullptr
        && sd->bl_x >= x0 && sd->bl_x <= x1
        && sd->bl_y >= y0 && sd->bl_y <= y1
        && m == sd->bl_m->name_;
    lua_pushboolean(L, in);
    return 1;
}

static
int lp_p_warp(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    MapName m = check_mapname(L, 2);
    int x = check_int(L, 3);
    int y = check_int(L, 4);
    if (sd == nullptr)
        return 0;
    {
        pc_setpos(sd, m, x, y, BeingRemoveWhy::GONE);
    }
    return 0;
}

static
int lp_p_mapmask(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int mask = check_int(L, 2);
    bool persist = !lua_isnoneornil(L, 3) && lua_toboolean(L, 3);
    if (sd == nullptr)
        return 0;
    {
        if (persist && sd->bl_m != borrow(undefined_gat))
            sd->bl_m->mask = mask;
        clif_send_mask(sd, mask);
    }
    return 0;
}

static
int lp_p_sendcollision(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    MapName m = check_mapname(L, 2);
    int mask = check_int(L, 3);
    int x1 = check_int(L, 4);
    int y1 = check_int(L, 5);
    int x2 = opt_int(L, 6, x1);
    int y2 = opt_int(L, 7, y1);
    if (sd == nullptr)
        return 0;
    {
        clif_update_collision(sd,
                static_cast<short>(x1), static_cast<short>(y1),
                static_cast<short>(x2), static_cast<short>(y2),
                m, mask);
    }
    return 0;
}

static
int lp_p_music(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    ZString name = check_string(L, 2);
    if (sd == nullptr)
        return 0;
    {
        clif_change_music(sd, name);
    }
    return 0;
}

// ------------------------------------------------------------------------
// skills and pools (doc/lua-api.md 5.4, builtins.md 1.6)

static
bool skill_id_ok(int id)
{
    return id >= 0 && id < static_cast<int>(MAX_SKILL);
}

static
int lp_p_skill(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int id = check_int(L, 2);
    int level = check_int(L, 3);
    int flag = opt_int(L, 4, 1);
    if (sd == nullptr)
        return 0;
    if (!skill_id_ok(id))
    {
        lua_warn(STRPRINTF("skill: bad skill id %d"_fmt, id));
        return 0;
    }
    {
        pc_skill(sd, static_cast<SkillID>(id), level, flag);
        clif_skillinfoblock(sd);
    }
    return 0;
}

static
int lp_p_setskill(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int id = check_int(L, 2);
    int level = check_int(L, 3);
    if (sd == nullptr)
        return 0;
    if (!skill_id_ok(id))
    {
        lua_warn(STRPRINTF("setskill: bad skill id %d"_fmt, id));
        return 0;
    }
    {
        if (level > MAX_SKILL_LEVEL)
            level = MAX_SKILL_LEVEL;
        if (level < 0)
            level = 0;
        sd->status.skill[static_cast<SkillID>(id)].lv = level;
        clif_skillinfoblock(sd);
    }
    return 0;
}

static
int lp_p_getskilllv(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int id = check_int(L, 2);
    if (sd == nullptr)
        return 0;
    if (!skill_id_ok(id))
    {
        lua_warn(STRPRINTF("getskilllv: bad skill id %d"_fmt, id));
        return luac::push_int(L, 0);
    }
    int lv;
    {
        lv = pc_checkskill(sd, static_cast<SkillID>(id));
    }
    return luac::push_int(L, lv);
}

// appends {id=, lv=, flag=, name=} as sequence entry n of the table below
// the top of the stack
static
void push_skill_entry(lua_State* L, dumb_ptr<map_session_data> sd,
        SkillID skill_id, int n)
{
    lua_createtable(L, 0, 4);
    luac::push_int(L, static_cast<uint16_t>(skill_id));
    lua_setfield(L, -2, "id");
    luac::push_int(L, sd->status.skill[skill_id].lv);
    lua_setfield(L, -2, "lv");
    luac::push_int(L,
            static_cast<uint16_t>(sd->status.skill[skill_id].flags));
    lua_setfield(L, -2, "flag");
    push_string(L, skill_name(skill_id));
    lua_setfield(L, -2, "name");
    lua_rawseti(L, -2, n);
}

static
int lp_p_getactivatedpoolskilllist(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr)
        return 0;
    SkillID pool_skills[MAX_SKILL_POOL];
    int pool_size;
    {
        pool_size = skill_pool(sd, pool_skills);
    }
    lua_newtable(L);
    int count = 0;
    for (int i = 0; i < pool_size; ++i)
    {
        SkillID skill_id = pool_skills[i];
        if (sd->status.skill[skill_id].lv)
            push_skill_entry(L, sd, skill_id, ++count);
    }
    return 1;
}

static
int lp_p_getunactivatedpoolskilllist(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr)
        return 0;
    lua_newtable(L);
    int count = 0;
    for (size_t i = 0; i < skill_pool_skills.size(); ++i)
    {
        SkillID skill_id = skill_pool_skills[i];
        if (sd->status.skill[skill_id].lv
            && !bool(sd->status.skill[skill_id].flags
                & SkillFlags::POOL_ACTIVATED))
            push_skill_entry(L, sd, skill_id, ++count);
    }
    return 1;
}

static
int lp_p_poolskill(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int id = check_int(L, 2);
    if (sd == nullptr || !skill_id_ok(id))
        return 0;
    {
        skill_pool_activate(sd, static_cast<SkillID>(id));
        clif_skillinfoblock(sd);
    }
    return 0;
}

static
int lp_p_unpoolskill(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int id = check_int(L, 2);
    if (sd == nullptr || !skill_id_ok(id))
        return 0;
    {
        skill_pool_deactivate(sd, static_cast<SkillID>(id));
        clif_skillinfoblock(sd);
    }
    return 0;
}

static
int lp_p_overrideattack(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (lua_gettop(L) <= 1)
    {
        // explicit discharge
        if (sd == nullptr)
            return 0;
        {
            sd->attack_spell_override = BlockId();
            pc_set_weapon_icon(sd, 0, StatusChange::ZERO, ItemNameId());
            pc_set_attack_info(sd, interval_t::zero(), 0);
            pc_calcstatus(sd, static_cast<int>(CalcStatusKind::NORMAL_RECALC));
        }
        return 0;
    }
    int delay = check_int(L, 2);
    int range = check_int(L, 3);
    int icon = check_int(L, 4);
    int look = check_int(L, 5);
    if (lua_type(L, 6) == LUA_TFUNCTION)
        // DESIGN DEVIATION: the design stores a LuaCallback in
        // sd->magic_attack, but that field is still the old NpcEvent until
        // host integration (map.hpp edit list), so the function form cannot
        // be stored yet. The named-event form covers all existing content;
        // this raises loudly instead of silently dropping the handler.
        return luaL_error(L,
                "overrideattack: function handlers need host integration; use a \"Npc::OnX\" event");
    NpcEvent event = check_event(L, 6);
    int charges = opt_int(L, 7, 1);
    if (sd == nullptr)
        return 0;
    {
        sd->attack_spell_override = lua_current_ctx().npc;
        sd->attack_spell_charges = static_cast<short>(charges);
        sd->magic_attack = event;
        pc_set_weapon_icon(sd, 1, static_cast<StatusChange>(icon),
                wrap<ItemNameId>(static_cast<uint16_t>(look)));
        pc_set_attack_info(sd, static_cast<interval_t>(delay), range);
    }
    return 0;
}

// ------------------------------------------------------------------------
// inventory and equipment (doc/lua-api.md 5.5)

static
int lp_p_countitem(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    ItemNameId nameid = check_item(L, 2);
    if (sd == nullptr)
        return 0;
    int count = 0;
    {
        if (nameid)
        {
            for (IOff0 i : IOff0::iter())
            {
                if (sd->status.inventory[i].nameid == nameid)
                    count += sd->status.inventory[i].amount;
            }
        }
    }
    return luac::push_int(L, count);
}

static
int lp_p_checkweight(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    ItemNameId nameid = check_item(L, 2);
    int amount = check_int(L, 3);
    bool ok;
    {
        ok = sd != nullptr && nameid && amount > 0
            && itemdb_weight(nameid) * amount + sd->weight <= sd->max_weight;
    }
    lua_pushboolean(L, ok);
    return 1;
}

static
int lp_p_getitem(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    ItemNameId nameid = check_item(L, 2);
    int amount = check_int(L, 3);
    if (sd == nullptr || !nameid || amount <= 0)
        return 0;
    {
        Item item_tmp {};
        item_tmp.nameid = nameid;
        PickupFail flag = pc_additem(sd, &item_tmp, amount);
        if (flag != PickupFail::OKAY)
        {
            // inventory overflow: drop to the floor, as the old builtin
            clif_additem(sd, IOff0::from(0), 0, flag);
            map_addflooritem(&item_tmp, amount,
                    sd->bl_m, sd->bl_x, sd->bl_y,
                    nullptr, nullptr, nullptr);
        }
    }
    return 0;
}

static
int lp_p_delitem(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    ItemNameId nameid = check_item(L, 2);
    int amount = check_int(L, 3);
    if (sd == nullptr || !nameid || amount <= 0)
        return 0;
    {
        // removes what exists, no error (old behaviour)
        for (IOff0 i : IOff0::iter())
        {
            if (sd->status.inventory[i].nameid != nameid)
                continue;
            int avail = sd->status.inventory[i].amount;
            if (avail >= amount)
            {
                pc_delitem(sd, i, amount, 0);
                break;
            }
            else if (pc_delitem(sd, i, avail, 0) == 0)
                amount -= avail;
        }
    }
    return 0;
}

static
int lp_p_getinventorylist(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr)
        return 0;
    lua_newtable(L);
    int n = 0;
    for (IOff0 i : IOff0::iter())
    {
        if (!sd->status.inventory[i].nameid
            || sd->status.inventory[i].amount <= 0)
            continue;
        lua_createtable(L, 0, 4);
        luac::push_int(L,
                unwrap<ItemNameId>(sd->status.inventory[i].nameid));
        lua_setfield(L, -2, "id");
        luac::push_int(L, sd->status.inventory[i].amount);
        lua_setfield(L, -2, "amount");
        luac::push_int(L,
                static_cast<uint16_t>(sd->status.inventory[i].equip));
        lua_setfield(L, -2, "equip");
        // the raw (0-based) inventory slot, matching server internals
        luac::push_int(L, i.get0());
        lua_setfield(L, -2, "index");
        lua_rawseti(L, -2, ++n);
    }
    return 1;
}

static
int lp_p_getequipid(lua_State* L)
{
    // the same pos -> EPOS table the old builtin indexed (1-based)
    static const EPOS equip_slots[11] =
    {
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
    };
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int pos = check_int(L, 2);
    if (pos < 1 || pos > 11)
        luaL_argerror(L, 2, "equip position 1..11 expected");
    if (sd == nullptr)
        return 0;
    int result;
    {
        IOff0 i = pc_checkequip(sd, equip_slots[pos - 1]);
        if (i.ok())
        {
            result = 0;
            Option<P<struct item_data>> item_ = sd->inventory_data[i];
            OMATCH_BEGIN_SOME (item, item_)
            {
                result = unwrap<ItemNameId>(item->nameid);
            }
            OMATCH_END ();
        }
        else
            result = -1;
    }
    return luac::push_int(L, result);
}

static
int lp_p_nude(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr)
        return 0;
    {
        for (EQUIP i : EQUIPs)
        {
            IOff0 idx = sd->equip_index_maybe[i];
            if (idx.ok())
                pc_unequipitem(sd, idx, CalcStatus::LATER);
        }
        pc_calcstatus(sd, static_cast<int>(CalcStatusKind::NORMAL_RECALC));
    }
    return 0;
}

static
int lp_p_unequipbyid(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int slot = check_int(L, 2);
    if (slot < 0 || slot >= static_cast<int>(EQUIP::COUNT))
        luaL_argerror(L, 2, "equip slot 0..10 expected");
    if (sd == nullptr)
        return 0;
    {
        IOff0 idx = sd->equip_index_maybe[static_cast<EQUIP>(slot)];
        if (idx.ok())
            pc_unequipitem(sd, idx, CalcStatus::LATER);
        pc_calcstatus(sd, static_cast<int>(CalcStatusKind::NORMAL_RECALC));
    }
    return 0;
}

// ------------------------------------------------------------------------
// messages and effects (doc/lua-api.md 5.6)

static
int lp_p_message(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    check_text(L, 2);
    if (sd == nullptr)
        return 0;
    {
        AString msg = text_to_string(L, 2);
        clif_displaymessage(sd->sess, msg);
    }
    return 0;
}

static
int lp_p_smsg(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int type = 0;
    int textidx = 2;
    if (lua_gettop(L) >= 3)
    {
        type = check_int(L, 2);
        textidx = 3;
    }
    check_text(L, textidx);
    if (sd == nullptr)
        return 0;
    if (type < 0 || type > 0xff)
        type = 0;
    {
        AString msg = text_to_string(L, textidx);
        clif_server_message(sd, static_cast<uint8_t>(type), msg);
    }
    return 0;
}

static
int lp_p_remotecmd(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    ZString cmd = check_string(L, 2);
    if (sd == nullptr)
        return 0;
    {
        clif_remote_command(sd, cmd);
    }
    return 0;
}

static
int lp_p_specialeffect(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int fx = check_int(L, 2);
    if (sd == nullptr)
        return 0;
    {
        clif_specialeffect(sd, fx, 0);
    }
    return 0;
}

static
int lp_p_announce(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    ZString text = check_string(L, 2);
    int flag = check_int(L, 3);
    if (sd == nullptr)
        return 0;
    {
        // the player is the source (doc/lua-api.md: flag & 8 == 0 form);
        // flag & 0x0f == 0 broadcasts world-wide through the char server
        if (flag & 0x0f)
            clif_GMmessage(sd, text, flag);
        else
            intif_GMmessage(text);
    }
    return 0;
}

static
int lp_p_gmlog(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    ZString text = check_string(L, 2);
    if (sd == nullptr)
        return 0;
    {
        AString msg = STRPRINTF("{SCRIPT} %s"_fmt, text);
        log_atcommand(sd, msg);
    }
    return 0;
}

// ------------------------------------------------------------------------
// timers and events on the player (doc/lua-api.md 5.7)

static
int lp_p_addtimer(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int ms = check_int(L, 2);
    // "Npc::OnX" string or function; takes a registry ref for the function
    // form and records the current ctx NPC as self
    LuaCallback cb = lua_cb_from_stack(L, 3);
    if (sd == nullptr || ms < 0)
    {
        lua_cb_release(cb);
        return 0;
    }
    {
        // 32 slots; when full the slot machinery releases cb and logs
        pc_addeventtimer(sd, static_cast<interval_t>(ms), cb);
    }
    return 0;
}

// p:event is installed as a pure-Lua closure (see the registration below)
// so that the inline dispatch path of npc.event can yield across it.

// ------------------------------------------------------------------------
// property dispatch

// upvalue 1: the player methods table
static
int player_index(lua_State* L)
{
    // stack: handle, key
    if (lua_type(L, 2) != LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_pushvalue(L, 2);
    lua_rawget(L, lua_upvalueindex(1));
    if (!lua_isnil(L, -1))
        return 1;   // a method
    lua_pop(L, 1);

    size_t klen;
    ZString key = luac::to_string(L, 2, &klen);
    dumb_ptr<map_session_data> sd = table_player(L, 1);
    if (key == "online"_s)
    {
        // valid on stale handles: false once the session is gone
        lua_pushboolean(L, sd != nullptr);
        return 1;
    }
    if (sd == nullptr)
    {
        lua_warn(STRPRINTF("stale player handle (read of '%s')"_fmt, key));
        lua_pushnil(L);
        return 1;
    }

    if (key == "id"_s)
        return luac::push_int(L,
                static_cast<int>(unwrap<BlockId>(sd->bl_id)));
    if (key == "charid"_s)
        return luac::push_int(L,
                static_cast<int>(unwrap<CharId>(sd->status_key.char_id)));
    if (key == "partyid"_s)
        return luac::push_int(L,
                static_cast<int>(unwrap<PartyId>(sd->status.party_id)));
    if (key == "guildid"_s)
        return luac::push_int(L, 0);
    if (key == "name"_s)
    {
        push_string(L, sd->status_key.name.to__actual());
        return 1;
    }
    if (key == "partyname"_s)
    {
        PartyName name;
        {
            Option<PartyPair> p_ = party_search(sd->status.party_id);
            name = p_.pmd_pget(&PartyMost::name).copy_or(PartyName());
        }
        push_string(L, name);
        return 1;
    }
    if (key == "map"_s)
    {
        if (sd->bl_m == borrow(undefined_gat))
            push_string(L, ""_s);
        else
            push_string(L, sd->bl_m->name_);
        return 1;
    }
    if (key == "x"_s)
        return luac::push_int(L, sd->bl_x);
    if (key == "y"_s)
        return luac::push_int(L, sd->bl_y);
    if (key == "dir"_s)
        return luac::push_int(L, static_cast<uint8_t>(sd->dir));
    if (key == "dead"_s)
    {
        lua_pushboolean(L, pc_isdead(sd));
        return 1;
    }
    if (key == "gmlevel"_s)
        return luac::push_int(L,
                static_cast<int>(pc_isGM(sd).get_all_bits()));
    if (key == "partnerid"_s)
        return luac::push_int(L,
                static_cast<int>(unwrap<CharId>(sd->status.partner_id)));
    if (key == "version"_s)
        return luac::push_int(L,
                static_cast<int>(unwrap<ClientVersion>(sd->client_version)));
    if (key == "opt2"_s)
        return luac::push_int(L, static_cast<uint16_t>(sd->opt2));
    if (key == "pvpchannel"_s)
        return luac::push_int(L,
                static_cast<int>(sd->state.pvpchannel));
    if (key == "hidden"_s)
    {
        lua_pushboolean(L, bool(sd->status.option & Opt0::HIDE));
        return 1;
    }
    if (key == "mask"_s)
    {
        if (sd->bl_m == borrow(undefined_gat))
            return luac::push_int(L, -1);
        return luac::push_int(L, sd->bl_m->mask);
    }
    if (key == "type"_s)
    {
        push_string(L, "player"_s);
        return 1;
    }

    int sp;
    if (lua_param_lookup(key, &sp))
    {
        int v;
        {
            v = pc_readparam(sd, static_cast<SP>(sp));
        }
        return luac::push_int(L, v);
    }

    // unknown names read nil (doc/lua-engine.md 7.1)
    lua_pushnil(L);
    return 1;
}

static
int player_newindex(lua_State* L)
{
    // stack: handle, key, value
    if (lua_type(L, 2) != LUA_TSTRING)
        return luaL_error(L, "player handle keys must be strings");
    size_t klen;
    ZString key = luac::to_string(L, 2, &klen);

    if (key == "opt2"_s)
    {
        int v = lua_check_int32_wrap(L, 3);
        dumb_ptr<map_session_data> sd = table_player(L, 1);
        if (sd == nullptr)
        {
            lua_warn("stale player handle (write of 'opt2' ignored)"_s);
            return 0;
        }
        {
            if (static_cast<uint16_t>(sd->opt2)
                != static_cast<uint16_t>(v))
            {
                sd->opt2 = static_cast<Opt2>(static_cast<uint16_t>(v));
                clif_changeoption(sd);
                pc_calcstatus(sd,
                        static_cast<int>(CalcStatusKind::NORMAL_RECALC));
            }
        }
        return 0;
    }
    if (key == "pvpchannel"_s)
    {
        int v = lua_check_int32_wrap(L, 3);
        if (v < 0)
            v = 0;   // write clamps at 0 (old setpvpchannel)
        dumb_ptr<map_session_data> sd = table_player(L, 1);
        if (sd == nullptr)
        {
            lua_warn("stale player handle (write of 'pvpchannel' ignored)"_s);
            return 0;
        }
        sd->state.pvpchannel = v;
        return 0;
    }

    int sp;
    if (lua_param_lookup(key, &sp))
    {
        if (!sp_is_writable(static_cast<SP>(sp)))
            return luaL_error(L, "param '%s' is read-only",
                    lua_tostring(L, 2));
        int v = lua_check_int32_wrap(L, 3);
        dumb_ptr<map_session_data> sd = table_player(L, 1);
        if (sd == nullptr)
        {
            lua_warn(STRPRINTF("stale player handle (write of '%s' ignored)"_fmt,
                        key));
            return 0;
        }
        {
            pc_setparam(sd, static_cast<SP>(sp), v);
        }
        return 0;
    }

    return luaL_error(L,
            "player handle has no field '%s' (use p.tmp for script data)",
            lua_tostring(L, 2));
}

static
const luaL_Reg player_funcs[] =
{
    {"heal", lp_p_heal},
    {"getexp", lp_p_getexp},
    {"setlook", lp_p_setlook},
    {"getlook", lp_p_getlook},
    {"savepoint", lp_p_savepoint},
    {"getsavepoint", lp_p_getsavepoint},
    {"resetstatus", lp_p_resetstatus},
    {"bonus", lp_p_bonus},
    {"bonus2", lp_p_bonus2},
    {"param", lp_p_param},
    {"setparam", lp_p_setparam},
    {"marriage", lp_p_marriage},
    {"divorce", lp_p_divorce},
    {"isat", lp_p_isat},
    {"isin", lp_p_isin},
    {"warp", lp_p_warp},
    {"mapmask", lp_p_mapmask},
    {"sendcollision", lp_p_sendcollision},
    {"music", lp_p_music},
    {"skill", lp_p_skill},
    {"setskill", lp_p_setskill},
    {"getskilllv", lp_p_getskilllv},
    {"getactivatedpoolskilllist", lp_p_getactivatedpoolskilllist},
    {"getunactivatedpoolskilllist", lp_p_getunactivatedpoolskilllist},
    {"poolskill", lp_p_poolskill},
    {"unpoolskill", lp_p_unpoolskill},
    {"overrideattack", lp_p_overrideattack},
    {"countitem", lp_p_countitem},
    {"checkweight", lp_p_checkweight},
    {"getitem", lp_p_getitem},
    {"delitem", lp_p_delitem},
    {"getinventorylist", lp_p_getinventorylist},
    {"getequipid", lp_p_getequipid},
    {"nude", lp_p_nude},
    {"unequipbyid", lp_p_unequipbyid},
    {"message", lp_p_message},
    {"smsg", lp_p_smsg},
    {"remotecmd", lp_p_remotecmd},
    {"specialeffect", lp_p_specialeffect},
    {"announce", lp_p_announce},
    {"gmlog", lp_p_gmlog},
    {"addtimer", lp_p_addtimer},
    // emotion, misceffect, sc_start, sc_end, sc_check, exists, distance,
    // target, injure, issummon come from the shared being methods; the
    // dialog verbs are registered by lua_dialog_register_methods; "event"
    // is the pure-Lua closure installed below
    {nullptr, nullptr},
};

// ------------------------------------------------------------------------
// registration (called from lua_register_handle_metatables in
// lua-handle-being.cpp) and handle creation

void lua_register_player_handle_parts(lua_State* L, int being_methods_idx,
        int env_idx)
{
    // player methods = shared being methods + player funcs + dialog verbs
    lua_newtable(L);
    int pm = lua_gettop(L);
    lua_pushnil(L);
    while (lua_next(L, being_methods_idx) != 0)
    {
        lua_pushvalue(L, -2);   // key
        lua_pushvalue(L, -2);   // value
        lua_rawset(L, pm);
        lua_pop(L, 1);          // value; key stays for lua_next
    }
    luac::register_funcs(L, pm, player_funcs);
    lua_dialog_register_methods(L, pm);

    // p:event(ev [, args]) == npc.event(ev, p, args), as a pure-Lua closure
    // so the inline dispatch path (same player's running dialog) can yield
    // across it (doc/lua-engine.md section 9). npc.event is resolved from
    // the sandbox at call time; lua_register_lib_npc runs after us.
    static const char p_event_src[] =
        "local env = ...\n"
        "return function(p, ev, args)\n"
        "    return env.npc.event(ev, p, args)\n"
        "end\n";
    if (luac::load_chunk(L, "=builtin:p.event"_s,
                ZString(p_event_src, p_event_src + sizeof (p_event_src) - 1,
                    nullptr)))
    {
        luac::set_chunk_env(L, -1, env_idx);
        lua_pushvalue(L, env_idx);
        if (lua_pcall(L, 1, 1, 0) == LUA_OK)
            lua_setfield(L, pm, "event");
        else
        {
            lua_warn("internal: building p.event failed"_s);
            lua_pop(L, 1);
        }
    }
    else
    {
        lua_warn("internal: compiling p.event failed"_s);
        lua_pop(L, 1);
    }

    // the three variable-proxy metatables (p.vars / p.acc / p.acc2)
    for (int scope = 0; scope < 3; ++scope)
    {
        lua_newtable(L);
        lua_pushinteger(L, scope);
        luac::push_cclosure(L, vars_index, "vars.__index", 1);
        lua_setfield(L, -2, "__index");
        lua_pushinteger(L, scope);
        luac::push_cclosure(L, vars_newindex, "vars.__newindex", 1);
        lua_setfield(L, -2, "__newindex");
        g_scope_mt_ref[scope] = luac::ref(L);
    }

    // the player metatable: registry["tmwa.player"]
    luaL_newmetatable(L, "tmwa.player");
    lua_pushvalue(L, pm);
    luac::push_cclosure(L, player_index, "player.__index", 1);
    lua_setfield(L, -2, "__index");
    luac::push_cfunction(L, player_newindex, "player.__newindex");
    lua_setfield(L, -2, "__newindex");
    lua_pushvalue(L, -1);
    g_player_mt_ref = luac::ref(L);
    lua_pop(L, 1);

    lua_pop(L, 1);   // pm
}

void lua_push_player_handle(lua_State* L, dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
    {
        lua_pushnil(L);
        return;
    }
    if (sd->lua.handle_ref != lua_noref)
    {
        luac::push_ref(L, sd->lua.handle_ref);
        return;
    }
    int id = static_cast<int>(unwrap<BlockId>(sd->bl_id));
    lua_createtable(L, 0, 8);
    int h = lua_gettop(L);
    // no metatable yet, so lua_setfield below is a raw write
    lua_pushinteger(L, id);
    lua_setfield(L, h, "__id");
    lua_pushinteger(L, sd->lua.serial);
    lua_setfield(L, h, "__serial");
    // p.tmp / p.tmpstr: plain tables with the typed default metatables
    lua_newtable(L);
    lua_push_engine_table(L, LuaTable::INT_DEFAULT_MT);
    lua_setmetatable(L, -2);
    lua_setfield(L, h, "tmp");
    lua_newtable(L);
    lua_push_engine_table(L, LuaTable::STR_DEFAULT_MT);
    lua_setmetatable(L, -2);
    lua_setfield(L, h, "tmpstr");
    // p.vars / p.acc / p.acc2 proxies
    static const char* const scope_names[3] = {"vars", "acc", "acc2"};
    for (int scope = 0; scope < 3; ++scope)
    {
        lua_createtable(L, 0, 2);
        lua_pushinteger(L, id);
        lua_setfield(L, -2, "__id");
        lua_pushinteger(L, sd->lua.serial);
        lua_setfield(L, -2, "__serial");
        luac::push_ref(L, g_scope_mt_ref[scope]);
        lua_setmetatable(L, -2);
        lua_setfield(L, h, scope_names[scope]);
    }
    luac::push_ref(L, g_player_mt_ref);
    lua_setmetatable(L, h);
    // cache: PLAYERS[id] = handle (removed by lua_session_detach)
    lua_push_engine_table(L, LuaTable::PLAYERS);
    lua_pushvalue(L, h);
    lua_rawseti(L, -2, id);
    lua_pop(L, 1);
    // cache: the session's handle_ref (owner: session)
    lua_pushvalue(L, h);
    sd->lua.handle_ref = luac::ref(L);
}
} // namespace map
} // namespace tmwa
