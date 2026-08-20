//    lua-alloc.cpp - the memory-capped Lua allocator.
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

// This TU deliberately does NOT include "../poison.hpp": a lua_Alloc has to
// call realloc/free, which are poisoned everywhere else (same pattern as
// src/generic/oops.cpp). Keep this file free of any other logic.

#include <cstddef>
#include <cstdlib>


namespace tmwa
{
namespace map
{
static size_t alloc_used = 0;
static size_t alloc_limit = 0;   // 0 = unlimited (before lua_init sets it)
static int alloc_failures = 0;

void lua_alloc_set_limit(size_t bytes)
{
    alloc_limit = bytes;
}

size_t lua_alloc_used()
{
    return alloc_used;
}

int lua_alloc_failures()
{
    return alloc_failures;
}

// The lua_Alloc contract: nsize == 0 frees and must return nullptr;
// otherwise behave like realloc, returning nullptr on failure (Lua turns
// that into LUA_ERRMEM, which surfaces through the normal error path).
// Enforces the configured cap on the running total.
void* lua_capped_alloc(void*, void* ptr, size_t osize, size_t nsize)
{
    // osize is only meaningful when ptr is non-null
    size_t old = ptr ? osize : 0;

    if (nsize == 0)
    {
        free(ptr);
        alloc_used -= old;
        return nullptr;
    }

    if (nsize > old && alloc_limit)
    {
        size_t grow = nsize - old;
        if (alloc_used + grow > alloc_limit)
        {
            ++alloc_failures;
            return nullptr;
        }
    }

    void* p = realloc(ptr, nsize);
    if (!p)
    {
        ++alloc_failures;
        return nullptr;
    }
    alloc_used += nsize;
    alloc_used -= old;
    return p;
}
} // namespace map
} // namespace tmwa
