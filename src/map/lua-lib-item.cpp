#include "lua-libs.hpp"
//    lua-lib-item.cpp - the `item` namespace.
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
#include "../strings/literal.hpp"
#include "../strings/vstring.hpp"
#include "../strings/zstring.hpp"

#include "../io/cxxstdio.hpp"

#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"

#include "itemdb.hpp"
#include "map.hpp"
#include "lua-callback.hpp"
#include "lua-engine.hpp"
#include "lua-handles.hpp"
#include "lua-internal.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
// Every binding follows the longjmp discipline of doc/lua-engine.md
// section 11: raising checks first (trivial locals only), then a worker
// scope with tmwa types and no raising Lua API, then result pushes.

// item.getitemlink(item) -> string ("@@<id>|@@" client markup, or
// "Unknown Item")   Old: getitemlink
static
int li_getitemlink(lua_State* L)
{
    ItemNameId nameid = check_item(L, 1);
    if (!nameid)
    {
        lua_pushliteral(L, "Unknown Item");
        return 1;
    }
    VString<23> buf;
    SNPRINTF(buf, 24, "@@%d|@@"_fmt, unwrap<ItemNameId>(nameid));
    push_string(L, buf);
    return 1;
}

// item.id(name) -> int (0 unknown); no warning: this is the lookup function
static
int li_id(lua_State* L)
{
    ZString name = check_string(L, 1);
    int id = 0;
    {
        Option<P<struct item_data>> item_ = itemdb_searchname(name);
        OMATCH_BEGIN_SOME (item, item_)
        {
            id = unwrap<ItemNameId>(item->nameid);
        }
        OMATCH_END ();
    }
    return luac::push_int(L, id);
}

// item.name(item) -> string ("" unknown)
static
int li_name(lua_State* L)
{
    ItemNameId nameid = check_item(L, 1);
    ItemName name;
    if (nameid)
    {
        name = itemdb_search(nameid)->name;
    }
    push_string(L, name);
    return 1;
}

// item.exists(item) -> bool; never warns (this is the test function)
static
int li_exists(lua_State* L)
{
    bool r = false;
    if (lua_type(L, 1) == LUA_TSTRING)
    {
        ZString name = check_string(L, 1);
        {
            r = itemdb_searchname(name).is_some();
        }
    }
    else
    {
        int num = check_int(L, 1);
        if (num > 0 && num <= 65535)
        {
            r = itemdb_exists(wrap<ItemNameId>(static_cast<uint16_t>(num)))
                    .is_some();
        }
    }
    lua_pushboolean(L, r);
    return 1;
}

// item.weight(item) -> int (0 unknown)
static
int li_weight(lua_State* L)
{
    ItemNameId nameid = check_item(L, 1);
    int weight = 0;
    if (nameid)
    {
        weight = itemdb_search(nameid)->weight;
    }
    return luac::push_int(L, weight);
}

// ------------------------------------------------------------------------

static
const luaL_Reg item_funcs[] =
{
    {"getitemlink", li_getitemlink},
    {"id", li_id},
    {"name", li_name},
    {"exists", li_exists},
    {"weight", li_weight},
    {nullptr, nullptr},
};

void lua_register_lib_item(lua_State* L, int env_idx)
{
    int env = lua_absindex(L, env_idx);
    lua_newtable(L);
    luac::register_funcs(L, -1, item_funcs);
    lua_setfield(L, env, "item");
}
} // namespace map
} // namespace tmwa
