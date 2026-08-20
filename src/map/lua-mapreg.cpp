#include "lua-mapreg.hpp"
//    lua-mapreg.cpp - persistent server-wide variables ($ mapreg).
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

#include "../compat/option.hpp"

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"

#include "../generic/db.hpp"
#include "../generic/intern-pool.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/read.hpp"
#include "../io/lock.hpp"

#include "../net/timer.hpp"

#include "globals.hpp"
#include "map.hpp"
#include "map_conf.hpp"
// TEMPORARY (coexistence): SIR lives in script-persist.hpp until the old
// engine is deleted; the storage keys stay shared with it so both engines
// see the same mapreg data (see the module instructions in
// doc/lua-engine.md section 1: the dbs move here at integration time).
#include "script-persist.hpp"
#include "lua-engine.hpp"
#include "lua-internal.hpp"
#include "lua-libs.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
constexpr std::chrono::milliseconds MAPREG_AUTOSAVE_INTERVAL = 10_s;

// ------------------------------------------------------------------------
// storage (shared with the old engine during the transition:
// globals.hpp mapreg_db / mapregstr_db / mapreg_dirty, names interned in
// variable_names in the full old spelling "$NAME" / "$NAME$")

static
SIR mapreg_key(XString name, int idx, bool is_str)
{
    AString full;
    if (is_str && !name.endswith('$'))
        full = STRPRINTF("$%s$"_fmt, AString(name));
    else
        full = STRPRINTF("$%s"_fmt, AString(name));
    size_t id = variable_names.intern(full);
    return SIR::from(id, static_cast<uint8_t>(idx));
}

int lua_mapreg_get_int(XString name, int idx)
{
    return mapreg_db.get(mapreg_key(name, idx, false));
}

RString lua_mapreg_get_str(XString name, int idx)
{
    Option<Borrowed<RString>> v = mapregstr_db.search(mapreg_key(name, idx, true));
    OMATCH_BEGIN_SOME (s, v)
    {
        return *s;
    }
    OMATCH_END ();
    return RString();
}

void lua_mapreg_set_int(XString name, int idx, int val)
{
    // DMap::put erases the key for val == 0, as mapreg_setreg did
    mapreg_db.put(mapreg_key(name, idx, false), val);
    mapreg_dirty = 1;
}

void lua_mapreg_set_str(XString name, int idx, XString val)
{
    SIR key = mapreg_key(name, idx, true);
    if (!val)
        mapregstr_db.erase(key);
    else
        mapregstr_db.insert(key, RString(val));
    mapreg_dirty = 1;
}

// ------------------------------------------------------------------------
// load / save (byte-identical to the old script_load_mapreg /
// script_save_mapreg: "name[,idx]\tvalue" lines, ints first then strings,
// "$@" temporaries never saved)

static
void lua_mapreg_load(void)
{
    io::ReadFile in(map_conf.mapreg_txt);

    if (!in.is_open())
        return;

    AString line;
    while (in.getline(line))
    {
        XString buf1, buf2;
        int index = 0;
        if (extract(line,
                    record<'\t'>(
                        record<','>(&buf1, &index),
                        &buf2))
            || extract(line,
                    record<'\t'>(
                        record<','>(&buf1),
                        &buf2)))
        {
            size_t s = variable_names.intern(buf1);
            SIR key = SIR::from(s, index);
            if (buf1.back() == '$')
            {
                mapregstr_db.insert(key, buf2);
            }
            else
            {
                int v;
                if (!extract(buf2, &v))
                    goto borken;
                mapreg_db.put(key, v);
            }
        }
        else
        {
        borken:
            PRINTF("%s: %s broken data !\n"_fmt, map_conf.mapreg_txt, AString(buf1));
            continue;
        }
    }
    mapreg_dirty = 0;
}

