#!/usr/bin/env python3
# convert-npc-data.py - convert an old tmwa serverdata npc tree to the Lua tree.
#
# See doc/lua-engine.md section 15.1 and doc/lua-api.md section 8.
#
# Data entries (warp, shop, monster, mapflag) are fully converted into
# npc.warp{} / npc.shop{} / npc.monster{} / npc.mapflag{} calls.
# script/function bodies become bootable PORTME() stubs with the original
# source embedded verbatim in a Lua long comment, labels pre-split into the
# constructor fields (on_click, on_touch, on_init, on_timer[N], events.OnX).
# '-- AUDIT:' comments mark the behaviour-change hotspots of
# doc/lua-api.md section 13.
#
# Deterministic and idempotent: re-running rewrites only stubs still marked
# PORTME; blocks whose PORTME() call was replaced by ported code are kept.
#
# Usage:
#   convert-npc-data.py [--src NPCDIR] --out OUTDIR
#   convert-npc-data.py [--src NPCDIR] --single FILE   (prints Lua to stdout)
#
# Python 3 stdlib only.

import argparse
import os
import re
import sys

DEFAULT_SRC = "/home/bjorn/projects/tmw/serverdata/world/map/npc"

LUA_KEYWORDS = {
    "and", "break", "do", "else", "elseif", "end", "false", "for", "function",
    "goto", "if", "in", "local", "nil", "not", "or", "repeat", "return",
    "then", "true", "until", "while",
}

BROADCAST_LABELS = {
    "OnPCLoginEvent", "OnPCLogoutEvent", "OnPCDieEvent", "OnPCKillEvent",
    "OnMobKillEvent",
}
CLOCK_LABEL_RE = re.compile(
    r"^(OnMinute[0-9]{2}|OnClock[0-9]{4}|OnHour[0-9]{2}|OnDay[0-9]{4})$")
TIMER_LABEL_RE = re.compile(r"^OnTimer([0-9]+)$")

DIALOG_BUILTINS = {"mes", "menu", "next", "close", "close2", "input"}


class ConvertError(Exception):
    pass


def read_text(path):
    with open(path, "r", encoding="utf-8", errors="surrogateescape") as f:
        return f.read()


