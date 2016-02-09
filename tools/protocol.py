#!/usr/bin/env python
# coding: utf-8

#   protocol.py - generator for entire TMWA network protocol
#
#   Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

from __future__ import print_function

import glob
import itertools
import os
from pipes import quote
from posixpath import relpath
from weakref import ref as wr

try:
    unicode
except NameError:
    unicode = str

## For various reasons this is all one file, so let's navigate with a
##
## Table of Contents
##
## TOC_HEAD
## TOC_TYPES
## TOC_EVENT
## TOC_CHAN
## TOC_DATA
##  TOC_GENTYPE
##  TOC_CLIENT
##  TOC_LOGINCHAR
##  TOC_CHARMAP
##  TOC_INTERMAP
##  TOC_MISC
##  TOC_LOGINADMIN
##  TOC_NEW
## TOC_DRAWING
## TOC_MAIN
##

# The following code should be relatively easy to understand, but please
# keep your sanity fastened and your arms and legs inside at all times.

# important note: all numbers in this file will make a lot more sense in
# decimal, but they're written in hex.


# TOC_HEAD

generated = '// This is a generated file, edit %s instead\n' % __file__

copyright = '''//    {filename} - {description}
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

class OpenWrite(object):
    def __init__(self, filename):
        self.filename = filename

    def __enter__(self):
        self.handle = open(self.filename + '.tmp', 'w')
        return self.handle

    def __exit__(self, ty, v, tb):
        self.handle.close()
        if ty is not None:
            return
        frag = '''
        if cmp -s {0}.tmp {0}.old
        then
            : echo Unchanged: {0}
            rm {0}.tmp
            mv {0}.old {0}
        else
            echo Changed: {0}
            rm {0}.old
            mv {0}.tmp {0}
        fi
        '''.format(quote(self.filename))
        os.system(frag)


# TOC_

def gvq(s):
    return u'"%s"' % s.replace(u'"', u'\\"')

def gva(d):
    if d:
        return u' [%s]' % u', '.join(u'%s=%s' % (ak, gvq(av)) for (ak, av) in sorted(d.items()))
    return u''

class Attributes(object):
    __slots__ = (u'_attributes')

    def __init__(self):
        self._attributes = {}

    def __getitem__(self, k):
        assert isinstance(k, unicode)
        return self._attributes[k]

    def __setitem__(self, k, v):
        assert isinstance(k, unicode)
        assert isinstance(v, unicode)
        self._attributes[k] = v

    def __delitem__(self, k):
        assert isinstance(k, unicode)
        del self._attributes[k]

    def merge(self, *others):
        for other in others:
            assert self.__class__ == other.__class__
            # if an attribute is present on multiple inputs, prefer
            # the last one. This is not necessarily what you want, though.
            self._attributes.update(other._attributes)

class Graph(Attributes):
    __slots__ = (u'default_vertex', u'default_edge', u'_vertices', u'_edges', u'_vertex_lookup')

    def __init__(self):
        Attributes.__init__(self)
        self.default_vertex = Attributes()
        self.default_edge = Attributes()
        self._vertices = set()
        self._edges = {}
        self._vertex_lookup = {}

    def vertex(self, name, insert=True):
        assert isinstance(name, unicode)
        vert = self._vertex_lookup.get(name)
        if insert and vert is None:
            vert = Vertex(name)
            self._vertex_lookup[name] = vert
            self._vertices.add(vert)
        return vert

    def _fix_vertex(self, v, insert=True):
        if isinstance(v, Vertex):
            return v
        return self.vertex(v, insert)

    def edge(self, v1, v2, insert=True):
        v1 = self._fix_vertex(v1, insert)
        if v1 is None:
            return None
        v2 = self._fix_vertex(v2, insert)
        if v2 is None:
            return None
        ek = (v1, v2)
        edge = self._edges.get(ek)
        if insert and edge is None:
            edge = Edge(v1, v2)
            v1._post.add(wr(v2))
            v2._pre.add(wr(v1))
            self._edges[ek] = edge
        return edge

    def del_edge(self, v1, v2):
        edge = self.edge(v1, v2, False)
        if edge is None:
            return
        v1 = edge._from
        v2 = edge._to
        ek = (v1, v2)
        v1._post.remove(wr(v2))
        v2._pre.remove(wr(v1))
        del self._edges[ek]

    def del_vertex(self, v):
        v = self._fix_vertex(v, False)
        if v is None:
            return
        pre = list(v._pre)
        for vp in pre:
            vp = vp()
            self.del_edge(vp, v)
        post = list(v._post)
        for vp in post:
            vp = vp()
            self.del_edge(v, vp)
        del self._vertex_lookup[v._key]
        self._vertices.remove(v)

    def splice_out_vertex(self, v):
        v = self._fix_vertex(v, False)
        if v is None:
            return
        self.del_edge(v, v)
        for vp in v._pre:
            vp = vp()
            ep = self.edge(vp, v, False)
            for vn in v._post:
                vn = vn()
                en = self.edge(v, vn, False)
                self.edge(vp, vn).merge(ep, en)
        self.del_vertex(v)

    def dot(self, out, close):
        if close:
            with out:
                self.dot(out, False)
                return

        def p(*args):
            for x in args:
                out.write(unicode(x))
            out.write(u'\n')
        p(u'digraph')
        p(u'{')
        for ak, av in sorted(self._attributes.items()):
            p(u'  ', ak, u'=', gvq(av), u';')
        for ak, av in sorted(self.default_vertex._attributes.items()):
            p(u'  node [', ak, u'=', gvq(av), u'];')
        for ak, av in sorted(self.default_edge._attributes.items()):
            p(u'  edge [', ak, u'=', gvq(av), u'];')
        for n in sorted(self._vertices):
            p(u'  ', n)
        for _, e in sorted(self._edges.items()):
            p(u'  ', e)
        p(u'}')

    def dot_str(self):
        from io import StringIO
        out = StringIO()
        self.dot(out, False)
        return out.getvalue()

    def dot_file(self, name):
        with open(name, u'w') as f:
            self.dot(f, False)

    def preview(self, block):
        from subprocess import Popen, PIPE
        proc = Popen([u'dot', u'-Txlib', u'/dev/stdin'], stdin=PIPE, universal_newlines=True)
        self.dot(proc.stdin, True)
        if block:
            proc.wait()

class Vertex(Attributes):
    __slots__ = (u'_key', u'_post', u'_pre', u'__weakref__')

    def __init__(self, key):
        Attributes.__init__(self)
        self._key = key
        self._post = set()
        self._pre = set()

    def __str__(self):
        return u'%s%s;' % (gvq(self._key), gva(self._attributes))

    def __lt__(self, other):
        return self._key < other._key

class Edge(Attributes):
    __slots__ = (u'_from', u'_to')

    def __init__(self, f, t):
        Attributes.__init__(self)
        self._from = f
        self._to = t

    def __str__(self):
        return u'%s -> %s%s;' % (gvq(self._from._key), gvq(self._to._key), gva(self._attributes))


# TOC_TYPES

class LowType(object):
    __slots__ = ('includes')

    def __repr__(self):
        return '%s(%s)' % (type(self).__name__, self.name)

class NativeType(LowType):
    __slots__ = ('name')

    def __init__(self, name, include):
        self.name = name
        self.includes = frozenset({include}) if include else frozenset()

    def __repr__(self):
        return 'NativeType(%r)' % (self.name)


    def a_tag(self):
        return self.name

class NetworkType(LowType):
    __slots__ = ('name')

    def __init__(self, name, include):
        self.name = name
        self.includes = frozenset({include}) if include else frozenset()

    def __repr__(self):
        return 'NetworkType(%r)' % (self.name)


    def e_tag(self):
        return self.name


class Type(object):
    __slots__ = ('includes', 'do_dump')

    def __new__(cls, *args):
        rv = object.__new__(cls)
        rv.do_dump = True
        return rv

class NeutralType(Type):
    __slots__ = ('name')

    def __init__(self, name, include):
        self.name = name
        identity = '#include "../proto-base/net-neutral.hpp"\n'
        self.includes = frozenset({include, identity}) if include else frozenset({identity})

    def __repr__(self):
        return 'NeutralType(%r)' % (self.name)


    def __repr__(self):
        return 'NeutralType(%r)' % (self.name)


    def native_tag(self):
        return self.name

    def network_tag(self):
        return self.name

    e_tag = network_tag

class PolyFakeType(object):
    __slots__ = ('already', 'do_dump', 'nat_tag')

    def __init__(self, already):
        self.already = already = tuple(already)
        self.do_dump = True

        assert len(already) >= 2
        self.nat_tag = nat_tag = already[0].native_tag()
        for a in self.already:
            a.do_dump = False
            assert nat_tag == a.native_tag()

    def __repr__(self):
        return 'PolyFakeType(*%r)' % (self.already,)


    def native_tag(self):
        return self.nat_tag

    def add_headers_to(self, headers):
        for a in self.already:
            a.add_headers_to(headers)

    def dump_fwd(self, f):
        for a in self.already:
            a.dump_fwd(f)

    def dump(self, f):
        for a in self.already:
            a.dump(f)

class StringType(Type):
    __slots__ = ('native')

    def __init__(self, native):
        self.native = native
        self.includes = native.includes | {'#include "../proto-base/net-string.hpp"\n'}

    def __repr__(self):
        return 'StringType(%r)' % (self.native)


    def __repr__(self):
        return 'StringType(%r)' % self.native


    def native_tag(self):
        return self.native.a_tag()

    def network_tag(self):
        return 'NetString<sizeof(%s)>' % self.native.a_tag()

    # not implemented properly, uses a template in net-string.hpp instead
    #do_dump = False
    # in Context.string() instead because reasons

class ProvidedType(Type):
    __slots__ = ('native', 'network')

    def __init__(self, native, network):
        self.native = native
        self.network = network
        self.includes = native.includes | network.includes

    def __repr__(self):
        return 'ProvidedType(%r, %r)' % (self.native, self.network)


    def __repr__(self):
        return 'ProvidedType(native=%r, network=%r)' % (self.native, self.network)


    def native_tag(self):
        return self.native.a_tag()

    def network_tag(self):
        return self.network.e_tag()

class EnumType(Type):
    __slots__ = ('native', 'under')

    def __init__(self, native, under):
        self.native = native
        self.under = under
        name = native.a_tag()
        self.includes = frozenset({'#include "net-%s.hpp"\n' % name})

    def __repr__(self):
        return 'EnumType(%r, %r)' % (self.native, self.under)


    def __repr__(self):
        return 'EnumType(%r, %r)' % (self.native, self.under)


    def native_tag(self):
        return self.native.a_tag()

    def network_tag(self):
        return self.under.network_tag()

    def add_headers_to(self, headers):
        headers.update(self.native.includes)
        headers.update(self.under.includes)

    def dump_fwd(self, f):
        # nothing to see here
        pass

    def dump(self, f):
        native = self.native_tag()
        under = self.under.native_tag()
        network = self.network_tag()

        f.write('inline __attribute__((warn_unused_result))\n')
        f.write('bool native_to_network({0} *network, {1} native)\n{{\n'.format(network, native))
        f.write('    bool rv = true;\n')
        f.write('    {0} tmp = static_cast<{0}>(native);\n'.format(under))
        f.write('    rv &= native_to_network(network, tmp);\n')
        f.write('    return rv;\n')
        f.write('}\n')
        f.write('inline __attribute__((warn_unused_result))\n')
        f.write('bool network_to_native({0} *native, {1} network)\n{{\n'.format(native, network))
        f.write('    bool rv = true;\n')
        f.write('    {0} tmp;\n'.format(under))
        f.write('    rv &= network_to_native(&tmp, network);\n')
        f.write('    *native = static_cast<{0}>(tmp);\n'.format(native))
        f.write('    // TODO this is what really should be doing a checked cast\n')
        f.write('    return rv;\n')
        f.write('}\n')

class WrappedType(Type):
    __slots__ = ('native', 'under')

    def __init__(self, native, under):
        self.native = native
        self.under = under
        name = self.native.a_tag()
        self.includes = native.includes | under.includes | {'#include "net-%s.hpp"\n' % name}

    def __repr__(self):
        return 'WrappedType(%r, %r)' % (self.native, self.under)


    def __repr__(self):
        return 'WrappedType(%r, %r)' % (self.native, self.under)


    def native_tag(self):
        return self.native.a_tag()

    def network_tag(self):
        return self.under.network_tag()

    def add_headers_to(self, headers):
        headers.update(self.native.includes)
        headers.update(self.under.includes)

    def dump_fwd(self, f):
        # nothing to see here
        pass

    def dump(self, f):
        native = self.native_tag()
        under = self.under.native_tag()
        network = self.network_tag()

        f.write('inline __attribute__((warn_unused_result))\n')
        f.write('bool native_to_network({0} *network, {1} native)\n{{\n'.format(network, native))
        f.write('    bool rv = true;\n')
        f.write('    {0} tmp = unwrap<{1}>(native);\n'.format(under, native))
        f.write('    rv &= native_to_network(network, tmp);\n')
        f.write('    return rv;\n')
        f.write('}\n')
        f.write('inline __attribute__((warn_unused_result))\n')
        f.write('bool network_to_native({0} *native, {1} network)\n{{\n'.format(native, network))
        f.write('    bool rv = true;\n')
        f.write('    {0} tmp;\n'.format(under))
        f.write('    rv &= network_to_native(&tmp, network);\n')
        f.write('    *native = wrap<{0}>(tmp);\n'.format(native))
        f.write('    return rv;\n')
        f.write('}\n')

class SkewLengthType(Type):
    __slots__ = ('type', 'skew')

    def __init__(self, ty, skew):
        self.type = ty
        self.skew = skew
        self.includes = ty.includes | {'#include "../proto-base/net-skewed-length.hpp"\n'}

    def __repr__(self):
        return 'SkewLengthType(%r, %r)' % (self.ty, self.skew)


    def native_tag(self):
        return self.type.native_tag()

    def network_tag(self):
        return 'SkewedLength<%s, %d>' % (self.type.network_tag(), self.skew)

    def dumpx_fwd(self, f):
        # nothing to see here
        pass

    def dumpx(self):
        # not registered properly, so the method name is wrong
        # TODO remind myself to kill this code with fire before release
        # not implemented properly, uses a template in the meta instead
        # insert another comment here so that it lines up properly
        pass

class StructType(Type):
    __slots__ = ('id', 'name', 'fields', 'size', 'ctor')

    def __init__(self, id, name, fields, size, ctor=False):
        self.id = id
        self.name = name
        self.fields = fields
        self.size = size
        self.ctor = ctor
        # only used if id is None
        # internally uses add_headers_to()
        self.includes = frozenset({'#include "net-%s.hpp"\n' % name})

    def __repr__(self):
        return '<StructType(%r) with %d fields>' % (self.name, len(self.fields))


    def __repr__(self):
        return '<StructType(id=%r, name=%r, size=%r, ctor=%r) with %d fields>' % (self.id, self.name, self.size, self.ctor, len(self.fields))


    def native_tag(self):
        return self.name

    def network_tag(self):
        return 'Net' + self.name

    def add_headers_to(self, headers):
        # only used directly if id is not None
        # used indirectly if id is None
        for (o, l, n) in self.fields:
            headers.update(l.includes)
        headers.add('#include <cstddef>\n') # for offsetof

    def dump(self, f):
        self.dump_native(f)
        self.dump_network(f)
        self.dump_convert(f)
        f.write('\n')

    def dump_fwd(self, fwd):
        if self.id is not None:
            fwd.write('template<>\n')
            assert False
        fwd.write('struct %s;\n' % self.name)
        return
        if self.id is not None:
            fwd.write('template<>\n')
        fwd.write('struct Net%s;\n' % self.name)

    def dump_native(self, f):
        name = self.name
        if self.id is not None:
            f.write('template<>\n')
        f.write('struct %s\n{\n' % name)
        if self.id is not None:
            f.write('    static const uint16_t PACKET_ID = 0x%04x;\n\n' % self.id)
        for (o, l, n) in self.fields:
            if n in ('magic_packet_id', 'magic_packet_length'):
                f.write('    // TODO remove this\n')
            if n == 'magic_packet_id':
                f.write('    %s %s = PACKET_ID;\n' % (l.native_tag(), n))
            else:
                f.write('    %s %s = {};\n' % (l.native_tag(), n))
        if self.ctor:
            f.write('    %s() = default;\n' % name)
            f.write('    %s(' % name)
            f.write(', '.join('%s _%s' % (l.native_tag(), n) for (_, l, n) in self.fields))
            f.write(') : ')
            f.write(', '.join('%s(_%s)' % (n, n) for (_, _, n) in self.fields))
            f.write(' {}\n')
        f.write('};\n')

    def dump_network(self, f):
        name = 'Net%s' % self.name
        if self.id is not None:
            f.write('template<>\n')
        f.write('struct %s\n{\n' % name)
        for (o, l, n) in self.fields:
            f.write('    %s %s;\n' % (l.network_tag(), n))
        f.write('};\n')
        for (o, l, n) in self.fields:
            if o is not None:
                s = 'offsetof(%s, %s) == %d' % (name, n, o)
                f.write('static_assert({0}, "{0}");\n'.format(s))
        if self.size is not None:
            s = 'sizeof(%s) == %d' % (name, self.size)
            f.write('static_assert({0}, "{0}");\n'.format(s))
        s = 'alignof(%s) == 1' % (name)
        f.write('static_assert({0}, "{0}");\n'.format(s))

    def dump_convert(self, f):
        f.write('inline __attribute__((warn_unused_result))\n')
        f.write('bool native_to_network(Net{0} *network, {0} native)\n{{\n'.format(self.name))
        f.write('    bool rv = true;\n')
        for (o, l, n) in self.fields:
            f.write('    rv &= native_to_network(&network->{0}, native.{0});\n'.format(n))
        f.write('    return rv;\n')
        f.write('}\n')

        f.write('inline __attribute__((warn_unused_result))\n')
        f.write('bool network_to_native({0} *native, Net{0} network)\n{{\n'.format(self.name))
        f.write('    bool rv = true;\n')
        for (o, l, n) in self.fields:
            f.write('    rv &= network_to_native(&native->{0}, network.{0});\n'.format(n))
        f.write('    return rv;\n')
        f.write('}\n')

class PartialStructType(Type):
    __slots__ = ('native', 'body')

    def __init__(self, native, body):
        self.native = native
        self.body = body
        name = native.a_tag()
        self.includes = frozenset({'#include "net-%s.hpp"\n' % name})

    def __repr__(self):
        return '<PartialStructType(%r) with %d fields>' % (self.native, len(self.body))


    def __repr__(self):
        return '<PartialStructType(native=%r) with %d pieces of a body>' % (self.native, len(self.body))


    def native_tag(self):
        return self.native.a_tag()

    def network_tag(self):
        return 'Net%s' % self.native_tag()

    def add_headers_to(self, headers):
        headers.update(self.native.includes)
        for n, t in self.body:
            headers.update(t.includes)

    def dump_fwd(self, f):
        # nothing to see here
        pass

    def dump(self, f):
        f.write('struct %s\n{\n' % self.network_tag())
        for n, t in self.body:
            f.write('    %s %s;\n' % (t.network_tag(), n))
        f.write('};\n')
        f.write('inline __attribute__((warn_unused_result))\n')
        f.write('bool native_to_network(Net{0} *network, {0} native)\n{{\n'.format(self.native_tag()))
        f.write('    bool rv = true;\n')
        for n, t in self.body:
            u = t.native_tag()
            # probably not necessary
            f.write('    {1} {0} = native.{0}; rv &= native_to_network(&network->{0}, {0});\n'.format(n, u))
        f.write('    return rv;\n')
        f.write('}\n')
        f.write('inline __attribute__((warn_unused_result))\n')
        f.write('bool network_to_native({0} *native, Net{0} network)\n{{\n'.format(self.native_tag()))
        f.write('    bool rv = true;\n')
        for n, t in self.body:
            # the temporary is ABSOLUTELY NECESSARY here
            # the native type is permitted to differ from the declared type
            # e.g. HumanTimeDiff uses int16_t instead of uint16_t
            u = t.native_tag()
            f.write('    {1} {0}; rv &= network_to_native(&{0}, network.{0}); native->{0} = {0};\n'.format(n, u))
        f.write('    return rv;\n')
        f.write('}\n')
        f.write('\n')

class ArrayType(Type):
    __slots__ = ('element', 'count')

    def __init__(self, element, count):
        self.element = element
        self.count = count
        self.includes = element.includes | {'#include "../proto-base/net-array.hpp"\n'}

    def __repr__(self):
        return 'ArrayType(%r, %r)' % (self.element, self.count)


    def native_tag(self):
        return 'Array<%s, %s>' % (self.element.native_tag(), self.count)

    def network_tag(self):
        return 'NetArray<%s, %s>' % (self.element.network_tag(), self.count)

class EArrayType(Type):
    __slots__ = ('element', 'index', 'count')

    def __init__(self, element, index, count):
        self.element = element
        self.index = index
        self.count = count
        self.includes = element.includes | {'#include "../proto-base/net-array.hpp"\n'}

    def __repr__(self):
        return 'EArrayType(%r, %r)' % (self.element, self.index, self.count)


    def native_tag(self):
        return 'earray<%s, %s, %s>' % (self.element.native_tag(), self.index, self.count)

    def network_tag(self):
        return 'NetArray<%s, static_cast<size_t>(%s)>' % (self.element.network_tag(), self.count)

class InvArrayType(Type):
    __slots__ = ('element', 'index', 'count')

    def __init__(self, element, index, count):
        self.element = element
        self.index = index
        self.count = count
        self.includes = element.includes | {
                '#include "../proto-base/net-array.hpp"\n',
                '#include "../mmo/clif.t.hpp"\n',
                '#include "../mmo/consts.hpp"\n',
        }

    def __repr__(self):
        return 'InvArrayType(%r, %r)' % (self.element, self.index, self.count)


    def native_tag(self):
        return 'GenericArray<%s, InventoryIndexing<%s, %s>>' % (self.element.native_tag(), self.index, self.count)

    def network_tag(self):
        return 'NetArray<%s, %s>' % (self.element.network_tag(), self.count)


# TOC_EVENT
# special event origins
class SpecialEventOrigin(object):
    __slots__ = ('name')

    def __init__(self, name):
        self.name = name

    def __repr__(self):
        return self.name

ADMIN = SpecialEventOrigin('ADMIN')
BOOT = SpecialEventOrigin('BOOT')
FINISH = SpecialEventOrigin('FINISH')
GM = SpecialEventOrigin('GM')
HUMAN = SpecialEventOrigin('HUMAN')
IDLE = SpecialEventOrigin('IDLE')
MAGIC = SpecialEventOrigin('MAGIC')
NOTHING = SpecialEventOrigin('NOTHING')
OTHER = SpecialEventOrigin('OTHER')
PRETTY = SpecialEventOrigin('PRETTY')
SCRIPT = SpecialEventOrigin('SCRIPT')
TIMER = SpecialEventOrigin('TIMER')

def event_name(p):
    if isinstance(p, SpecialEventOrigin):
        return p.name
    if isinstance(p, int):
        return 'packet 0x%04x' % p
    assert False, 'Unknown event: %r' % p

def event_link(p):
    if isinstance(p, SpecialEventOrigin):
        return p.name
    if isinstance(p, int):
        return '[[Packet 0x%04x]]' % p
    assert False, 'Unknown event: %r' % p


# TOC_CHAN

class Include(object):
    __slots__ = ('path', '_types')

    def __init__(self, path):
        self.path = path
        self._types = []

    def __repr__(self):
        return '<Include(%r) with %d types>' % (self.path, len(self._types))


    def testcase(self, outdir):
        basename = os.path.basename(self.path.strip('<">'))
        root = os.path.splitext(basename)[0]
        filename = 'include_%s_test.cpp' % root.replace('.', '_')
        desc = 'testsuite for protocol includes'
        poison = relpath('src/poison.hpp', outdir)
        with OpenWrite(os.path.join(outdir, filename)) as f:
            f.write(self.pp(0))
            f.write(copyright.format(filename=filename, description=desc))
            f.write('\n')
            f.write('#include "%s"\n\nnamespace tmwa\n{\n' % poison)


    def pp(self, n):
        return '#%*sinclude %s\n' % (n, '', self.path)

    def native(self, name):
        ty = NativeType(name, self.pp(0))
        self._types.append(ty)
        return ty

    def network(self, name):
        ty = NetworkType(name, self.pp(0))
        self._types.append(ty)
        return ty

    def neutral(self, name):
        ty = NeutralType(name, self.pp(0))
        self._types.append(ty)
        return ty

class BasePacket(object):
    __slots__ = ('id', 'name', 'define', 'pre', 'post', 'xpost', 'desc')

    def __init__(self, **kwargs):
        for s in BasePacket.__slots__:
            setattr(self, s, kwargs[s])
            del kwargs[s]
        assert not kwargs, 'Unknown kwargs: %s' % repr(kwargs)

    def base_repr_fragment(self):
        return ', '.join('%s=%r' % (s, getattr(self, s)) for s in BasePacket.__slots__)

    def comment_doc(self):
        id = self.id
        name = self.name
        define = self.define
        desc = self.desc
        pre = self.pre
        post = self.post

        comment = 'Packet 0x%04x: "%s"\n' % (id, name)
        if define:
            comment += 'define: ' + define + '\n'
        if True:
            prestr = ', '.join(event_name(x) for x in pre) or 'none'
            poststr = ', '.join(event_name(x) for x in post) or 'none'
            comment += 'pre:  ' + prestr + '\n'
            comment += 'post: ' + poststr + '\n'
            comment += desc
        comment = ''.join('// ' + c + '\n' if c else '//\n' for c in comment.split('\n'))
        return comment

    def wiki_doc(self):
        # TODO do markdown magic
        id = self.id
        name = self.name
        desc = self.desc
        pre = self.pre
        post = self.post
        xpost = self.xpost

        wiki = 'Packet 0x%04x: "%s"\n\n' % (id, name)
        if True:
            prestr = ', '.join(event_link(x) for x in pre) or 'none'
            poststr = ', '.join(event_link(x) for x in fix_sort(post + xpost)) or 'none'
            wiki += 'pre:  ' + prestr + '\n\n'
            wiki += 'post: ' + poststr + '\n\n'
            wiki += desc
            wiki += '\n\n'
            wiki += '![](packets-around-0x%04x.png)\n' % id
        return wiki

    def pre_set(self, d, n=float('inf'), accum=None):
        if accum is None:
            accum = set()
        if self.id in accum:
            return accum
        accum.add(self.id)
        if not n:
            return accum
        for p in self.pre:
            # ignore specials
            if isinstance(p, SpecialEventOrigin):
                continue
            # ignore weak links
            if self.id not in d[p].post:
                continue
            d[p].pre_set(d, n - 1 + (len([z for z in self.pre if isinstance(z, SpecialEventOrigin) or self.id in d[z].post]) == 1), accum)
        return accum

    def post_set(self, d, n=float('inf'), accum=None):
        if accum is None:
            accum = set()
        if self.id in accum:
            return accum
        accum.add(self.id)
        if not n:
            return accum
        for p in self.post:
            # ignore specials
            if isinstance(p, SpecialEventOrigin):
                continue
            # weak links are in xpost

            d[p].post_set(d, n - 1 + (len(self.post) == 1), accum)
        return accum


class FixedPacket(BasePacket):
    __slots__ = ('fixed_struct')

    def __init__(self, fixed_struct, **kwargs):
        BasePacket.__init__(self, **kwargs)
        self.fixed_struct = fixed_struct

    def __repr__(self):
        return 'FixedPacket(%r, %s)' % (self.fixed_struct, self.base_repr_fragment())


    def add_headers_to(self, headers):
        self.fixed_struct.add_headers_to(headers)

    def dump_fwd(self, fwd):
        self.fixed_struct.dump_fwd(fwd)
        fwd.write('\n')

    def dump_native(self, f):
        f.write(self.comment_doc())
        self.fixed_struct.dump_native(f)
        f.write('\n')

    def dump_network(self, f):
        self.fixed_struct.dump_network(f)
        f.write('\n')

    def dump_convert(self, f):
        self.fixed_struct.dump_convert(f)
        f.write('\n')

class VarPacket(BasePacket):
    __slots__ = ('head_struct', 'repeat_struct')

    def __init__(self, head_struct, repeat_struct, **kwargs):
        BasePacket.__init__(self, **kwargs)
        self.head_struct = head_struct
        self.repeat_struct = repeat_struct

    def __repr__(self):
        return 'VarPacket(%r, %r, %s)' % (self.head_struct, self.repeat_struct, self.base_repr_fragment())


    def add_headers_to(self, headers):
        self.head_struct.add_headers_to(headers)
        self.repeat_struct.add_headers_to(headers)

    def dump_fwd(self, fwd):
        self.head_struct.dump_fwd(fwd)
        self.repeat_struct.dump_fwd(fwd)
        fwd.write('\n')

    def dump_native(self, f):
        f.write(self.comment_doc())
        self.head_struct.dump_native(f)
        self.repeat_struct.dump_native(f)
        f.write('\n')

    def dump_network(self, f):
        self.head_struct.dump_network(f)
        self.repeat_struct.dump_network(f)
        f.write('\n')

    def dump_convert(self, f):
        self.head_struct.dump_convert(f)
        self.repeat_struct.dump_convert(f)
        f.write('\n')

def sanitize_line(line, n):
    if not line:
        return line
    m = len(line) - len(line.lstrip(' '))
    assert m >= n, 'not %d: %r' % (n, line)
    return line[n:]

def sanitize_multiline(text):
    text = text.strip('\n').rstrip(' ')
    assert '\r' not in text
    assert '\t' not in text
    n = len(text) - len(text.lstrip(' '))
    return '\n'.join(sanitize_line(l, n) for l in text.split('\n'))

def packet(id, name, define=None,
        fixed=None, fixed_size=None,
        payload=None, payload_size=None,
        head=None, head_size=None,
        repeat=None, repeat_size=None,
        option=None, option_size=None,
        pre=None, post=None, xpost=None, desc=None,
):
    assert (fixed is None) <= (fixed_size is None)
    assert (payload is None) <= (payload_size is None)
    assert (head is None) <= (head_size is None)
    assert (repeat is None) <= (repeat_size is None)
    assert (option is None) <= (option_size is None)
    assert (pre is not None) and (post is not None) and desc.strip()
    if xpost is None:
        xpost = []

    desc = sanitize_multiline(desc)

    if fixed is not None:
        assert not head and not repeat and not option and not payload
        return FixedPacket(
                StructType(id, 'Packet_Fixed<0x%04x>' % id, fixed, fixed_size),
                id=id, name=name, define=define, pre=pre, post=post, xpost=xpost, desc=desc,
        )
    elif payload is not None:
        assert not head and not repeat and not option
        return FixedPacket(
                StructType(id, 'Packet_Payload<0x%04x>' % id, payload, payload_size),
                id=id, name=name, define=define, pre=pre, post=post, xpost=xpost, desc=desc,
        )
    else:
        assert head
        if option:
            assert not repeat
            return VarPacket(
                    StructType(id, 'Packet_Head<0x%04x>' % id, head, head_size),
                    StructType(id, 'Packet_Option<0x%04x>' % id, option, option_size),
                    id=id, name=name, define=define, pre=pre, post=post, xpost=xpost, desc=desc,
            )
        else:
            assert repeat
            return VarPacket(
                    StructType(id, 'Packet_Head<0x%04x>' % id, head, head_size),
                    StructType(id, 'Packet_Repeat<0x%04x>' % id, repeat, repeat_size),
                    id=id, name=name, define=define, pre=pre, post=post, xpost=xpost, desc=desc,
            )


# TODO this whole idea is wrong
# in particular, 'any'.
# instead, have just one channel for all packets, but store send/recv/type
# with each packet.
# Then during codegen, emit:
#   #if PACKET_LOGIN || PACKET_CHAR || PACKET_MAP || PACKET_ADMIN || PACKET_USER
# (for an 'all' packet) around the send/recv portions separately
class Channel(object):
    __slots__ = ('server', 'client', 'packets')

    def __init__(self, server, client):
        self.server = server
        self.client = client
        self.packets = []

    def __repr__(self):
        return '<Channel(%r, %r) with %d packets>' % (
                self.server,
                self.client,
                len(self.packets),
        )


    def x(self, id, name, **kwargs):
        self.packets.append(packet(id, name, **kwargs))
    r = x
    s = x

    def dump(self, outdir):
        server = self.server
        client = self.client
        header = '%s-%s.hpp' % (server, client)
        desc = 'TMWA network protocol: %s/%s' % (server, client)
        with OpenWrite(os.path.join(outdir, header)) as f:
            type_headers = set()
            for p in self.packets:
                p.add_headers_to(type_headers)
            type_headers = sorted(type_headers)
            proto2 = relpath(outdir, 'src')
            f.write('#pragma once\n')
            f.write(copyright.format(filename=header, description=desc))
            f.write('\n')
            f.write(generated)
            f.write('\n')
            f.write('#include "fwd.hpp"\n\n')
            f.write('// sorry these are not ordered by hierarchy\n')
            for h in type_headers:
                f.write(h)
            f.write('\n')
            f.write('namespace tmwa\n{\n')
            if client == 'user':
                f.write('// This is a public protocol, and changes require client cooperation\n')
            else:
                f.write('// This is an internal protocol, and can be changed without notice\n')
            f.write('\n')
            for p in self.packets:
                p.dump_native(f)
            f.write('\n')
            for p in self.packets:
                p.dump_network(f)
            f.write('\n')
            for p in self.packets:
                p.dump_convert(f)
            f.write('} // namespace tmwa\n')


ident_translation = ''.join(chr(c) if chr(c).isalnum() else '_' for c in range(256))

def ident(s):
    if s == 'packet id':
        return 'magic_packet_id'
    if s == 'packet length':
        return 'magic_packet_length'
    return s.translate(ident_translation)


def at(o, l, n):
    return (o, l, ident(n))


class Context(object):
    __slots__ = ('outdir', '_includes', '_channels', '_types')

    def __init__(self, outdir):
        self.outdir = outdir
        self._includes = []
        self._channels = []
        self._types = []

    def __repr__(self):
        return '<Context(%r) with %d _includes, %d _channels, %d _types>' % (
                self.outdir,
                len(self._includes),
                len(self._channels),
                len(self._types),
        )


    def sysinclude(self, name):
        rv = Include('<%s>' % name)
        self._includes.append(rv)
        return rv

    def include(self, name):
        rv = Include('"%s"' % relpath(name, self.outdir))
        self._includes.append(rv)
        return rv

    def chan(self, server, client):
        ch = Channel(server, client)
        self._channels.append(ch)
        return ch

    def dump(self):
        outdir = self.outdir
        for g in glob.glob(os.path.join(outdir, '*.[ch]pp')):
            os.rename(g, g + '.old')
        proto2 = relpath(outdir, 'src')
        with OpenWrite(os.path.join(outdir, 'fwd.hpp')) as f:
            header = '%s/fwd.hpp' % proto2
            desc = 'Forward declarations of network packet body structs'
            sanity = relpath('src/sanity.hpp', outdir)

            f.write('#pragma once\n')
            f.write(copyright.format(filename=header, description=desc))
            f.write('\n')

            f.write('#include "%s"\n\n' % sanity)
            f.write('#include <cstdint>\n\n')

            f.write('#include "../ints/fwd.hpp" // rank 1\n')
            f.write('#include "../strings/fwd.hpp" // rank 1\n')
            f.write('#include "../compat/fwd.hpp" // rank 2\n')
            f.write('#include "../net/fwd.hpp" // rank 5\n')
            f.write('#include "../mmo/fwd.hpp" // rank 6\n')
            f.write('#include "../proto-base/fwd.hpp" // rank 7\n')
            f.write('// proto2/fwd.hpp is rank 8\n')
            f.write('\n\n')

            f.write('namespace tmwa\n{\n')
            for b in ['Fixed', 'Payload', 'Head', 'Repeat', 'Option']:
                c = 'Packet_' + b
                f.write('template<uint16_t PACKET_ID> class %s;\n' % c)
                f.write('template<uint16_t PACKET_ID> class Net%s;\n' % c)
            f.write('\n')

            for ty in self._types:
                if not ty.do_dump:
                    continue
                ty.dump_fwd(f)
            for ch in self._channels:
                ch.dump(outdir)

            f.write('} // namespace tmwa\n')

        for ty in self._types:
            header = 'net-%s.hpp' % ty.native_tag()
            if not ty.do_dump:
                continue
            type_headers = set()
            ty.add_headers_to(type_headers)
            type_headers = sorted(type_headers)
            with OpenWrite(os.path.join(outdir, header)) as f:
                header = '%s/%s' % (proto2, header)
                desc = 'Packet structures and conversions'
                f.write('#pragma once\n')
                f.write(copyright.format(filename=header, description=desc))
                f.write('\n')
                f.write('#include "fwd.hpp"\n\n')

                for inc in type_headers:
                    f.write(inc)
                f.write('\n')
                f.write('namespace tmwa\n{\n')
                ty.dump(f)
                f.write('} // namespace tmwa\n')

        for g in glob.glob(os.path.join(outdir, '*.old')):
            print('Obsolete: %s' % g)
            os.remove(g)

    def finish_types(self):
        s = set()
        for t in self._types:
            if not t.do_dump:
                continue
            n = t.native_tag()
            assert n not in s, n
            s.add(n)

    # types

    def poly(self, *already):
        rv = PolyFakeType(already)
        self._types.append(rv)
        return rv

    def provided(self, native, network):
        # the whole point of 'provided' is to not be implemented
        return ProvidedType(native, network)

    def enum(self, native, under):
        rv = EnumType(native, under)
        self._types.append(rv)
        return rv

    def wrap(self, native, under):
        rv = WrappedType(native, under)
        self._types.append(rv)
        return rv

    def string(self, native):
        rv = StringType(native)
        rv.do_dump = False
        self._types.append(rv)
        return rv

    def struct(self, name, body, size, ctor=False):
        rv = StructType(None, name, body, size, ctor)
        self._types.append(rv)
        return rv

    def partial_struct(self, native, body):
        rv = PartialStructType(native, body)
        self._types.append(rv)
        return rv

    def array(self, element, count):
        rv = ArrayType(element, count)
        return rv

    def earray(self, element, index, count=None):
        if count is None:
            count = index + '::COUNT'
        rv = EArrayType(element, index, count)
        return rv

    def invarray(self, element, index, count):
        rv = InvArrayType(element, index, count)
        return rv


# TOC_DATA

def build_context():

    ## setup

    ctx = Context(outdir='src/proto2/')


    ## headers

    cstdint = ctx.sysinclude('cstdint')

    endians_h = ctx.include('src/ints/little.hpp')

    vstring_h = ctx.include('src/strings/vstring.hpp')

    timet_h = ctx.include('src/compat/time_t.hpp')

    ip_h = ctx.include('src/net/ip.hpp')
    timer_th = ctx.include('src/net/timer.t.hpp')

    consts_h = ctx.include('src/mmo/consts.hpp')
    enums_h = ctx.include('src/mmo/enums.hpp')
    human_time_diff_h = ctx.include('src/mmo/human_time_diff.hpp')
    ids_h = ctx.include('src/mmo/ids.hpp')
    strs_h = ctx.include('src/mmo/strs.hpp')
    utils_h = ctx.include('src/net/timestamp-utils.hpp')
    version_h = ctx.include('src/mmo/version.hpp')

    login_th = ctx.include('src/mmo/login.t.hpp')

    clif_th = ctx.include('src/mmo/clif.t.hpp')
    skill_th = ctx.include('src/mmo/skill.t.hpp')

    ## primitive types
    char = NeutralType('char', None)
    bit = NeutralType('bool', None)

    ## included types

    uint8_t = cstdint.native('uint8_t')
    uint16_t = cstdint.native('uint16_t')
    uint32_t = cstdint.native('uint32_t')
    uint64_t = cstdint.native('uint64_t')
    int8_t = cstdint.native('int8_t')
    int16_t = cstdint.native('int16_t')
    int32_t = cstdint.native('int32_t')
    int64_t = cstdint.native('int64_t')

    Byte = endians_h.neutral('Byte')
    Little16 = endians_h.network('Little16')
    Little32 = endians_h.network('Little32')
    Little64 = endians_h.network('Little64')

    SEX = enums_h.native('SEX')
    Opt0 = enums_h.native('Opt0')
    EPOS = enums_h.native('EPOS')
    ItemLook = enums_h.native('ItemLook')

    Species = ids_h.native('Species')
    AccountId = ids_h.native('AccountId')
    CharId = ids_h.native('CharId')
    PartyId = ids_h.native('PartyId')
    ItemNameId = ids_h.native('ItemNameId')
    BlockId = ids_h.native('BlockId')
    GmLevel = ids_h.native('GmLevel')

    party_member = consts_h.native('PartyMember')

    HumanTimeDiff = human_time_diff_h.native('HumanTimeDiff')

    Version = version_h.native('Version')

    ip4 = ip_h.neutral('IP4Address')

    TimeT = timet_h.native('TimeT')

    VString16 = vstring_h.native('VString<15>')
    VString20 = vstring_h.native('VString<19>')
    VString24 = vstring_h.native('VString<23>')
    VString32 = vstring_h.native('VString<31>')
    VString40 = vstring_h.native('VString<39>')

    # not all of these are used on the network side of things
    # should this set of numbers be +1'ed ?
    timestamp_seconds_buffer = utils_h.native('timestamp_seconds_buffer')
    timestamp_milliseconds_buffer = utils_h.native('timestamp_milliseconds_buffer')
    AccountName = strs_h.native('AccountName')
    AccountPass = strs_h.native('AccountPass')
    #AccountCrypt = strs_h.native('AccountCrypt')
    AccountEmail = strs_h.native('AccountEmail')
    ServerName = strs_h.native('ServerName')
    PartyName = strs_h.native('PartyName')
    VarName = strs_h.native('VarName')
    CharName = strs_h.native('CharName')
    MapName = strs_h.native('MapName')
    #MobName = map_t_h.native('MobName')
    #NpcName = map_t_h.native('NpcName')
    #ScriptLabel = map_t_h.native('ScriptLabel')
    #ItemName = map_t_h.native('ItemName')
    #md5_native = md5_h.native('md5_native')
    #SaltString = md5_h.native('SaltString')

    Position1 = clif_th.native('Position1')
    NetPosition1 = clif_th.network('NetPosition1')
    Position2 = clif_th.native('Position2')
    NetPosition2 = clif_th.network('NetPosition2')
    BeingRemoveWhy = clif_th.native('BeingRemoveWhy')
    DIR = clif_th.native('DIR')
    Opt1 = clif_th.native('Opt1')
    Opt2 = clif_th.native('Opt2')
    Opt3 = clif_th.native('Opt3')
    ItemType = clif_th.native('ItemType')
    PickupFail = clif_th.native('PickupFail')
    DamageType = clif_th.native('DamageType')
    SP = clif_th.native('SP')
    LOOK = clif_th.native('LOOK')
    IOff2 = clif_th.native('IOff2')
    SOff1 = clif_th.native('SOff1')

    SkillID = skill_th.native('SkillID')
    StatusChange = skill_th.native('StatusChange')
    SkillFlags = skill_th.native('SkillFlags')

    tick_t = timer_th.native('tick_t')
    interval_t = timer_th.native('interval_t')

    ## generated types
    # TOC_GENTYPE

    u8 = ctx.provided(uint8_t, Byte)
    u16 = ctx.provided(uint16_t, Little16)
    u32 = ctx.provided(uint32_t, Little32)
    u64 = ctx.provided(uint64_t, Little64)
    i8 = ctx.provided(int8_t, Byte)
    i16 = ctx.provided(int16_t, Little16)
    i32 = ctx.provided(int32_t, Little32)
    i64 = ctx.provided(int64_t, Little64)

    sex_char = ctx.provided(SEX, char)

    dir = ctx.enum(DIR, u8)
    pos1 = ctx.provided(Position1, NetPosition1)
    pos2 = ctx.provided(Position2, NetPosition2)
    being_remove_why = ctx.enum(BeingRemoveWhy, u8)
    opt1 = ctx.enum(Opt1, u16)
    opt2 = ctx.enum(Opt2, u16)
    opt3 = ctx.enum(Opt3, u16)
    item_type = ctx.enum(ItemType, u8)
    pickup_fail = ctx.enum(PickupFail, u8)
    damage_type = ctx.enum(DamageType, u8)
    sp = ctx.enum(SP, u16)
    look = ctx.enum(LOOK, u8)
    ioff2 = ctx.provided(IOff2, Little16)
    soff1 = ctx.provided(SOff1, Little16)

    skill_id = ctx.enum(SkillID, u16)
    status_change = ctx.enum(StatusChange, u16)
    skill_flags = ctx.enum(SkillFlags, u16)

    tick32 = ctx.provided(tick_t, Little32)
    interval32 = ctx.provided(interval_t, Little32)
    interval16 = ctx.provided(interval_t, Little16)

    sex = ctx.enum(SEX, u8)
    option = ctx.enum(Opt0, u16)
    epos = ctx.enum(EPOS, u16)
    item_look = ctx.enum(ItemLook, u16)

    species = ctx.wrap(Species, u16)
    account_id = ctx.wrap(AccountId, u32)
    char_id = ctx.wrap(CharId, u32)
    party_id = ctx.wrap(PartyId, u32)
    item_name_id = ctx.wrap(ItemNameId, u16)
    item_name_id4 = ctx.wrap(ItemNameId, u32)
    ctx.poly(item_name_id, item_name_id4)
    block_id = ctx.wrap(BlockId, u32)

    time32 = ctx.provided(TimeT, Little32)
    time64 = ctx.provided(TimeT, Little64)
    #ctx.poly(time32, time32)

    gm1 = ctx.provided(GmLevel, Byte)
    gm2 = ctx.provided(GmLevel, Little16)
    gm = ctx.provided(GmLevel, Little32)
    #ctx.poly(gm1, gm2, gm)

    # poly is not currently needed for ProvidedType

    str16 = ctx.string(VString16)
    str20 = ctx.string(VString20)
    str24 = ctx.string(VString24)
    str32 = ctx.string(VString32)
    str40 = ctx.string(VString40)

    seconds = ctx.string(timestamp_seconds_buffer)
    millis = ctx.string(timestamp_milliseconds_buffer)
    account_name = ctx.string(AccountName)
    account_pass = ctx.string(AccountPass)
    account_email = ctx.string(AccountEmail)
    server_name = ctx.string(ServerName)
    party_name = ctx.string(PartyName)
    var_name = ctx.string(VarName)
    char_name = ctx.string(CharName)
    map_name = ctx.string(MapName)

    # this will be *so* useful when I do the party copy!
    human_time_diff = ctx.partial_struct(
            HumanTimeDiff,
            [
                ('year', i16),
                ('month', i16),
                ('day', i16),
                ('hour', i16),
                ('minute', i16),
                ('second', i16),
            ]
    )

    version = ctx.partial_struct(
            Version,
            [
                ('major', u8),
                ('minor', u8),
                ('patch', u8),
                ('devel', u8),
                ('flags', u8),
                ('which', u8),
                ('vend', u16),
            ]
    )

    stats6 = ctx.struct(
            'Stats6',
            [
                at(0, u8, 'str'),
                at(1, u8, 'agi'),
                at(2, u8, 'vit'),
                at(3, u8, 'int_'),
                at(4, u8, 'dex'),
                at(5, u8, 'luk'),
            ],
            size=6,
    )
    char_select = ctx.struct(
            'CharSelect',
            [
                at(0, char_id, 'char id'),
                at(4, u32, 'base exp'),
                at(8, u32, 'zeny'),
                at(12, u32, 'job exp'),
                at(16, u32, 'job level'),

                at(20, item_name_id, 'shoes'),
                at(22, item_name_id, 'gloves'),
                at(24, item_name_id, 'cape'),
                at(26, item_name_id, 'misc1'),
                at(28, option, 'option'),
                at(30, u16, 'unused'),

                at(32, u32, 'karma'),
                at(36, u32, 'manner'),

                at(40, u16, 'status point'),
                at(42, u16, 'hp'),
                at(44, u16, 'max hp'),
                at(46, u16, 'sp'),
                at(48, u16, 'max sp'),
                at(50, u16, 'speed'),
                at(52, species, 'species'),
                at(54, u16, 'hair style'),
                at(56, u16, 'weapon'),
                at(58, u16, 'base level'),
                at(60, u16, 'skill point'),
                at(62, item_name_id, 'head bottom'),
                at(64, item_name_id, 'shield'),
                at(66, item_name_id, 'head top'),
                at(68, item_name_id, 'head mid'),
                at(70, u16, 'hair color'),
                at(72, item_name_id, 'misc2'),

                at(74, char_name, 'char name'),

                at(98, stats6, 'stats'),
                at(104, u8, 'char num'),
                at(105, sex, 'sex'),
            ],
            size=106,
    )
    skill_info = ctx.struct(
            'SkillInfo',
            [
                at(0, skill_id, 'skill id'),
                at(2, u16, 'type or inf'),
                at(4, skill_flags, 'flags'),
                at(6, u16, 'level'),
                at(8, u16, 'sp'),
                at(10, u16, 'range'),
                at(12, str24, 'unused'),
                at(36, u8, 'can raise'),
            ],
            size=37,
    )

    item = ctx.struct(
            'Item',
            [
                at(None, item_name_id, 'nameid'),
                at(None, i16, 'amount'),
                at(None, epos, 'equip'),
            ],
            size=None,
    )

    point = ctx.struct(
            'Point',
            [
                at(None, map_name, 'map_'),
                at(None, i16, 'x'),
                at(None, i16, 'y'),
            ],
            size=None,
            ctor=True,
    )

    skill_value = ctx.struct(
            'SkillValue',
            [
                at(None, u16, 'lv'),
                at(None, skill_flags, 'flags'),
            ],
            size=None,
    )

    global_reg = ctx.struct(
            'GlobalReg',
            [
                at(None, var_name, 'str'),
                at(None, i32, 'value'),
            ],
            size=None,
    )

    char_key = ctx.struct(
            'CharKey',
            [
                at(None, char_name, 'name'),
                at(None, account_id, 'account id'),
                at(None, char_id, 'char id'),
                at(None, u8, 'char num'),
            ],
            size=None,
    )
    char_data = ctx.struct(
            'CharData',
            [
                at(None, char_id, 'partner id'),
                at(None, i32, 'base exp'),
                at(None, i32, 'job exp'),
                at(None, i32, 'zeny'),
                at(None, species, 'species'),
                at(None, i16, 'status point'),
                at(None, i16, 'skill point'),
                at(None, i32, 'hp'),
                at(None, i32, 'max hp'),
                at(None, i32, 'sp'),
                at(None, i32, 'max sp'),
                at(None, option, 'option'),
                at(None, i16, 'karma'),
                at(None, i16, 'manner'),
                at(None, i16, 'hair'),
                at(None, i16, 'hair color'),
                at(None, i16, 'clothes color'),
                at(None, party_id, 'party id'),
                at(None, item_look, 'weapon'),
                at(None, item_name_id, 'shield'),
                at(None, item_name_id, 'head top'),
                at(None, item_name_id, 'head mid'),
                at(None, item_name_id, 'head bottom'),
                at(None, u8, 'base level'),
                at(None, u8, 'job level'),
                at(None, ctx.earray(i16, 'ATTR'), 'attrs'),
                at(None, sex, 'sex'),
                at(None, ip4, 'mapip'),
                at(None, u16, 'mapport'),
                at(None, point, 'last point'),
                at(None, point, 'save point'),
                at(None, ctx.invarray(item, 'IOff0', 'MAX_INVENTORY'), 'inventory'),
                at(None, ctx.earray(skill_value, 'SkillID', 'MAX_SKILL'), 'skill'),
                at(None, i32, 'global reg num'),
                at(None, ctx.array(global_reg, 'GLOBAL_REG_NUM'), 'global reg'),
                at(None, i32, 'account reg num'),
                at(None, ctx.array(global_reg, 'ACCOUNT_REG_NUM'), 'account reg'),
                at(None, i32, 'account reg2 num'),
                at(None, ctx.array(global_reg, 'ACCOUNT_REG2_NUM'), 'account reg2'),
            ],
            size=None,
    )

    party_member = ctx.partial_struct(
            party_member,
            [
                ('account_id', account_id),
                ('name', char_name),
                ('map', map_name),
                ('leader', i32),
                ('online', i32),
                ('lv', i32),
            ]
    )

    party_most = ctx.struct(
            'PartyMost',
            [
                at(None, party_name, 'name'),
                at(None, i32, 'exp'),
                at(None, i32, 'item'),
                at(None, ctx.array(party_member, 'MAX_PARTY'), 'member'),
            ],
            size=None,
    )

    # TODO move 'account id' out somehow.
    storage = ctx.struct(
            'Storage',
            [
                at(None, bit, 'dirty'),
                at(None, account_id, 'account id'),
                at(None, i16, 'storage status'),
                at(None, i16, 'storage amount'),
                at(None, ctx.invarray(item, 'SOff0', 'MAX_STORAGE'), 'storage_'),
            ],
            size=None,
    )

    ctx.finish_types()


    ## packet channels

    # this is a somewhat simplistic view. For packets that get forwarded,
    # it may be worth pretending something like admin->char, map->login ...
    # that does break the tree format though

    # (long term) TODO reimagine this with dbus terminology:
    # every request generates exactly one reply or error, *even* if the
    # other end dies halfway through. There are also signals, which are
    # broadcast and have no reply and no error, even if no one is listening

    # (short term)
    # Having 'repeat' as a separate struct is a wart. Ideally it would be
    # a std::vector or AString member of the packet itself. Also, it should
    # be Slice or XString for input - so we need 3 types. Except, net type
    # is turning out to be not as meaningful as we thought it would be.
    #
    # A native type is either a builtin type or a type from a header.
    # A type is one of:
    #   - a native type
    #   - a structure
    #   - a string of fixed size
    #   - a string of implicit size (deprecated)
    #   - a string of explicit size
    #   - an array of fixed size
    #   - an array of implicit size (deprecated)
    #   - an array of explicit size
    #   - a map of fixed size
    #   - a map of implicit size (deprecated)
    #   - a map of explicit size
    #   - a custom-translated type
    # The last field of a struct may be of implicit size, in which case
    # the struct itself is also of implicit size. Otherwise, the struct is
    # explicitly sized if any of its members is explicitly sized, and fixed
    # sized if all of its members are fixed sized.
    #
    # The element type of an array shall not be of implicit size.
    # The element type of an array *may* be of explicit size, in which case
    # the layout is like option 1 below (see below for discussion).
    #
    # A string is just an array of characters, except that it may be padded
    # with '\0' bytes even when it is sized.
    # A map is just an array of two-element structs (key, value)
    # However, strings and maps have custom classes used to represent them
    # on the sender and receiver (earray also has this (but maybe shouldn't?)).
    # Ytf is tmwa still using fixed-size arrays for this crap anyways?
    # It probably should be a map in this case.
    #
    # It would probably be a good idea if everybody parsed network input as
    # padded with '\0's if it is too short, and ignored the extra if it is
    # too long. However, there are efficiency concerns with this, since we
    # don't want to branch everywhere. But parsing in reverse might work ...

    # possible array layouts:
    # 1:
    # | array1 size | array1 data | array2 size | array2 data |
    # Advantage: linear writing (except final size?).
    # Disadvantage: linear reading.
    # This is what I originally imagined, but ...
    # 2:
    # | array1 size/offset | array2 size/offset | array1 data | array2 data |
    # Advantage: random-access reading.
    # Disadvantage: slightly larger packets, more complicated writing?.
    # TODO verify whether you really need offset or not?
    # TODO think about array-of-array case, it's complicated.
    # | array0 size | array1 size | array 0.0 size | array 0.1 size | array 1.0 size | array 1.1 size | element 0.0.0 | element 0.0.1 | element 0.1.0 | element 0.1.1 | element 1.0.0 | element 1.0.1 | element 1.1.0 | element 1.1.1 |
    # data a=(b=[d=[h=0.0.0, i=0.0.1], e=[j=0.1.0, k=0.1.1]], c=[f=[l=1.0.0, m=1.0.1], g=[n=1.1.0, o=1.1.1]])
    # I'm not sure how this can work. If I wrote:
    # def write_a():
    #   write_head_b()  # size
    #   write_head_c()  # size
    #   write_data_b()
    #   write_data_c()
    # def write_data_b():
    #   write_head_d()
    #   write_head_e()
    #   write_data_d()
    #   write_data_e()
    # def write_data_c():
    #   write_head_f()
    #   write_head_g()
    #   write_data_f()
    #   write_data_g()
    # ... that would yield |s0|s1|s0.0|s0.1|d0.0.0|d0.0.1|d0.1.0|d0.1.1|s1.0|s1.1|d1.0.0|d1.0.1|d1.1.0|d1.1.1|
    # Can't do random-access writes because you don't know the offset yet.
    # I would have to do a bunch of 'write depth n' functions.
    # Maybe this was a bad idea.
    # And what exactly is the use of random-access reads? We're doing full
    # translation for all data anyway. It's probably a bad idea for any
    # packet to ever contain information that you don't look at.


    login_char = ctx.chan('login', 'char')
    login_admin = ctx.chan('login', 'admin')
    login_bot = NotImplemented
    login_user = ctx.chan('login', 'user')

    char_map = ctx.chan('char', 'map')
    char_admin = NotImplemented
    char_bot = NotImplemented
    char_user = ctx.chan('char', 'user')

    map_admin = NotImplemented
    map_bot = NotImplemented
    map_user = ctx.chan('map', 'user')

    any_user = ctx.chan('any', 'user')


    ## legacy packets

    # TOC_CLIENT
    # * user
    char_user.r(0x0061, 'change password',
        define='CMSG_CHAR_PASSWORD_CHANGE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_pass, 'old pass'),
            at(26, account_pass, 'new pass'),
        ],
        fixed_size=50,
        pre=[HUMAN],
        post=[0x2740],
        desc='''
            Sent by a client to the character server to request a password change.
        ''',
    )
    char_user.s(0x0062, 'change password result',
        define='SMSG_CHAR_PASSWORD_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'status'),
        ],
        fixed_size=3,
        pre=[0x2741],
        post=[PRETTY],
        desc='''
            Sent by the character server with the response of a password change request.

            Status:
                0: The account was not found.
                1: Success.
                2: The old password was incorrect.
                3: The new password was too short.
        ''',
    )
    login_user.s(0x0063, 'update host notify',
        define='SMSG_UPDATE_HOST',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
        pre=[0x0064],
        post=[IDLE],
        desc='''
            This packet gives the client the location of the update server URL, such as http://tmwdata.org/updates/

            It is only sent if an update host is specified for the server (there is one in the default configuration) and the client identifies as accepting an update host (which all supported clients do).
        ''',
    )
    login_user.r(0x0064, 'account login',
        define='CMSG_LOGIN_REGISTER',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'unknown'),
            at(6, account_name, 'account name'),
            at(30, account_pass, 'account pass'),
            at(54, u8, 'version'),
        ],
        fixed_size=55,
        pre=[HUMAN, 0x7531],
        post=[0x0063, 0x0069, 0x006a, 0x0081],
        desc='''
            Authenticate a client by user/password.

            All clients must now set both defined version 2 flags.
        ''',
    )
    char_user.r(0x0065, 'connect char',
        define='CMSG_CHAR_SERVER_CONNECT',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'login id1'),
            at(10, u32, 'login id2'),
            at(14, u16, 'packet client version'),
            at(16, sex, 'sex'),
        ],
        fixed_size=17,
        pre=[0x0069, 0x0092, 0x00b3],
        post=[0x006b, 0x006c, 0x2712, 0x2716],
        desc='''
            Begin connection to the char server, based on keys the login
            server gave us.
        ''',
    )
    char_user.r(0x0066, 'select character',
        define='CMSG_CHAR_SELECT',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
        pre=[0x006b],
        post=[FINISH, 0x0071, 0x0081],
        desc='''
            Choose a character to enter the map.
        ''',
    )
    char_user.r(0x0067, 'create character',
        define='CMSG_CHAR_CREATE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'char name'),
            at(26, stats6, 'stats'),
            at(32, u8, 'slot'),
            at(33, u16, 'hair color'),
            at(35, u16, 'hair style'),
        ],
        fixed_size=37,
        pre=[0x006b],
        post=[0x006d, 0x006e],
        desc='''
            Create a new character.
        ''',
    )
    char_user.r(0x0068, 'delete character',
        define='CMSG_CHAR_DELETE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, account_email, 'unused email'),
        ],
        fixed_size=46,
        pre=[0x006b],
        post=[0x006f, 0x0070, 0x2afe, 0x2b12, 0x3821, 0x3824, 0x3826],
        desc='''
            Delete an existing character.

            There is no authentication on the server besides what has
            already been performed to create the connection. "Are you sure?"
            is solely the client's job.
        ''',
    )
    login_user.s(0x0069, 'account login success',
        define='SMSG_LOGIN_DATA',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, u32, 'login id1'),
            at(8, account_id, 'account id'),
            at(12, u32, 'login id2'),
            at(16, u32, 'unused'),
            at(20, millis, 'last login string'),
            at(44, u16, 'unused2'),
            at(46, sex, 'sex'),
        ],
        head_size=47,
        repeat=[
            at(0, ip4, 'ip'),
            at(4, u16, 'port'),
            at(6, server_name, 'server name'),
            at(26, u16, 'users'),
            at(28, u16, 'maintenance'),
            at(30, u16, 'is new'),
        ],
        repeat_size=32,
        pre=[0x0064],
        post=[0x0065],
        desc='''
            Big blob of information available once when you authenticate:

            * dumb session keys
            * sex and last login
            * list of char server
        ''',
    )
    login_user.s(0x006a, 'account login error',
        define='SMSG_LOGIN_ERROR',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'error code'),
            at(3, seconds, 'error message'),
        ],
        fixed_size=23,
        pre=[0x0064],
        post=[PRETTY],
        desc='''
            Failure to log in.

            Error codes:
            * 0: unregistered id
            * 1: incorrect password
            * 2: expired id (unused?)
            * 3: rejected from server (unused?)
            * 4: permanently blocked
            * 5: client too old (unused?)
            * 6: temporary ban (date in 'error message' field)
            * 7: server full
            * 8: no message
            * 99: id erased
        ''',
    )
    char_user.s(0x006b, 'connect char success',
        define='SMSG_CHAR_LOGIN',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, str20, 'unused'),
        ],
        head_size=24,
        repeat=[
            at(0, char_select, 'char select'),
        ],
        repeat_size=106,
        pre=[0x0065, 0x2713],
        post=[PRETTY],
        xpost=[0x0066, 0x0067, 0x0068],
        desc='''
            List account's characters on this server.
        ''',
    )
    char_user.s(0x006c, 'connect char error',
        define='SMSG_CHAR_LOGIN_ERROR',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
        pre=[0x0065, 0x2713],
        post=[PRETTY],
        desc='''
            Refuse connection.

            Status:
                0: Overpopulated
                0x42: Auth failed
        ''',
    )
    char_user.s(0x006d, 'create character success',
        define='SMSG_CHAR_CREATE_SUCCEEDED',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_select, 'char select'),
        ],
        fixed_size=108,
        pre=[0x0067],
        post=[PRETTY],
        desc='''
            Give information about newly-created character.
        ''',
    )
    char_user.s(0x006e, 'create character error',
        define='SMSG_CHAR_CREATE_FAILED',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
        pre=[0x0067],
        post=[PRETTY],
        desc='''
            Failure to create a new character.
        ''',
    )
    char_user.s(0x006f, 'delete character success',
        define='SMSG_CHAR_DELETE_SUCCEEDED',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[0x0068],
        post=[PRETTY],
        desc='''
            Character successfully deleted.
        ''',
    )
    char_user.s(0x0070, 'delete character error',
        define='SMSG_CHAR_DELETE_FAILED',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
        pre=[0x0068],
        post=[PRETTY],
        desc='''
            Failure to delete character.
        ''',
    )
    char_user.s(0x0071, 'select character success',
        define='SMSG_CHAR_MAP_INFO',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, map_name, 'map name'),
            at(22, ip4, 'ip'),
            at(26, u16, 'port'),
        ],
        fixed_size=28,
        pre=[0x0066],
        post=[0x0072],
        desc='''
            Return character location and map server IP.
        ''',
    )
    map_user.r(0x0072, 'connect map',
        define='CMSG_MAP_SERVER_CONNECT',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_id, 'char id'),
            at(10, u32, 'login id1'),
            at(14, u32, 'client tick'),
            at(18, sex, 'sex'),
        ],
        fixed_size=19,
        pre=[0x0071, 0x0092],
        post=[0x0081, 0x2afc],
        desc='''
            Begin connection to the map server, based on keys the login
            server gave us.
        ''',
    )
    map_user.s(0x0073, 'connect map success',
        define='SMSG_MAP_LOGIN_SUCCESS',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, tick32, 'tick'),
            at(6, pos1, 'pos'),
            at(9, u8, 'five1'),
            at(10, u8, 'five2'),
        ],
        fixed_size=11,
        pre=[0x2afd],
        post=[PRETTY, 0x007e],
        desc='''
            Successfully auth'd to the map server
        ''',
    )
    map_user.s(0x0078, 'being appear notify',
        define='SMSG_BEING_VISIBLE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, interval16, 'speed'),
            at(8, opt1, 'opt1'),
            at(10, opt2, 'opt2'),
            at(12, option, 'option'),
            at(14, species, 'species'),
            at(16, u8, 'hair style'),
            at(17, u8, 'look'),
            at(18, u16, 'weapon'),
            at(20, u16, 'head bottom'),
            at(22, u16, 'shield'),
            at(24, u16, 'head top'),
            at(26, u16, 'head mid'),
            at(28, u8, 'hair color'),
            at(29, u8, 'unused 1'),
            at(30, u16, 'shoes or clothes color'),
            at(32, u16, 'gloves or part of hp'),
            at(34, u16, 'part of guild id or part of hp'),
            at(36, u16, 'part of guild id or part of max hp'),
            at(38, u16, 'guild emblem or part of max hp'),
            at(40, u16, 'manner'),
            at(42, u16, 'opt3'),
            at(44, u8, 'karma or attack range'),
            at(45, sex, 'sex'),
            at(46, pos1, 'pos'),
            at(49, u8, 'unused 2'),
            at(50, u8, 'unused 3'),
            at(51, u8, 'unused 4'),
            at(52, u8, 'unused 5'),
            at(53, u8, 'unused 6'),
        ],
        fixed_size=54,
        pre=[BOOT, FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00b8, 0x00b9, 0x00e6, 0x00f7, 0x0143, 0x0146, 0x01d5],
        post=[0x0094],
        desc='''
            A being is stationary.
        ''',
    )
    map_user.s(0x007b, 'being move notify',
        define='SMSG_BEING_MOVE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, interval16, 'speed'),
            at(8, opt1, 'opt1'),
            at(10, opt2, 'opt2'),
            at(12, option, 'option'),
            at(14, species, 'mob class'),
            at(16, u8, 'hair style'),
            at(17, u8, 'look'),
            at(18, u16, 'unused weapon'),
            at(20, u16, 'unused head bottom'),
            at(22, tick32, 'tick'),
            at(26, u16, 'unused shield'),
            at(28, u16, 'unused head top'),
            at(30, u16, 'unused head mid'),
            at(32, u8, 'unused hair color'),
            at(33, u8, 'unused 1'),
            at(34, u16, 'unused clothes color'),
            at(36, u16, 'gloves or part of hp'),
            at(38, u16, 'part of guild id or part of hp'),
            at(40, u16, 'part of guild id or part of max hp'),
            at(42, u16, 'guild emblem or part of max hp'),
            at(44, u16, 'manner'),
            at(46, u16, 'opt3'),
            at(48, u8, 'karma or attack range'),
            at(49, u8, 'unused sex'),
            at(50, pos2, 'pos2'),
            at(55, u8, 'unused 2'),
            at(56, u8, 'unused 3'),
            at(57, u8, 'unused 4'),
            at(58, u8, 'unused 5'),
            at(59, u8, 'unused 6'),
        ],
        fixed_size=60,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f],
        post=[0x0094],
        desc='''
            A being is moving.
        ''',
    )
    map_user.s(0x007c, 'being spawn notify',
        define='SMSG_BEING_SPAWN',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, interval16, 'speed'),
            at(8, opt1, 'opt1'),
            at(10, opt2, 'opt2'),
            at(12, option, 'option'),
            at(14, u16, 'unknown 1'),
            at(16, u16, 'unknown 2'),
            at(18, u16, 'unknown 3'),
            at(20, species, 'species'),
            at(22, u16, 'unknown 4'),
            at(24, u16, 'unknown 5'),
            at(26, u16, 'unknown 6'),
            at(28, u16, 'unknown 7'),
            at(30, u16, 'unknown 8'),
            at(32, u16, 'unknown 9'),
            at(34, u16, 'unknown 10'),
            at(36, pos1, 'pos'),
            at(39, u16, 'unknown 11'),
        ],
        fixed_size=41,
        pre=[BOOT, FINISH, GM, MAGIC, SCRIPT, TIMER, 0x0089, 0x008c, 0x00b8, 0x00b9, 0x00e6, 0x00f7, 0x0143, 0x0146, 0x01d5],
        post=[PRETTY],
        desc='''
            A being is created.
        ''',
    )
    map_user.r(0x007d, 'map loaded',
        define='CMSG_MAP_LOADED',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[0x0091],
        post=[SCRIPT, 0x0078, 0x007b, 0x009d, 0x00a4, 0x00b0, 0x00b1, 0x00bd, 0x00fb, 0x0101, 0x010f, 0x0119, 0x013a, 0x0141, 0x019b, 0x01d7, 0x01d8, 0x01d9, 0x01da, 0x01ee, 0x3025, 0x3028],
        xpost=[0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x013c, 0x0196, 0x01b1, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Changed map; need info.
        ''',
    )
    map_user.r(0x007e, 'ping',
        define='CMSG_MAP_PING',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'client tick'),
        ],
        fixed_size=6,
        pre=[TIMER, 0x0073],
        post=[0x007f],
        desc='''
            Request ping.
        ''',
    )
    map_user.s(0x007f, 'pong',
        define='SMSG_SERVER_PING',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, tick32, 'tick'),
        ],
        fixed_size=6,
        pre=[0x007e],
        post=[NOTHING],
        desc='''
            Provide ping.
        ''',
    )
    map_user.s(0x0080, 'remove being notify',
        define='SMSG_BEING_REMOVE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, being_remove_why, 'type'),
        ],
        fixed_size=7,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x0090, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            A being disappeared.
        ''',
    )
    any_user.s(0x0081, 'connect foo error',
        define='SMSG_CONNECTION_PROBLEM',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'error code'),
        ],
        fixed_size=3,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x0064, 0x0066, 0x0072, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2afe, 0x2b06, 0x2b0d],
        post=[PRETTY],
        desc='''
            Failed to connect to some server (multiple meanings).
        ''',
    )
    map_user.r(0x0085, 'walk',
        define='CMSG_PLAYER_CHANGE_DEST',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, pos1, 'pos'),
        ],
        fixed_size=5,
        pre=[HUMAN],
        post=[0x0080, 0x0087, 0x01d7, 0x01da],
        xpost=[SCRIPT, 0x0078, 0x007b, 0x0081, 0x0088, 0x0091, 0x009d, 0x00a0, 0x00a1, 0x00ac, 0x00b0, 0x00b1, 0x00b4, 0x00b6, 0x00be, 0x00c0, 0x00c4, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d8, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Start walking somewhere.
        ''',
    )
    # 0x0086 define='SMSG_BEING_MOVE2',
    map_user.s(0x0087, 'walk success',
        define='SMSG_WALK_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, tick32, 'tick'),
            at(6, pos2, 'pos2'),
            at(11, u8, 'zero'),
        ],
        fixed_size=12,
        pre=[0x0085, 0x0089],
        post=[IDLE],
        desc='''
            Confirm that you did walk somewhere.

            No corresponding error!
        ''',
    )
    map_user.s(0x0088, 'stop walking notify',
        define='SMSG_PLAYER_STOP',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u16, 'x'),
            at(8, u16, 'y'),
        ],
        fixed_size=10,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Being stopped walking.
        ''',
    )
    map_user.r(0x0089, 'player action',
        define='CMSG_PLAYER_CHANGE_ACT',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'target id'),
            at(6, damage_type, 'action'),
        ],
        fixed_size=7,
        pre=[HUMAN],
        post=[MAGIC, 0x0080, 0x008a, 0x0110, 0x0139],
        xpost=[SCRIPT, 0x0078, 0x007b, 0x007c, 0x0081, 0x0087, 0x0088, 0x0091, 0x009e, 0x00a0, 0x00a1, 0x00ac, 0x00af, 0x00b0, 0x00b1, 0x00b4, 0x00b6, 0x00be, 0x00c0, 0x00c4, 0x00e9, 0x00ee, 0x00fb, 0x00fd, 0x0101, 0x0106, 0x010f, 0x0119, 0x013a, 0x013b, 0x0141, 0x0196, 0x019b, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x01de, 0x2b01, 0x2b05, 0x3011, 0x3022, 0x3025, 0x3028],
        desc='''
            Perform an action.

            Action:
            * single attack
            * continuous attack
            * sit
            * stand
            * (other actions on return only - special calls)
        ''',
    )
    map_user.s(0x008a, 'being action notify',
        define='SMSG_BEING_ACTION',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'src id'),
            at(6, block_id, 'dst id'),
            at(10, tick32, 'tick'),
            at(14, interval32, 'sdelay'),
            at(18, interval32, 'ddelay'),
            at(22, u16, 'damage'),
            at(24, u16, 'div'),
            at(26, damage_type, 'damage type'),
            at(27, u16, 'damage2'),
        ],
        fixed_size=29,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x0089, 0x009f],
        post=[PRETTY],
        desc='''
            Somebody performed an action on something/somebody.
        ''',
    )
    map_user.r(0x008c, 'global chat',
        define='CMSG_CHAT_MESSAGE',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[HUMAN],
        post=[GM, MAGIC, 0x008d, 0x008e],
        xpost=[SCRIPT, 0x0078, 0x007b, 0x007c, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00af, 0x00b0, 0x00b1, 0x00b4, 0x00b6, 0x00be, 0x00c0, 0x00c4, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x2b0e, 0x3003, 0x3011, 0x3022],
        desc='''
            Talk to everyone nearby.
        ''',
    )
    map_user.s(0x008d, 'global chat notify',
        define='SMSG_BEING_CHAT',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, block_id, 'block id'),
        ],
        head_size=8,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[GM, SCRIPT, 0x008c],
        post=[PRETTY],
        desc='''
            Somebody is talking (not just a player).
        ''',
    )
    map_user.s(0x008e, 'global chat result',
        define='SMSG_PLAYER_CHAT',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[OTHER, 0x008c],
        post=[PRETTY],
        desc='''
            You talked.
        ''',
    )
    map_user.r(0x0090, 'npc click',
        define='CMSG_NPC_TALK',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u8, 'unused'),
        ],
        fixed_size=7,
        pre=[HUMAN],
        post=[SCRIPT, 0x0080, 0x00b4, 0x00b6, 0x00c4],
        xpost=[0x00c0],
        desc='''
            Click on an NPC, to invoke its script/shop/message.

            Error if already talking to an NPC or too far.

            No error if clicking on a warp.
        ''',
    )
    map_user.s(0x0091, 'change map notify',
        define='SMSG_PLAYER_WARP',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, map_name, 'map name'),
            at(18, u16, 'x'),
            at(20, u16, 'y'),
        ],
        fixed_size=22,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[0x007d],
        desc='''
            You teleported to a new map (possible the same map), but on the
            same server.
        ''',
    )
    map_user.s(0x0092, 'change map server notify',
        define='SMSG_CHANGE_MAP_SERVER',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, map_name, 'map name'),
            at(18, u16, 'x'),
            at(20, u16, 'y'),
            at(22, ip4, 'ip'),
            at(26, u16, 'port'),
        ],
        fixed_size=28,
        pre=[0x2b06],
        post=[0x0065, 0x0072],
        desc='''
            You teleported to another server.
        ''',
    )
    map_user.r(0x0094, 'get being name',
        define='CMSG_NAME_REQUEST',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
        pre=[TIMER, 0x0078, 0x007b, 0x01d8, 0x01d9, 0x01da],
        post=[0x0095, 0x0195, 0x020c],
        desc='''
            Request a beings name. No reply if wrong type.

            Also send misc other info about players.
        ''',
    )
    map_user.s(0x0095, 'get being name result',
        define='SMSG_BEING_NAME_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, char_name, 'char name'),
        ],
        fixed_size=30,
        pre=[0x0094],
        post=[PRETTY],
        desc='''
            Somebody has a name.
        ''',
    )
    map_user.r(0x0096, 'whisper',
        define='CMSG_CHAT_WHISPER',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, char_name, 'target name'),
        ],
        head_size=28,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[HUMAN],
        post=[GM, 0x0097, 0x0098, 0x3001],
        xpost=[0x2b0e, 0x3003],
        desc='''
            Talk to someone privately.
        ''',
    )
    map_user.s(0x0097, 'receive whisper',
        define='SMSG_WHISPER',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, char_name, 'char name'),
        ],
        head_size=28,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x0096, 0x3801, 0x3803],
        post=[PRETTY],
        desc='''
            Somebody is talking to you privately.
        '''
    )
    map_user.s(0x0098, 'whisper result',
        define='SMSG_WHISPER_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'flag'),
        ],
        fixed_size=3,
        pre=[0x0096, 0x3802],
        post=[PRETTY],
        desc='''
            Did you successfully talk to someone?
        ''',
    )
    # 0x0099 define='CMSG_ADMIN_ANNOUNCE',
    map_user.s(0x009a, 'gm announcement notify',
        define='SMSG_GM_CHAT',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[GM, SCRIPT, 0x3800],
        post=[PRETTY],
        desc='''
            The GMs are shouting in red.
        ''',
    )
    map_user.r(0x009b, 'face direction',
        define='CMSG_PLAYER_CHANGE_DIR',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'unused'),
            at(4, u8, 'client dir'),
        ],
        fixed_size=5,
        pre=[HUMAN],
        post=[0x009c],
        desc='''
            Look in a different direction, without moving.
        ''',
    )
    map_user.s(0x009c, 'face direction notify',
        define='SMSG_BEING_CHANGE_DIRECTION',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u16, 'zero'),
            at(8, u8, 'client dir'),
        ],
        fixed_size=9,
        pre=[0x009b],
        post=[PRETTY],
        desc='''
            Somebody looked in a different direction, without moving.
        ''',
    )
    map_user.s(0x009d, 'item visible notify',
        define='SMSG_ITEM_VISIBLE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, item_name_id, 'name id'),
            at(8, u8, 'identify'),
            at(9, u16, 'x'),
            at(11, u16, 'y'),
            at(13, u16, 'amount'),
            at(15, u8, 'subx'),
            at(16, u8, 'suby'),
        ],
        fixed_size=17,
        pre=[0x007d, 0x0085],
        post=[PRETTY],
        desc='''
            An item appeared on the ground.
        ''',
    )
    map_user.s(0x009e, 'item dropped notify',
        define='SMSG_ITEM_DROPPED',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, item_name_id, 'name id'),
            at(8, u8, 'identify'),
            at(9, u16, 'x'),
            at(11, u16, 'y'),
            at(13, u8, 'subx'),
            at(14, u8, 'suby'),
            at(15, u16, 'amount'),
        ],
        fixed_size=17,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x0089, 0x00a2],
        post=[PRETTY],
        desc='''
            An item was dropped on the ground.
        ''',
    )
    map_user.r(0x009f, 'item pickup',
        define='CMSG_ITEM_PICKUP',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'object id'),
        ],
        fixed_size=6,
        pre=[HUMAN],
        post=[SCRIPT, 0x0080, 0x008a, 0x0091, 0x00a0, 0x00a1, 0x00b0],
        xpost=[0x0078, 0x007b, 0x0081, 0x0088, 0x00ac, 0x00b1, 0x00b4, 0x00b6, 0x00be, 0x00c0, 0x00c4, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Hey, it's just lying around on the floor, I want it.
        ''',
    )
    map_user.s(0x00a0, 'inventory add notify',
        define='SMSG_PLAYER_INVENTORY_ADD',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u16, 'amount'),
            at(6, item_name_id, 'name id'),
            at(8, u8, 'identify'),
            at(9, u8, 'broken or attribute'),
            at(10, u8, 'refine'),
            at(11, u16, 'card0'),
            at(13, u16, 'card1'),
            at(15, u16, 'card2'),
            at(17, u16, 'card3'),
            at(19, epos, 'epos'),
            at(21, item_type, 'item type'),
            at(22, pickup_fail, 'pickup fail'),
        ],
        fixed_size=23,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Item appeared in your inventory.
        ''',
    )
    map_user.s(0x00a1, 'flooritem delete notify',
        define='SMSG_ITEM_REMOVE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x0085, 0x0089, 0x009f, 0x00a2],
        post=[PRETTY],
        desc='''
            Item disappeared from the floor.
        ''',
    )
    map_user.r(0x00a2, 'drop item',
        define='CMSG_PLAYER_INVENTORY_DROP',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u16, 'amount'),
        ],
        fixed_size=6,
        pre=[HUMAN],
        post=[0x0080, 0x009e, 0x00ac, 0x00af, 0x01d7],
        xpost=[SCRIPT, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00a1, 0x00b0, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            This is worthless, just leave it lying on the ground.

            Also used by people who don't understand trades.
        ''',
    )
    map_user.s(0x00a4, 'inventory equipment notify',
        define='SMSG_PLAYER_EQUIPMENT',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, ioff2, 'ioff2'),
            at(2, item_name_id, 'name id'),
            at(4, item_type, 'item type'),
            at(5, u8, 'identify'),
            at(6, epos, 'epos pc'),
            at(8, epos, 'epos inv'),
            at(10, u8, 'broken or attribute'),
            at(11, u8, 'refine'),
            at(12, u16, 'card0'),
            at(14, u16, 'card1'),
            at(16, u16, 'card2'),
            at(18, u16, 'card3'),
        ],
        repeat_size=20,
        pre=[0x007d],
        post=[PRETTY],
        desc='''
            Complete list of equipment in inventory.
        ''',
    )
    map_user.s(0x00a6, 'storage equipment notify',
        define='SMSG_PLAYER_STORAGE_EQUIP',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, soff1, 'soff1'),
            at(2, item_name_id, 'name id'),
            at(4, item_type, 'item type'),
            at(5, u8, 'identify'),
            at(6, epos, 'epos id'),
            at(8, epos, 'epos stor'),
            at(10, u8, 'broken or attribute'),
            at(11, u8, 'refine'),
            at(12, u16, 'card0'),
            at(14, u16, 'card1'),
            at(16, u16, 'card2'),
            at(18, u16, 'card3'),
        ],
        repeat_size=20,
        pre=[GM, SCRIPT, 0x3810],
        post=[PRETTY],
        desc='''
            Complete list of equipment in storage.
        ''',
    )
    map_user.r(0x00a7, 'use item',
        define='CMSG_PLAYER_INVENTORY_USE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u32, 'unused id'),
        ],
        fixed_size=8,
        pre=[HUMAN],
        post=[SCRIPT, 0x0080, 0x00a8, 0x00ac, 0x00af, 0x00b0, 0x01c8, 0x01d7],
        xpost=[0x0081, 0x0088, 0x0091, 0x00a0, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Use a consumable item in your inventory.
        ''',
    )
    map_user.s(0x00a8, 'use item result',
        define='SMSG_ITEM_USE_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u16, 'amount'),
            at(6, u8, 'ok'),
        ],
        fixed_size=7,
        pre=[0x00a7],
        post=[PRETTY],
        desc='''
            You used (or tried to use) an item in your inventory.

            Currently only used in the failure case.
        ''',
    )
    map_user.r(0x00a9, 'equip item',
        define='CMSG_PLAYER_EQUIP',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, epos, 'epos ignored'),
        ],
        fixed_size=6,
        pre=[HUMAN],
        post=[SCRIPT, 0x0080, 0x00aa, 0x00ac, 0x013b, 0x013c, 0x01d7],
        xpost=[0x0081, 0x0088, 0x0091, 0x00a0, 0x00b0, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Put on armor or something.
        ''',
    )
    map_user.s(0x00aa, 'equip item result',
        define='SMSG_PLAYER_EQUIP',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, epos, 'epos'),
            at(6, u8, 'ok'),
        ],
        fixed_size=7,
        pre=[0x00a9],
        post=[PRETTY],
        desc='''
            Result of trying to wear something.
        ''',
    )
    map_user.r(0x00ab, 'unequip item',
        define='CMSG_PLAYER_UNEQUIP',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
        ],
        fixed_size=4,
        pre=[HUMAN],
        post=[SCRIPT, 0x0080, 0x00ac, 0x01d7],
        xpost=[0x0081, 0x0088, 0x0091, 0x00a0, 0x00b0, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Take off armor or something.
        ''',
    )
    map_user.s(0x00ac, 'unequip item result',
        define='SMSG_PLAYER_UNEQUIP',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, epos, 'epos'),
            at(6, u8, 'ok'),
        ],
        fixed_size=7,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Result of trying to unwear something.
        ''',
    )
    map_user.s(0x00af, 'inventory delete notify',
        define='SMSG_PLAYER_INVENTORY_REMOVE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u16, 'amount'),
        ],
        fixed_size=6,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x0089, 0x008c, 0x00a2, 0x00a7, 0x00c9, 0x00ef, 0x00f3],
        post=[PRETTY],
        desc='''
            An item has been deleted from your inventory.
        ''',
    )
    map_user.s(0x00b0, 'player stat update 1 notify',
        define='SMSG_PLAYER_STAT_UPDATE_1',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u32, 'value'),
        ],
        fixed_size=8,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            One of many player stat packets with no real difference.
        ''',
    )
    map_user.s(0x00b1, 'player stat update 2 notify',
        define='SMSG_PLAYER_STAT_UPDATE_2',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u32, 'value'),
        ],
        fixed_size=8,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            One of many player stat packets with no real difference.
        ''',
    )
    map_user.r(0x00b2, 'respawn or switch character',
        define='CMSG_PLAYER_REBOOT',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'flag'),
        ],
        fixed_size=3,
        pre=[HUMAN, 0x01d8],
        post=[0x0080, 0x0091, 0x00b0, 0x018b, 0x2b02, 0x2b05],
        xpost=[SCRIPT, 0x0081, 0x0088, 0x00a0, 0x00ac, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x3011, 0x3022],
        desc='''
            If flag is 0, respawn after dying. If flag is 1, try to switch characters.
        ''',
    )
    map_user.s(0x00b3, 'character switch success',
        define='SMSG_CHAR_SWITCH_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'one'),
        ],
        fixed_size=3,
        pre=[0x2b03],
        post=[0x0065],
        desc='''
            Permission granted to switch characters.
        ''',
    )
    map_user.s(0x00b4, 'script message notify',
        define='SMSG_NPC_MESSAGE',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, block_id, 'block id'),
        ],
        head_size=8,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[MAGIC, SCRIPT, 0x0085, 0x0089, 0x008c, 0x0090, 0x009f],
        post=[PRETTY],
        desc='''
            An npc is droning on.
        ''',
    )
    map_user.s(0x00b5, '(reverse) script next',
        define='SMSG_NPC_NEXT',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
        pre=[SCRIPT],
        post=[0x00b9],
        desc='''
            An npc paused briefly to catch its breath.
        ''',
    )
    map_user.s(0x00b6, '(reverse) script close',
        define='SMSG_NPC_CLOSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
        pre=[GM, MAGIC, SCRIPT, 0x0085, 0x0089, 0x008c, 0x0090, 0x009f, 0x00b8, 0x00b9, 0x00e6, 0x00f7, 0x0143, 0x0146, 0x01d5],
        post=[0x0146],
        desc='''
            An npc finally shut up.
        ''',
    )
    map_user.s(0x00b7, '(reverse) script menu',
        define='SMSG_NPC_CHOICE',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, block_id, 'block id'),
        ],
        head_size=8,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[SCRIPT],
        post=[0x00b8],
        desc='''
            An npc let you choose how it will drone on.
        ''',
    )
    map_user.r(0x00b8, '(reverse) script menu result',
        define='CMSG_NPC_LIST_CHOICE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'npc id'),
            at(6, u8, 'menu entry'),
        ],
        fixed_size=7,
        pre=[0x00b7],
        post=[MAGIC, SCRIPT],
        xpost=[0x0078, 0x007c, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00b0, 0x00b1, 0x00b6, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Choose an entry from the menu (1-based), 0xff to run away.
        ''',
    )
    map_user.r(0x00b9, '(reverse) script next result',
        define='CMSG_NPC_NEXT_REQUEST',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'npc id'),
        ],
        fixed_size=6,
        pre=[0x00b5],
        post=[MAGIC, SCRIPT],
        xpost=[0x0078, 0x007c, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00b0, 0x00b1, 0x00b6, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Let the npc keep breathing.
        ''',
    )
    map_user.r(0x00bb, 'stat increase',
        define='CMSG_STAT_UPDATE_REQUEST',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'asp'),
            at(4, u8, 'unused'),
        ],
        fixed_size=5,
        pre=[HUMAN],
        post=[0x00b0, 0x00bc, 0x00be, 0x0141],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00b1, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Spend stat points.
        ''',
    )
    map_user.s(0x00bc, 'stat increase result',
        define='SMSG_PLAYER_STAT_UPDATE_4',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u8, 'ok'),
            at(5, u8, 'val'),
        ],
        fixed_size=6,
        pre=[SCRIPT, 0x00bb],
        post=[PRETTY],
        desc='''
            Spent stat points?
        ''',
    )
    map_user.s(0x00bd, 'player stat update 5 notify',
        define='SMSG_PLAYER_STAT_UPDATE_5',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'status point'),
            at(4, u8, 'str attr'),
            at(5, u8, 'str upd'),
            at(6, u8, 'agi attr'),
            at(7, u8, 'agi upd'),
            at(8, u8, 'vit attr'),
            at(9, u8, 'vit upd'),
            at(10, u8, 'int attr'),
            at(11, u8, 'int upd'),
            at(12, u8, 'dex attr'),
            at(13, u8, 'dex upd'),
            at(14, u8, 'luk attr'),
            at(15, u8, 'luk upd'),
            at(16, u16, 'atk sum'),
            at(18, u16, 'watk2'),
            at(20, u16, 'matk1'),
            at(22, u16, 'matk2'),
            at(24, u16, 'def'),
            at(26, u16, 'def2'),
            at(28, u16, 'mdef'),
            at(30, u16, 'mdef2'),
            at(32, u16, 'hit'),
            at(34, u16, 'flee'),
            at(36, u16, 'flee2'),
            at(38, u16, 'critical'),
            at(40, u16, 'karma'),
            at(42, u16, 'manner'),
        ],
        fixed_size=44,
        pre=[0x007d],
        post=[PRETTY],
        desc='''
            Bulk notify of stats.
        ''',
    )
    map_user.s(0x00be, 'stat price notify',
        define='SMSG_PLAYER_STAT_UPDATE_6',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u8, 'value'),
        ],
        fixed_size=5,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Cost of spending stat points.
        ''',
    )
    map_user.r(0x00bf, 'emote',
        define='CMSG_PLAYER_EMOTE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'emote'),
        ],
        fixed_size=3,
        pre=[HUMAN, TIMER],
        post=[0x00c0, 0x110],
        desc='''
            Don't act like a faceless robot.
        ''',
    )
    map_user.s(0x00c0, 'emote notify',
        define='SMSG_BEING_EMOTION',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u8, 'type'),
        ],
        fixed_size=7,
        pre=[BOOT, FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x0090, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00bf, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d, 0x3800, 0x3821, 0x3823, 0x3824, 0x3825, 0x3826, 0x3827],
        post=[PRETTY],
        desc='''
            Somebody is claiming to not be a faceless robot.
        ''',
    )
    # 0x00c1 define='CMSG_CHAT_WHO', (unused by this name)
    # 0x00c1 define='CMSG_WHO_REQUEST',
    # 0x00c2 define='SMSG_WHO_ANSWER',
    # 0x00c3 define='SMSG_BEING_CHANGE_LOOKS',
    map_user.s(0x00c4, 'npc click result shop',
        define='SMSG_NPC_BUY_SELL_CHOICE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
        pre=[MAGIC, SCRIPT, 0x0085, 0x0089, 0x008c, 0x0090, 0x009f],
        post=[0x00c5],
        desc='''
            That npc you clicked on was a shop.
        ''',
    )
    map_user.r(0x00c5, 'npc shop buy/sell select',
        define='CMSG_NPC_BUY_SELL_REQUEST',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u8, 'type'),
        ],
        fixed_size=7,
        pre=[0x00c4],
        post=[0x00c6, 0x00c7],
        desc='''
            Choose whether to buy or sell in a shop.

            type 0: buy
            type 1: sell
        ''',
    )
    map_user.s(0x00c6, 'npc shop buy select result',
        define='SMSG_NPC_BUY',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u32, 'base price'),
            at(4, u32, 'actual price'),
            at(8, item_type, 'type'),
            at(9, item_name_id, 'name id'),
        ],
        repeat_size=11,
        pre=[0x00c5],
        post=[0x00c8],
        desc='''
            List of items you can buy from the shop.
        ''',
    )
    map_user.s(0x00c7, 'npc shop sell select result',
        define='SMSG_NPC_SELL',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, ioff2, 'ioff2'),
            at(2, u32, 'base price'),
            at(6, u32, 'actual price'),
        ],
        repeat_size=10,
        pre=[0x00c5],
        post=[0x00c9],
        desc='''
            List of items you can sell to the shop.

            Currently the server doesn't support customizing this list,
            but in theory it could.
        ''',
    )
    map_user.r(0x00c8, 'npc shop buy',
        define='CMSG_NPC_BUY_REQUEST',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u16, 'count'),
            at(2, item_name_id, 'name id'),
        ],
        repeat_size=4,
        pre=[0x00c6],
        post=[0x00a0, 0x00b0, 0x00b1, 0x00ca],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00ac, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Purchase a bunch of items from a shop.
        ''',
    )
    map_user.r(0x00c9, 'npc shop sell',
        define='CMSG_NPC_SELL_REQUEST',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, ioff2, 'ioff2'),
            at(2, u16, 'count'),
        ],
        repeat_size=4,
        pre=[0x00c7],
        post=[0x00ac, 0x00af, 0x00b0, 0x00b1, 0x00cb, 0x01d7],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Sell a bunch of items to a shop.
        ''',
    )
    map_user.s(0x00ca, 'npc shop buy result',
        define='SMSG_NPC_BUY_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
        pre=[0x00c8],
        post=[PRETTY],
        desc='''
            Result of purchasing items.
        ''',
    )
    map_user.s(0x00cb, 'npc shop sell result',
        define='SMSG_NPC_SELL_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
        pre=[0x00c9],
        post=[PRETTY],
        desc='''
            Result of purchasing items.
        ''',
    )
    # 0x00cc define='CMSG_ADMIN_KICK',
    map_user.s(0x00cd, 'kick result',
        define='SMSG_ADMIN_KICK_ACK',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[GM],
        post=[PRETTY],
        desc='''
            Successfully used @kick.
        ''',
    )
    # 0x00cf define='CMSG_IGNORE_NICK',
    # 0x00d0 define='CMSG_IGNORE_ALL',
    # 0x00d2 define='SMSG_IGNORE_ALL_RESPONSE',
    map_user.r(0x00e4, 'trade please',
        define='CMSG_TRADE_REQUEST',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
        pre=[HUMAN],
        post=[0x00e5, 0x00e7, 0x00ee, 0x0110],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00b0, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Ask someone to trade.
        ''',
    )
    map_user.s(0x00e5, 'incoming trade request',
        define='SMSG_TRADE_REQUEST',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'char name'),
        ],
        fixed_size=26,
        pre=[GM, 0x00e4],
        post=[0x00e6],
        desc='''
            Somebody wants to trade with you.
        ''',
    )
    map_user.r(0x00e6, 'incoming trade request result',
        define='CMSG_TRADE_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'type'),
        ],
        fixed_size=3,
        pre=[0x00e5],
        post=[0x00e7],
        xpost=[MAGIC, SCRIPT, 0x0078, 0x007c, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00b0, 0x00b1, 0x00b6, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00f8, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            You agree/disagree to begin a trade with someone.
        ''',
    )
    map_user.s(0x00e7, 'trade please result',
        define='SMSG_TRADE_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'type'),
        ],
        fixed_size=3,
        pre=[GM, 0x00e4, 0x00e6],
        post=[PRETTY],
        desc='''
            The original result of asking for a trade.
        ''',
    )
    map_user.r(0x00e8, 'trade add',
        define='CMSG_TRADE_ITEM_ADD_REQUEST',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'zeny or ioff2'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
        pre=[HUMAN],
        post=[0x00ac, 0x00e9, 0x01b1, 0x01d7],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00b0, 0x00b1, 0x00be, 0x00c0, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Add item/zeny to your trade offer.
        ''',
    )
    map_user.s(0x00e9, 'trade item added notify',
        define='SMSG_TRADE_ITEM_ADD',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'amount'),
            at(6, item_name_id, 'name id'),
            at(8, u8, 'identify'),
            at(9, u8, 'broken or attribute'),
            at(10, u8, 'refine'),
            at(11, u16, 'card0'),
            at(13, u16, 'card1'),
            at(15, u16, 'card2'),
            at(17, u16, 'card3'),
        ],
        fixed_size=19,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Item successfully added.
        ''',
    )
    map_user.r(0x00eb, 'trade lock',
        define='CMSG_TRADE_ADD_COMPLETE',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[HUMAN],
        post=[0x00ec, 0x1b1],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00b0, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Indicate readiness to end trade.

            Do this when the other person has added all the items you want.
        ''',
    )
    map_user.s(0x00ec, 'trade lock notify',
        define='SMSG_TRADE_OK',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
        pre=[0x00eb],
        post=[PRETTY],
        desc='''
            Somebody locked their half of the trade (0=self, 1=other).
        ''',
    )
    map_user.r(0x00ed, 'trade cancel',
        define='CMSG_TRADE_CANCEL_REQUEST',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[HUMAN],
        post=[0x00a0, 0x00b0, 0x00ee],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00ac, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Cancel an ongoing trade.
        ''',
    )
    map_user.s(0x00ee, 'trade cancel notify',
        define='SMSG_TRADE_CANCEL',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[],
        desc='''
            Somebody cancelled your trade.
        ''',
    )
    map_user.r(0x00ef, 'trade commit',
        define='CMSG_TRADE_OK',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[HUMAN],
        post=[0x00a0, 0x00af, 0x00b0, 0x00f0],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00ac, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Actually perform the trade. Requires half-lock from both sides.
        ''',
    )
    map_user.s(0x00f0, 'trade complete notify',
        define='SMSG_TRADE_COMPLETE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
        pre=[0x00ef],
        post=[PRETTY],
        desc='''
            Trade is finally over.
        ''',
    )
    map_user.s(0x00f2, 'storage size notify',
        define='SMSG_PLAYER_STORAGE_STATUS',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'current slots'),
            at(4, u16, 'max slots'),
        ],
        fixed_size=6,
        pre=[GM, SCRIPT, 0x00f3, 0x00f5, 0x3810],
        post=[PRETTY],
        desc='''
            Update the current/max storage size.

            It's easy to change the max storage on any basis, pity about
            the inventory limit ...
        ''',
    )
    map_user.r(0x00f3, 'storage put',
        define='CMSG_MOVE_TO_STORAGE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
        pre=[HUMAN],
        post=[0x00ac, 0x00af, 0x00b0, 0x00f2, 0x00f4, 0x01d7],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Move item from inventory to storage.
        ''',
    )
    map_user.s(0x00f4, 'storage added notify',
        define='SMSG_PLAYER_STORAGE_ADD',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, soff1, 'soff1'),
            at(4, u32, 'amount'),
            at(8, item_name_id, 'name id'),
            at(10, u8, 'identify'),
            at(11, u8, 'broken or attribute'),
            at(12, u8, 'refine'),
            at(13, u16, 'card0'),
            at(15, u16, 'card1'),
            at(17, u16, 'card2'),
            at(19, u16, 'card3'),
        ],
        fixed_size=21,
        pre=[GM, 0x00f3],
        post=[PRETTY],
        desc='''
            Item was added to your storage.
        ''',
    )
    map_user.r(0x00f5, 'storage take',
        define='CSMG_MOVE_FROM_STORAGE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, soff1, 'soff1'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
        pre=[HUMAN],
        post=[0x00a0, 0x00b0, 0x00f2, 0x00f6],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00ac, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Move item from storage to inventory.
        ''',
    )
    map_user.s(0x00f6, 'storage removed notify',
        define='SMSG_PLAYER_STORAGE_REMOVE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, soff1, 'soff1'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
        pre=[0x00f5],
        post=[PRETTY],
        desc='''
            Item was removed from your storage.
        ''',
    )
    map_user.r(0x00f7, 'storage close',
        define='CMSG_CLOSE_STORAGE',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[HUMAN],
        post=[MAGIC, SCRIPT, 0x00f8, 0x2b01, 0x3011],
        xpost=[0x0078, 0x007c, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00b0, 0x00b1, 0x00b6, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b05, 0x3022],
        desc='''
            Close your storage.
        ''',
    )
    map_user.s(0x00f8, 'storage closed notify',
        define='SMSG_PLAYER_STORAGE_CLOSE',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[GM, 0x00e6, 0x00f7],
        post=[PRETTY],
        desc='''
            Your storage was closed.
        ''',
    )
    map_user.r(0x00f9, 'party create',
        define='CMSG_PARTY_CREATE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_name, 'party name'),
        ],
        fixed_size=26,
        pre=[HUMAN],
        post=[0x00fa, 0x110, 0x3020],
        desc='''
            Create a new party with yourself.
        ''',
    )
    map_user.s(0x00fa, 'party create result',
        define='SMSG_PARTY_CREATE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'flag'),
        ],
        fixed_size=3,
        pre=[GM, 0x00f9, 0x3820],
        post=[PRETTY],
        desc='''
            Result of creating a new party.

            flag 0: success
            flag 1: bad name
            flag 2: already in party
        ''',
    )
    map_user.s(0x00fb, 'party info notify',
        define='SMSG_PARTY_INFO',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, party_name, 'party name'),
        ],
        head_size=28,
        repeat=[
            at(0, account_id, 'account id'),
            at(4, char_name, 'char name'),
            at(28, map_name, 'map name'),
            at(44, u8, 'leader'),
            at(45, u8, 'online'),
        ],
        repeat_size=46,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0089, 0x3821, 0x3825],
        post=[PRETTY],
        desc='''
            Full info about your party.
        ''',
    )
    map_user.r(0x00fc, 'party invite',
        define='CMSG_PARTY_INVITE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[HUMAN],
        post=[0x00fd, 0x00fe],
        desc='''
            Invite player to your party.
        ''',
    )
    map_user.s(0x00fd, 'party invite result',
        define='SMSG_PARTY_INVITE_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'char name'),
            at(26, u8, 'flag'),
        ],
        fixed_size=27,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x00fc, 0x00ff, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d, 0x3822],
        post=[PRETTY],
        desc='''
            Party invitation response.

            flag 0: already in party
            flag 1: reject
            flag 2: accept
            flag 3: party full
            flag 4: same party
        ''',
    )
    map_user.s(0x00fe, '(reverse) party invitation',
        define='SMSG_PARTY_INVITED',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, party_name, 'party name'),
        ],
        fixed_size=30,
        pre=[0x00fc],
        post=[0x00ff],
        desc='''
            You're invited to join someone's party.
        ''',
    )
    map_user.r(0x00ff, '(reverse) party invitation result',
        define='CMSG_PARTY_INVITED',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'flag'),
        ],
        fixed_size=10,
        pre=[0x00fe],
        post=[0x00fd, 0x0110, 0x3022],
        desc='''
            Reply to party invitation.
        ''',
    )
    map_user.r(0x0100, 'party leave',
        define='CMSG_PARTY_LEAVE',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[HUMAN],
        post=[0x3024],
        desc='''
            I'm sick of your lame party.
        ''',
    )
    map_user.s(0x0101, 'party option notify',
        define='SMSG_PARTY_SETTINGS',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'exp'),
            at(4, u16, 'item'),
        ],
        fixed_size=6,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0089, 0x3821, 0x3823],
        post=[PRETTY],
        desc='''
            Party settings changed.
        ''',
    )
    map_user.r(0x0102, 'party option',
        define='CMSG_PARTY_SETTINGS',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'exp'),
            at(4, u16, 'item'),
        ],
        fixed_size=6,
        pre=[HUMAN],
        post=[0x3023],
        desc='''
            Please change party settings.
        ''',
    )
    map_user.r(0x0103, 'party kick',
        define='CMSG_PARTY_KICK',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'unused char name'),
        ],
        fixed_size=30,
        pre=[HUMAN],
        post=[0x3024],
        desc='''
            Forcibly remove party member.
        ''',
    )
    # 0x0104 define='SMSG_PARTY_MOVE',
    map_user.s(0x0105, 'party left notify',
        define='SMSG_PARTY_LEAVE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'char name'),
            at(30, u8, 'flag'),
        ],
        fixed_size=31,
        pre=[0x3824, 0x3826],
        post=[PRETTY],
        desc='''
            Somebody got tired of your party.
        ''',
    )
    map_user.s(0x0106, 'party hp notify',
        define='SMSG_PARTY_UPDATE_HP',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u16, 'hp'),
            at(8, u16, 'max hp'),
        ],
        fixed_size=10,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Party member hp update.
        ''',
    )
    map_user.s(0x0107, 'party xy notify',
        define='SMSG_PARTY_UPDATE_COORDS',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u16, 'x'),
            at(8, u16, 'y'),
        ],
        fixed_size=10,
        pre=[TIMER],
        post=[PRETTY],
        desc='''
            Party member location update.
        ''',
    )
    map_user.r(0x0108, 'party message',
        define='CMSG_PARTY_MESSAGE',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[HUMAN],
        post=[0x2b0e, 0x3003, 0x3027],
        desc='''
            Talk privately to your party.
        ''',
    )
    map_user.s(0x0109, 'party message notify',
        define='SMSG_PARTY_MESSAGE',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x3827],
        post=[PRETTY],
        desc='''
            Somebody in your party is talking to you.
        ''',
    )
    # 0x010c define='SMSG_MVP',
    map_user.s(0x010e, 'skill raise result',
        # define='SMSG_GUILD_SKILL_UP',
        define='SMSG_PLAYER_SKILL_UP',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, skill_id, 'skill id'),
            at(4, u16, 'level'),
            at(6, u16, 'sp'),
            at(8, u16, 'range'),
            at(10, u8, 'can raise'),
        ],
        fixed_size=11,
        pre=[0x0112],
        post=[PRETTY],
        desc='''
            Successfully raised a skill.
        ''',
    )
    map_user.s(0x010f, 'skill info notify',
        define='SMSG_PLAYER_SKILLS',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, skill_info, 'info'),
        ],
        repeat_size=37,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Enumeration of your skills.
        ''',
    )
    map_user.s(0x0110, 'skill failed',
        define='SMSG_SKILL_FAILED',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, skill_id, 'skill id'),
            at(4, u16, 'btype'),
            at(6, u16, 'zero1'),
            at(8, u8, 'zero2'),
            at(9, u8, 'type'),
        ],
        fixed_size=10,
        pre=[0x0089, 0x00bf, 0x00e4, 0x00f9, 0x00ff],
        post=[PRETTY],
        desc='''
            Failed to perform a skill.
        ''',
    )
    map_user.r(0x0112, 'skill raise',
        define='CMSG_SKILL_LEVELUP_REQUEST',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, skill_id, 'skill id'),
        ],
        fixed_size=4,
        pre=[HUMAN],
        post=[0x00b0, 0x010e, 0x010f],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Spend skill points to raise your skills.
        ''',
    )
    # 0x0113 define='CMSG_SKILL_USE_BEING',
    # 0x0116 define='CMSG_SKILL_USE_POSITION',
    map_user.r(0x0118, 'attack stop',
        define='CMSG_PLAYER_STOP_ATTACK',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[HUMAN],
        post=[IDLE],
        desc='''
            Cancel a continuous attack.
        ''',
    )
    map_user.s(0x0119, 'player option notify',
        define='SMSG_PLAYER_STATUS_CHANGE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, opt1, 'opt1'),
            at(8, opt2, 'opt2'),
            at(10, option, 'option'),
            at(12, u8, 'zero'),
        ],
        fixed_size=13,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Update 3/4ths of the option fields.
        ''',
    )
    # 0x011a define='SMSG_SKILL_NO_DAMAGE',
    # 0x011b define='CMSG_SKILL_USE_MAP',
    map_user.s(0x0139, 'player move attack range notify',
        define='SMSG_PLAYER_MOVE_TO_ATTACK',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u16, 'bl x'),
            at(8, u16, 'bl y'),
            at(10, u16, 'sd x'),
            at(12, u16, 'sd y'),
            at(14, u16, 'range'),
        ],
        fixed_size=16,
        pre=[0x0089],
        post=[PRETTY],
        desc='''
            You are moving to enter attack range.
        ''',
    )
    map_user.s(0x013a, 'player attack range notify',
        define='SMSG_PLAYER_ATTACK_RANGE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'attack range'),
        ],
        fixed_size=4,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Update attack range field.
        ''',
    )
    map_user.s(0x013b, 'player arrow fail notify',
        define='SMSG_PLAYER_ARROW_MESSAGE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'type'),
        ],
        fixed_size=4,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x0089, 0x00a9],
        post=[PRETTY],
        desc='''
            Arrow status: failed (0) or equipped (3, ignored).
        ''',
    )
    map_user.s(0x013c, 'player arrow equip notify',
        define='SMSG_PLAYER_ARROW_EQUIP',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
        ],
        fixed_size=4,
        pre=[0x007d, 0x00a9],
        post=[PRETTY],
        desc='''
            Arrow equip inventory slot.
        ''',
    )
    # 0x013e define='SMSG_SKILL_CASTING',
    map_user.s(0x0141, 'player stat update 3',
        define='SMSG_PLAYER_STAT_UPDATE_3',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u16, 'zero'),
            at(6, u32, 'value status'),
            at(10, u32, 'value b e'),
        ],
        fixed_size=14,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Update base and cumulative (?) stat points.
        ''',
    )
    map_user.s(0x0142, '(reverse) script input integer',
        define='SMSG_NPC_INT_INPUT',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
        pre=[SCRIPT],
        post=[0x0143],
        desc='''
            Npc wants an integer.
        ''',
    )
    map_user.r(0x0143, '(reverse) script input integer result',
        define='CMSG_NPC_INT_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u32, 'input int value'),
        ],
        fixed_size=10,
        pre=[0x0142],
        post=[MAGIC, SCRIPT],
        xpost=[0x0078, 0x007c, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00b0, 0x00b1, 0x00b6, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Npc can have an integer.
        ''',
    )
    map_user.r(0x0146, '(reverse) script close response',
        define='CMSG_NPC_CLOSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
        pre=[0x00b6],
        post=[MAGIC, SCRIPT],
        xpost=[0x0078, 0x007c, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00b0, 0x00b1, 0x00b6, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Interactive npc chat closed, maybe finish noninteractively now.
        ''',
    )
    map_user.s(0x0148, 'being resurrected notify',
        define='SMSG_BEING_RESURRECT',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u16, 'type'),
        ],
        fixed_size=8,
        pre=[GM],
        post=[PRETTY],
        desc='''
            It's alive!
        ''',
    )
    # 0x0149 define='CMSG_ADMIN_MUTE',
    # 0x014c define='SMSG_GUILD_ALIANCE_INFO',
    # 0x014d define='CMSG_GUILD_CHECK_MASTER',
    # 0x014e define='SMSG_GUILD_MASTER_OR_MEMBER',
    # 0x014f define='CMSG_GUILD_REQUEST_INFO',
    # 0x0151 define='CMSG_GUILD_REQUEST_EMBLEM',
    # 0x0152 define='SMSG_GUILD_EMBLEM',
    # 0x0153 define='CMSG_GUILD_CHANGE_EMBLEM',
    # 0x0154 define='SMSG_GUILD_MEMBER_LIST',
    # 0x0155 define='CMSG_GUILD_CHANGE_MEMBER_POS',
    # 0x0156 define='SMSG_GUILD_MEMBER_POS_CHANGE',
    # 0x0159 define='CMSG_GUILD_LEAVE',
    # 0x015a define='SMSG_GUILD_LEAVE',
    # 0x015b define='CMSG_GUILD_EXPULSION',
    # 0x015c define='SMSG_GUILD_EXPULSION',
    # 0x015d define='CMSG_GUILD_BREAK',
    # 0x015e define='SMSG_GUILD_BROKEN',
    # 0x0160 define='SMSG_GUILD_POS_INFO_LIST',
    # 0x0161 define='CMSG_GUILD_CHANGE_POS_INFO',
    # 0x0162 define='SMSG_GUILD_SKILL_INFO',
    # 0x0163 define='SMSG_GUILD_EXPULSION_LIST',
    # 0x0165 define='CMSG_GUILD_CREATE',
    # 0x0166 define='SMSG_GUILD_POS_NAME_LIST',
    # 0x0167 define='SMSG_GUILD_CREATE_RESPONSE',
    # 0x0168 define='CMSG_GUILD_INVITE',
    # 0x0169 define='SMSG_GUILD_INVITE_ACK',
    # 0x016a define='SMSG_GUILD_INVITE',
    # 0x016b define='CMSG_GUILD_INVITE_REPLY',
    # 0x016c define='SMSG_GUILD_POSITION_INFO',
    # 0x016d define='SMSG_GUILD_MEMBER_LOGIN',
    # 0x016e define='CMSG_GUILD_CHANGE_NOTICE',
    # 0x016f define='SMSG_GUILD_NOTICE',
    # 0x0170 define='CMSG_GUILD_ALLIANCE_REQUEST',
    # 0x0171 define='SMSG_GUILD_REQ_ALLIANCE',
    # 0x0172 define='CMSG_GUILD_ALLIANCE_REPLY',
    # 0x0173 define='SMSG_GUILD_REQ_ALLIANCE_ACK',
    # 0x0174 define='SMSG_GUILD_POSITION_CHANGED',
    # 0x017e define='CMSG_GUILD_MESSAGE',
    # 0x017f define='SMSG_GUILD_MESSAGE',
    # 0x0180 define='CMSG_GUILD_OPPOSITION',
    # 0x0181 define='SMSG_GUILD_OPPOSITION_ACK',
    # 0x0183 define='CMSG_GUILD_ALLIANCE_DELETE',
    # 0x0184 define='SMSG_GUILD_DEL_ALLIANCE',
    map_user.r(0x018a, 'client quit',
        define='CMSG_CLIENT_QUIT',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'unused'),
        ],
        fixed_size=4,
        pre=[NOTHING],
        post=[0x018b],
        desc='''
            Request explicit end of connection.
        ''',
    )
    map_user.s(0x018b, 'client quit result',
        define='SMSG_MAP_QUIT_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'okay'),
        ],
        fixed_size=4,
        pre=[GM, 0x00b2, 0x018a],
        post=[PRETTY],
        desc='''
            Result of explicit end of connection.
        ''',
    )
    # 0x0190 define='CMSG_SKILL_USE_POSITION_MORE',
    # 0x0193 define='CMSG_SOLVE_CHAR_NAME',
    # 0x0194 define='SMSG_SOLVE_CHAR_NAME',
    map_user.s(0x0195, 'guild party info notify',
        define='SMSG_PLAYER_GUILD_PARTY_INFO',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, party_name, 'party name'),
            at(30, str24, 'guild name'),
            at(54, str24, 'guild pos'),
            at(78, str24, 'guild pos again'),
        ],
        fixed_size=102,
        pre=[0x0094],
        post=[PRETTY],
        desc='''
            Name of player's party and guild.
        ''',
    )
    map_user.s(0x0196, 'being status change notify',
        define='SMSG_BEING_STATUS_CHANGE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, status_change, 'sc type'),
            at(4, block_id, 'block id'),
            at(8, u8, 'flag'),
        ],
        fixed_size=9,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Being adds/removes a persistent status effect.
        ''',
    )
    map_user.s(0x0199, 'map pvp status',
        define='SMSG_PVP_MAP_MODE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'status'),
        ],
        fixed_size=4,
        pre=[NOTHING],
        post=[PRETTY],
        desc='''
            Send the map pvp status
        ''',
    )
    map_user.s(0x019a, 'being pvp status',
        define='SMSG_PVP_SET',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u32, 'rank'),
            at(10, u32, 'channel'),
        ],
        fixed_size=14,
        pre=[NOTHING],
        post=[PRETTY],
        desc='''
            Send the pvp status
        ''',
    )
    map_user.s(0x019b, 'being effect',
        define='SMSG_BEING_SELFEFFECT',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u32, 'type'),
        ],
        fixed_size=10,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0089],
        post=[PRETTY],
        desc='''
            Being runs a one-shot (?) effect.
        ''',
    )
    # 0x019C define='CMSG_ADMIN_LOCAL_ANNOUNCE',
    # 0x019D define='CMSG_ADMIN_HIDE',
    map_user.s(0x01b1, 'trade add result',
        define='SMSG_TRADE_ITEM_ADD_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u16, 'amount'),
            at(6, u8, 'fail'),
        ],
        fixed_size=7,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Result of trying to add an item/zeny to your trade offer.
        ''',
    )
    # 0x01b6 define='SMSG_GUILD_BASIC_INFO',
    # 0x01b9 define='SMSG_SKILL_CAST_CANCEL',
    map_user.s(0x01c8, 'use item result',
        define='SMSG_PLAYER_INVENTORY_USE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, item_name_id, 'name id'),
            at(6, block_id, 'block id'),
            at(10, u16, 'amount'),
            at(12, u8, 'ok'),
        ],
        fixed_size=13,
        pre=[0x00a7],
        post=[PRETTY],
        desc='''
            You used (or tried to use) an item in your inventory.

            Currently used only in the success case.
        ''',
    )
    map_user.s(0x01d4, '(reverse) script input string',
        define='SMSG_NPC_STR_INPUT',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
        pre=[SCRIPT],
        post=[0x01d5],
        desc='''
            Npc wants a string.
        ''',
    )
    map_user.r(0x01d5, '(reverse) script input string result',
        define='CMSG_NPC_STR_RESPONSE',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, block_id, 'block id'),
        ],
        head_size=8,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x01d4],
        post=[MAGIC, SCRIPT],
        xpost=[0x0078, 0x007c, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00ac, 0x00b0, 0x00b1, 0x00b6, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Npc can have an integer.
        ''',
    )
    map_user.s(0x01d7, 'being change look',
        define='SMSG_BEING_CHANGE_LOOKS2',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, look, 'look type'),
            at(7, u16, 'weapon or name id or value'),
            at(9, item_name_id, 'shield'),
        ],
        fixed_size=11,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[PRETTY],
        desc='''
            Being change appearance.

            This may be a weapon type, an item nameid, or just a value -
            it all depends on which look type.
        ''',
    )
    # very similar to, but not compatible with, 0x01d9 and 0x01da
    map_user.s(0x01d8, 'player appear notify',
        define='SMSG_PLAYER_UPDATE_1',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, interval16, 'speed'),
            at(8, opt1, 'opt1'),
            at(10, opt2, 'opt2'),
            at(12, option, 'option'),
            at(14, species, 'species'),
            at(16, u16, 'hair style'),
            at(18, item_name_id, 'weapon'),
            at(20, item_name_id, 'shield'),
            at(22, item_name_id, 'head bottom'),
            at(24, item_name_id, 'head top'),
            at(26, item_name_id, 'head mid'),
            at(28, u16, 'hair color'),
            at(30, u16, 'clothes color'),
            at(32, dir, 'head dir'),
            at(33, u8, 'unused2'),
            at(34, u32, 'guild id'),
            at(38, u16, 'guild emblem id'),
            at(40, u16, 'manner'),
            at(42, opt3, 'opt3'),
            at(44, u8, 'karma'),
            at(45, sex, 'sex'),
            at(46, pos1, 'pos'),
            at(49, u16, 'gm bits'),
            at(51, u8, 'dead sit'),
            at(52, u16, 'unused'),
        ],
        fixed_size=54,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[0x0094, 0x00b2],
        desc='''
            A player appeared (unmoving).

            Like 0x0078 but for players, and only in the "enter area" case.
        ''',
    )
    # very similar to, but not compatible with, 0x01d8 and 0x01da
    map_user.s(0x01d9, 'player appear notify',
        define='SMSG_PLAYER_UPDATE_2',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, interval16, 'speed'),
            at(8, opt1, 'opt1'),
            at(10, opt2, 'opt2'),
            at(12, option, 'option'),
            at(14, species, 'species'),
            at(16, u16, 'hair style'),
            at(18, item_name_id, 'weapon'),
            at(20, item_name_id, 'shield'),
            at(22, item_name_id, 'head bottom'),
            at(24, item_name_id, 'head top'),
            at(26, item_name_id, 'head mid'),
            at(28, u16, 'hair color'),
            at(30, u16, 'clothes color'),
            at(32, dir, 'head dir'),
            at(33, u8, 'unused2'),
            at(34, u32, 'guild id'),
            at(38, u16, 'guild emblem id'),
            at(40, u16, 'manner'),
            at(42, opt3, 'opt3'),
            at(44, u8, 'karma'),
            at(45, sex, 'sex'),
            at(46, pos1, 'pos'),
            at(49, u16, 'gm bits'),
            at(51, u16, 'unused'),
        ],
        fixed_size=53,
        pre=[0x007d],
        post=[0x0094],
        desc='''
            A wild player appeared!

            Like 0x0078 but for players, but only in the "spawn on map" case.
        ''',
    )
    # very similar to, but not compatible with, 0x01d8 and 0x01d9
    map_user.s(0x01da, 'player move notify',
        define='SMSG_PLAYER_MOVE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, interval16, 'speed'),
            at(8, opt1, 'opt1'),
            at(10, opt2, 'opt2'),
            at(12, option, 'option'),
            at(14, species, 'species'),
            at(16, u16, 'hair style'),
            at(18, item_name_id, 'weapon'),
            at(20, item_name_id, 'shield'),
            at(22, item_name_id, 'head bottom'),
            at(24, tick32, 'tick'),
            at(28, item_name_id, 'head top'),
            at(30, item_name_id, 'head mid'),
            at(32, u16, 'hair color'),
            at(34, u16, 'clothes color'),
            at(36, dir, 'head dir'),
            at(37, u8, 'unused2'),
            at(38, u32, 'guild id'),
            at(42, u16, 'guild emblem id'),
            at(44, u16, 'manner'),
            at(46, opt3, 'opt3'),
            at(48, u8, 'karma'),
            at(49, sex, 'sex'),
            at(50, pos2, 'pos2'),
            at(55, u16, 'gm bits'),
            at(57, u8, 'five'),
            at(58, u16, 'unused'),
        ],
        fixed_size=60,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[0x0094],
        desc='''
            A player appeared, moving.

            Like 0x007b but for players.
        ''',
    )
    map_user.s(0x01de, 'deal skill damage',
        # define='CMSG_LOGIN_REGISTER2', with a different body ...
        define='SMSG_SKILL_DAMAGE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, skill_id, 'skill id'),
            at(4, block_id, 'src id'),
            at(8, block_id, 'dst id'),
            at(12, tick32, 'tick'),
            at(16, interval32, 'sdelay'),
            at(20, interval32, 'ddelay'),
            at(24, u32, 'damage'),
            at(28, u16, 'skill level'),
            at(30, u16, 'div'),
            at(32, u8, 'type or hit'),
        ],
        fixed_size=33,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x0089],
        post=[PRETTY],
        desc='''
            Took damage from a skill.
        ''',
    )
    map_user.s(0x01ee, 'inventory list notify',
        define='SMSG_PLAYER_INVENTORY',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, ioff2, 'ioff2'),
            at(2, item_name_id, 'name id'),
            at(4, item_type, 'item type'),
            at(5, u8, 'identify'),
            at(6, u16, 'amount'),
            at(8, epos, 'epos'),
            at(10, u16, 'card0'),
            at(12, u16, 'card1'),
            at(14, u16, 'card2'),
            at(16, u16, 'card3'),
        ],
        repeat_size=18,
        pre=[0x007d],
        post=[PRETTY],
        desc='''
            List of all items in inventory.
        ''',
    )
    map_user.s(0x01f0, 'storage list notify',
        define='SMSG_PLAYER_STORAGE_ITEMS',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, soff1, 'soff1'),
            at(2, item_name_id, 'name id'),
            at(4, item_type, 'item type'),
            at(5, u8, 'identify'),
            at(6, u16, 'amount'),
            at(8, epos, 'epos zero'),
            at(10, u16, 'card0'),
            at(12, u16, 'card1'),
            at(14, u16, 'card2'),
            at(16, u16, 'card3'),
        ],
        repeat_size=18,
        pre=[GM, SCRIPT, 0x3810],
        post=[PRETTY],
        desc='''
            List of all items in storage.
        ''',
    )
    map_user.s(0x020c, 'player ip notify',
        define='SMSG_BEING_IP_RESPONSE',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, ip4, 'ip'),
        ],
        fixed_size=10,
        pre=[0x0094],
        post=[PRETTY],
        desc='''
            Show a player's (hashed) IP, for detecting alts.

            The hash is rerandomized every restart.
        ''',
    )
    # 0x0210 define='CMSG_ONLINE_LIST',
    # 0x0211 define='SMSG_ONLINE_LIST',
    map_user.s(0x0212, 'npc command',
        define='SMSG_NPC_COMMAND',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'npc id'),
            at(6, u16, 'command'),
            at(8, u32, 'id'),
            at(12, u16, 'x'),
            at(14, u16, 'y'),
        ],
        fixed_size=16,
        pre=[NOTHING],
        post=[PRETTY],
        desc='''
            Make an npc do fancy things (for Manaplus).
        ''',
    )
    # 0x0213 define='CMSG_SET_STATUS',
    # 0x0214 define='SMSG_QUEST_SET_VAR',
    map_user.s(0x0214, 'send quest',
        define='SMSG_QUEST_SET_VAR',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'variable'),
            at(4, u32, 'value'),
        ],
        fixed_size=8,
        pre=[NOTHING],
        post=[PRETTY],
        desc='''
            Set Quest Log Variable to Value.
        ''',
    )
    # 0x0215 define='SMSG_QUEST_PLAYER_VARS',
    map_user.s(0x0215, 'send all quest',
        define='SMSG_QUEST_PLAYER_VARS',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u16, 'variable'),
            at(2, u32, 'value'),
        ],
        repeat_size=6,
        pre=[NOTHING],
        post=[PRETTY],
        desc='''
            Set All Quest Log Variable to Value.
        ''',
    )
    # 0x0220 define='SMSG_BEING_NAME_RESPONSE2',
    # 0x0221 define='SMSG_CHAR_CREATE_SUCCEEDED2',
    # 0x0222 define='CMSG_CHAT_MESSAGE2',
    # 0x0223 define='SMSG_BEING_CHAT2',
    # 0x0224 define='SMSG_PLAYER_CHAT2',
    map_user.s(0x0225, 'being move 3',
        define='SMSG_BEING_MOVE3',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, block_id, 'id'),
            at(8, interval16, 'speed'),
            at(10, u16, 'x position'),
            at(12, u16, 'y position'),
        ],
        head_size=14,
        repeat=[
            at(0, dir, 'move'),
        ],
        repeat_size=1,
        pre=[NOTHING],
        post=[PRETTY],
        desc='''
            Send mob walkpath data to client
        ''',
    )
    map_user.s(0x0226, 'send map mask',
        define='SMSG_MAP_MASK',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'mask'),
            at(6, u32, 'unused'),
        ],
        fixed_size=10,
        pre=[NOTHING],
        post=[PRETTY],
        desc='''
            Set map mask
        ''',
    )
    map_user.s(0x0227, 'change map music',
        define='SMSG_MAP_MUSIC',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[NOTHING],
        post=[PRETTY],
        desc='''
            Change map music
        ''',
    )
    map_user.s(0x0228, 'npc change title',
        define='SMSG_NPC_CHANGETITLE',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, block_id, 'npc id'),
            at(8, u16, 'string length'),
        ],
        head_size=10,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[NOTHING],
        post=[PRETTY],
        desc='''
            Change npc title
        ''',
    )

    # TOC_LOGINCHAR
    # login char
    login_char.r(0x2710, 'server connect char',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'account pass'),
            at(50, u32, 'unknown'),
            at(54, ip4, 'ip'),
            at(58, u16, 'port'),
            at(60, server_name, 'server name'),
            at(80, u16, 'unknown2'),
            at(82, u16, 'maintenance'),
            at(84, u16, 'is new'),
        ],
        fixed_size=86,
        pre=[BOOT],
        post=[0x2711, 0x2732],
        desc='''
            Become an authenticated char server.
        ''',
    )
    login_char.s(0x2711, 'server connect char result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
        pre=[0x2710],
        post=[FINISH, IDLE],
        desc='''
            Result of auth'ing as a char server.
        ''',
    )
    login_char.r(0x2712, 'account auth',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'login id1'),
            at(10, u32, 'login id2'),
            at(14, sex, 'sex'),
            at(15, ip4, 'ip'),
        ],
        fixed_size=19,
        pre=[0x0065],
        post=[0x2713, 0x2729],
        desc='''
            Check whether client's cookies are okay.
        ''',
    )
    login_char.s(0x2713, 'account auth result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'invalid'),
            at(7, account_email, 'email'),
            at(47, time32, 'unused connect until'),
        ],
        fixed_size=51,
        pre=[0x2712],
        post=[0x006b, 0x006c],
        desc='''
            Send account auth status to tmwa-char.

            Status:
                0: good
                1: bad
        ''',
    )
    login_char.r(0x2714, 'online count notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'users'),
        ],
        fixed_size=6,
        pre=[TIMER],
        post=[IDLE],
        desc='''
            Update count of online users.

            This occurs every few seconds, so is also used for antifreeze if enabled.
        ''',
    )
    login_char.r(0x2716, 'email limit',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x0065],
        post=[0x2717],
        desc='''
            Fetch current email (and previously, validity limit) for an account.
        ''',
    )
    login_char.s(0x2717, 'email limit result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_email, 'email'),
            at(46, time32, 'unused connect until'),
        ],
        fixed_size=50,
        pre=[0x2716],
        post=[IDLE],
        desc='''
            Yield current email (and previously, validity limit) for an account.
        ''',
    )
    login_char.r(0x2720, 'become gm account',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
        pre=[0x2b0a],
        post=[0x2721, 0x2732],
        desc='''
            Grant privileges by password.
        ''',
    )
    login_char.s(0x2721, 'become gm account reply',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, gm, 'gm level'),
        ],
        fixed_size=10,
        pre=[0x2720],
        post=[0x2b0b],
        desc='''
            Maybe granted privileges?
        ''',
    )
    login_char.r(0x2722, 'account email change',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_email, 'old email'),
            at(46, account_email, 'new email'),
        ],
        fixed_size=86,
        pre=[0x2b0c],
        post=[IDLE],
        desc='''
            Change account email.
        ''',
    )
    login_char.s(0x2723, 'changesex result/notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, sex, 'sex'),
        ],
        fixed_size=7,
        pre=[0x2727, 0x793c],
        post=[0x2b0d],
        desc='''
            Set was flipped (if from char server) or set to an absolute value (if set from admin).
        ''',
    )
    login_char.r(0x2724, 'block status',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'status'),
        ],
        fixed_size=10,
        pre=[0x2b0e],
        post=[0x2731],
        desc='''
            Set an account's blocked status, see 0x006a but add one.

            The only really useful values here are 0 ("unblock") and
            5 ("block"). Don't use this for state 7 ("ban") because it
            requires a timestamp argument.
        ''',
    )
    login_char.r(0x2725, 'ban add',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, human_time_diff, 'ban add'),
        ],
        fixed_size=18,
        pre=[0x2b0e],
        post=[0x2731],
        desc='''
            Adjust the time of an account's temporary ban. If the account
            is not already banned, first set the ban to the current time.
        ''',
    )
    # evil packet, see also 0x794e
    login_admin.s(0x2726, 'broadcast',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'unused'),
            at(4, SkewLengthType(u32, 8), 'magic packet length'),
        ],
        head_size=8,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x794e],
        post=[0x3800],
        desc='''
            This packet is evil.
        ''',
    )
    login_char.r(0x2727, 'change sex',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, sex, 'sex'),
        ],
        fixed_size=7,
        pre=[0x2b0e],
        post=[0x2723],
        desc='''
            Flip account's gender.
        ''',
    )
    login_char.r(0x2728, 'update account reg2',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[
            at(0, var_name, 'name'),
            at(32, u32, 'value'),
        ],
        repeat_size=36,
        pre=[0x2b10],
        post=[0x2729],
        desc='''
            Update account's login-stored variables.

            These start with ## and are unused because:

            1. They didn't work at all before I started working on the server.
            2. We only run one char-server so we might as well store vars there.
            3. The meaning would be weird anyway.
        ''',
    )
    login_char.s(0x2729, 'update account reg2 notify',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[
            at(0, var_name, 'name'),
            at(32, u32, 'value'),
        ],
        repeat_size=36,
        pre=[0x2712, 0x2728],
        post=[0x2b11],
        desc='''
            Updated account's ##variables.

            This is sent both in response to 0x2728 and in response to
            0x2712 cookie check.
        '''
    )
    login_char.r(0x272a, 'unban',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x2b0e],
        post=[IDLE],
        desc='''
            Clear an account's temporary ban date.
        ''',
    )
    login_char.s(0x2730, 'account deleted notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x7932],
        post=[0x2afe, 0x2b12, 0x2b13, 0x3821, 0x3824, 0x3826],
        desc='''
            Account just doesn't exist anymore.
        ''',
    )
    login_char.s(0x2731, 'status or ban changed notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'ban not status'),
            at(7, time32, 'status or ban until'),
        ],
        fixed_size=11,
        pre=[0x2724, 0x2725, 0x7936, 0x794a, 0x794c],
        post=[0x2b14],
        desc='''
            Account has a new ban date or block status.
        ''',
    )
    login_char.s(0x2732, 'gm account list notify',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, account_id, 'account id'),
            at(4, gm1, 'gm level'),
        ],
        repeat_size=5,
        pre=[TIMER, 0x2710, 0x2720, 0x793e, 0x7955],
        post=[0x2b15],
        desc='''
            Send GM accounts to all character servers.
        ''',
    )
    login_char.r(0x2740, 'change password',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_pass, 'old pass'),
            at(30, account_pass, 'new pass'),
        ],
        fixed_size=54,
        pre=[0x0061],
        post=[0x2741],
        desc='''
            Change password of an account.
        ''',
    )
    login_char.s(0x2741, 'change password result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'status'),
        ],
        fixed_size=7,
        pre=[0x2740],
        post=[0x0062],
        desc='''
            Password changed or not.
        ''',
    )

    # TOC_CHARMAP
    # char map
    char_map.r(0x2af8, 'server connect map',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'account pass'),
            at(50, u32, 'unused'),
            at(54, ip4, 'ip'),
            at(58, u16, 'port'),
        ],
        fixed_size=60,
        pre=[TIMER],
        post=[0x2af9, 0x2b15],
        desc='''
            Become an authenticated map server.
        ''',
    )
    char_map.s(0x2af9, 'server connect map result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
        pre=[0x2af8],
        post=[FINISH, SCRIPT, 0x2afa],
        desc='''
            Did I auth?
        ''',
    )
    # wtf duplicate v
    # formerly 0x2afa, now fixed, but now sorted wrong
    # (should go below, but don't want to break diff)
    char_map.r(0x2afa, 'map list',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, map_name, 'map name'),
        ],
        repeat_size=16,
        pre=[0x2af9],
        post=[0x2afb, 0x2b04],
        desc='''
            Tell the char server what maps I have.
        ''',
    )
    # wtf duplicate ^
    # formerly 0x2afa, now fixed, but now sorted wrong
    # (should go below, but don't want to break diff)
    #
    # (now erased)

    char_map.s(0x2afb, 'map list result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'unknown'),
            at(3, char_name, 'unused whisper name'),
        ],
        fixed_size=27,
        pre=[0x2afa],
        post=[FINISH, IDLE],
        desc='''
            I got your maps.

            (having pronoun consistency problems here)
        ''',
    )
    char_map.r(0x2afc, 'character auth',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_id, 'char id'),
            at(10, u32, 'login id1'),
            at(14, u32, 'login id2'),
            at(18, ip4, 'ip'),
        ],
        fixed_size=22,
        pre=[0x0072],
        post=[0x2afd, 0x2afe],
        desc='''
            Check whether client's cookies are okay.
        ''',
    )
    char_map.s(0x2afd, 'character auth success',
        payload=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
            at(8, u32, 'login id2'),
            at(12, time32, 'unused connect until'),
            at(16, u16, 'packet client version'),
            at(18, char_key, 'char key'),
            at(None, char_data, 'char data'),
        ],
        payload_size=None,
        pre=[0x2afc],
        post=[0x0073, 0x0081, 0x0091, 0x3005, 0x3021],
        xpost=[GM, SCRIPT, 0x0080, 0x0088, 0x00a0, 0x00ac, 0x00b0, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d7, 0x01d8, 0x01da, 0x2b01, 0x2b05, 0x3011, 0x3022],
        desc='''
            Account authentication succeeded.
        ''',
    )
    char_map.s(0x2afe, 'character auth error',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x0068, 0x2730, 0x2afc],
        post=[0x0081],
        desc='''
            Send account id to tmwa-map for disconnection.
        ''',
    )
    char_map.r(0x2aff, 'user list',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, u16, 'users'),
        ],
        head_size=6,
        repeat=[
            at(0, char_id, 'char id'),
        ],
        repeat_size=4,
        pre=[TIMER],
        post=[IDLE],
        desc='''
            Where can everybody be found?
        ''',
    )
    char_map.s(0x2b00, 'total users',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'users'),
        ],
        fixed_size=6,
        pre=[TIMER],
        post=[IDLE],
        desc='''
            How many people are there in total?
        ''',
    )
    char_map.r(0x2b01, 'character save',
        payload=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
            at(8, char_id, 'char id'),
            at(12, char_key, 'char key'),
            at(None, char_data, 'char data'),
        ],
        payload_size=None,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[IDLE],
        desc='''
            Full character save.
        ''',
    )
    char_map.r(0x2b02, 'char select',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'login id1'),
            at(10, u32, 'login id2'),
            at(14, ip4, 'ip'),
        ],
        fixed_size=18,
        pre=[0x00b2],
        post=[0x2b03],
        desc='''
            Client is going back to select character again.
        ''',
    )
    char_map.s(0x2b03, 'char select result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'unknown'),
        ],
        fixed_size=7,
        pre=[0x2b02],
        post=[0x00b3],
        desc='''
            Client can go back to select character again.
        ''',
    )
    char_map.s(0x2b04, 'map list notify',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, ip4, 'ip'),
            at(8, u16, 'port'),
        ],
        head_size=10,
        repeat=[
            at(0, map_name, 'map name'),
        ],
        repeat_size=16,
        pre=[0x2afa],
        post=[IDLE],
        desc='''
            Send remote map information to everyone.
        ''',
    )
    char_map.r(0x2b05, 'change map server',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'login id1'),
            at(10, u32, 'login id2'),
            at(14, char_id, 'char id'),
            at(18, map_name, 'map name'),
            at(34, u16, 'x'),
            at(36, u16, 'y'),
            at(38, ip4, 'map ip'),
            at(42, u16, 'map port'),
            at(44, sex, 'sex'),
            at(45, ip4, 'client ip'),
        ],
        fixed_size=49,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[0x2b06],
        desc='''
            Client wants to use a remote map.
        ''',
    )
    char_map.s(0x2b06, 'change map server ack',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'error'),
            at(10, u32, 'unknown'),
            at(14, char_id, 'char id'),
            at(18, map_name, 'map name'),
            at(34, u16, 'x'),
            at(36, u16, 'y'),
            at(38, ip4, 'map ip'),
            at(42, u16, 'map port'),
        ],
        fixed_size=44,
        pre=[0x2b05],
        post=[0x0081, 0x0092],
        desc='''
            Client can use a remote map.
        ''',
    )
    char_map.r(0x2b0a, 'become gm',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
        pre=[GM],
        post=[0x2720, 0x2b0b],
        desc='''
            Get a GM level from a password.
        ''',
    )
    char_map.s(0x2b0b, 'become gm result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, gm, 'gm level'),
        ],
        fixed_size=10,
        pre=[0x2721, 0x2b0a],
        post=[IDLE],
        desc='''
            Got a GM level from a password, or not.
        ''',
    )
    char_map.r(0x2b0c, 'change email',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_email, 'old email'),
            at(46, account_email, 'new email'),
        ],
        fixed_size=86,
        pre=[GM],
        post=[0x2722],
        desc='''
            Player used @email.
        ''',
    )
    char_map.s(0x2b0d, 'sex changed notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, sex, 'sex'),
        ],
        fixed_size=7,
        pre=[NOTHING],
        post=[0x00ac, 0x01d7, 0x2b01, 0x3011],
        xpost=[SCRIPT, 0x0080, 0x0081, 0x0088, 0x0091, 0x00a0, 0x00b0, 0x00b1, 0x00be, 0x00c0, 0x00e9, 0x00ee, 0x00fd, 0x0106, 0x010f, 0x0119, 0x013a, 0x0141, 0x0196, 0x01b1, 0x01d8, 0x01da, 0x2b05, 0x3022],
        desc='''
            Kick someone who had a sex-change operation.
        ''',
    )
    char_map.r(0x2b0e, 'named char operation',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'char name'),
            at(30, u16, 'operation'),
            at(32, human_time_diff, 'ban add'),
        ],
        fixed_size=44,
        pre=[GM, SCRIPT, 0x008c, 0x0096, 0x0108],
        post=[0x2724, 0x2725, 0x2727, 0x272a, 0x2b0f],
        desc='''
            Perform block/ban/unblock/unban/sexchange by name.
        ''',
    )
    char_map.r(0x2b0f, 'named char operation result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'char name'),
            at(30, u16, 'operation'),
            at(32, u16, 'error'),
        ],
        fixed_size=34,
        pre=[0x2b0e],
        post=[IDLE],
        desc='''
            Maybe performed block/ban/unblock/unban/sexchange by name.
        ''',
    )
    char_map.r(0x2b10, 'account reg2',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[
            at(0, var_name, 'name'),
            at(32, u32, 'value'),
        ],
        repeat_size=36,
        pre=[SCRIPT],
        post=[0x2728],
        desc='''
            Update login-stored ##registers.
        ''',
    )
    char_map.s(0x2b11, 'account reg2 notify',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[
            at(0, var_name, 'name'),
            at(32, u32, 'value'),
        ],
        repeat_size=36,
        pre=[0x2729],
        post=[IDLE],
        desc='''
            Broadcast login-stored ##registers.
        ''',
    )
    char_map.s(0x2b12, 'divorce notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, char_id, 'partner id'),
        ],
        fixed_size=10,
        pre=[0x0068, 0x2730, 0x2b16],
        post=[IDLE],
        desc='''
            Somebody either performed a divorce or had it done to them.
        ''',
    )
    char_map.s(0x2b13, 'account delete notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x2730],
        post=[IDLE],
        desc='''
            Disconnect player who was deleted.
        ''',
    )
    char_map.s(0x2b14, 'status or ban notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'ban not status'),
            at(7, time32, 'status or ban until'),
        ],
        fixed_size=11,
        pre=[0x2731],
        post=[IDLE],
        desc='''
            Somebody was banned or otherwise had their status changed.
        ''',
    )
    char_map.s(0x2b15, 'gm account list notify',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, account_id, 'account id'),
            at(4, gm1, 'gm level'),
        ],
        repeat_size=5,
        pre=[0x2732, 0x2af8],
        post=[IDLE],
        desc='''
            Update full list of GM accounts.
        ''',
    )
    char_map.r(0x2b16, 'divorce',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
        ],
        fixed_size=6,
        pre=[SCRIPT],
        post=[0x2b12],
        desc='''
            Live the single life again.
        ''',
    )
    # 2bfa/2bfb are injected above

    # TOC_INTERMAP

    char_map.r(0x3000, 'gm broadcast begin',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[GM, SCRIPT],
        post=[0x3800],
        desc='''
            Yell at everybody.
        ''',
    )
    char_map.r(0x3001, 'whisper remote begin',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, char_name, 'from char name'),
            at(28, char_name, 'to char name'),
        ],
        head_size=52,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x0096],
        post=[0x3801, 0x3802],
        desc='''
            Begin forwarding a private message to a nonlocal map server.
        ''',
    )
    char_map.r(0x3002, 'whisper remote forward result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, u8, 'flag'),
        ],
        fixed_size=7,
        pre=[0x3801],
        post=[0x3802],
        desc='''
            Intermediate result of forwarding a private message to a nonlocal map server.
        ''',
    )
    char_map.r(0x3003, 'wgm remote begin',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, char_name, 'char name'),
            at(28, gm2, 'min gm level'),
        ],
        head_size=30,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[GM, 0x008c, 0x0096, 0x0108],
        post=[0x3803],
        desc='''
            Begin GM whisper to all map servers.
        ''',
    )
    char_map.r(0x3004, 'save account reg',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[
            at(0, var_name, 'name'),
            at(32, u32, 'value'),
        ],
        repeat_size=36,
        pre=[MAGIC, SCRIPT],
        post=[0x3804],
        desc='''
            Save all char-server-stored account registers.
        ''',
    )
    char_map.r(0x3005, 'want account reg',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x2afd],
        post=[0x3804],
        desc='''
            Explicitly request values of char-server-stored account registers.
        ''',
    )
    char_map.r(0x3010, 'load storage',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[GM, SCRIPT],
        post=[0x3810],
        desc='''
            Request access to storage.
        ''',
    )
    char_map.r(0x3011, 'save storage',
        payload=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
            at(8, storage, 'storage'),
        ],
        payload_size=None,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[0x3811],
        desc='''
            Transmission of changed storage.
        ''',
    )
    char_map.r(0x3020, 'party create',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, party_name, 'party name'),
            at(30, char_name, 'char name'),
            at(54, map_name, 'map name'),
            at(70, u16, 'level'),
        ],
        fixed_size=72,
        pre=[GM, 0x00f9],
        post=[0x3820, 0x3821],
        desc='''
            Request creation of a party.
        ''',
    )
    char_map.r(0x3021, 'party needinfo',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
        ],
        fixed_size=6,
        pre=[0x2afd],
        post=[0x3821],
        desc='''
            Request full info about a party.
        ''',
    )
    char_map.r(0x3022, 'party add member',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, char_name, 'char name'),
            at(34, map_name, 'map name'),
            at(50, u16, 'level'),
        ],
        fixed_size=52,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0085, 0x0089, 0x008c, 0x009f, 0x00a2, 0x00a7, 0x00a9, 0x00ab, 0x00b2, 0x00b8, 0x00b9, 0x00bb, 0x00c8, 0x00c9, 0x00e4, 0x00e6, 0x00e8, 0x00eb, 0x00ed, 0x00ef, 0x00f3, 0x00f5, 0x00f7, 0x00ff, 0x0112, 0x0143, 0x0146, 0x01d5, 0x2afd, 0x2b0d],
        post=[0x3821, 0x3822, 0x3823],
        desc='''
            Request to add a party member in a slot somewhere.
        ''',
    )
    char_map.r(0x3023, 'party change option',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, u16, 'exp'),
            at(12, u16, 'item'),
        ],
        fixed_size=14,
        pre=[0x0102],
        post=[0x3823],
        desc='''
            Explicitly request a change of party settings.
        ''',
    )
    char_map.r(0x3024, 'party leave',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
        ],
        fixed_size=10,
        pre=[0x0100, 0x0103, 0x3822],
        post=[0x3821, 0x3824, 0x3826],
        desc='''
            Remove a member from party.
        ''',
    )
    char_map.r(0x3025, 'party change map',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, map_name, 'map name'),
            at(26, u8, 'online'),
            at(27, u16, 'level'),
        ],
        fixed_size=29,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0089],
        post=[0x3823, 0x3825],
        desc='''
            A party member has:

            1. loaded a new map
            2. levelled up
            3. logged out
        ''',
    )
    char_map.r(0x3027, 'party message remote begin',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, party_id, 'party id'),
            at(8, account_id, 'account id'),
        ],
        head_size=12,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x0108],
        post=[0x3827],
        desc='''
            Begin to send a party message to other map servers.
        ''',
    )
    char_map.r(0x3028, 'party check conflict',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, char_name, 'char name'),
        ],
        fixed_size=34,
        pre=[FINISH, GM, MAGIC, SCRIPT, TIMER, 0x007d, 0x0089, 0x3822],
        post=[0x3821, 0x3824, 0x3826],
        desc='''
            Explicitly request a conflict check.

            (I am not sure this is actually meaningful).
        ''',
    )

    char_map.s(0x3800, 'gm broadcast forward',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x2726, 0x3000],
        post=[0x009a, 0x00c0],
        desc='''
            Yell, reaching everybody.
        ''',
    )
    char_map.s(0x3801, 'whisper remote forward',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, char_id, 'whisper id'),
            at(8, char_name, 'src char name'),
            at(32, char_name, 'dst char name'),
        ],
        head_size=56,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x3001],
        post=[0x0097, 0x3002],
        desc='''
            Intermediate forwarding a private message to a nonlocal map server.
        ''',
    )
    char_map.s(0x3802, 'whisper remote begin result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'sender char name'),
            at(26, u8, 'flag'),
        ],
        fixed_size=27,
        pre=[0x3001, 0x3002],
        post=[0x0098],
        desc='''
            Final result of forwarding a private message to a nonlocal map server.
        ''',
    )
    char_map.s(0x3803, 'wgm remote forward',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, char_name, 'char name'),
            at(28, gm2, 'min gm level'),
        ],
        head_size=30,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x3003],
        post=[0x0097],
        desc='''
            Forward of GM whisper.
        ''',
    )
    char_map.s(0x3804, 'account reg notify',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[
            at(0, var_name, 'name'),
            at(32, u32, 'value'),
        ],
        repeat_size=36,
        pre=[0x3004, 0x3005],
        post=[IDLE],
        desc='''
            Char-server-stored account regs have changed.

            Also sent on explicit request.
        ''',
    )
    char_map.s(0x3810, 'load storage result',
        payload=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
            at(8, storage, 'storage'),
        ],
        payload_size=None,
        pre=[0x3010],
        post=[0x00a6, 0x00f2, 0x01f0],
        desc='''
            Transmission of requested storage.
        ''',
    )
    char_map.s(0x3811, 'save storage result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'unknown'),
        ],
        fixed_size=7,
        pre=[0x3011],
        post=[IDLE],
        desc='''
            Acknowledgement of saved storage.
        ''',
    )
    char_map.s(0x3820, 'party create result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'error'),
            at(7, party_id, 'party id'),
            at(11, party_name, 'party name'),
        ],
        fixed_size=35,
        pre=[0x3020],
        post=[0x00fa],
        desc='''
            Result of requesting party creation.
        ''',
    )
    char_map.s(0x3821, 'party needinfo result',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, party_id, 'party id'),
        ],
        head_size=8,
        option=[
            at(0, party_most, 'party most'),
        ],
        option_size=None,
        pre=[0x0068, 0x2730, 0x3020, 0x3021, 0x3022, 0x3024, 0x3028],
        post=[0x00c0, 0x00fb, 0x0101],
        desc='''
            Full info about a party, it present.

            Payload may be missing if no such party (*should* not
            happen, but ...).
        ''',
    )
    char_map.s(0x3822, 'party add member result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, u8, 'flag'),
        ],
        fixed_size=11,
        pre=[0x3022],
        post=[0x00fd, 0x3024, 0x3028],
        desc='''
            Result of trying to add a party member.

            Flag is 1 if party is no slots or something.
        ''',
    )
    char_map.s(0x3823, 'party change option notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, u16, 'exp'),
            at(12, u16, 'item'),
            at(14, u8, 'flag'),
        ],
        fixed_size=15,
        pre=[0x3022, 0x3023, 0x3025],
        post=[0x00c0, 0x0101],
        desc='''
            Party options were changed.

            Often generated implicitly.
        ''',
    )
    char_map.s(0x3824, 'party leave notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, char_name, 'char name'),
        ],
        fixed_size=34,
        pre=[0x0068, 0x2730, 0x3024, 0x3028],
        post=[0x00c0, 0x0105],
        desc='''
            Member is no longer in party.
        ''',
    )
    char_map.s(0x3825, 'party change map notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, map_name, 'map name'),
            at(26, u8, 'online'),
            at(27, u16, 'level'),
        ],
        fixed_size=29,
        pre=[0x3025],
        post=[0x00c0, 0x00fb],
        desc='''
            Party member location/level/online status has changed.
        ''',
    )
    char_map.s(0x3826, 'party broken notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, u8, 'flag'),
        ],
        fixed_size=7,
        pre=[BOOT, 0x0068, 0x2730, 0x3024, 0x3028],
        post=[0x00c0, 0x0105],
        desc='''
            A party just isn't there anymore.
        ''',
    )
    char_map.s(0x3827, 'party message remote forward',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, party_id, 'party id'),
            at(8, account_id, 'account id'),
        ],
        head_size=12,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x3027],
        post=[0x00c0, 0x0109],
        desc='''
            Actually send a party message to other map servers.
        ''',
    )

    # TOC_MISC
    # any client
    any_user.r(0x7530, 'version',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[ADMIN, BOOT, 0x7919],
        post=[0x7531],
        desc='''
            Request from client or ladmin for server version.
        ''',
    )
    any_user.s(0x7531, 'version result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, version, 'version'),
        ],
        fixed_size=10,
        pre=[0x7530],
        post=[0x0064],
        desc='''
            Response to client's request for server version.
        ''',
    )
    any_user.r(0x7532, 'disconnect',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[HUMAN],
        post=[FINISH],
        desc='''
            Request from client or ladmin to disconnect.
        ''',
    )
    # 0x7530 define='CMSG_SERVER_VERSION_REQUEST',
    # 0x7531 define='SMSG_SERVER_VERSION_RESPONSE',
    # 0x7532 define='CMSG_CLIENT_DISCONNECT',

    # TOC_LOGINADMIN
    # login admin
    login_admin.r(0x7918, 'admin auth',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'encryption zero'),
            at(4, account_pass, 'account pass'),
        ],
        fixed_size=28,
        pre=[BOOT],
        post=[0x7919],
        desc='''
            Authenticate as an admin.
        ''',
    )
    login_admin.s(0x7919, 'admin auth result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'error'),
        ],
        fixed_size=3,
        pre=[0x7918],
        post=[0x7530],
        desc='''
            Status of admin authentication.
        ''',
    )
    login_admin.r(0x7920, 'account list',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'start account id'),
            at(6, account_id, 'end account id'),
        ],
        fixed_size=10,
        pre=[ADMIN, 0x7921],
        post=[0x7921],
        desc='''
            Request list of all accounts (between bounds if given).
        ''',
    )
    login_admin.s(0x7921, 'account list result',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, account_id, 'account id'),
            at(4, gm1, 'gm level'),
            at(5, account_name, 'account name'),
            at(29, sex, 'sex'),
            at(30, u32, 'login count'),
            at(34, u32, 'status'),
        ],
        repeat_size=38,
        pre=[0x7920],
        post=[IDLE, 0x7920],
        desc='''
            May be truncated, if so should retry with new lower bound.
        ''',
    )
    login_admin.r(0x7930, 'account create',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'password'),
            at(50, sex_char, 'sex'),
            at(51, account_email, 'email'),
        ],
        fixed_size=91,
        pre=[ADMIN],
        post=[0x7931],
        desc='''
            Account creation request.
        ''',
    )
    login_admin.s(0x7931, 'account create result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7930],
        post=[IDLE],
        desc='''
            Account creation response.
        ''',
    )
    login_admin.r(0x7932, 'account delete',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
        ],
        fixed_size=26,
        pre=[ADMIN],
        post=[0x2730, 0x7933],
        desc='''
            Account deletion request.
        ''',
    )
    login_admin.s(0x7933, 'account delete result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7932],
        post=[IDLE],
        desc='''
            Account deletion response.
        ''',
    )
    login_admin.r(0x7934, 'password change',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'password'),
        ],
        fixed_size=50,
        pre=[ADMIN],
        post=[0x7935],
        desc='''
            Change password request.
        ''',
    )
    login_admin.s(0x7935, 'password change result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7934],
        post=[IDLE],
        desc='''
            Change password response.
        ''',
    )
    login_admin.r(0x7936, 'account state change',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, u32, 'status'),
            at(30, seconds, 'error message'),
        ],
        fixed_size=50,
        pre=[ADMIN],
        post=[0x2731, 0x7937],
        desc='''
            Account state change request.
        ''',
    )
    login_admin.s(0x7937, 'account state change result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
            at(30, u32, 'status'),
        ],
        fixed_size=34,
        pre=[0x7936],
        post=[IDLE],
        desc='''
            Account state change response.
        ''',
    )
    login_admin.r(0x7938, 'server list',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[ADMIN],
        post=[0x7939],
        desc='''
            Server list and player count request.
        ''',
    )
    login_admin.s(0x7939, 'server list result',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, ip4, 'ip'),
            at(4, u16, 'port'),
            at(6, server_name, 'name'),
            at(26, u16, 'users'),
            at(28, u16, 'maintenance'),
            at(30, u16, 'is new'),
        ],
        repeat_size=32,
        pre=[0x7938],
        post=[IDLE],
        desc='''
            Server list and player count response.
        ''',
    )
    login_admin.r(0x793a, 'password check',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'password'),
        ],
        fixed_size=50,
        pre=[ADMIN],
        post=[0x793b],
        desc='''
            Password check request.
        ''',
    )
    login_admin.s(0x793b, 'password check result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x793a],
        post=[IDLE],
        desc='''
            Password check response.
        ''',
    )
    login_admin.r(0x793c, 'change sex',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, sex_char, 'sex'),
        ],
        fixed_size=27,
        pre=[ADMIN],
        post=[0x2723, 0x793d],
        desc='''
            Modify sex request.
        ''',
    )
    login_admin.s(0x793d, 'change sex result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x793c],
        post=[IDLE],
        desc='''
            Modify sex response.
        ''',
    )
    login_admin.r(0x793e, 'adjust gm level',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, gm1, 'gm level'),
        ],
        fixed_size=27,
        pre=[ADMIN],
        post=[0x2732, 0x793f],
        desc='''
            Modify GM level request.
        ''',
    )
    login_admin.s(0x793f, 'adjust gm level result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x793e],
        post=[IDLE],
        desc='''
            Modify GM level response.
        ''',
    )
    login_admin.r(0x7940, 'change email',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_email, 'email'),
        ],
        fixed_size=66,
        pre=[ADMIN],
        post=[0x7941],
        desc='''
            Modify e-mail request.
        ''',
    )
    login_admin.s(0x7941, 'change email result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7940],
        post=[IDLE],
        desc='''
            Modify e-mail response.
        ''',
    )
    # this packet is insane
    login_admin.r(0x7942, 'change memo',
        head=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, SkewLengthType(u16, 28), 'magic packet length'),
        ],
        head_size=28,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[ADMIN],
        post=[0x7943],
        desc='''
            Modify memo request.
        ''',
    )
    login_admin.s(0x7943, 'change memo result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7942],
        post=[IDLE],
        desc='''
            Modify memo response.
        ''',
    )
    login_admin.r(0x7944, 'account id lookup',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
        ],
        fixed_size=26,
        pre=[ADMIN],
        post=[0x7945],
        desc='''
            Find account id request.
        ''',
    )
    login_admin.s(0x7945, 'account id lookup result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7944],
        post=[IDLE],
        desc='''
            Find account id response.
        ''',
    )
    login_admin.r(0x7946, 'account name lookup',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[ADMIN],
        post=[0x7947],
        desc='''
            Find account name request.
        ''',
    )
    login_admin.s(0x7947, 'account name lookup result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7946],
        post=[IDLE],
        desc='''
            Find account name response.
        ''',
    )
    login_admin.r(0x794a, 'ban absolute',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, time32, 'ban until'),
        ],
        fixed_size=30,
        pre=[ADMIN],
        post=[0x2731, 0x794b],
        desc='''
            Ban date end change request.
        ''',
    )
    login_admin.s(0x794b, 'ban absolute result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
            at(30, time32, 'ban until'),
        ],
        fixed_size=34,
        pre=[0x794a],
        post=[IDLE],
        desc='''
            Ban date end change response.
        ''',
    )
    login_admin.r(0x794c, 'ban relative',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, human_time_diff, 'ban add'),
        ],
        fixed_size=38,
        pre=[ADMIN],
        post=[0x2731, 0x794d],
        desc='''
            Ban date end change request (2).
        ''',
    )
    login_admin.s(0x794d, 'ban relative result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
            at(30, time32, 'ban until'),
        ],
        fixed_size=34,
        pre=[0x794c],
        post=[IDLE],
        desc='''
            Ban date end change response (2).
        ''',
    )
    # evil packet (see also 0x2726)
    login_admin.r(0x794e, 'broadcast message',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'unused'),
            at(4, SkewLengthType(u32, 8), 'magic packet length'),
        ],
        head_size=8,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[ADMIN],
        post=[0x2726, 0x794f],
        desc='''
            Send broadcast message request.
        ''',
    )
    login_admin.s(0x794f, 'broadcast message result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'error'),
        ],
        fixed_size=4,
        pre=[0x794e],
        post=[IDLE],
        desc='''
            Send broadcast message response.
        ''',
    )
    login_admin.r(0x7952, 'account name info',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
        ],
        fixed_size=26,
        pre=[ADMIN],
        post=[0x7953],
        desc='''
            Account information by name request.
        ''',
    )
    # this packet is insane
    login_admin.s(0x7953, 'account info result',
        head=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, gm1, 'gm level'),
            at(7, account_name, 'account name'),
            at(31, sex, 'sex'),
            at(32, u32, 'login count'),
            at(36, u32, 'state'),
            at(40, seconds, 'error message'),
            at(60, millis, 'last login string'),
            at(84, str16, 'ip string'),
            at(100, account_email, 'email'),
            at(140, time32, 'unused connect until'),
            at(144, time32, 'ban until'),
            at(148, SkewLengthType(u16, 150), 'magic packet length'),
        ],
        head_size=150,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x7952, 0x7954],
        post=[IDLE],
        desc='''
            Account information by name or id response.
        ''',
    )
    login_admin.r(0x7954, 'account id info',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[ADMIN],
        post=[0x7953],
        desc='''
            Account information by id request.
        ''',
    )
    login_admin.r(0x7955, 'reload gm',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[ADMIN],
        post=[0x2732],
        desc='''
            Reload GM file request.
        ''',
    )

    # TOC_NEW
    ## new-style packets
    # notify packets, standalone, can occur at any time; always 'payload'
    any_user.s(0x8000, 'special hold notify',
        payload=[
            at(0, u16, 'packet id'),
            # packet 0x8000 is handled specially
            at(2, u16, 'packet length'),
        ],
        payload_size=4,
        pre=[OTHER],
        post=[OTHER],
        desc='''
            A special packet that is handled specially.

            The "is the packet complete" logic will read its given length,
            but the "skip packet" will only skip 4 bytes, thus allowing
            transactions. I'm still not entirely convinced that this is a
            good idea - perhaps explicit 'being transaction buffer' and
            'end transaction buffer' packets? Allow nesting?
        ''',
    )
    # invoke/return/error packets, threaded; always 'payload'
    # invoke:   0x9000
    # return:   0xA000
    # error:    0xB000
    # invoke:   0x9001
    # return:   0xA001
    # error:    0xB001
    # ... or should it be implicit via a wrapper?
    #
    # add_call_packet(from=ANY, to=ANY, id='0x?001', name='version info'
    # invoke_type=struct(), return_type=struct(), error_type=struct())
    # for that matter, should these things be read from separate files?
    # still a lot of ambiguous stuff here ...
    #
    # no, merging is a bad idea at the low level
    # ... but for doc it is ...

    return ctx

