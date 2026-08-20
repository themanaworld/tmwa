#include "lua-internal.hpp"
//    lua-prelude.cpp - the built-in pure-Lua prelude.
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

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
// Loaded into the sandbox at init, before any content; the chunk returns the
// module table M, which the engine keeps as LuaTable::PRELUDE. Rationale
// (doc/lua-engine.md section 9): dialog primitives must be able to yield
// across every frame between the coroutine and the yield point; pure Lua
// frames guarantee that, C trampolines do not. Everything that can be C++ is
// C++; only yield-crossing glue lives here.
static
const char prelude_src[] = R"lua(
local M = {}

-- p:menu table form (doc/lua-api.md 5.3). tbl is a 1-based sequence of
-- strings or pairs {text, value}; entries that are false or "" are skipped
-- (not sent, not selectable) but positions keep counting. Optional keys:
-- tbl.title (shown first via p:title), tbl.cancel (client cancel returns this
-- value instead of terminating the handler).
--
-- cmenu is the C menu primitive provided by lua-dialog.cpp:
--   cmenu(p, display, nsent, cancelable) -> sent-position in 1..nsent,
--   or nil for client cancel when cancelable is true. On cancel with
--   cancelable == false it does not return (the driver ends the handler,
--   like p:close()). It sends the menu packet, records nsent as the
--   selectable count, and yields; out-of-range answers are ignored by the
--   resume validation, so the mapping tables below stay alive across the
--   yield in this Lua frame.
function M.menu_table(p, tbl, cmenu)
    local title = rawget(tbl, "title")
    if title ~= nil then
        p:title(title)
    end
    local has_cancel = rawget(tbl, "cancel") ~= nil
    local parts = {}
    local origin = {}    -- sent position -> original index in tbl
    local pairform = {}  -- sent position -> true when the entry was {text, value}
    local values = {}    -- sent position -> the pair's value
    local nsent = 0
    local i = 1
    while true do
        local e = rawget(tbl, i)
        if e == nil then
            break
        end
        local text, val, ispair
        if type(e) == "table" then
            text = e[1]
            val = e[2]
            ispair = true
        else
            text = e
            ispair = false
        end
        if text ~= false and text ~= "" then
            if type(text) ~= "string" then
                error("menu entry " .. i .. ": string expected", 3)
            end
            nsent = nsent + 1
            parts[nsent] = text
            origin[nsent] = i
            pairform[nsent] = ispair
            values[nsent] = val
        end
        i = i + 1
    end
    if nsent == 0 then
        error("menu: no selectable entries", 3)
    end
    local sel = cmenu(p, table.concat(parts, ":") .. ":", nsent, has_cancel)
    if sel == nil then
        return rawget(tbl, "cancel")
    end
    if pairform[sel] then
        return values[sel], origin[sel]
    end
    return origin[sel]
end

-- Inline dispatch (doc/lua-engine.md section 5): npc.event called from
-- inside a dialog handler for that same player must run the target handler
-- in the current coroutine, crossing only Lua frames so it may itself yield.
function M.call_inline(handler, self, p, args)
    return handler(self, p, args)
end

-- Builds the npc.event function installed in the sandbox.
--   resolve_c(ev, p) -> handler, self  when the inline path applies
--                       (same player's running dialog coroutine), else nil.
--   event_c(ev, p, args) -> bool       the C++ lua_npc_event entry.
function M.wrap_npc_event(resolve_c, event_c)
    return function(ev, p, args)
        local handler, self = resolve_c(ev, p)
        if handler ~= nil then
            M.call_inline(handler, self, p, args)
            return true
        end
        return event_c(ev, p, args)
    end
end

return M
)lua";

ZString lua_prelude_source()
{
    return ZString(prelude_src, prelude_src + sizeof(prelude_src) - 1, nullptr);
}
} // namespace map
} // namespace tmwa
