#!/usr/bin/env python3
# Shared helper for the script-language documentation tooling.
#
# The authoritative documentation for each script builtin is a structured
# doc comment placed immediately above its builtin_<name> function in
# src/map/script-fun.cpp.  This module knows how to:
#
#   * parse the builtin_functions[] table (names and BUILTIN() argsigs);
#   * parse the structured doc comments;
#   * cross-check the two and render an AsciiDoc definition-list entry.
#
# It is used by:
#
#   tools/gen_script_docs.py    - regenerates the builtin reference in
#                                 doc/tmwa-script.7.adoc (run by hand)
#   tools/check_script_docs.py  - the ctest drift check
#
# It is deliberately NOT run at build time; doc/tmwa-script.7.adoc is a
# committed, hand-editable artifact.
#
# ----------------------------------------------------------------------------
# The doc-comment format
# ----------------------------------------------------------------------------
#
# Every builtin_<name> function is preceded by a block comment in the usual
# TMWA /*===...===*/ house style.  A comment counts as a builtin doc comment
# when one of its lines is an "@doc" tag.  The shape is:
#
#   /*====================
#    * One-line summary of what the builtin does.
#    *
#    * @doc name
#    * @arg     argname: type; meaning of the argument.
#    * @optarg  argname: type; meaning of the optional argument.
#    * @rest    argname: type; meaning of a repeatable trailing argument.
#    * @ret     type; meaning of the return value.
#    *====================*/
#
# Tag reference:
#
#   @doc <name>
#       Marks the comment as documentation for builtin_<name>.  The name
#       must match a builtin in builtin_functions[].  Exactly one per
#       comment.
#
#   @arg <name>: <type>; <description>
#       A required positional argument.  <name> is a meaningful identifier
#       (rendered in italics in the man page), <type> is a short type word
#       (GID, map, item, coordinate, amount, ...), <description> is free
#       text.  A long <description> may wrap onto further comment lines:
#       any non-tag line that follows is joined onto it until a blank line
#       or the next tag.
#
#   @optarg <name>: <type>; <description>
#       Like @arg, but the argument is optional; it is rendered inside
#       [square brackets].  All @optarg tags must follow all @arg tags.
#
#   @rest <name>: <type>; <description>
#       A repeatable trailing argument, rendered as "name..." (inside the
#       brackets when it is optional, see below).  At most one @rest tag,
#       and it must be last.  A @rest after the first @optarg (or with no
#       @arg/@optarg of its own) is an optional variadic.
#
#   @ret none
#   @ret <type>; <description>
#       The return value.  "none" marks a statement-only builtin.
#       Exactly one @ret per comment.  The <description> may wrap the same
#       way an @arg description does.
#
#   @desc <text>
#       An extra description paragraph, shown after the summary.  Optional
#       and repeatable; consecutive lines join into one paragraph and a
#       blank line starts a new one.
#
# The summary is every comment line above the first tag, joined into one
# paragraph.  Each tag value (an @arg/@optarg/@rest/@ret description, or an
# @desc paragraph) may wrap across following comment lines until a blank
# line or the next tag.
#
# The argument *count* and the optional / variadic markers are still
# checked against the BUILTIN() argsig string, so the prose cannot silently
# disagree with the interpreter.

import os
import re


# Meaning of the trailing ret character of a BUILTIN() entry.  Only used to
# sanity-check @ret none against statement builtins; see script-call.cpp.
RET_IS_VALUE = {
    '\0': False,    # statement
    'i': True,
    's': True,
    'v': True,
    'r': True,
    'l': True,
    '.': True,
}


def repo_root():
    """Return the absolute path of the repository root."""
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.dirname(here)


def script_fun_path():
    return os.path.join(repo_root(), 'src', 'map', 'script-fun.cpp')


# --------------------------------------------------------------------------
# builtin_functions[] table
# --------------------------------------------------------------------------

# A single BUILTIN() entry; ret is a one-character string ('' means '\0').
_BUILTIN_RE = re.compile(
    r'BUILTIN\(\s*(\w+)\s*,\s*"([^"]*)"_s\s*,\s*'
    r"(?:'((?:\\0)|.)'|\"([^\"]*)\")\s*\)"
)


class TableEntry(object):
    """One row of builtin_functions[]."""
    __slots__ = ('name', 'argsig', 'ret')

    def __init__(self, name, argsig, ret):
        self.name = name
        self.argsig = argsig
        self.ret = ret