# TOC_DRAWING

def fix_sort(idlst):
    idlst = set(idlst)
    return (sorted([i for i in idlst if isinstance(i, SpecialEventOrigin)], key=lambda e: e.name)
            + sorted([i for i in idlst if not isinstance(i, SpecialEventOrigin)]))

def check(ctx):
    d = {}

    for ch in ctx._channels:
        for p in ch.packets:
            id = p.id
            if id in d:
                print('packet 0x%04x duplicated (old=%r, new=%r)' % (id, d[id], p))
            d[id] = p

    for (id, packet) in sorted(d.items()):
        if packet.pre != fix_sort(packet.pre):
            print('packet 0x%04x pre is not sorted' % id)
        for pre in packet.pre:
            if isinstance(pre, SpecialEventOrigin):
                continue
            if pre not in d:
                print('packet 0x%04x pre 0x%04x does not exist' % (id, pre))
            elif id not in d[pre].post and id not in d[pre].xpost:
                print('packet 0x%04x pre 0x%04x does not have me in post or xpost' % (id, pre))
        if packet.post != fix_sort(packet.post):
            print('packet 0x%04x post is not sorted' % id)
        if packet.xpost != fix_sort(packet.xpost):
            print('packet 0x%04x xpost is not sorted' % id)
        if len(set(packet.post) | set(packet.xpost)) != len(packet.post) + len(packet.xpost):
            print('packet 0x%04x post intersects xpost' % id)
        for post in packet.post:
            if isinstance(post, SpecialEventOrigin):
                continue
            if post not in d:
                print('packet 0x%04x post 0x%04x does not exist' % (id, post))
            elif id not in d[post].pre:
                print('packet 0x%04x post 0x%04x does not have me in pre' % (id, post))

    return d

