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

from script_builtins import parse_builtins, render_signature, repo_root

# Marker comments that bracket the generated definition list inside
# doc/tmwa-script.7.adoc.  They must be unique: BEGIN must not be a substring
# of END (or of any prose), or text.find() would match the wrong place.
BEGIN = '//>>> GENERATED-BUILTIN-LIST'
END = '//<<< GENERATED-BUILTIN-LIST'
TODO = 'TODO: describe this builtin.'


def adoc_path():
    return os.path.join(repo_root(), 'doc', 'tmwa-script.7.adoc')


def parse_existing_descriptions(text):
    """Return {builtin_name: description} for entries already in the file.

    A definition-list entry looks like:

        *name*::
        +
        `signature`
        +
        Some description text.

    We capture the description (everything after the signature line up to the
    next entry) so hand-written prose survives regeneration.
    """
    descriptions = {}
    block = extract_block(text)
    if block is None:
        return descriptions

    # Split into entries: each entry starts with a "*name*::" line.
    entry_re = re.compile(r'^\*([A-Za-z_][\w]*)\*::\s*$', re.MULTILINE)
    matches = list(entry_re.finditer(block))
    for i, m in enumerate(matches):
        name = m.group(1)
        start = m.end()
        end = matches[i + 1].start() if i + 1 < len(matches) else len(block)
        body = block[start:end]
        # Body is:  \n+\n`signature`\n+\n<description>\n
        # Drop the leading "+", signature line and following "+".
        lines = body.splitlines()
        # Trim leading blank lines.
        while lines and not lines[0].strip():
            lines.pop(0)
        # Expect "+"
        if lines and lines[0].strip() == '+':
            lines.pop(0)
        # Expect "`signature`"
        if lines and lines[0].strip().startswith('`'):
            lines.pop(0)
        # Expect "+"
        if lines and lines[0].strip() == '+':
            lines.pop(0)
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
    """Render the generated definition-list block (between the markers)."""
    out = ['']
    for b in builtins:
        desc = descriptions.get(b.name, TODO)
        out.append('*%s*::' % b.name)
        out.append('+')
        out.append('`%s`' % render_signature(b))
        out.append('+')
        out.append(desc)
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
