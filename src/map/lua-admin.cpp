#include "lua-admin.hpp"
//    lua-admin.cpp - @setvar/@getvar/@luastats backends.
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

#include <cstdlib>

#include "../strings/xstring.hpp"

#include "../io/cxxstdio.hpp"

#include "../mmo/strs.hpp"

#include "map.hpp"
#include "pc.hpp"
#include "lua-callback.hpp"
#include "lua-engine.hpp"
#include "lua-internal.hpp"
#include "lua-mapreg.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
// varname keeps the old GM-command spelling (lua-admin.hpp): "X" char,
// "#X" / "##X" account, "$X" / "$X$" mapreg, "@X" / "@X$" the player's
// p.tmp / p.tmpstr tables. The "." NPC scope and the "$@" temporaries are
// plain Lua data now and are not reachable from the GM commands (D5).

// ------------------------------------------------------------------------
// p.tmp / p.tmpstr access. All stack traffic uses raw gets/sets: no
// metamethods run, so nothing here can raise (host top-level code must not
// longjmp).

// Push the tmp (or tmpstr) table of sd's handle. False when the player has
// no handle (no Lua session attached).
static
bool admin_push_tmp_table(lua_State* L, dumb_ptr<map_session_data> sd,
        bool str)
{
    push_player(L, sd);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }
    lua_pushlstring(L, str ? "tmpstr" : "tmp", str ? 6 : 3);
    lua_rawget(L, -2);
    if (!lua_istable(L, -1))
    {
        // normally created at session attach; be defensive
        lua_pop(L, 1);
        lua_newtable(L);
        lua_push_engine_table(L,
                str ? LuaTable::STR_DEFAULT_MT : LuaTable::INT_DEFAULT_MT);
        lua_setmetatable(L, -2);
        lua_pushlstring(L, str ? "tmpstr" : "tmp", str ? 6 : 3);
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);
    }
    lua_remove(L, -2);   // the handle
    return true;
}

// With the tmp table on top: replace it by the nested array table stored
// under key (creating it like the global array() helper does).
static
void admin_push_array_table(lua_State* L, XString key, bool str)
{
    push_string(L, key);
    lua_rawget(L, -2);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_push_engine_table(L,
                str ? LuaTable::STR_DEFAULT_MT : LuaTable::INT_DEFAULT_MT);
        lua_setmetatable(L, -2);
        push_string(L, key);
        lua_pushvalue(L, -2);
        lua_rawset(L, -4);
    }
    lua_remove(L, -2);   // the tmp table
}

static
bool admin_tmp_write(dumb_ptr<map_session_data> sd, XString key, int idx,
        bool str, ZString value)
{
    lua_State* L = lua_state();
    if (!L)
        return false;
    if (!admin_push_tmp_table(L, sd, str))
        return false;
    if (idx != 0)
    {
        // @X[i] lives in a nested table, like array(p.tmp, "X")[i]
        admin_push_array_table(L, key, str);
        lua_pushinteger(L, idx);
    }
    else
        push_string(L, key);
    if (str)
        push_string(L, value);
    else
        lua_pushinteger(L, atoi(value.c_str()));
    lua_rawset(L, -3);
    lua_pop(L, 1);
    return true;
}

// Format the value on top of the stack for GM feedback, then pop it.
static
AString admin_format_value(lua_State* L, bool str)
{
    AString out;
    switch (lua_type(L, -1))
    {
    case LUA_TNIL:
        out = str ? ""_s : "0"_s;
        break;
    case LUA_TNUMBER:
    {
        int v;
        if (luac::to_int(L, -1, &v))
            out = STRPRINTF("%d"_fmt, v);
        else
            out = "<non-integer number>"_s;
        break;
    }
    case LUA_TSTRING:
    {
        size_t len;
        ZString z = luac::to_string(L, -1, &len);
        out = AString(XString(z));
        break;
    }
    case LUA_TBOOLEAN:
        out = lua_toboolean(L, -1) ? "true"_s : "false"_s;
        break;
    default:
        out = STRPRINTF("<%s>"_fmt,
                ZString(strings::really_construct_from_a_pointer,
                    lua_typename(L, lua_type(L, -1)), nullptr));
        break;
    }
    lua_pop(L, 1);
    return out;
}

static
bool admin_tmp_read(dumb_ptr<map_session_data> sd, XString key, int idx,
        bool str, AString* out)
{
    lua_State* L = lua_state();
    if (!L)
        return false;
    if (!admin_push_tmp_table(L, sd, str))
        return false;
    if (idx != 0)
    {
        admin_push_array_table(L, key, str);
        lua_pushinteger(L, idx);
    }
    else
        push_string(L, key);
    lua_rawget(L, -2);
    *out = admin_format_value(L, str);
    lua_pop(L, 1);
    return true;
}

// ------------------------------------------------------------------------