def partition(d):
    ''' given a directed graph in the form
        {1: [2, 3], 2: [3, 4], 3: [], 4: [], 5: [6], 6: []}
        return a list of sets of connected keys, in the form
        [{1, 2, 3, 4}, {5, 6}]
    '''
    leaders = {k: k for k in d}

    # this code looks nothing like what I was intending to write
    changed = True
    while changed:
        changed = False
        for k, vlist in d.items():
            if vlist:
                m = min(leaders[v] for v in vlist)
                if m < leaders[k]:
                    changed = True
                    leaders[k] = m
                else:
                    m = leaders[k]
            else:
                m = leaders[k]
            for v in vlist:
                if m < leaders[v]:
                    changed = True
                    leaders[v] = m

    followers = {}
    for k, v in leaders.items():
        followers.setdefault(v, []).append(k)
    return [set(v) for v in followers.values()]

def ids_only(vpost):
    rv = [e for e in vpost if not isinstance(e, SpecialEventOrigin)]
    if len(rv) == len(vpost):
        rv = vpost
    return rv

def make_dots(ctx):
    d = check(ctx)
    # automatically excludes xpost and does not touch pre
    # disabled because that's still just too many packets for the 0x0063
    #p = partition({k: ids_only(v.post) for (k, v) in d.items()})

    if not os.path.exists('doc-gen'):
        # generate.make will succeed if missing the wiki repo
        # but 'make doc' will fail
        return
    for g in glob.glob('doc-gen/*.gv'):
        os.rename(g, g + '.old')
    for g in glob.glob('doc-gen/Packet-*.md'):
        os.rename(g, g + '.old')

    for (id, p) in d.items():
        md = 'doc-gen/Packet-0x%04x.md' % id
        dot = 'doc-gen/packets-around-0x%04x.gv' % id
        with OpenWrite(md) as f:
            f.write(p.wiki_doc())
        with OpenWrite(dot) as f:
            # The goal looks sort of like this:
            #
            #     O   O O   O
            #      \ /   \ /
            #       O     O
            #        \   /
            #         \ /
            #          O
            #         / \
            #        /   \
            #       O     O
            #      / \   / \
            #     O   O O   O
            #
            # except that we also connect any extra connections between those
            # in particular, this will DTRT for the common skip-case:
            #
            #   |
            #   O
            #   |\
            #   | O
            #   | |
            #   | O
            #   |/
            #   O
            #   |
            #
            # regardless of which node you're drawing from.
            #
            # however, note that we do *not* list siblings.
            #
            # The above comment is wrong.
            # Also, remember that none of this would be necessary without
            # the overbroad dependencies!
            #
            # ... except that I got rid of the overbroad dependencies but
            # had to kept this still. It's arguably nicer anyway, so I
            # also made it the only way.
            covered_nodes = sorted(p.pre_set(d, 2) | p.post_set(d, 2))
            covered_edges = [(a, b) for a in covered_nodes for b in covered_nodes if b in d[a].post]
            g = Graph()
            g.default_vertex[u'shape'] = u'box'
            # g[u'layout'] = u'twopi'
            # g[u'root'] = u'0x%04x' % id
            for n in covered_nodes:
                v = g.vertex(u'0x%04x' % n)
                v[u'label'] = u'Packet \\N: %s' % d[n].name
                if n == id:
                    v[u'style'] = u'filled'
            for (a, b) in covered_edges:
                g.edge(u'0x%04x' % a, u'0x%04x' % b)
            for n in covered_nodes:
                # the center node will be covered specially below
                if n == id:
                    continue
                # backward links (both strong and weak)
                count = 0
                for p in d[n].pre:
                    # pre specials are always strong links
                    if isinstance(p, SpecialEventOrigin):
                        count += 1
                    elif n in d[p].xpost:
                        # weak backward links
                        pass
                    elif p not in covered_nodes:
                        # don't show mere siblings unless also ancestor/descendent
                        count += 1
                if count:
                    v = g.vertex(u'0x%04x...pre' % n)
                    v[u'label'] = u'%d more' % count
                    v[u'style'] = u'dashed'
                    g.edge(v, u'0x%04x' % n)
                # strong forward links
                count = 0
                for p in d[n].post:
                    if isinstance(p, SpecialEventOrigin):
                        count += 1
                    elif p not in covered_nodes:
                        count += 1
                if count:
                    v = g.vertex(u'0x%04x...post' % n)
                    v[u'label'] = u'%d more' % count
                    v[u'style'] = u'dashed'
                    g.edge(u'0x%04x' % n, v)
            # for the immediate node, also cover specials and weaks
            for p in d[id].pre:
                # (there are no weak backward specials)
                # strong backward specials
                if isinstance(p, SpecialEventOrigin):
                    g.edge(unicode(p.name), u'0x%04x' % id)
                # weak backward nodes
                elif id in d[p].xpost:
                    e = g.edge(u'0x%04x' % p, u'0x%04x' % id)
                    e[u'style']=u'dashed'
                    e[u'weight'] = u'0'
            for p in d[id].post:
                # strong forward specials
                if isinstance(p, SpecialEventOrigin):
                    g.edge(u'0x%04x' % id, unicode(p.name))
            for p in d[id].xpost:
                # weak forward specials
                if isinstance(p, SpecialEventOrigin):
                    e = g.edge(u'0x%04x' % id, unicode(p.name))
                    e[u'style'] = u'dashed'
                    e[u'weight'] = u'0'
                # weak forward nodes
                elif p not in covered_nodes:
                    e = g.edge(u'0x%04x' % id, u'0x%04x' % p)
                    e[u'style'] = u'dashed'
                    e[u'weight'] = u'0'
            g.dot(f, False)


    for g in glob.glob('doc-gen/*.old'):
        print('Obsolete: %s' % g)
        os.remove(g)

# TOC_MAIN

def main():
    ctx = build_context()
    ## teardown
    make_dots(ctx)
    ctx.dump()

if __name__ == '__main__':
    main()
