#!/usr/bin/env python3
# Regenerate the "Builtin functions" definition list in doc/tmwa-script.7.adoc
# from the authoritative builtin_functions[] table in src/map/script-fun.cpp.
#
# This is a hand-run helper, NOT a build step.  doc/tmwa-script.7.adoc is a
# committed, hand-editable artifact: the prose (Name, Synopsis, Description,
# per-builtin descriptions) is written by humans.  This script only rewrites
# the block delimited by the BEGIN/END marker comments (see BEGIN/END below);
# the markers are deliberately distinct strings so neither matches the other
# nor any prose.
#
# When a builtin already has a hand-written description, that description is
# preserved across regeneration.  New builtins get a "TODO" placeholder so the
# drift check (tools/check_script_docs.py) still passes and the missing prose
# is obvious.
#
# Usage:
#   tools/gen_script_docs.py            update doc/tmwa-script.7.adoc in place
#   tools/gen_script_docs.py --stdout   print the regenerated file to stdout
#   tools/gen_script_docs.py --check    exit non-zero if the file is stale

import os
import re
import sys

from script_builtins import parse_builtins, render_adoc_term, repo_root

# Marker comments that bracket the generated definition list inside
# doc/tmwa-script.7.adoc.  They must be unique: BEGIN must not be a substring
# of END (or of any prose), or text.find() would match the wrong place.
BEGIN = '//>>> GENERATED-BUILTIN-LIST'
END = '//<<< GENERATED-BUILTIN-LIST'
TODO = 'TODO: describe this builtin.'


def adoc_path():
    return os.path.join(repo_root(), 'doc', 'tmwa-script.7.adoc')


# A definition-list term line, e.g.
#   *setparam*(_param_, _value_[, _gid_]) -> int::
# We only need the builtin name (group 1); everything up to the trailing
# "::" is the mechanically rendered signature.
_TERM_RE = re.compile(r'^\*([A-Za-z_][\w]*)\*\([^\n]*::\s*$', re.MULTILINE)


def parse_existing_descriptions(text):
    """Return {builtin_name: description} for entries already in the file.

    A definition-list entry is a single term line followed by an indented
    description:

        *name*(_args_) -> type::
          Some description text, possibly spanning
          several indented lines.

    We capture the description (the indented continuation lines up to the
    next term) so hand-written prose survives regeneration.
    """
    descriptions = {}
    block = extract_block(text)
    if block is None:
        return descriptions

    matches = list(_TERM_RE.finditer(block))
    for i, m in enumerate(matches):
        name = m.group(1)
        start = m.end()
        end = matches[i + 1].start() if i + 1 < len(matches) else len(block)
        body = block[start:end]
        # The body is the indented description.  Strip the common two-space
        # indentation and surrounding blank lines.
        lines = []
        for line in body.splitlines():
            if line.startswith('  '):
                lines.append(line[2:])
            else:
                lines.append(line)
        desc = '\n'.join(lines).strip()
        if desc:
            descriptions[name] = desc
    return descriptions


def extract_block(text):
    """Return the text between the BEGIN/END markers, or None."""
    bi = text.find(BEGIN)
    ei = text.find(END)
    if bi < 0 or ei < 0:
        return None
    return text[bi + len(BEGIN):ei]


def render_block(builtins, descriptions):
    """Render the generated definition-list block (between the markers).

    Each builtin becomes exactly one definition-list entry: a term line
    holding the marked-up signature, followed by the indented description.
    """
    out = ['']
    for b in builtins:
        desc = descriptions.get(b.name, TODO)
        out.append('%s::' % render_adoc_term(b))
        # Indent every description line by two spaces so it is part of the
        # definition-list item.
        for line in desc.splitlines():
            out.append(('  ' + line).rstrip())
        out.append('')
    return '\n'.join(out)


def regenerate(text, builtins):
    """Return the file text with the generated block refreshed."""
    bi = text.find(BEGIN)
    ei = text.find(END)
    if bi < 0 or ei < 0:
        raise SystemExit(
            'doc/tmwa-script.7.adoc is missing the BEGIN/END markers')
    descriptions = parse_existing_descriptions(text)
    block = render_block(builtins, descriptions)
    return text[:bi + len(BEGIN)] + block + text[ei:]


def main(argv):
    builtins = parse_builtins()
    path = adoc_path()
    if not os.path.exists(path):
        raise SystemExit('doc/tmwa-script.7.adoc does not exist yet; '
                         'create it with the markers first')
    with open(path, 'r', encoding='utf-8') as f:
        text = f.read()
    new_text = regenerate(text, builtins)

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
                         % (path, len(builtins)))
    else:
        sys.stderr.write('doc/tmwa-script.7.adoc already up to date\n')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