def parse_table(text):
    """Parse builtin_functions[] from script-fun.cpp source text.

    Returns a list of TableEntry in source order.
    """
    start = text.find('builtin_functions[]')
    if start < 0:
        raise SystemExit('could not find builtin_functions[] in script-fun.cpp')
    end = text.find('};', start)
    if end < 0:
        raise SystemExit('could not find end of builtin_functions[] table')
    table = text[start:end]

    entries = []
    for m in _BUILTIN_RE.finditer(table):
        name = m.group(1)
        argsig = m.group(2)
        ret = m.group(3)
        if ret == r'\0' or ret is None:
            ret = '\0'
        entries.append(TableEntry(name, argsig, ret))
    return entries


# Type characters that may appear in a BUILTIN() arg-signature string.
# script-parse.cpp only enforces argument *count*; the type letters are
# documentation.  Anything not in this set (and not '?' or '*') is treated
# as a type character all the same.
_ARGSIG_TYPES = set('isexyLFNIPEtmT')


def argsig_shape(argsig):
    """Reduce a BUILTIN() argsig to (required, optional, variadic).

    required  - number of mandatory positional arguments
    optional  - number of optional positional arguments (excluding any
                trailing variadic argument)
    variadic  - True if the argument list ends with a repeatable argument

    The argsig is processed left to right exactly the way the man-page
    signature is built:

      * a type character contributes one argument;
      * '?' opens the optional run; a bare '?' (one not immediately
        followed by a type character) is itself one optional argument;
      * '*' makes the preceding argument repeatable.  '?*' is an optional
        repeatable argument.

    This matches the count enforcement in script-parse.cpp.
    """
    required = 0
    optional = 0
    variadic = False
    in_optional = False

    # last_bucket: which counter the most recent argument went into,
    # so a following '*' can convert it to the variadic.
    last_bucket = None  # 'req', 'opt', or None

    chars = list(argsig)
    i = 0
    while i < len(chars):
        ch = chars[i]
        if ch == '?':
            in_optional = True
            nxt = chars[i + 1] if i + 1 < len(chars) else ''
            if nxt == '*':
                # optional repeatable trailing argument
                variadic = True
                i += 2
                continue
            if nxt and nxt != '?' and nxt in _ARGSIG_TYPES:
                # '?' only opens the optional run; the type char is the arg
                i += 1
                continue
            # bare '?' is itself one optional argument
            optional += 1
            last_bucket = 'opt'
            i += 1
            continue
        if ch == '*':
            # turn the most recent argument into the variadic
            if last_bucket == 'opt':
                optional -= 1
            elif last_bucket == 'req':
                required -= 1
            variadic = True
            last_bucket = None
            i += 1
            continue
        # an ordinary type character
        if in_optional:
            optional += 1
            last_bucket = 'opt'
        else:
            required += 1
            last_bucket = 'req'
        i += 1
    return required, optional, variadic


# --------------------------------------------------------------------------
# Doc comments
# --------------------------------------------------------------------------

class Param(object):
    """One @arg / @optarg / @rest entry."""
    __slots__ = ('name', 'type', 'desc', 'optional', 'rest')

    def __init__(self, name, type_, desc, optional, rest):
        self.name = name
        self.type = type_
        self.desc = desc
        self.optional = optional
        self.rest = rest


class DocComment(object):
    __slots__ = ('name', 'summary', 'desc_paras', 'params', 'ret_type',
                 'ret_desc', 'lineno')

    def __init__(self, name, summary, desc_paras, params,
                 ret_type, ret_desc, lineno):
        self.name = name            # builtin name from @doc
        self.summary = summary      # one-line summary
        self.desc_paras = desc_paras  # extra @desc paragraphs
        self.params = params        # list[Param]
        self.ret_type = ret_type    # None for "@ret none"
        self.ret_desc = ret_desc    # '' when ret has no description
        self.lineno = lineno        # 1-based line of the comment start


# A param tag line: "<name>: <type>; <description>".
_PARAM_RE = re.compile(r'^([A-Za-z_][\w]*)\s*:\s*([^;]+?)\s*;\s*(.+)$')


