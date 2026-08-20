#!/usr/bin/env python3
# lint.py - lint tmwa Lua content files for the portable subset.
#
# See doc/lua-engine.md section 15.2. Checks, per file:
#   * luac -p (syntax), when luac 5.4 is available
#   * token scan for the forbidden subset: goto / :: labels, native bitwise
#     operators (& | ~ << >>) and integer division //, <const> / <close>,
#     require / load / loadfile / dofile / io. / debug. / package., os.*
#     beyond time/clock/date; 'continue' as an identifier is a warning
#   * unknown globals: free reads (GETTABUP on _ENV from luac -l) checked
#     against the API surface (a saved `tmwa-map --dump-lua-api` output),
#     const db names, content-defined globals (SETTABUP, two passes over all
#     given files), and PORTME
#   * PORTME stub report (informational)
#   * warnings: a file indexing tables with both literal [0] and [1]
#     (mixed-base hazard), handle values assigned into vars tables
#
# Exit status: 1 if any error finding, else 0.
#
# Usage:
#   lint.py [--api DUMPFILE] [--const-db FILE ...] [--luac LUAC] file.lua ...

import argparse
import os
import re
import shutil
import subprocess
import sys

LUA_KEYWORDS = {
    "and", "break", "do", "else", "elseif", "end", "false", "for", "function",
    "goto", "if", "in", "local", "nil", "not", "or", "repeat", "return",
    "then", "true", "until", "while",
}

FORBIDDEN_GLOBALS = {"require", "load", "loadfile", "dofile", "io", "debug",
                     "package"}
OS_ALLOWED = {"time", "clock", "date"}


class Finding:
    def __init__(self, level, path, line, msg):
        self.level = level  # error | warning | info
        self.path = path
        self.line = line
        self.msg = msg

    def __str__(self):
        return "%s:%s: %s: %s" % (self.path, self.line, self.level, self.msg)


# ---------------------------------------------------------------------------
# Lua tokenizer (enough for the subset scan; strings and comments exact)

class Tok:
    __slots__ = ("kind", "text", "line")

    def __init__(self, kind, text, line):
        self.kind = kind  # id | str | num | op
        self.text = text
        self.line = line


def long_bracket_level(s, i):
    """If s[i:] starts a long bracket [=*[, return its level, else None."""
    if s[i] != "[":
        return None
    j = i + 1
    while j < len(s) and s[j] == "=":
        j += 1
    if j < len(s) and s[j] == "[":
        return j - i - 1
    return None


def lex_lua(src, path, findings):
    toks = []
    i = 0
    line = 1
    n = len(src)
    while i < n:
        c = src[i]
        if c == "\n":
            line += 1
            i += 1
            continue
        if c in " \t\r":
            i += 1
            continue
        if src.startswith("--", i):
            lvl = long_bracket_level(src, i + 2) if i + 2 < n else None
            if lvl is not None:
                close = "]" + "=" * lvl + "]"
                j = src.find(close, i + 2)
                if j < 0:
                    findings.append(Finding("error", path, line,
                                            "unterminated long comment"))
                    break
                line += src.count("\n", i, j + len(close))
                i = j + len(close)
            else:
                j = src.find("\n", i)
                i = n if j < 0 else j
            continue
        if c == "[":
            lvl = long_bracket_level(src, i)
            if lvl is not None:
                close = "]" + "=" * lvl + "]"
                start = i + lvl + 2
                j = src.find(close, start)
                if j < 0:
                    findings.append(Finding("error", path, line,
                                            "unterminated long string"))
                    break
                toks.append(Tok("str", src[start:j], line))
                line += src.count("\n", i, j + len(close))
                i = j + len(close)
                continue
            toks.append(Tok("op", "[", line))
            i += 1
            continue
        if c in "'\"":
            q = c
            j = i + 1
            buf = []
            while j < n and src[j] != q:
                if src[j] == "\n":
                    findings.append(Finding("error", path, line,
                                            "unterminated string"))
                    break
                if src[j] == "\\" and j + 1 < n:
                    buf.append(src[j:j + 2])
                    j += 2
                else:
                    buf.append(src[j])
                    j += 1
            toks.append(Tok("str", "".join(buf), line))
            i = j + 1
            continue
        m = re.match(r"[A-Za-z_][A-Za-z0-9_]*", src[i:])
        if m:
            toks.append(Tok("id", m.group(0), line))
            i += len(m.group(0))
            continue
        m = re.match(r"0[xX][0-9a-fA-F]+|[0-9]+(\.[0-9]*)?([eE][+-]?[0-9]+)?",
                     src[i:])
        if m:
            toks.append(Tok("num", m.group(0), line))
            i += len(m.group(0))
            continue
        for op in ("...", "..", "::", "<<", ">>", "//", "==", "~=", "<=",
                   ">="):
            if src.startswith(op, i):
                toks.append(Tok("op", op, line))
                i += len(op)
                break
        else:
            toks.append(Tok("op", c, line))
            i += 1
    return toks


