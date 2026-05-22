#!/usr/bin/env python3
# Drift check for the script-language documentation.
#
# The authoritative documentation for each builtin is a structured doc
# comment above its builtin_<name> function in src/map/script-fun.cpp (see
# tools/script_builtins.py for the format).  doc/tmwa-script.7.adoc is a
# committed artifact generated from those comments by tools/gen_script_docs.py.
#
# Registered with CTest (see CMakeLists.txt).  The test fails if:
#
#   * a builtin in builtin_functions[] has no doc comment;
#   * a doc comment names a builtin not in builtin_functions[];
#   * a doc comment is structurally malformed;
#   * the documented argument shape (required / optional / variadic counts)
#     disagrees with the BUILTIN() arg-signature the interpreter enforces;
#   * a statement builtin claims a return value, or vice versa;
#   * the committed doc/tmwa-script.7.adoc does not match what the doc
#     comments currently generate.
#
# This keeps the man page honest: it cannot silently drift from the code the
# way Hercules' script_commands.txt or TMWA's old doc/script_ref.txt did.
#
# If this test fails, fix the doc comments in script-fun.cpp, then run
# tools/gen_script_docs.py to refresh doc/tmwa-script.7.adoc.

import sys

from script_builtins import argsig_shape, RET_IS_VALUE
from gen_script_docs import load_builtins, adoc_path, regenerate, render_block


def main():
    entries, docs_by_name, errors = load_builtins()
    table_names = [e.name for e in entries]
    table_set = set(table_names)
    doc_set = set(docs_by_name)

    # Missing doc comments.
    for name in sorted(table_set - doc_set):
        errors.append('builtin %r has no doc comment in script-fun.cpp'
                      % name)

    # Doc comments for builtins that no longer exist.
    for name in sorted(doc_set - table_set):
        errors.append('doc comment for %r has no builtin_functions[] entry'
                      % name)

    # Per-builtin shape checks for the builtins that do have a doc comment.
    by_table = {e.name: e for e in entries}
    for name in sorted(table_set & doc_set):
        e = by_table[name]
        d = docs_by_name[name]
        req, opt, variadic = argsig_shape(e.argsig)

        doc_req = sum(1 for p in d.params if not p.optional)
        doc_opt = sum(1 for p in d.params if p.optional and not p.rest)
        doc_var = any(p.rest for p in d.params)

        if doc_req != req or doc_opt != opt or doc_var != variadic:
            errors.append(
                '%s: doc comment shape (%d req, %d opt, variadic=%s) '
                'disagrees with argsig %r (%d req, %d opt, variadic=%s)'
                % (name, doc_req, doc_opt, doc_var,
                   e.argsig, req, opt, variadic))

        ret_is_value = RET_IS_VALUE.get(e.ret, True)
        if ret_is_value and d.ret_type is None:
            errors.append(
                '%s: builtin returns a value but doc comment says "@ret none"'
                % name)
        if not ret_is_value and d.ret_type is not None:
            errors.append(
                '%s: builtin is a statement but doc comment declares a '
                'return type %r' % (name, d.ret_type))

    if errors:
        sys.stderr.write(
            'script documentation drift detected:\n\n  '
            + '\n  '.join(errors)
            + '\n\nFix the doc comments in src/map/script-fun.cpp.\n')
        return 1

    # All doc comments are sound: the committed man page must match them.
    block = render_block(entries, docs_by_name)
    try:
        with open(adoc_path(), 'r', encoding='utf-8') as f:
            text = f.read()
    except OSError as ex:
        sys.stderr.write('cannot read doc/tmwa-script.7.adoc: %s\n' % ex)
        return 1
    if regenerate(text, block) != text:
        sys.stderr.write(
            'doc/tmwa-script.7.adoc is out of date with the doc comments '
            'in script-fun.cpp.\n'
            'Run tools/gen_script_docs.py to regenerate it.\n')
        return 1

    sys.stdout.write(
        'script docs in sync: %d builtins documented.\n' % len(entries))
    return 0


if __name__ == '__main__':
    sys.exit(main())