static
void lua_mapreg_save_intsub(SIR key, int data, io::WriteFile& fp)
{
    int num = key.base(), i = key.index();
    ZString name = variable_names.outtern(num);
    if (name[1] != '@')
    {
        if (i == 0)
            FPRINTF(fp, "%s\t%d\n"_fmt, name, data);
        else
            FPRINTF(fp, "%s,%d\t%d\n"_fmt, name, i, data);
    }
}

static
void lua_mapreg_save_strsub(SIR key, ZString data, io::WriteFile& fp)
{
    int num = key.base(), i = key.index();
    ZString name = variable_names.outtern(num);
    if (name[1] != '@')
    {
        if (i == 0)
            FPRINTF(fp, "%s\t%s\n"_fmt, name, data);
        else
            FPRINTF(fp, "%s,%d\t%s\n"_fmt, name, i, data);
    }
}

static
void lua_mapreg_save(void)
{
    io::WriteLock fp(map_conf.mapreg_txt);
    if (!fp.is_open())
        return;
    for (auto& pair : mapreg_db)
        lua_mapreg_save_intsub(pair.first, pair.second, fp);
    for (auto& pair : mapregstr_db)
        lua_mapreg_save_strsub(pair.first, pair.second, fp);
    mapreg_dirty = 0;
}

static
void lua_mapreg_autosave(TimerData *, tick_t)
{
    if (mapreg_dirty)
        lua_mapreg_save();
}

void mapreg_init()
{
    lua_mapreg_load();

    Timer(gettick() + MAPREG_AUTOSAVE_INTERVAL,
            lua_mapreg_autosave,
            MAPREG_AUTOSAVE_INTERVAL
    ).detach();
}

void mapreg_final()
{
    if (mapreg_dirty >= 0)
        lua_mapreg_save();
}

// ------------------------------------------------------------------------
// the world / world.str / world.array proxies (doc/lua-api.md section 9.6)

// [A-Za-z0-9_]+ with an optional single trailing '$' when allow_str;
// *is_str is set when the trailing '$' is present
static
bool world_name_ok(XString name, bool allow_str, bool* is_str)
{
    XString base = name;
    bool str = false;
    if (name.endswith('$'))
    {
        if (!allow_str)
            return false;
        str = true;
        base = name.xrslice_h(1);
    }
    if (!base)
        return false;
    for (char c : base)
    {
        if (!(('0' <= c && c <= '9')
                || ('A' <= c && c <= 'Z')
                || ('a' <= c && c <= 'z')
                || c == '_'))
            return false;
    }
    if (is_str)
        *is_str = str;
    return true;
}

// world.NAME read (int namespace, index 0)
static
int lw_world_index(lua_State* L)
{
    ZString name = check_string(L, 2);
    if (!world_name_ok(name, false, nullptr))
        return luaL_error(L,
                "world: invalid variable name '%s' (use world.str for string variables)",
                name.c_str());
    return luac::push_int(L, lua_mapreg_get_int(name, 0));
}

// world.NAME = v (int namespace, index 0; 0 / nil deletes)
static
int lw_world_newindex(lua_State* L)
{
    ZString name = check_string(L, 2);
    if (!world_name_ok(name, false, nullptr))
        return luaL_error(L,
                "world: invalid variable name '%s' (use world.str for string variables)",
                name.c_str());
    int val = 0;
    if (lua_type(L, 3) != LUA_TNIL)
        val = check_int(L, 3);
    lua_mapreg_set_int(name, 0, val);
    return 0;
}

// world.str.NAME read (string namespace, index 0)
static
int lw_worldstr_index(lua_State* L)
{
    ZString name = check_string(L, 2);
    if (!world_name_ok(name, false, nullptr))
        return luaL_error(L, "world.str: invalid variable name '%s'",
                name.c_str());
    RString v = lua_mapreg_get_str(name, 0);
    push_string(L, v);
    return 1;
}

// world.str.NAME = v (string namespace, index 0; "" / nil deletes)
static
int lw_worldstr_newindex(lua_State* L)
{
    ZString name = check_string(L, 2);
    if (!world_name_ok(name, false, nullptr))
        return luaL_error(L, "world.str: invalid variable name '%s'",
                name.c_str());
    ZString val;
    if (lua_type(L, 3) != LUA_TNIL)
        val = check_string(L, 3);
    lua_mapreg_set_str(name, 0, val);
    return 0;
}

