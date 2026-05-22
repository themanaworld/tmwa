#!/usr/bin/env python3
# Regenerate the "Builtin functions" definition list in doc/tmwa-script.7.adoc
# from the structured doc comments above each builtin_<name> function in
# src/map/script-fun.cpp.
#
# This is a hand-run helper, NOT a build step.  doc/tmwa-script.7.adoc is a
# committed, hand-editable artifact: its prose (Name, Synopsis, Description,
# Variable scopes, See also) is written by humans.  This script only rewrites
# the block delimited by the BEGIN/END marker comments; the markers are
# deliberately distinct strings so neither matches the other nor any prose.
#
# The source of truth for the per-builtin reference is the doc comments
# themselves (see tools/script_builtins.py for the comment format).  To add
# or change a builtin's documentation, edit its doc comment in
# script-fun.cpp and re-run this script.
#
# Usage:
#   tools/gen_script_docs.py            update doc/tmwa-script.7.adoc in place
#   tools/gen_script_docs.py --stdout   print the regenerated file to stdout
#   tools/gen_script_docs.py --check    exit non-zero if the file is stale

import os
import sys

from script_builtins import (
    parse_table, parse_doc_comments, render_adoc_entry, repo_root,
    script_fun_path,
)

# Marker comments that bracket the generated definition list inside
# doc/tmwa-script.7.adoc.  They must be unique: BEGIN must not be a substring
# of END (or of any prose), or text.find() would match the wrong place.
BEGIN = '//>>> GENERATED-BUILTIN-LIST'
END = '//<<< GENERATED-BUILTIN-LIST'


def adoc_path():
    return os.path.join(repo_root(), 'doc', 'tmwa-script.7.adoc')


def extract_block(text):
    """Return the text between the BEGIN/END markers, or None."""
    bi = text.find(BEGIN)
    ei = text.find(END)
    if bi < 0 or ei < 0:
        return None
    return text[bi + len(BEGIN):ei]


def load_builtins():
    """Parse script-fun.cpp; return (entries, docs_by_name, errors).

    entries     - list[TableEntry] in builtin_functions[] order
    docs_by_name- {name: DocComment}
    errors      - list[str], any structural problem found
    """
    with open(script_fun_path(), 'r', encoding='utf-8') as f:
        src = f.read()

    entries = parse_table(src)
    docs, errors = parse_doc_comments(src)

    docs_by_name = {}
    for d in docs:
        if d.name in docs_by_name:
            errors.append('duplicate doc comment for builtin %r' % d.name)
        docs_by_name[d.name] = d

    return entries, docs_by_name, errors


def render_block(entries, docs_by_name):
    """Render the generated definition-list block, in table order."""
    out = ['']
    for e in entries:
        doc = docs_by_name[e.name]
        out.extend(render_adoc_entry(doc))
        out.append('')
    return '\n'.join(out)


def regenerate(text, block):
    """Return the file text with the generated block replaced."""
    bi = text.find(BEGIN)
    ei = text.find(END)
    if bi < 0 or ei < 0:
        raise SystemExit(
            'doc/tmwa-script.7.adoc is missing the BEGIN/END markers')
    return text[:bi + len(BEGIN)] + block + text[ei:]


def main(argv):
    entries, docs_by_name, errors = load_builtins()

    table_names = [e.name for e in entries]
    # Every builtin must have a doc comment before we can render.
    for name in table_names:
        if name not in docs_by_name:
            errors.append('builtin %r has no doc comment' % name)
    for name in docs_by_name:
        if name not in set(table_names):
            errors.append(
                'doc comment for %r has no builtin_functions[] entry' % name)

    if errors:
        sys.stderr.write('cannot regenerate; fix these first:\n  '
                         + '\n  '.join(errors) + '\n')
        return 1

    block = render_block(entries, docs_by_name)

    path = adoc_path()
    if not os.path.exists(path):
        raise SystemExit('doc/tmwa-script.7.adoc does not exist yet; '
                         'create it with the markers first')
    with open(path, 'r', encoding='utf-8') as f:
        text = f.read()
    new_text = regenerate(text, block)

    if '--stdout' in argv:
        sys.stdout.write(new_text)
        return 0
    if '--check' in argv:
        if new_text != text:
            sys.stderr.write(
                'doc/tmwa-script.7.adoc is stale; '
                'run tools/gen_script_docs.py\n')
            return 1
        return 0

    if new_text != text:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new_text)
        sys.stderr.write('updated %s (%d builtins)\n'
                         % (path, len(entries)))
    else:
        sys.stderr.write('doc/tmwa-script.7.adoc already up to date\n')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