def parse_doc_comments(text):
    """Parse every builtin doc comment from script-fun.cpp source text.

    A doc comment is a /* ... */ block that contains an "@doc" tag.
    Returns (list[DocComment], list[str_errors]).
    """
    docs = []
    errors = []

    # Find each /* ... */ block and the line it starts on.  The body
    # pattern forbids an inner "*/" so a comment never spans into the next
    # one.
    comment_re = re.compile(r'/\*(?:[^*]|\*(?!/))*\*/', re.DOTALL)
    for m in comment_re.finditer(text):
        block = m.group(0)
        if '@doc' not in block:
            continue
        lineno = text.count('\n', 0, m.start()) + 1
        doc, errs = _parse_one_comment(block, lineno)
        if doc is not None:
            docs.append(doc)
        errors.extend(errs)
    return docs, errors


def _strip_comment_lines(block):
    """Yield the meaningful text of each line inside a /* ... */ block."""
    inner = block
    if inner.startswith('/*'):
        inner = inner[2:]
    if inner.endswith('*/'):
        inner = inner[:-2]
    for raw in inner.splitlines():
        line = raw.strip()
        # Drop a leading run of '=' (the /*===...===*/ rule) entirely.
        if line and all(c == '=' for c in line):
            continue
        # Drop the conventional leading '*' of a block comment.
        if line.startswith('*'):
            line = line[1:]
        line = line.strip()
        # A line that is only '=' rule characters after the '*'.
        if line and all(c == '=' for c in line):
            continue
        yield line


# Tag keywords recognised at the start of a doc-comment line.  A word like
# "@menu" or "@inventorylist_id" appearing mid-sentence is NOT a tag: it is
# only a tag when one of these keywords stands alone (followed by whitespace
# or end of line).
_TAGS = ('doc', 'arg', 'optarg', 'rest', 'ret', 'desc')


def _tag_word(line):
    """Return the doc tag keyword starting the line, or None.

    "@arg foo" -> 'arg'; "@inventorylist_id, ..." -> None (not a tag).
    """
    if not line.startswith('@'):
        return None
    rest = line[1:]
    for kw in _TAGS:
        if rest == kw or rest.startswith(kw + ' ') or rest.startswith(kw + '\t'):
            return kw
    return None


def _parse_one_comment(block, lineno):
    errors = []
    name = None
    summary_lines = []
    desc_paras = []
    params = []
    ret_type = 'MISSING'
    ret_desc = ''
    seen_optional = False
    seen_rest = False
    seen_tag = False

    cur_desc = []

    def flush_desc():
        if cur_desc:
            desc_paras.append(' '.join(cur_desc))
            del cur_desc[:]

    # "sink" names where a continuation line (a non-tag, non-empty line
    # that wraps a long tag value) is appended:
    #   None      - no sink, the line is summary text or stray prose
    #   'param'   - append to params[-1].desc
    #   'ret'     - append to ret_desc
    #   'desc'    - append to the current @desc paragraph
    sink = None

    for line in _strip_comment_lines(block):
        tag = _tag_word(line)
        if tag == 'doc':
            seen_tag = True
            sink = None
            rest = line[len('@doc'):].strip()
            if not rest:
                errors.append('line %d: @doc has no builtin name' % lineno)
            elif name is not None:
                errors.append('line %d: more than one @doc tag' % lineno)
            else:
                name = rest.split()[0]
            continue
        if tag in ('arg', 'optarg', 'rest'):
            seen_tag = True
            flush_desc()
            kind = tag
            body = line[1 + len(kind):].strip()
            pm = _PARAM_RE.match(body)
            if not pm:
                errors.append(
                    'line %d: malformed %s tag: %r' % (lineno, kind, body))
                sink = None
                continue
            optional = kind in ('optarg', 'rest')
            rest = kind == 'rest'
            if rest:
                if seen_rest:
                    errors.append(
                        'line %d: more than one @rest tag' % lineno)
                seen_rest = True
            else:
                if seen_rest:
                    errors.append(
                        'line %d: @arg/@optarg after @rest' % lineno)
                if kind == 'arg' and seen_optional:
                    errors.append(
                        'line %d: @arg after @optarg' % lineno)
            if kind == 'optarg':
                seen_optional = True
            params.append(Param(pm.group(1), pm.group(2).strip(),
                                pm.group(3).strip(), optional, rest))
            sink = 'param'
            continue
        if tag == 'ret':
            seen_tag = True
            flush_desc()
            body = line[len('@ret'):].strip()
            if ret_type != 'MISSING':
                errors.append('line %d: more than one @ret tag' % lineno)
            if body == 'none':
                ret_type = None
                sink = None
            elif ';' in body:
                t, d = body.split(';', 1)
                ret_type = t.strip()
                ret_desc = d.strip()
                sink = 'ret'
            elif body:
                ret_type = body.strip()
                sink = None
            else:
                errors.append('line %d: @ret has no type' % lineno)
                sink = None
            continue
        if tag == 'desc':
            seen_tag = True
            flush_desc()
            body = line[len('@desc'):].strip()
            if body:
                cur_desc.append(body)
            sink = 'desc'
            continue
        # A line starting with '@' but not a known tag is ordinary prose
        # (for example a sentence naming the @menu variable).
        if not seen_tag:
            # Still in the summary.
            if line:
                summary_lines.append(line)
            continue
        # After the first tag: either a wrapped continuation of the most
        # recent tag value, or a blank line that closes it.
        if not line:
            if sink == 'desc':
                flush_desc()
            sink = None
            continue
        if sink == 'param':
            p = params[-1]
            p.desc = (p.desc + ' ' + line).strip()
        elif sink == 'ret':
            ret_desc = (ret_desc + ' ' + line).strip()
        else:
            # A continuation paragraph of @desc, or stray prose after a
            # tag with no description sink: treat as a @desc paragraph.
            cur_desc.append(line)
            sink = 'desc'
    flush_desc()

    if name is None:
        errors.append('line %d: doc comment has no @doc name' % lineno)
        return None, errors

    summary = ' '.join(summary_lines).strip()
    if not summary:
        errors.append('%s: doc comment has no summary line' % name)
    if ret_type == 'MISSING':
        errors.append('%s: doc comment has no @ret tag' % name)
        ret_type = None

    return DocComment(name, summary, desc_paras, params,
                      ret_type, ret_desc, lineno), errors