# ---------------------------------------------------------------------------
# forbidden-subset scan

def scan_subset(toks, path, findings):
    portme = 0
    idx_literals = set()
    for k, t in enumerate(toks):
        nxt = toks[k + 1] if k + 1 < len(toks) else None
        prv = toks[k - 1] if k > 0 else None
        if t.kind == "id":
            if t.text == "goto":
                findings.append(Finding("error", path, t.line,
                                        "goto is not in the portable subset"))
            elif t.text == "continue":
                findings.append(Finding("warning", path, t.line,
                                        "'continue' as an identifier (Luau "
                                        "keyword hazard)"))
            elif t.text == "PORTME":
                portme += 1
            elif t.text in FORBIDDEN_GLOBALS and not (
                    (prv and prv.kind == "op" and prv.text in (".", ":"))
                    # table-constructor key or assignment target, not a
                    # read of the forbidden global ('==' is one token)
                    or (nxt and nxt.kind == "op" and nxt.text == "=")):
                findings.append(Finding("error", path, t.line,
                                        "'%s' is not available in the "
                                        "sandbox" % t.text))
            elif t.text == "os" and not (
                    (prv and prv.kind == "op" and prv.text in (".", ":"))
                    or (nxt and nxt.kind == "op" and nxt.text == "=")):
                if nxt and nxt.kind == "op" and nxt.text == ".":
                    mem = toks[k + 2] if k + 2 < len(toks) else None
                    if mem is None or mem.kind != "id" \
                            or mem.text not in OS_ALLOWED:
                        findings.append(Finding(
                                "error", path, t.line,
                                "os.%s: only os.time/os.clock/os.date are "
                                "allowed" % (mem.text if mem else "?")))
                else:
                    findings.append(Finding("error", path, t.line,
                                            "bare 'os' reference"))
        elif t.kind == "op":
            if t.text == "::":
                findings.append(Finding("error", path, t.line,
                                        "'::' label (goto) is not in the "
                                        "portable subset"))
            elif t.text in ("<<", ">>", "//"):
                findings.append(Finding("error", path, t.line,
                                        "native operator '%s' is not in the "
                                        "portable subset (use bit32/idiv)"
                                        % t.text))
            elif t.text in ("&", "|"):
                findings.append(Finding("error", path, t.line,
                                        "native bitwise operator '%s' is not "
                                        "in the portable subset (use bit32)"
                                        % t.text))
            elif t.text == "~":
                findings.append(Finding("error", path, t.line,
                                        "native bitwise operator '~' is not "
                                        "in the portable subset (use bit32)"))
            elif t.text == "<" and nxt and nxt.kind == "id" \
                    and nxt.text in ("const", "close"):
                after = toks[k + 2] if k + 2 < len(toks) else None
                if after and after.kind == "op" and after.text == ">":
                    findings.append(Finding("error", path, t.line,
                                            "<%s> attribute is not in the "
                                            "portable subset" % nxt.text))
            elif t.text == "[" and nxt and nxt.kind == "num" \
                    and nxt.text in ("0", "1"):
                after = toks[k + 2] if k + 2 < len(toks) else None
                if after and after.kind == "op" and after.text == "]":
                    idx_literals.add(nxt.text)
    if idx_literals == {"0", "1"}:
        findings.append(Finding("warning", path, 0,
                                "file indexes tables with both literal [0] "
                                "and [1] (mixed-base hazard)"))
    return portme


HANDLE_ASSIGN_RE = re.compile(
        r"\.vars(\.[A-Za-z0-9_]+|\[[^\]\n]+\])\s*=\s*"
        r"(p\b(?![.\[:])|self\b(?![.\[:])|being\s*\(|npc\.get\s*\(|npc\.byid\s*\()")


