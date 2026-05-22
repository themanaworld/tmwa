#!/usr/bin/env python3
# Drift check between the authoritative builtin_functions[] table in
# src/map/script-fun.cpp and the documented builtins in
# doc/tmwa-script.7.adoc.
#
# Registered with CTest (see CMakeLists.txt).  The test fails if:
#
#   * a builtin in script-fun.cpp has no entry in tmwa-script.7.adoc, or
#   * tmwa-script.7.adoc documents a builtin that no longer exists, or
#   * a documented signature does not match the one derived from the
#     current arg-signature string.
#
# This is the key improvement over Hercules' script_commands.txt (which
# silently drifts out of sync) and over TMWA's own deleted doc/script_ref.txt.
#
# If this test fails, run tools/gen_script_docs.py to refresh the generated
# builtin list, then fill in any new TODO descriptions by hand.

import re
import sys

from script_builtins import parse_builtins, render_signature
from gen_script_docs import adoc_path, extract_block


def documented_builtins():
    """Return {name: signature} parsed from the generated block."""
    with open(adoc_path(), 'r', encoding='utf-8') as f:
        text = f.read()
    block = extract_block(text)
    if block is None:
        raise SystemExit(
            'doc/tmwa-script.7.adoc is missing the BEGIN/END markers')

    result = {}
    entry_re = re.compile(
        r'^\*([A-Za-z_][\w]*)\*::\s*\n\+\n`([^`]*)`',
        re.MULTILINE)
    for m in entry_re.finditer(block):
        result[m.group(1)] = m.group(2)
    return result


def main():
    builtins = parse_builtins()
    table = {b.name: render_signature(b) for b in builtins}
    documented = documented_builtins()

    table_names = set(table)
    doc_names = set(documented)

    errors = []

    missing = sorted(table_names - doc_names)
    if missing:
        errors.append(
            'builtins in script-fun.cpp but NOT documented in '
            'tmwa-script.7.adoc:\n  ' + '\n  '.join(missing))

    extra = sorted(doc_names - table_names)
    if extra:
        errors.append(
            'builtins documented in tmwa-script.7.adoc but NOT in '
            'script-fun.cpp:\n  ' + '\n  '.join(extra))

    mismatched = []
    for name in sorted(table_names & doc_names):
        if table[name] != documented[name]:
            mismatched.append(
                '  %s\n    table: %s\n    docs:  %s'
                % (name, table[name], documented[name]))
    if mismatched:
        errors.append(
            'signatures disagree between script-fun.cpp and '
            'tmwa-script.7.adoc:\n' + '\n'.join(mismatched))

    if errors:
        sys.stderr.write(
            'script documentation drift detected:\n\n'
            + '\n\n'.join(errors)
            + '\n\nRun tools/gen_script_docs.py to regenerate the builtin '
              'list in doc/tmwa-script.7.adoc.\n')
        return 1

    sys.stdout.write(
        'script docs in sync: %d builtins documented.\n' % len(table))
    return 0


if __name__ == '__main__':
    sys.exit(main())