def write_text(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", errors="surrogateescape") as f:
        f.write(text)


# ---------------------------------------------------------------------------
# top-level npc .txt parsing (mirrors src/ast/npc.cpp)

class Item:
    # kind: comment | warp | shop | monster | mapflag |
    #       script_function | script_none | script_map
    def __init__(self, kind, line):
        self.kind = kind
        self.line = line


def split_words(component):
    return component


class TopParser:
    """Parses the old |-separated top-level grammar.

    An item is one physical line of |-separated components, each a
    ,-separated list of words, ending at end of line or at '{' (script
    body start, read verbatim until the first '}')."""

    def __init__(self, text, filename):
        self.text = text
        self.filename = filename
        self.pos = 0
        self.line = 1

    def err(self, msg):
        return ConvertError("%s:%d: %s" % (self.filename, self.line, msg))

    def at_end(self):
        return self.pos >= len(self.text)

    def peek(self):
        return self.text[self.pos] if self.pos < len(self.text) else ""

    def adv(self):
        c = self.text[self.pos]
        self.pos += 1
        if c == "\n":
            self.line += 1
        return c

    def skip_blank_lines(self):
        while not self.at_end() and self.peek() == "\n":
            self.adv()

    def lex_head(self):
        """Returns (bits, hit_brace). bits is a list of components, each a
        list of words. Assumes not at EOL."""
        bits = [[]]
        word = []
        expect_word = True
        while True:
            c = self.peek()
            if c == "" or c == "\n" or c == "{":
                break
            if c in ",|":
                if word:
                    bits[-1].append("".join(word))
                    word = []
                # collapse adjacent separators like the old lexer (warning
                # there; silent here)
                self.adv()
                while self.peek() in ",|":
                    c = self.adv()
                if self.peek() in ("", "\n", "{"):
                    break  # separator at EOL: warning in the old lexer
                if c == "|":
                    bits.append([])
                continue
            word.append(self.adv())
        if word:
            bits[-1].append("".join(word))
        hit_brace = self.peek() == "{"
        if self.peek() == "\n":
            self.adv()
        return bits, hit_brace

    def read_body(self, hit_brace):
        """parse_script_body: skip spaces/newlines, expect '{', accumulate
        verbatim until the FIRST '}'. Returns (body_text, first_line)."""
        if not hit_brace:
            while not self.at_end() and self.peek() in " \t\n":
                self.adv()
        if self.peek() != "{":
            raise self.err("expected '{' to start script body")
        self.adv()
        # skip to end of the '{' line so the body starts on a fresh line
        # position: keep exactly what follows the brace
        start_line = self.line
        chars = []
        while True:
            if self.at_end():
                raise self.err("unterminated script body (missing '}')")
            c = self.adv()
            if c == "}":
                break
            chars.append(c)
        body = "".join(chars)
        # consume the rest of the '}' line if it is only whitespace
        while not self.at_end() and self.peek() in " \t":
            self.adv()
        if self.peek() == "\n":
            self.adv()
        return body, start_line

    def parse_int(self, word, what):
        try:
            return int(word, 10)
        except ValueError:
            raise self.err("failed to extract %s from '%s'" % (what, word))

    def parse_delay_ms(self, word, what):
        m = re.match(r"^([0-9]+)(ms|s|m|h|d)?$", word)
        if not m:
            raise self.err("failed to extract %s from '%s'" % (what, word))
        n = int(m.group(1), 10)
        unit = m.group(2)
        mult = {None: 1, "ms": 1, "s": 1000, "m": 60000,
                "h": 3600000, "d": 86400000}[unit]
        return n * mult

    def next_item(self):
        self.skip_blank_lines()
        if self.at_end():
            return None
        line0 = self.line
        # comment lines: first token starts with //
        rest = self.text[self.pos:]
        stripped = rest.lstrip(" \t")
        if stripped.startswith("//"):
            eol = self.text.find("\n", self.pos)
            if eol < 0:
                eol = len(self.text)
            comment = self.text[self.pos:eol]
            while self.pos < eol:
                self.adv()
            if self.peek() == "\n":
                self.adv()
            it = Item("comment", line0)
            it.text = comment.strip()
            return it
        bits, hit_brace = self.lex_head()
        if len(bits) < 2:
            raise self.err("Expected a line with |s in it")
        for b in bits:
            if not b:
                raise self.err("Empty components are not cool")
        if len(bits[1]) != 1:
            raise self.err("Expected a single word in type position")
        kind = bits[1][0]
        if kind == "warp":
            return self.parse_warp(bits, line0)
        if kind == "shop":
            return self.parse_shop(bits, line0)
        if kind == "monster":
            return self.parse_monster(bits, line0)
        if kind == "mapflag":
            return self.parse_mapflag(bits, line0)
        if kind == "script":
            return self.parse_script(bits, hit_brace, line0)
        raise self.err("Unknown type '%s'" % kind)

    def parse_warp(self, bits, line0):
        # map,x,y|warp|xs,ys,to_map,to_x,to_y
        if len(bits) != 3:
            raise self.err("warp: expect 3 |component|s")
        if len(bits[0]) != 3:
            raise self.err("warp: in |component 1| expect 3 ,component,s")
        if len(bits[2]) != 5:
            raise self.err("warp: in |component 3| expect 5 ,component,s")
        it = Item("warp", line0)
        it.m = bits[0][0]
        it.x = self.parse_int(bits[0][1], "x")
        it.y = self.parse_int(bits[0][2], "y")
        it.xs = self.parse_int(bits[2][0], "xs")
        it.ys = self.parse_int(bits[2][1], "ys")
        it.to_m = bits[2][2]
        it.to_x = self.parse_int(bits[2][3], "to_x")
        it.to_y = self.parse_int(bits[2][4], "to_y")
        return it

    def parse_shop(self, bits, line0):
        # map,x,y,dir|shop|Name|class,Item:price,...
        if len(bits) != 4:
            raise self.err("shop: expect 4 |component|s")
        if len(bits[0]) != 4:
            raise self.err("shop: in |component 1| expect 4 ,component,s")
        if len(bits[2]) != 1:
            raise self.err("shop: in |component 3| expect 1 ,component,s")
        if len(bits[3]) < 2:
            raise self.err("shop: in |component 4| expect at least 2 ,component,s")
        it = Item("shop", line0)
        it.m = bits[0][0]
        it.x = self.parse_int(bits[0][1], "x")
        it.y = self.parse_int(bits[0][2], "y")
        it.d = self.parse_int(bits[0][3], "dir")
        it.name = bits[2][0]
        it.npc_class = self.parse_int(bits[3][0], "class")
        it.items = []
        for w in bits[3][1:]:
            if ":" not in w:
                raise self.err("shop: failed to split item:value '%s'" % w)
            iname, value = w.split(":", 1)
            it.items.append((iname, value))
        return it

    def parse_monster(self, bits, line0):
        # map,x,y[,xs,ys]|monster|Name|class,num[,delay1,delay2[,event]]
        if len(bits) != 4:
            raise self.err("monster: expect 4 |component|s")
        if len(bits[0]) not in (3, 5):
            raise self.err("monster: in |component 1| expect 3 or 5 ,component,s")
        if len(bits[2]) != 1:
            raise self.err("monster: in |component 3| expect 1 ,component,s")
        if len(bits[3]) not in (2, 4, 5):
            raise self.err("monster: in |component 4| expect 2, 4, or 5 ,component,s")
        it = Item("monster", line0)
        it.m = bits[0][0]
        it.x = self.parse_int(bits[0][1], "x")
        it.y = self.parse_int(bits[0][2], "y")
        it.has_area = len(bits[0]) == 5
        it.xs = self.parse_int(bits[0][3], "xs") if it.has_area else 0
        it.ys = self.parse_int(bits[0][4], "ys") if it.has_area else 0
        it.name = bits[2][0]
        it.mob_class = self.parse_int(bits[3][0], "class")
        it.num = self.parse_int(bits[3][1], "num")
        it.delay1 = 0
        it.delay2 = 0
        it.event = None
        if len(bits[3]) >= 4:
            it.delay1 = self.parse_delay_ms(bits[3][2], "delay1")
            it.delay2 = self.parse_delay_ms(bits[3][3], "delay2")
            if len(bits[3]) >= 5:
                ev = bits[3][4]
                if "::" not in ev:
                    raise self.err("monster: bad event '%s'" % ev)
                it.event = ev
        return it

    def parse_mapflag(self, bits, line0):
        # map|mapflag|flag[|extra,args]
        if len(bits) not in (3, 4):
            raise self.err("mapflag: expect 3 or 4 |component|s")
        if len(bits[0]) != 1:
            raise self.err("mapflag: in |component 1| expect 1 ,component,s")
        if len(bits[2]) != 1:
            raise self.err("mapflag: in |component 3| expect 1 ,component,s")
        it = Item("mapflag", line0)
        it.m = bits[0][0]
        it.flag = bits[2][0]
        it.extra = bits[3] if len(bits) == 4 else []
        return it

    def parse_script(self, bits, hit_brace, line0):
        # function|script|Fun Name{...}
        # -|script|Npc Name|32767{...}
        # map,x,y,dir|script|Npc Name|class[,xs,ys]{...}
        if bits[0][0] == "function":
            if len(bits) != 3:
                raise self.err("script function: expect 3 |component|s")
            if len(bits[2]) != 1:
                raise self.err("script function: in |component 3| expect 1 ,component,s")
            it = Item("script_function", line0)
            it.name = bits[2][0]
        elif bits[0][0] == "-":
            if len(bits) != 4:
                raise self.err("floating script: expect 4 |component|s")
            if len(bits[2]) != 1:
                raise self.err("floating script: in |component 3| expect 1 ,component,s")
            if bits[3] != ["32767"]:
                raise self.err("floating script: |component 4| should be just 32767")
            it = Item("script_none", line0)
            it.name = bits[2][0]
        else:
            if len(bits) != 4:
                raise self.err("map script: expect 4 |component|s")
            if len(bits[0]) != 4:
                raise self.err("map script: in |component 1| expect 4 ,component,s")
            if len(bits[2]) != 1:
                raise self.err("map script: in |component 3| expect 1 ,component,s")
            if len(bits[3]) not in (1, 3):
                raise self.err("map script: in |component 4| expect 1 or 3 ,component,s")
            it = Item("script_map", line0)
            it.m = bits[0][0]
            it.x = self.parse_int(bits[0][1], "x")
            it.y = self.parse_int(bits[0][2], "y")
            it.d = self.parse_int(bits[0][3], "dir")
            it.name = bits[2][0]
            it.npc_class = self.parse_int(bits[3][0], "class")
            it.has_area = len(bits[3]) == 3
            # keep the raw file numbers (the engine stores 2n+1 itself)
            it.xs = self.parse_int(bits[3][1], "xs") if it.has_area else 0
            it.ys = self.parse_int(bits[3][2], "ys") if it.has_area else 0
        it.body, it.body_line = self.read_body(hit_brace)
        return it

    def parse_all(self):
        items = []
        while True:
            it = self.next_item()
            if it is None:
                return items
            items.append(it)


# ---------------------------------------------------------------------------
# old script body tokenizer (comments and strings per script-parse.cpp)

class Tok:
    __slots__ = ("kind", "text", "line", "pos")

    def __init__(self, kind, text, line, pos):
        self.kind = kind  # id | str | num | op
        self.text = text
        self.line = line
        self.pos = pos


ID_RE = re.compile(r"(\$@|\.@|##|[$@.#])?[A-Za-z0-9_]+\$?")
OPS2 = ("<<", ">>", "&&", "||", "==", "!=", "<=", ">=")


def lex_body(body, base_line):
    """Tokenize an old script body. Line numbers are relative to the file
    (base_line = line of the first body character)."""
    toks = []
    i = 0
    line = base_line
    n = len(body)
    while i < n:
        c = body[i]
        if c == "\n":
            line += 1
            i += 1
            continue
        if c in " \t\r":
            i += 1
            continue
        if c == "/" and i + 1 < n and body[i + 1] == "/":
            while i < n and body[i] != "\n":
                i += 1
            continue
        if c == "/" and i + 1 < n and body[i + 1] == "*":
            j = body.find("*/", i + 2)
            if j < 0:
                j = n
                line += body.count("\n", i)
                i = n
                continue
            line += body.count("\n", i, j + 2)
            i = j + 2
            continue
        if c == '"':
            j = i + 1
            buf = []
            while j < n and body[j] != '"':
                if body[j] == "\\" and j + 1 < n:
                    buf.append(body[j + 1])
                    j += 2
                else:
                    buf.append(body[j])
                    j += 1
            toks.append(Tok("str", "".join(buf), line, i))
            i = j + 1
            continue
        m = ID_RE.match(body, i)
        if m and (c.isalnum() or c in "$@.#_"):
            text = m.group(0)
            kind = "num" if re.match(r"^[0-9]+$", text) else "id"
            # hex numbers
            if text.startswith("0x") or text.startswith("0X"):
                kind = "num"
            toks.append(Tok(kind, text, line, i))
            i = m.end()
            continue
        two = body[i:i + 2]
        if two in OPS2:
            toks.append(Tok("op", two, line, i))
            i += 2
            continue
        toks.append(Tok("op", c, line, i))
        i += 1
    return toks


def find_labels(toks):
    """Returns list of (tok_index, name) for statement-start labels."""
    labels = []
    prev_signif = None  # None | ';' | 'label'
    k = 0
    while k < len(toks):
        t = toks[k]
        if (t.kind == "id"
                and re.match(r"^(On|L_|S_)", t.text)
                and not re.match(r"^[$@.#]", t.text)
                and k + 1 < len(toks)
                and toks[k + 1].kind == "op" and toks[k + 1].text == ":"
                and prev_signif in (None, ";", "label")):
            labels.append((k, t.text))
            prev_signif = "label"
            k += 2
            continue
        if t.kind == "op" and t.text == ";":
            prev_signif = ";"
        else:
            prev_signif = t.text if t.kind == "op" else "x"
        k += 1
    return labels


# ---------------------------------------------------------------------------
# audit scanning (doc/lua-api.md section 13 hotspots; lua-engine.md 15.1)

def split_args(toks, start):
    """Splits tokens from start until ';' into top-level comma-separated
    argument token lists. Returns (args, index_of_semicolon_or_end)."""
    args = [[]]
    depth = 0
    k = start
    while k < len(toks):
        t = toks[k]
        if t.kind == "op":
            if t.text in "([":
                depth += 1
            elif t.text in ")]":
                depth -= 1
            elif t.text == ";" and depth <= 0:
                break
            elif t.text == "," and depth == 0:
                args.append([])
                k += 1
                continue
        args[-1].append(t)
        k += 1
    if args == [[]]:
        args = []
    return args, k


def audit_scan(toks, label_kind_of_tok):
    """Returns list of (kind, line, detail). label_kind_of_tok maps token
    index -> segment label kind ('' for the click body)."""
    out = []
    seen = set()

    def add(kind, line, detail=""):
        key = (kind, line, detail)
        if key not in seen:
            seen.add(key)
            out.append((kind, line, detail))

    for k, t in enumerate(toks):
        if t.kind == "op":
            if t.text in ("&", "|", "^", "<<", ">>"):
                add("bitwise-op", t.line,
                    "'%s' expression: C precedence differed from Lua; "
                    "use bit32.*" % t.text)
            continue
        if t.kind != "id":
            continue
        name = t.text
        if name == "sc_start":
            args, _ = split_args(toks, k + 1)
            if len(args) >= 2 and len(args[1]) == 1 and args[1][0].kind == "num":
                tick = int(args[1][0].text, 0)
                if tick < 1000:
                    add("sc_start-tick", t.line,
                        "literal tick %d < 1000 (seconds heuristic)" % tick)
        elif name == "mobcount":
            add("mobcount", t.line,
                "mobcount comparison (old builtin returned count minus one)")
        elif name == "getarraysize":
            add("getarraysize", t.line,
                "returns 0 for an empty array (old returned 1)")
        elif name == "fakenpcname":
            add("fakenpcname", t.line,
                "self:rename really renames (updates registries)")
        elif name == "menu":
            args, _ = split_args(toks, k + 1)
            # entries alternate text,label
            for a_i in range(0, len(args), 2):
                a = args[a_i]
                if len(a) == 1 and a[0].kind == "str" and a[0].text == "":
                    add("menu-empty-entry", t.line,
                        "empty menu entry: entries past it are hidden and "
                        "no longer selectable")
                elif not (len(a) == 1 and a[0].kind == "str"):
                    add("menu-entry-expr", t.line,
                        "menu entry is an expression: now evaluated once "
                        "(old RERUNLINE re-evaluated)")
        elif name == "get":
            if k + 1 < len(toks) and toks[k + 1].kind == "op" \
                    and toks[k + 1].text == "(":
                args, _ = split_args(toks, k + 2)
                if len(args) >= 2:
                    add("get-target", t.line,
                        "get(VAR, target): plain variables now read the "
                        "permanent variable (old read temp regs)")
        elif name in ("monster", "areamonster", "summon"):
            args, _ = split_args(toks, k + 1)
            if args and len(args[0]) == 1 and args[0][0].kind == "str" \
                    and args[0][0].text == "this":
                add("monster-this", t.line,
                    "'\"this\"' map convenience is gone: pass p.map/p.x/p.y")
        elif name == "@menu":
            add("menu-register", t.line,
                "@menu register is replaced by the menu return value")
        elif re.match(r"^@inventorylist_|^@skilllist_|^\$@MobDrop", name):
            add("result-register", t.line,
                "'%s' result register is replaced by return values" % name)
        if name in DIALOG_BUILTINS and label_kind_of_tok:
            seg = label_kind_of_tok.get(k, "")
            if seg and (seg in BROADCAST_LABELS or seg == "OnInit"
                        or TIMER_LABEL_RE.match(seg)
                        or CLOCK_LABEL_RE.match(seg)):
                add("dialog-in-event", t.line,
                    "dialog primitive '%s' inside %s (no player dialog "
                    "in broadcast/timer labels)" % (name, seg))
    return out


# ---------------------------------------------------------------------------
# Lua emission

def lua_quote(s):
    out = ['"']
    for c in s:
        if c == '"':
            out.append('\\"')
        elif c == "\\":
            out.append("\\\\")
        elif c == "\n":
            out.append("\\n")
        else:
            out.append(c)
    out.append('"')
    return "".join(out)


def long_comment_fences(text):
    lvl = 2
    while ("]" + "=" * lvl + "]") in text:
        lvl += 1
    return "--[" + "=" * lvl + "[", "]" + "=" * lvl + "]"


def mangle_name(name, taken):
    m = re.sub(r"[^A-Za-z0-9_]", "_", name)
    if not m or m[0].isdigit():
        m = "_" + m
    if m in LUA_KEYWORDS:
        m = m + "_"
    base = m
    i = 2
    while m in taken and taken[m] != name:
        m = "%s_%d" % (base, i)
        i += 1
    taken[m] = name
    return m


def emit_stub(field, params, seg_text, seg_line0, seg_line1, audits, indent):
    """One stub function value: PORTME() + audits + original source."""
    pre = " " * indent
    lines = []
    lines.append("function(%s)" % params)
    for kind, line, detail in audits:
        lines.append(pre + "    -- AUDIT: %s (line %d): %s"
                     % (kind, line, detail))
    lines.append(pre + "    PORTME()")
    op, cl = long_comment_fences(seg_text)
    lines.append(pre + "    %s original lines %d-%d:" % (op, seg_line0, seg_line1))
    body = seg_text.rstrip("\n")
    lines.append(body if body else "")
    lines.append(pre + "    %s" % cl)
    lines.append(pre + "end")
    return "\n".join(lines)


class FileStats:
    def __init__(self):
        self.data = {"warp": 0, "shop": 0, "monster": 0, "mapflag": 0}
        self.script_npcs = 0
        self.functions = 0
        self.stubs = 0
        self.audits = 0
        self.audit_kinds = {}
        self.mangled = []  # (original, mangled)


def segments_of_body(body, body_line, filename, item_line):
    """Splits a body into (label, text, line0, line1, toks_range) segments at
    top-level On* labels. Returns (segments, toks, label_kind_of_tok).
    Segment label '' is the click body (pos 0)."""
    toks = lex_body(body, body_line)
    labels = find_labels(toks)
    on_labels = [(k, name) for (k, name) in labels if name.startswith("On")]
    # duplicate label check (the old compiler errored)
    seen = set()
    for _, name in labels:
        if name in seen:
            raise ConvertError("%s:%d: duplicate label %s"
                               % (filename, item_line, name))
        seen.add(name)
    bounds = []
    # click segment: body start .. first On label
    first_on_pos = toks[on_labels[0][0]].pos if on_labels else len(body)
    first_on_tok = on_labels[0][0] if on_labels else len(toks)
    bounds.append(("", 0, first_on_pos, 0, first_on_tok))
    for i, (k, name) in enumerate(on_labels):
        end_pos = toks[on_labels[i + 1][0]].pos if i + 1 < len(on_labels) \
            else len(body)
        end_tok = on_labels[i + 1][0] if i + 1 < len(on_labels) else len(toks)
        bounds.append((name, toks[k].pos, end_pos, k, end_tok))
    label_kind_of_tok = {}
    segments = []
    for name, p0, p1, t0, t1 in bounds:
        text = body[p0:p1]
        line0 = body_line + body.count("\n", 0, p0)
        line1 = body_line + body.count("\n", 0, max(p0, p1 - 1))
        for k in range(t0, t1):
            label_kind_of_tok[k] = name
        has_code = any(toks[k].kind != "op" or toks[k].text != ":"
                       for k in range(t0, t1))
        segments.append({
            "label": name, "text": text, "line0": line0, "line1": line1,
            "t0": t0, "t1": t1, "has_code": has_code,
        })
    return segments, toks, label_kind_of_tok


def convert_script_item(it, filename, stats, mangle_taken, funcdefs):
    """Returns (marker_key, lua_text)."""
    if it.kind == "script_function":
        return convert_function_item(it, filename, stats, mangle_taken,
                                     funcdefs)
    segments, toks, seg_of_tok = segments_of_body(
            it.body, it.body_line, filename, it.line)
    audits = audit_scan(toks, seg_of_tok)
    stats.audits += len(audits)
    for kind, _, _ in audits:
        stats.audit_kinds[kind] = stats.audit_kinds.get(kind, 0) + 1
    audits_by_seg = {}
    for a in audits:
        kind, line, detail = a
        # attribute to the segment containing that line
        best = ""
        for s in segments:
            if s["line0"] <= line <= s["line1"]:
                best = s["label"]
                break
        audits_by_seg.setdefault(best, []).append(a)

    fields = []
    fields.append("    name = %s," % lua_quote(it.name))
    if it.kind == "script_map":
        fields.append("    map = %s, x = %d, y = %d, dir = %d,"
                      % (lua_quote(it.m), it.x, it.y, it.d))
        fields.append("    sprite = %d," % it.npc_class)
        if it.has_area:
            fields.append("    xs = %d, ys = %d," % (it.xs, it.ys))

    on_timer = []   # (N, stub)
    events = []     # (label, stub)
    special = {}
    for s in segments:
        label = s["label"]
        if label == "":
            if not s["has_code"]:
                continue
            stub = emit_stub("on_click", "self, p", s["text"],
                             s["line0"], s["line1"],
                             audits_by_seg.get("", []), 4)
            special["on_click"] = stub
            stats.stubs += 1
            continue
        seg_audits = audits_by_seg.get(label, [])
        tm = TIMER_LABEL_RE.match(label)
        if label == "OnTouch":
            special["on_touch"] = emit_stub(label, "self, p", s["text"],
                                            s["line0"], s["line1"],
                                            seg_audits, 4)
        elif label == "OnInit":
            special["on_init"] = emit_stub(label, "self", s["text"],
                                           s["line0"], s["line1"],
                                           seg_audits, 4)
        elif tm and int(tm.group(1)) > 0:
            on_timer.append((int(tm.group(1)),
                             emit_stub(label, "self", s["text"],
                                       s["line0"], s["line1"],
                                       seg_audits, 8)))
        else:
            params = "self" if CLOCK_LABEL_RE.match(label) else "self, p"
            events.append((label,
                           emit_stub(label, params, s["text"],
                                     s["line0"], s["line1"],
                                     seg_audits, 8)))
        stats.stubs += 1

    if "on_click" not in special and it.kind == "script_map" \
            and it.npc_class != 32767 and segments and len(segments) > 1:
        fields.append("    -- AUDIT: body starts at a label; an old click "
                      "ran from position 0 and fell into %s"
                      % segments[1]["label"])
    for key in ("on_click", "on_touch", "on_init"):
        if key in special:
            fields.append("    %s = %s," % (key, special[key]))
    if on_timer:
        lines = ["    on_timer = {"]
        for n, stub in on_timer:
            lines.append("        [%d] = %s," % (n, stub))
        lines.append("    },")
        fields.append("\n".join(lines))
    if events:
        lines = ["    events = {"]
        for label, stub in events:
            if re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", label) \
                    and label not in LUA_KEYWORDS:
                key = label
            else:
                key = "[%s]" % lua_quote(label)
            lines.append("        %s = %s," % (key, stub))
        lines.append("    },")
        fields.append("\n".join(lines))

    stats.script_npcs += 1
    lua = "npc.script{\n" + "\n".join(fields) + "\n}"
    return "script:%s" % it.name, lua


def convert_function_item(it, filename, stats, mangle_taken, funcdefs):
    toks = lex_body(it.body, it.body_line)
    audits = audit_scan(toks, None)
    stats.audits += len(audits)
    for kind, _, _ in audits:
        stats.audit_kinds[kind] = stats.audit_kinds.get(kind, 0) + 1
    mangled = mangle_name(it.name, mangle_taken)
    note = ""
    if mangled != it.name:
        stats.mangled.append((it.name, mangled))
        note = ("-- NOTE: original name %s mangled to %s\n"
                % (lua_quote(it.name), mangled))
    stub = emit_stub(mangled, "self, p", it.body, it.body_line,
                     it.body_line + it.body.count("\n"), audits, 0)
    # emit_stub returns "function(params) ..."; make it a named global
    assert stub.startswith("function(")
    lua = note + "function %s(%s" % (mangled, stub[len("function("):])
    stats.functions += 1
    stats.stubs += 1
    # funcdefs: @-variable reads/writes
    reads, writes = set(), set()
    k = 0
    prev = None
    while k < len(toks):
        t = toks[k]
        if t.kind == "id" and t.text in ("set", "setarray", "cleararray",
                                         "input") \
                and prev in (None, ";", ":"):
            args, _ = split_args(toks, k + 1)
            if args and args[0]:
                a0 = args[0][0]
                if a0.kind == "id" and a0.text.startswith("@") \
                        and not a0.text.startswith("@@"):
                    writes.add(a0.text)
                for a in args[1:]:
                    for at in a:
                        if at.kind == "id" and at.text.startswith("@"):
                            reads.add(at.text)
        elif t.kind == "id" and t.text.startswith("@") \
                and not t.text.startswith("@@"):
            reads.add(t.text)
        prev = t.text if t.kind == "op" else "x"
        k += 1
    funcdefs.append((it.name, mangled, filename,
                     sorted(reads), sorted(writes)))
    return "function:%s" % it.name, lua


def convert_file(src_path, rel, existing_text, stats, funcdefs,
                 mangle_taken=None):
    """Converts one .txt file; returns the .lua text."""
    text = read_text(src_path)
    parser = TopParser(text, rel)
    items = parser.parse_all()

    # existing PORT blocks that are already ported (no PORTME left)
    preserved = {}
    if existing_text is not None:
        for m in re.finditer(
                r"^-- BEGIN PORT (.+?)$\n(.*?)^-- END PORT \1$\n",
                existing_text, re.M | re.S):
            key, block = m.group(1), m.group(0)
            if "PORTME(" not in block:
                preserved[key] = block

    if mangle_taken is None:
        mangle_taken = {}
    out = []
    out.append("-- Converted from %s by tools/lua-port/convert-npc-data.py" % rel)
    out.append("-- Stubs marked PORTME() are regenerated on re-conversion;")
    out.append("-- ported blocks (no PORTME left) are preserved.")
    out.append("")
    for it in items:
        if it.kind == "comment":
            c = it.text
            if c.startswith("//"):
                c = c[2:]
            out.append("--" + c)
            continue
        if it.kind == "warp":
            stats.data["warp"] += 1
            out.append("npc.warp{ map = %s, x = %d, y = %d, xs = %d, ys = %d,"
                       % (lua_quote(it.m), it.x, it.y, it.xs, it.ys))
            out.append("          to_map = %s, to_x = %d, to_y = %d }"
                       % (lua_quote(it.to_m), it.to_x, it.to_y))
            continue
        if it.kind == "shop":
            stats.data["shop"] += 1
            lines = ["npc.shop{ name = %s, map = %s, x = %d, y = %d, dir = %d,"
                     % (lua_quote(it.name), lua_quote(it.m), it.x, it.y, it.d)]
            lines.append("          sprite = %d," % it.npc_class)
            entries = []
            for iname, value in it.items:
                if re.match(r"^[0-9]+$", iname):
                    ilua = iname
                else:
                    ilua = lua_quote(iname)
                if re.match(r"^-?[0-9]+$", value):
                    vlua = value
                else:
                    vlua = lua_quote(value)  # "*N" multiplier form, kept
                entries.append("{%s, %s}" % (ilua, vlua))
            lines.append("          items = { %s } }" % ", ".join(entries))
            out.append("\n".join(lines))
            continue
        if it.kind == "monster":
            stats.data["monster"] += 1
            f = ["map = %s" % lua_quote(it.m), "x = %d" % it.x,
                 "y = %d" % it.y]
            if it.has_area:
                f += ["xs = %d" % it.xs, "ys = %d" % it.ys]
            f += ["name = %s" % lua_quote(it.name),
                  "species = %d" % it.mob_class, "amount = %d" % it.num]
            f += ["delay1 = %d" % it.delay1, "delay2 = %d" % it.delay2]
            if it.event:
                f.append("event = %s" % lua_quote(it.event))
            out.append("npc.monster{ %s }" % ", ".join(f))
            continue
        if it.kind == "mapflag":
            stats.data["mapflag"] += 1
            f = ["map = %s" % lua_quote(it.m), "flag = %s" % lua_quote(it.flag)]
            if it.flag in ("nosave", "resave"):
                if len(it.extra) != 3:
                    raise ConvertError("%s:%d: mapflag %s needs map,x,y"
                                       % (rel, it.line, it.flag))
                f += ["to = %s" % lua_quote(it.extra[0]),
                      "x = %s" % it.extra[1], "y = %s" % it.extra[2]]
            elif it.flag == "mask":
                if len(it.extra) != 1:
                    raise ConvertError("%s:%d: mapflag mask needs one int"
                                       % (rel, it.line))
                f.append("mask = %s" % it.extra[0])
            elif it.extra:
                raise ConvertError("%s:%d: mapflag %s takes no extra args"
                                   % (rel, it.line, it.flag))
            out.append("npc.mapflag{ %s }" % ", ".join(f))
            continue
        # script kinds
        key, lua = convert_script_item(it, rel, stats, mangle_taken, funcdefs)
        block = "-- BEGIN PORT %s\n%s\n-- END PORT %s\n" % (key, lua, key)
        if key in preserved:
            block = preserved[key]
        out.append(block.rstrip("\n"))
    return "\n".join(out).rstrip("\n") + "\n"


# ---------------------------------------------------------------------------
# conf chain

class ConfLine:
    def __init__(self, kind, raw, key=None, value=None):
        self.kind = kind  # raw | kv
        self.raw = raw
        self.key = key
        self.value = value


def parse_conf(path):
    lines = []
    for raw in read_text(path).splitlines():
        s = raw.strip()
        if s and not s.startswith("//") and ":" in raw:
            key, value = raw.split(":", 1)
            lines.append(ConfLine("kv", raw, key.strip(), value.strip()))
        else:
            lines.append(ConfLine("raw", raw))
    return lines


def walk_conf(base, conf_rel, npc_files, conf_files, missing):
    """Recursively walk the conf chain; record npc: files in load order and
    every conf file (for rewriting)."""
    path = os.path.join(base, conf_rel)
    if not os.path.exists(path):
        missing.append(conf_rel)
        return
    lines = parse_conf(path)
    conf_files.append((conf_rel, lines))
    for ln in lines:
        if ln.kind != "kv":
            continue
        if ln.key == "npc":
            npc_files.append(ln.value)
        elif ln.key == "import":
            walk_conf(base, ln.value, npc_files, conf_files, missing)


def rewrite_conf(conf_rel, lines, out_base, is_root):
    out = []
    inserted = False
    for ln in lines:
        if ln.kind == "kv" and ln.key == "npc" and ln.value.endswith(".txt"):
            if is_root and not inserted:
                out.append("npc: npc/_portme.lua")
                inserted = True
            out.append("npc: %s" % (ln.value[:-4] + ".lua"))
        else:
            out.append(ln.raw)
    if is_root and not inserted:
        out.insert(0, "npc: npc/_portme.lua")
    write_text(os.path.join(out_base, conf_rel), "\n".join(out) + "\n")


PORTME_LUA = """\
-- PORTME support for the converted tree (loaded first).
-- Every unported stub calls PORTME(): loading succeeds (so
-- tmwa-map --check-scripts passes on a half-ported tree) and calling
-- raises, making unported paths loud at runtime.
function PORTME()
    error("PORTME: this handler has not been ported to Lua yet", 2)
end
"""


# ---------------------------------------------------------------------------
# main

def main():
    ap = argparse.ArgumentParser(
            description="Convert an old serverdata npc tree to Lua.")
    ap.add_argument("--src", default=DEFAULT_SRC,
                    help="old npc directory (default: %(default)s)")
    ap.add_argument("--out", help="output tree root (mirrors world/map)")
    ap.add_argument("--single", metavar="FILE",
                    help="convert one .txt file and print the Lua to stdout")
    args = ap.parse_args()

    npc_root = os.path.abspath(args.src)
    base = os.path.dirname(npc_root)  # world/map; conf paths are npc/...

    stats = FileStats()
    funcdefs = []

    if args.single:
        rel = os.path.relpath(os.path.abspath(args.single), base)
        try:
            lua = convert_file(args.single, rel, None, stats, funcdefs)
        except ConvertError as e:
            print("error: %s" % e, file=sys.stderr)
            return 1
        sys.stdout.write(lua)
        print("-- single file: %d data entries, %d stubs, %d audits"
              % (sum(stats.data.values()), stats.stubs, stats.audits),
              file=sys.stderr)
        return 0

    if not args.out:
        ap.error("--out is required (or use --single FILE)")
    out_base = os.path.abspath(args.out)

    mangle_taken = {}
    npc_files = []
    conf_files = []
    missing = []
    root_conf = os.path.join(os.path.basename(npc_root), "scripts.conf")
    walk_conf(base, root_conf, npc_files, conf_files, missing)
    if missing:
        for m in missing:
            print("error: conf file not found: %s" % m, file=sys.stderr)
        return 1
    if not conf_files:
        print("error: no scripts.conf found under %s" % npc_root,
              file=sys.stderr)
        return 1

    nfiles = 0
    errors = 0
    for rel in npc_files:
        src_path = os.path.join(base, rel)
        if not rel.endswith(".txt"):
            print("warning: skipping non-.txt npc file %s" % rel,
                  file=sys.stderr)
            continue
        out_path = os.path.join(out_base, rel[:-4] + ".lua")
        existing = None
        if os.path.exists(out_path):
            existing = read_text(out_path)
        try:
            lua = convert_file(src_path, rel, existing, stats, funcdefs,
                               mangle_taken)
        except ConvertError as e:
            print("error: %s" % e, file=sys.stderr)
            errors += 1
            continue
        write_text(out_path, lua)
        nfiles += 1

    for i, (conf_rel, lines) in enumerate(conf_files):
        rewrite_conf(conf_rel, lines, out_base, conf_rel == root_conf)
    write_text(os.path.join(out_base, os.path.basename(npc_root),
                            "_portme.lua"), PORTME_LUA)

    # funcdefs.tsv: per callfunc function, the @ variables it reads/writes
    fd_lines = ["#function\tmangled\tfile\treads\twrites"]
    for name, mangled, filename, reads, writes in funcdefs:
        fd_lines.append("%s\t%s\t%s\t%s\t%s"
                        % (name, mangled, filename,
                           ",".join(reads), ",".join(writes)))
    write_text(os.path.join(out_base, "funcdefs.tsv"),
               "\n".join(fd_lines) + "\n")

    # unreferenced .txt files under the npc tree (conf files walked via
    # import: are referenced too)
    referenced = set(npc_files) | set(rel for rel, _ in conf_files)
    unref = []
    for dirpath, _, filenames in os.walk(npc_root):
        for fn in sorted(filenames):
            if fn.endswith(".txt"):
                rel = os.path.relpath(os.path.join(dirpath, fn), base)
                if rel not in referenced:
                    unref.append(rel)

    print("converted %d npc files (%d conf files rewritten)"
          % (nfiles, len(conf_files)))
    print("data entries: warp %d, shop %d, monster %d, mapflag %d"
          % (stats.data["warp"], stats.data["shop"], stats.data["monster"],
             stats.data["mapflag"]))
    print("script NPCs: %d, functions: %d, stubs: %d"
          % (stats.script_npcs, stats.functions, stats.stubs))
    print("audit markers: %d" % stats.audits)
    for kind in sorted(stats.audit_kinds):
        print("    %-20s %d" % (kind, stats.audit_kinds[kind]))
    if stats.mangled:
        print("mangled function names:")
        for orig, m in stats.mangled:
            print("    %r -> %s" % (orig, m))
    if unref:
        print("unreferenced .txt files (not converted):")
        for rel in unref:
            print("    %s" % rel)
    if errors:
        print("%d files failed" % errors)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