// world.get(name [, idx])
static
int lw_world_get(lua_State* L)
{
    ZString name = check_string(L, 1);
    int idx = opt_int(L, 2, 0);
    bool is_str = false;
    if (!world_name_ok(name, true, &is_str))
        return luaL_error(L, "world.get: invalid variable name '%s'",
                name.c_str());
    if (idx < 0 || idx > 255)
        return luaL_argerror(L, 2, "index 0..255 expected");
    if (is_str)
    {
        RString v = lua_mapreg_get_str(name, idx);
        push_string(L, v);
        return 1;
    }
    return luac::push_int(L, lua_mapreg_get_int(name, idx));
}

// world.set(name, idx, value)
static
int lw_world_set(lua_State* L)
{
    ZString name = check_string(L, 1);
    int idx = check_int(L, 2);
    bool is_str = false;
    if (!world_name_ok(name, true, &is_str))
        return luaL_error(L, "world.set: invalid variable name '%s'",
                name.c_str());
    if (idx < 0 || idx > 255)
        return luaL_argerror(L, 2, "index 0..255 expected");
    if (is_str)
    {
        ZString val;
        if (lua_type(L, 3) != LUA_TNIL)
            val = check_string(L, 3);
        lua_mapreg_set_str(name, idx, val);
        return 0;
    }
    int val = 0;
    if (lua_type(L, 3) != LUA_TNIL)
        val = check_int(L, 3);
    lua_mapreg_set_int(name, idx, val);
    return 0;
}

// ---- world.array proxies. The proxy is an empty table whose metatable
// routes integer keys to mapreg get/set; the generic setarray/cleararray/
// getarraysize/array_search helpers go through gettable/settable and so work
// on it transparently. Upvalue 1 of every closure is the full name
// ("NAME" or "NAME$").

// name upvalue -> is this the string namespace?
static
bool lw_arr_is_str(lua_State* L)
{
    size_t len;
    ZString name = luac::to_string(L, lua_upvalueindex(1), &len);
    return name.endswith('$');
}

// a:size(): getarraysize rule, index of the last non-0 / non-"" entry
// plus 1, scanning 0..255; 0 when empty
static
int lw_arr_size(lua_State* L)
{
    size_t len;
    ZString name = luac::to_string(L, lua_upvalueindex(1), &len);
    bool is_str = name.endswith('$');
    int last = 0;
    for (int i = 0; i < 256; ++i)
    {
        bool nonempty;
        if (is_str)
        {
            RString v = lua_mapreg_get_str(name, i);
            nonempty = bool(v);
        }
        else
        {
            nonempty = (lua_mapreg_get_int(name, i) != 0);
        }
        if (nonempty)
            last = i + 1;
    }
    return luac::push_int(L, last);
}

// a:clear(start, count): reset entries to 0 / ""
static
int lw_arr_clear(lua_State* L)
{
    int start = check_int(L, 2);
    int count = check_int(L, 3);
    if (start < 0 || start > 255)
        return luaL_argerror(L, 2, "start index 0..255 expected");
    bool is_str = lw_arr_is_str(L);
    size_t len;
    ZString name = luac::to_string(L, lua_upvalueindex(1), &len);
    for (int k = 0; k < count; ++k)
    {
        int idx = start + k;
        if (idx > 255)
            break;
        if (is_str)
            lua_mapreg_set_str(name, idx, ZString());
        else
            lua_mapreg_set_int(name, idx, 0);
    }
    return 0;
}