def scan_handle_assign(src, path, findings):
    for m in HANDLE_ASSIGN_RE.finditer(src):
        line = src.count("\n", 0, m.start()) + 1
        findings.append(Finding("warning", path, line,
                                "handle-typed value assigned into a vars "
                                "table (handles are not storable)"))


# ---------------------------------------------------------------------------
# globals via luac -l

GETTABUP_RE = re.compile(r"GETTABUP\s.*;\s*_ENV\s+\"([^\"]+)\"")
SETTABUP_RE = re.compile(r"SETTABUP\s.*;\s*_ENV\s+\"([^\"]+)\"")


def luac_listing(luac, path):
    r = subprocess.run([luac, "-p", "-l", path],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:
        return None, r.stderr.decode("utf-8", "replace").strip()
    return r.stdout.decode("utf-8", "replace"), None


def listing_globals(listing):
    reads = []
    writes = []
    for ln in listing.splitlines():
        m = GETTABUP_RE.search(ln)
        if m:
            lm = re.match(r"\s*\d+\s+\[(\d+)\]", ln)
            reads.append((m.group(1), int(lm.group(1)) if lm else 0))
        m = SETTABUP_RE.search(ln)
        if m:
            writes.append(m.group(1))
    return reads, writes


# ---------------------------------------------------------------------------
# API surface / const db loading

def load_api(path):
    names = set()
    for ln in open(path, encoding="utf-8", errors="replace"):
        parts = ln.split()
        if len(parts) >= 2 and parts[0] in ("global", "const", "param"):
            names.add(parts[1])
    return names


def load_const_db(path):
    names = set()
    for ln in open(path, encoding="utf-8", errors="replace"):
        s = ln.strip()
        if not s or s.startswith("//"):
            continue
        names.add(s.split()[0])
    return names


# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(
            description="Lint tmwa Lua content files (portable subset).")
    ap.add_argument("--api", help="saved output of tmwa-map --dump-lua-api")
    ap.add_argument("--const-db", action="append", default=[],
                    help="const db file (db/const.txt style); repeatable")
    ap.add_argument("--luac", default="luac", help="luac 5.4 binary")
    ap.add_argument("files", nargs="+", help=".lua content files")
    args = ap.parse_args()

    findings = []
    allowed = {"PORTME"}
    if args.api:
        allowed |= load_api(args.api)
    for cdb in args.const_db:
        allowed |= load_const_db(cdb)

    have_luac = shutil.which(args.luac) is not None
    if not have_luac:
        print("note: %s not found; skipping syntax and unknown-global checks"
              % args.luac, file=sys.stderr)

    sources = {}
    for path in args.files:
        try:
            sources[path] = open(path, encoding="utf-8",
                                 errors="surrogateescape").read()
        except OSError as e:
            findings.append(Finding("error", path, 0, "cannot read: %s" % e))

    # pass 1: token scans + collect content-defined globals
    listings = {}
    content_defined = set()
    portme_total = 0
    for path, src in sorted(sources.items()):
        toks = lex_lua(src, path, findings)
        portme = scan_subset(toks, path, findings)
        if portme:
            findings.append(Finding("info", path, 0,
                                    "%d PORTME stub call%s remaining"
                                    % (portme, "" if portme == 1 else "s")))
            portme_total += portme
        scan_handle_assign(src, path, findings)
        if have_luac:
            listing, err = luac_listing(args.luac, path)
            if listing is None:
                findings.append(Finding("error", path, 0,
                                        "luac: %s" % err))
            else:
                listings[path] = listing
                _, writes = listing_globals(listing)
                content_defined.update(writes)

    # pass 2: unknown global reads
    if args.api and have_luac:
        for path in sorted(listings):
            reads, _ = listing_globals(listings[path])
            for name, line in reads:
                if name not in allowed and name not in content_defined:
                    findings.append(Finding("error", path, line,
                                            "unknown global '%s'" % name))
    elif not args.api:
        print("note: no --api file; skipping unknown-global check",
              file=sys.stderr)

    errors = sum(1 for f in findings if f.level == "error")
    warnings = sum(1 for f in findings if f.level == "warning")
    for f in findings:
        print(f)
    print("lint: %d file(s), %d error(s), %d warning(s), %d PORTME stub(s)"
          % (len(sources), errors, warnings, portme_total))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