# --------------------------------------------------------------------------
# Rendering
# --------------------------------------------------------------------------

def render_adoc_term(doc):
    """Render a DocComment as an AsciiDoc definition-list term line.

    The result has the form

        *name*(_arg_, _arg_[, _opt_]) -> type

    with the function name in *bold*, each argument in _emphasis_ by its
    real name, optional arguments inside [square brackets], a repeatable
    trailing argument shown as "name...", and a trailing return type when
    the builtin yields a value.
    """
    required = [p for p in doc.params if not p.optional]
    optional = [p for p in doc.params if p.optional]

    def tok(p):
        return '_%s_...' % p.name if p.rest else '_%s_' % p.name

    req = ', '.join(tok(p) for p in required)
    opt = ', '.join(tok(p) for p in optional)

    if opt:
        if req:
            arglist = req + '[, ' + opt + ']'
        else:
            arglist = '[' + opt + ']'
    else:
        arglist = req

    term = '*%s*(%s)' % (doc.name, arglist)
    if doc.ret_type is not None:
        term += ' -> ' + doc.ret_type
    return term


def render_adoc_entry(doc):
    """Render a DocComment as a full AsciiDoc definition-list entry.

    The entry is one definition-list item.  The term line carries the
    marked-up signature; the body holds, in order, the summary, a labelled
    parameter list, any @desc paragraphs and the return description.

    The blocks of the body are joined with AsciiDoc list-continuation '+'
    lines so they all attach to the single definition-list item (without a
    '+' a block after a blank line would detach, and asciidoctor's manpage
    backend would render it as an over-indented literal).  Returns a list
    of lines (no trailing blank line).
    """
    lines = ['%s::' % render_adoc_term(doc)]

    # Each "block" is a list of text lines.
    blocks = []
    if doc.summary:
        blocks.append([doc.summary])

    if doc.params:
        plist = []
        for p in doc.params:
            opt = ', optional' if (p.optional and not p.rest) else ''
            rep = ', repeatable' if p.rest else ''
            plist.append('* _%s_ (_%s_%s%s) %s'
                         % (p.name, p.type, opt, rep, p.desc))
        blocks.append(plist)

    for para in doc.desc_paras:
        blocks.append([para])

    if doc.ret_type is not None:
        if doc.ret_desc:
            blocks.append(['Returns _%s_: %s' % (doc.ret_type, doc.ret_desc)])
        else:
            blocks.append(['Returns _%s_.' % doc.ret_type])

    for i, block in enumerate(blocks):
        if i > 0:
            lines.append('+')
        lines.extend(block)
    return lines
