#!/usr/bin/env python3
# Shared helper for the script-language documentation tooling.
#
# This module knows how to parse the authoritative builtin_functions[] table
# in src/map/script-fun.cpp and how to turn an arg-signature string into a
# readable function signature.  It is used by:
#
#   tools/gen_script_docs.py    - regenerates the builtin skeleton in
#                                 doc/tmwa-script.7.adoc (run by hand)
#   tools/check_script_docs.py  - the ctest drift check
#
# It is deliberately NOT run at build time; doc/tmwa-script.7.adoc is a
# committed, hand-editable artifact.

import os
import re

# Characters that may appear in a BUILTIN() arg-signature string.
# See the BUILTIN(name, "argsig", ret) entries near the end of
# src/map/script-fun.cpp.  The individual type characters are intent /
# documentation; script-parse.cpp only enforces argument *count* via the
# '?' (optional) and '*' (variadic) markers.
ARG_LEGEND = {
    'i': 'int',
    's': 'str',
    'e': 'expr',
    'M': 'map',
    'x': 'x',
    'y': 'y',
    'L': 'label',
    'F': 'func',
    'N': 'var',
    'I': 'item',
    'P': 'player',
    'E': 'event',
    't': 'timer',
    'm': 'mob',
    'T': 'status',
}

# Meaning of the trailing ret character of a BUILTIN() entry.
# See script-call.cpp run_func() and script-parse.cpp for the checks.
RET_LEGEND = {
    '\0': 'none (statement)',
    'i': 'int',
    's': 'str',
    'v': 'variant (int or str)',
    'r': 'variable reference',
    'l': 'position',
    '.': 'variant (may be absent)',
}


def repo_root():
    """Return the absolute path of the repository root."""
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.dirname(here)


def script_fun_path():
    return os.path.join(repo_root(), 'src', 'map', 'script-fun.cpp')


# A single BUILTIN() entry; ret is a one-character string ('' means '\0').
_BUILTIN_RE = re.compile(
    r'BUILTIN\(\s*(\w+)\s*,\s*"([^"]*)"_s\s*,\s*'
    r"(?:'((?:\\0)|.)'|\"([^\"]*)\")\s*\)"
)


class Builtin(object):
    __slots__ = ('name', 'argsig', 'ret')

    def __init__(self, name, argsig, ret):
        self.name = name
        self.argsig = argsig
        self.ret = ret


def parse_builtins(path=None):
    """Parse builtin_functions[] from script-fun.cpp.

    Returns a list of Builtin in source order.
    """
    if path is None:
        path = script_fun_path()
    with open(path, 'r', encoding='utf-8') as f:
        text = f.read()

    # Restrict to the builtin_functions[] table so we never pick up a stray
    # BUILTIN(...) usage elsewhere (the macro is defined just above it).
    start = text.find('builtin_functions[]')
    if start < 0:
        raise SystemExit('could not find builtin_functions[] in %s' % path)
    end = text.find('};', start)
    if end < 0:
        raise SystemExit('could not find end of builtin_functions[] table')
    table = text[start:end]

    builtins = []
    for m in _BUILTIN_RE.finditer(table):
        name = m.group(1)
        argsig = m.group(2)
        ret = m.group(3)
        if ret == r'\0' or ret is None:
            ret = '\0'
        builtins.append(Builtin(name, argsig, ret))
    return builtins


def render_signature(b):
    """Render a Builtin as a readable signature string.

    Example: monster(map, x, y, str, mob, int[, expr])

    The arg-signature string is processed left to right:

      * a type character (see ARG_LEGEND) contributes one argument;
      * '?' marks the rest of the argument list as optional;
      * '*' makes the preceding argument repeatable, rendered as '...';
        when no type precedes it the variadic is of unspecified type.

    A '?' that is immediately followed by '*' (as in call's "F?*")
    describes an optional, repeatable trailing argument.  When '?' has no
    explicit type character of its own the argument type is unspecified
    and rendered as 'expr'.
    """
    chars = list(b.argsig)
    parts = []          # rendered argument tokens
    optional_from = None  # index in parts where the optional tail starts
    i = 0
    while i < len(chars):
        ch = chars[i]
        if ch == '?':
            if optional_from is None:
                optional_from = len(parts)
            # A bare '?' (no type before, not turning a previous arg
            # variadic) introduces an optional argument of any type.
            nxt = chars[i + 1] if i + 1 < len(chars) else ''
            if nxt == '*':
                parts.append('expr, ...')
                i += 2
                continue
            if nxt != '?' and nxt in ARG_LEGEND:
                # '?' just opens the optional run; the next char is the
                # actual argument type.
                i += 1
                continue
            # standalone optional argument
            parts.append('expr')
            i += 1
            continue
        if ch == '*':
            if parts and not parts[-1].endswith(', ...'):
                parts[-1] = parts[-1] + ', ...'
            elif not parts:
                parts.append('...')
            # A run of '*' (as in menu's "sL**") still renders as a single
            # trailing '...'; the preceding arguments form the repeated tuple.
            i += 1
            continue
        parts.append(ARG_LEGEND.get(ch, ch))
        i += 1

    if optional_from is None or optional_from >= len(parts):
        arglist = ', '.join(parts)
    else:
        required = ', '.join(parts[:optional_from])
        optional = ', '.join(parts[optional_from:])
        if required:
            arglist = required + '[, ' + optional + ']'
        else:
            arglist = '[' + optional + ']'

    sig = '%s(%s)' % (b.name, arglist)
    if b.ret != '\0':
        sig += ' -> ' + RET_LEGEND.get(b.ret, repr(b.ret))
    return sig
