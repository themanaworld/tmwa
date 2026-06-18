#!/usr/bin/env python3
# coding: utf-8

#   check-fwd.py - verify the per-module fwd.hpp headers stay consistent
#
#   Copyright © 2026 The Mana World contributors
#
#   This file is part of The Mana World (Athena server)
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Affero General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Affero General Public License for more details.
#
#   You should have received a copy of the GNU Affero General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Every src/<module>/ directory has an fwd.hpp that forward-declares the
# module's public types and pulls in the fwd.hpp of every other module it
# depends on. Two invariants keep those headers honest; this script checks
# both. They used to live as grep rules in the (now removed) attoconf
# Makefile, which is why a regression in one of them could slip through CI.
#
#   1. check-fwd: nothing in an fwd.hpp is stale.
#        a. every type it forward-declares is actually defined by one of the
#           module's own headers, and
#        b. every "../other/fwd.hpp" it includes is actually used by the
#           module (some non-fwd file includes from ../other/).
#
#   2. rank-fwd: the inverse of 1b. Whenever a module file includes from
#        ../other/, the module's fwd.hpp must forward-include ../other/fwd.hpp.

import os
import re
import sys


SRC_DIR = os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, 'src'))

# Directory prefixes that are not modules with an fwd.hpp, so a dependency on
# them is never expected to be forward-declared.
RANK_IGNORED = frozenset(['..', 'conf-raw', '../conf', 'src/conf'])

# A forward declaration: one or more of these keywords followed by the type
# name and a trailing semicolon. The leading space on " struct " mirrors the
# original rule and lets indented "struct Foo;" lines match too.
DECL_RE = re.compile(r'(?:enum |class | struct |union )+([A-Za-z_0-9]+);$')

# An include of some other module's forward header, e.g. ../generic/fwd.hpp.
FWD_INCLUDE_RE = re.compile(r'#include.*"(.*/)fwd\.hpp"')

# Any quoted include whose path contains a directory component.
PATH_INCLUDE_RE = re.compile(r'#include.*"([^"]*/[^"]*)"')


def read(path):
    with open(path, encoding='utf-8') as f:
        return f.read()


def peers(module_dir, suffixes):
    """Non-fwd files in module_dir (non-recursive) with one of the suffixes."""
    out = []
    for name in sorted(os.listdir(module_dir)):
        if name == 'fwd.hpp':
            continue
        if name.endswith(suffixes):
            out.append(os.path.join(module_dir, name))
    return out


def defined_type(type_name, headers_text):
    """True if some header defines the given type (vs. forward-declaring it)."""
    pattern = re.compile(
        r'\b(?:enum|class|struct|union) ' + re.escape(type_name) + r'\b')
    return any(pattern.search(text) for text in headers_text)


def check_fwd(fwd_path, errors):
    module_dir = os.path.dirname(fwd_path)
    fwd_text = read(fwd_path)
    rel = os.path.relpath(fwd_path, SRC_DIR)

    # 1a. Every forward-declared type must be defined by a module header.
    header_text = [read(p) for p in peers(module_dir, ('.hpp',))]
    for lineno, line in enumerate(fwd_text.splitlines(), 1):
        m = DECL_RE.search(line)
        if m and not defined_type(m.group(1), header_text):
            errors.append('%s:%d: forward-declared type %r is not defined in '
                          'this module' % (rel, lineno, m.group(1)))

    # 1b. Every ../other/fwd.hpp include must be backed by a real use.
    user_text = [read(p) for p in peers(module_dir, ('.hpp', '.tcc', '.cpp'))]
    for lineno, line in enumerate(fwd_text.splitlines(), 1):
        m = FWD_INCLUDE_RE.search(line)
        if m and not any(m.group(1) in text for text in user_text):
            errors.append('%s:%d: includes %sfwd.hpp but nothing in this '
                          'module uses %s' % (rel, lineno, m.group(1),
                                              m.group(1)))


def check_rank(src_path, errors):
    module_dir = os.path.dirname(src_path)
    fwd_path = os.path.join(module_dir, 'fwd.hpp')
    if not os.path.isfile(fwd_path):
        return
    fwd_text = read(fwd_path)
    rel = os.path.relpath(src_path, SRC_DIR)

    deps = set()
    for line in read(src_path).splitlines():
        m = PATH_INCLUDE_RE.search(line)
        if m:
            deps.add(m.group(1).rsplit('/', 1)[0])
    for dep in sorted(deps - RANK_IGNORED):
        if (dep + '/fwd.hpp') not in fwd_text:
            errors.append('%s: includes from %s/ but %s/fwd.hpp does not '
                          'forward-include %s/fwd.hpp'
                          % (rel, dep, os.path.dirname(rel), dep))


def main():
    errors = []
    for dirpath, _dirnames, filenames in os.walk(SRC_DIR):
        if 'fwd.hpp' in filenames:
            check_fwd(os.path.join(dirpath, 'fwd.hpp'), errors)
        for name in sorted(filenames):
            if name == 'fwd.hpp' or name.endswith('_test.cpp'):
                continue
            if name.endswith(('.hpp', '.tcc', '.cpp')):
                check_rank(os.path.join(dirpath, name), errors)

    if errors:
        for error in sorted(errors):
            sys.stderr.write('error: %s\n' % error)
        sys.stderr.write('check-fwd: %d problem(s) found\n' % len(errors))
        return 1
    print('check-fwd: all fwd.hpp headers are consistent')
    return 0


if __name__ == '__main__':
    sys.exit(main())