bool lua_admin_setvar(dumb_ptr<map_session_data> target, ZString varname,
        int idx, ZString value, AString* out_msg)
{
    if (!varname)
    {
        *out_msg = "@setvar: empty variable name"_s;
        return false;
    }
    char prefix = varname.front();
    char postfix = varname.back();

    if (prefix == '.')
    {
        *out_msg = "@setvar: the '.' NPC scope was removed; NPC variables are Lua data (self.vars)"_s;
        return false;
    }

    if (prefix == '$')
    {
        if (varname[1] == '@')
        {
            *out_msg = "@setvar: '$@' temporaries are Lua data now (worldtmp / worldtmpstr)"_s;
            return false;
        }
        if (idx < 0 || idx > 255)
        {
            *out_msg = "@setvar: index 0..255 expected"_s;
            return false;
        }
        XString name = varname.xslice_t(1);
        if (!name || (postfix == '$' && name.size() < 2))
        {
            *out_msg = "@setvar: empty variable name"_s;
            return false;
        }
        if (postfix == '$')
            lua_mapreg_set_str(name, idx, value);
        else
            lua_mapreg_set_int(name, idx, atoi(value.c_str()));
        *out_msg = STRPRINTF("variable %s[%d] = `%s`."_fmt,
                varname, idx, value);
        return true;
    }

    // every remaining scope lives on a player
    if (target == nullptr)
    {
        *out_msg = "@setvar: this variable scope needs a target player"_s;
        return false;
    }

    if (prefix == '@')
    {
        bool str = (postfix == '$');
        XString key = varname.xslice_t(1);
        if (str)
            key = key.xrslice_h(1);
        if (!key)
        {
            *out_msg = "@setvar: empty variable name"_s;
            return false;
        }
        if (idx < 0 || idx > 255)
        {
            *out_msg = "@setvar: index 0..255 expected"_s;
            return false;
        }
        if (!admin_tmp_write(target, key, idx, str, value))
        {
            *out_msg = "@setvar: player has no Lua session"_s;
            return false;
        }
        *out_msg = STRPRINTF("variable %s[%d] = `%s` for player %s."_fmt,
                varname, idx, value, target->status_key.name);
        return true;
    }

    // persistent scopes: "X" char reg, "#X" account, "##X" account2.
    // These are int-only, and (as in the old engine) the index is ignored.
    if (postfix == '$')
    {
        *out_msg = "@setvar: only '$NAME$' and '@NAME$' string variables exist"_s;
        return false;
    }
    int val = atoi(value.c_str());
    VarName name = stringish<VarName>(varname);
    if (prefix == '#')
    {
        if (varname[1] == '#')
            pc_setaccountreg2(target, name, val);
        else
            pc_setaccountreg(target, name, val);
    }
    else
        pc_setglobalreg(target, name, val);
    *out_msg = STRPRINTF("variable %s[%d] = `%s` for player %s."_fmt,
            varname, idx, value, target->status_key.name);
    return true;
}

bool lua_admin_getvar(dumb_ptr<map_session_data> target, ZString varname,
        int idx, AString* out_msg)
{
    if (!varname)
    {
        *out_msg = "@getvar: empty variable name"_s;
        return false;
    }
    char prefix = varname.front();
    char postfix = varname.back();

    if (prefix == '.')
    {
        *out_msg = "@getvar: the '.' NPC scope was removed; NPC variables are Lua data (self.vars)"_s;
        return false;
    }

    if (prefix == '$')
    {
        if (varname[1] == '@')
        {
            *out_msg = "@getvar: '$@' temporaries are Lua data now (worldtmp / worldtmpstr)"_s;
            return false;
        }
        if (idx < 0 || idx > 255)
        {
            *out_msg = "@getvar: index 0..255 expected"_s;
            return false;
        }
        XString name = varname.xslice_t(1);
        if (!name || (postfix == '$' && name.size() < 2))
        {
            *out_msg = "@getvar: empty variable name"_s;
            return false;
        }
        AString sval;
        if (postfix == '$')
        {
            RString v = lua_mapreg_get_str(name, idx);
            sval = AString(v);
        }
        else
            sval = STRPRINTF("%d"_fmt, lua_mapreg_get_int(name, idx));
        *out_msg = STRPRINTF("variable %s[%d] == `%s`."_fmt,
                varname, idx, sval);
        return true;
    }

    if (target == nullptr)
    {
        *out_msg = "@getvar: this variable scope needs a target player"_s;
        return false;
    }

    if (prefix == '@')
    {
        bool str = (postfix == '$');
        XString key = varname.xslice_t(1);
        if (str)
            key = key.xrslice_h(1);
        if (!key)
        {
            *out_msg = "@getvar: empty variable name"_s;
            return false;
        }
        if (idx < 0 || idx > 255)
        {
            *out_msg = "@getvar: index 0..255 expected"_s;
            return false;
        }
        AString sval;
        if (!admin_tmp_read(target, key, idx, str, &sval))
        {
            *out_msg = "@getvar: player has no Lua session"_s;
            return false;
        }
        *out_msg = STRPRINTF("variable %s[%d] == `%s` for player %s."_fmt,
                varname, idx, sval, target->status_key.name);
        return true;
    }

    if (postfix == '$')
    {
        *out_msg = "@getvar: only '$NAME$' and '@NAME$' string variables exist"_s;
        return false;
    }
    int val;
    VarName name = stringish<VarName>(varname);
    if (prefix == '#')
    {
        if (varname[1] == '#')
            val = pc_readaccountreg2(target, name);
        else
            val = pc_readaccountreg(target, name);
    }
    else
        val = pc_readglobalreg(target, name);
    *out_msg = STRPRINTF("variable %s[%d] == `%d` for player %s."_fmt,
            varname, idx, val, target->status_key.name);
    return true;
}

AString lua_admin_stats()
{
    return STRPRINTF("lua: mem=%zu bytes, live_refs=%d, budget_aborts=%d, alloc_failures=%d"_fmt,
            lua_mem_used(), lua_live_refs(), lua_budget_aborts(),
            lua_alloc_failures());
}
} // namespace map
} // namespace tmwa