static
int lw_arr_index(lua_State* L)
{
    if (lua_type(L, 2) == LUA_TSTRING)
    {
        size_t len;
        ZString k = luac::to_string(L, 2, &len);
        if (k == "size"_s)
        {
            lua_pushvalue(L, lua_upvalueindex(1));
            luac::push_cclosure(L, lw_arr_size, "world.array.size", 1);
            return 1;
        }
        if (k == "clear"_s)
        {
            lua_pushvalue(L, lua_upvalueindex(1));
            luac::push_cclosure(L, lw_arr_clear, "world.array.clear", 1);
            return 1;
        }
        return luaL_error(L, "world.array: unknown field '%s'", k.c_str());
    }
    int idx = check_int(L, 2);
    if (idx < 0 || idx > 255)
        return luaL_argerror(L, 2, "index 0..255 expected");
    size_t len;
    ZString name = luac::to_string(L, lua_upvalueindex(1), &len);
    if (name.endswith('$'))
    {
        RString v = lua_mapreg_get_str(name, idx);
        push_string(L, v);
        return 1;
    }
    return luac::push_int(L, lua_mapreg_get_int(name, idx));
}

static
int lw_arr_newindex(lua_State* L)
{
    int idx = check_int(L, 2);
    if (idx < 0 || idx > 255)
        return luaL_argerror(L, 2, "index 0..255 expected");
    bool is_str = lw_arr_is_str(L);
    size_t len;
    ZString name = luac::to_string(L, lua_upvalueindex(1), &len);
    if (is_str)
    {
        ZString val;
        if (lua_type(L, 3) != LUA_TNIL)
            val = check_string(L, 3);
        lua_mapreg_set_str(name, idx, val);
        return 0;
    }
    int val = 0;
    if (lua_type(L, 3) != LUA_TNIL)
        val = check_int(L, 3);
    lua_mapreg_set_int(name, idx, val);
    return 0;
}

// world.array("NAME") / world.array("NAME$")
static
int lw_world_array(lua_State* L)
{
    ZString name = check_string(L, 1);
    if (!world_name_ok(name, true, nullptr))
        return luaL_error(L, "world.array: invalid variable name '%s'",
                name.c_str());
    lua_newtable(L);                    // the proxy (stays empty)
    lua_newtable(L);                    // its metatable
    lua_pushvalue(L, 1);
    luac::push_cclosure(L, lw_arr_index, "world.array.__index", 1);
    lua_setfield(L, -2, "__index");
    lua_pushvalue(L, 1);
    luac::push_cclosure(L, lw_arr_newindex, "world.array.__newindex", 1);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);
    return 1;
}

void lua_register_lib_world(lua_State* L, int env_idx)
{
    int env = lua_absindex(L, env_idx);

    // world
    lua_newtable(L);
    // world.str
    lua_newtable(L);
    lua_newtable(L);
    luac::push_cfunction(L, lw_worldstr_index, "world.str.__index");
    lua_setfield(L, -2, "__index");
    luac::push_cfunction(L, lw_worldstr_newindex, "world.str.__newindex");
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, "str");
    // world.get / world.set / world.array
    luac::push_cfunction(L, lw_world_get, "world.get");
    lua_setfield(L, -2, "get");
    luac::push_cfunction(L, lw_world_set, "world.set");
    lua_setfield(L, -2, "set");
    luac::push_cfunction(L, lw_world_array, "world.array");
    lua_setfield(L, -2, "array");
    // the proxy metatable: any other name is a mapreg int at index 0
    lua_newtable(L);
    luac::push_cfunction(L, lw_world_index, "world.__index");
    lua_setfield(L, -2, "__index");
    luac::push_cfunction(L, lw_world_newindex, "world.__newindex");
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);
    lua_setfield(L, env, "world");

    // worldtmp / worldtmpstr: plain tables, missing keys read 0 / ""
    lua_newtable(L);
    lua_push_engine_table(L, LuaTable::INT_DEFAULT_MT);
    lua_setmetatable(L, -2);
    lua_setfield(L, env, "worldtmp");
    lua_newtable(L);
    lua_push_engine_table(L, LuaTable::STR_DEFAULT_MT);
    lua_setmetatable(L, -2);
    lua_setfield(L, env, "worldtmpstr");
}
} // namespace map
} // namespace tmwa
