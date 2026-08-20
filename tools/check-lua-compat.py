#!/usr/bin/env python3
# check-lua-compat.py - enforce the lua-compat.hpp boundary.
#
# doc/lua-engine.md section 10: version-specific Lua C API calls must live
# only in src/map/lua-compat.hpp. This scans every other src/map/lua-*.cpp
# and lua-*.hpp for the forbidden names (word-boundary matches, comments
# stripped so prose mentioning an API name does not count) and fails if any
# appear. Wired into ctest.
#
# Exit status: 0 clean, 1 findings (or missing sources).

import os
import re
import sys

FORBIDDEN = [
    "lua_callk",
    "lua_pcallk",
    "lua_yieldk",
    "lua_newuserdatauv",
    "lua_setiuservalue",
    "lua_toclose",
    "lua_seti",
    "lua_geti",
    "lua_rawlen",
    "luaL_setfuncs",
    "luaL_newlib",
    "luaL_requiref",
    "luaL_ref",
    "luaL_unref",
    "lua_pushglobaltable",
    "lua_sethook",
    "lua_getextraspace",
    "luaL_loadbuffer",
    "luaL_loadstring",
    "luaL_dostring",
    "lua_isinteger",
    "luaL_traceback",
    "lua_setwarnf",
    "lua_dump",
    "__gc",
    "lua_Integer",
]

FORBIDDEN_RE = re.compile(
        r"\b(" + "|".join(re.escape(n) for n in FORBIDDEN) + r")\b")

LINE_COMMENT_RE = re.compile(r"//[^\n]*")
BLOCK_COMMENT_RE = re.compile(r"/\*.*?\*/", re.S)


def strip_comments(src):
    """Blank out comments, preserving newlines so line numbers hold."""
    def blank(m):
        return re.sub(r"[^\n]", " ", m.group(0))
    src = BLOCK_COMMENT_RE.sub(blank, src)
    src = LINE_COMMENT_RE.sub(blank, src)
    return src


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    map_dir = os.path.join(root, "src", "map")
    files = []
    for fn in sorted(os.listdir(map_dir)):
        if not fn.startswith("lua-"):
            continue
        if fn == "lua-compat.hpp":
            continue
        # unit tests drive the 5.4 engine directly (e.g. lua_Integer casts to
        # build out-of-int32 values); they are not part of the portable
        # engine surface the Luau port would rebuild
        if fn.endswith("_test.cpp"):
            continue
        if fn.endswith(".cpp") or fn.endswith(".hpp"):
            files.append(os.path.join(map_dir, fn))
    if not files:
        print("check-lua-compat: no src/map/lua-* sources found", file=sys.stderr)
        return 1

    findings = 0
    for path in files:
        with open(path, encoding="utf-8", errors="surrogateescape") as f:
            src = f.read()
        code = strip_comments(src)
        for m in FORBIDDEN_RE.finditer(code):
            line = code.count("\n", 0, m.start()) + 1
            print("%s:%d: forbidden outside lua-compat.hpp: %s"
                  % (os.path.relpath(path, root), line, m.group(1)))
            findings += 1

    if findings:
        print("check-lua-compat: %d finding(s)" % findings)
        return 1
    print("check-lua-compat: OK (%d files)" % len(files))
    return 0


if __name__ == "__main__":
    sys.exit(main())
