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

import glob
import itertools
import os
from pipes import quote
from posixpath import relpath

# The following code should be relatively easy to understand, but please
# keep your sanity fastened and your arms and legs inside at all times.

# important note: all numbers in this file will make a lot more sense in
# decimal, but they're written in hex.

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
        if cmp {0}.tmp {0}.old
        then
            echo Unchanged: {0}
            rm {0}.tmp
            mv {0}.old {0}
        else
            echo Changed: {0}
            rm {0}.old
            mv {0}.tmp {0}
        fi
        '''.format(quote(self.filename))
        os.system(frag)

class LowType(object):
    __slots__ = ()

class NativeType(LowType):
    __slots__ = ('name')

    def __init__(self, name):
        self.name = name

    def a_tag(self):
        return self.name

class NetworkType(LowType):
    __slots__ = ('name')

    def __init__(self, name):
        self.name = name

    def e_tag(self):
        return self.name


class Type(object):
    __slots__ = ()

class NeutralType(Type):
    __slots__ = ('name')

    def __init__(self, name):
        self.name = name

    def native_tag(self):
        return self.name

    def network_tag(self):
        return self.name

    e_tag = network_tag

class StringType(Type):
    __slots__ = ('native')

    def __init__(self, native):
        self.native = native

    def native_tag(self):
        return self.native.a_tag()

    def network_tag(self):
        return 'NetString<sizeof(%s)>' % self.native.a_tag()

    def dump(self, f):
        # not implemented properly, uses a template in the meta instead
        pass

class ProvidedType(Type):
    __slots__ = ('native', 'network')

    def __init__(self, native, network):
        self.native = native
        self.network = network

    def native_tag(self):
        return self.native.a_tag()

    def network_tag(self):
        return self.network.e_tag()

class EnumType(Type):
    __slots__ = ('native', 'under')

    def __init__(self, native, under):
        self.native = native
        self.under = under

    def native_tag(self):
        return self.native.a_tag()

    def network_tag(self):
        return self.under.network_tag()

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

    def native_tag(self):
        return self.native.a_tag()

    def network_tag(self):
        return self.under.network_tag()

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

    def native_tag(self):
        return self.type.native_tag()

    def network_tag(self):
        return 'SkewedLength<%s, %d>' % (self.type.network_tag(), self.skew)

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

    def native_tag(self):
        return self.name

    def network_tag(self):
        return 'Net' + self.name

    def dump(self, f):
        self.dump_native(f)
        self.dump_network(f)
        self.dump_convert(f)
        f.write('\n')

    def dump_fwd(self, fwd):
        if self.id is not None:
            fwd.write('template<>\n')
        fwd.write('struct %s;\n' % self.name)
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

    def native_tag(self):
        return self.native.a_tag()

    def network_tag(self):
        return 'Net%s' % self.native_tag()

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

    def native_tag(self):
        return 'GenericArray<%s, InventoryIndexing<%s, %s>>' % (self.element.native_tag(), self.index, self.count)

    def network_tag(self):
        return 'NetArray<%s, %s>' % (self.element.network_tag(), self.count)


class Include(object):
    __slots__ = ('path', '_types')

    def __init__(self, path):
        self.path = path
        self._types = []

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

            for t in self._types:
                f.write('using %s = %s;\n' % ('Test_' + ident(t.name), t.name))
            f.write('} // namespace tmwa\n')

    def pp(self, n):
        return '#%*sinclude %s\n' % (n, '', self.path)


    def native(self, name):
        ty = NativeType(name)
        self._types.append(ty)
        return ty

    def network(self, name):
        ty = NetworkType(name)
        self._types.append(ty)
        return ty

    def neutral(self, name):
        ty = NeutralType(name)
        self._types.append(ty)
        return ty


class FixedPacket(object):
    __slots__ = ('fixed_struct', 'comment')

    def __init__(self, fixed_struct, comment):
        self.fixed_struct = fixed_struct
        self.comment = comment

    def dump_fwd(self, fwd):
        self.fixed_struct.dump_fwd(fwd)
        fwd.write('\n')

    def dump_native(self, f):
        f.write(self.comment)
        self.fixed_struct.dump_native(f)
        f.write('\n')

    def dump_network(self, f):
        self.fixed_struct.dump_network(f)
        f.write('\n')

    def dump_convert(self, f):
        self.fixed_struct.dump_convert(f)
        f.write('\n')

class VarPacket(object):
    __slots__ = ('head_struct', 'repeat_struct', 'comment')

    def __init__(self, head_struct, repeat_struct, comment):
        self.head_struct = head_struct
        self.repeat_struct = repeat_struct
        self.comment = comment

    def dump_fwd(self, fwd):
        self.head_struct.dump_fwd(fwd)
        self.repeat_struct.dump_fwd(fwd)
        fwd.write('\n')

    def dump_native(self, f):
        f.write(self.comment)
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

def packet(id, name,
        fixed=None, fixed_size=None,
        payload=None, payload_size=None,
        head=None, head_size=None,
        repeat=None, repeat_size=None,
        option=None, option_size=None,
        pre=None, post=None, desc=None,
):
    assert (fixed is None) <= (fixed_size is None)
    assert (payload is None) <= (payload_size is None)
    assert (head is None) <= (head_size is None)
    assert (repeat is None) <= (repeat_size is None)
    assert (option is None) <= (option_size is None)
    assert (pre is None) == (post is None) == (not desc)

    comment = 'Packet 0x%04x: "%s"\n' % (id, name)
    if desc:
        desc = sanitize_multiline(desc)
        pre = ', '.join('packet 0x%04x' % x for x in pre) or 'none'
        post = ', '.join('packet 0x%04x' % x for x in post) or 'none'
        comment += 'pre:  ' + pre + '\n'
        comment += 'post: ' + post + '\n'
        comment += desc
    comment = ''.join('// ' + c + '\n' if c else '//\n' for c in comment.split('\n'))

    if fixed is not None:
        assert not head and not repeat and not option and not payload
        return FixedPacket(
                StructType(id, 'Packet_Fixed<0x%04x>' % id, fixed, fixed_size),
                comment=comment,
        )
    elif payload is not None:
        assert not head and not repeat and not option
        return FixedPacket(
                StructType(id, 'Packet_Payload<0x%04x>' % id, payload, payload_size),
                comment=comment,
        )
    else:
        assert head
        if option:
            return VarPacket(
                    StructType(id, 'Packet_Head<0x%04x>' % id, head, head_size),
                    StructType(id, 'Packet_Option<0x%04x>' % id, option, option_size),
                    comment=comment,
            )
        else:
            assert repeat
            return VarPacket(
                    StructType(id, 'Packet_Head<0x%04x>' % id, head, head_size),
                    StructType(id, 'Packet_Repeat<0x%04x>' % id, repeat, repeat_size),
                    comment=comment,
            )


class Channel(object):
    __slots__ = ('server', 'client', 'packets')

    def __init__(self, server, client):
        self.server = server
        self.client = client
        self.packets = []

    def x(self, id, name, **kwargs):
        self.packets.append(packet(id, name, **kwargs))
    r = x
    s = x

    def dump(self, outdir, fwd):
        server = self.server
        client = self.client
        header = '%s-%s.hpp' % (server, client)
        test = '%s-%s_test.cpp' % (server, client)
        desc = 'TMWA network protocol: %s/%s' % (server, client)
        with OpenWrite(os.path.join(outdir, header)) as f:
            proto2 = relpath(outdir, 'src')
            f.write('#pragma once\n')
            f.write(copyright.format(filename=header, description=desc))
            f.write('\n')
            f.write(generated)
            f.write('\n')
            f.write('#include "fwd.hpp"\n\n')
            f.write('#include "types.hpp"\n')
            f.write('\n')
            f.write('namespace tmwa\n{\n')
            if client == 'user':
                f.write('// This is a public protocol, and changes require client cooperation\n')
            else:
                f.write('// This is an internal protocol, and can be changed without notice\n')
            f.write('\n')
            for p in self.packets:
                p.dump_fwd(fwd)
            fwd.write('\n')
            for p in self.packets:
                p.dump_native(f)
            f.write('\n')
            for p in self.packets:
                p.dump_network(f)
            f.write('\n')
            for p in self.packets:
                p.dump_convert(f)
            f.write('} // namespace tmwa\n')

        with OpenWrite(os.path.join(outdir, test)) as f:
            poison = relpath('src/poison.hpp', outdir)
            f.write('#include "%s"\n' % header)
            f.write(copyright.format(filename=test, description=desc))
            f.write('\n')
            f.write(generated)
            f.write('\n')
            f.write('#include "%s"\n\nnamespace tmwa\n{\n' % poison)
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
            desc = 'Forward declarations of network packets'
            sanity = relpath('src/sanity.hpp', outdir)
            f.write('#pragma once\n')
            f.write(copyright.format(filename=header, description=desc))
            f.write('\n')
            f.write('#include "%s"\n\n' % sanity)
            f.write('#include <cstdint>\n\n')
            f.write('namespace tmwa\n{\n')
            for b in ['Fixed', 'Payload', 'Head', 'Repeat', 'Option']:
                c = 'Packet_' + b
                f.write('template<uint16_t PACKET_ID> class %s;\n' % c)
                f.write('template<uint16_t PACKET_ID> class Net%s;\n' % c)
            f.write('\n')

            for ch in self._channels:
                ch.dump(outdir, f)

            f.write('} // namespace tmwa\n')

        with OpenWrite(os.path.join(outdir, 'types.hpp')) as f:
            header = '%s/types.hpp' % proto2
            desc = 'Forward declarations of packet component types'
            f.write('#pragma once\n')
            f.write(copyright.format(filename=header, description=desc))
            f.write('\n')
            f.write('#include "fwd.hpp"\n\n')
            f.write('#include "../generic/array.hpp"\n')
            f.write('#include "../mmo/consts.hpp"\n')

            f.write('\n//TODO split the includes\n')
            for inc in self._includes:
                f.write(inc.pp(0))
                # this is writing another file
                inc.testcase(outdir)
            f.write('\n')
            f.write('namespace tmwa\n{\n')

            f.write('template<class T>\n')
            f.write('bool native_to_network(T *network, T native)\n{\n')
            f.write('    *network = native;\n')
            f.write('    return true;\n')
            f.write('}\n')
            f.write('template<class T>\n')
            f.write('bool network_to_native(T *native, T network)\n{\n')
            f.write('    *native = network;\n')
            f.write('    return true;\n')
            f.write('}\n')

            f.write('template<class T, size_t N>\n')
            f.write('struct NetArray\n{\n')
            f.write('    T data[N];\n')
            f.write('};\n')
            f.write('template<class T, class U, class I>\n')
            f.write('bool native_to_network(NetArray<T, I::alloc_size> *network, GenericArray<U, I> native)\n{\n')
            f.write('    for (size_t i = 0; i < I::alloc_size; ++i)\n')
            f.write('    {\n')
            f.write('        if (!native_to_network(&(*network).data[i], native[I::offset_to_index(i)]))\n')
            f.write('            return false;\n')
            f.write('    }\n')
            f.write('    return true;\n')
            f.write('}\n')
            f.write('template<class T, class U, class I>\n')
            f.write('bool network_to_native(GenericArray<U, I> *native, NetArray<T, I::alloc_size> network)\n{\n')
            f.write('    for (size_t i = 0; i < I::alloc_size; ++i)\n')
            f.write('    {\n')
            f.write('        if (!network_to_native(&(*native)[I::offset_to_index(i)], network.data[i]))\n')
            f.write('            return false;\n')
            f.write('    }\n')
            f.write('    return true;\n')
            f.write('}\n')
            f.write('\n')

            f.write('template<size_t N>\n')
            f.write('struct NetString\n{\n')
            f.write('    char data[N];\n')
            f.write('};\n')
            f.write('template<size_t N>\n')
            f.write('bool native_to_network(NetString<N> *network, VString<N-1> native)\n{\n')
            f.write('    // basically WBUF_STRING\n')
            f.write('    char *const begin = network->data;\n')
            f.write('    char *const end = begin + N;\n')
            f.write('    char *const mid = std::copy(native.begin(), native.end(), begin);\n')
            f.write('    std::fill(mid, end, \'\\0\');\n')
            f.write('    return true;\n')
            f.write('}\n')
            f.write('template<size_t N>\n')
            f.write('bool network_to_native(VString<N-1> *native, NetString<N> network)\n{\n')
            f.write('    // basically RBUF_STRING\n')
            f.write('    const char *const begin = network.data;\n')
            f.write('    const char *const end = begin + N;\n')
            f.write('    const char *const mid = std::find(begin, end, \'\\0\');\n')
            f.write('    *native = XString(begin, mid, nullptr);\n')
            f.write('    return true;\n')
            f.write('}\n')
            f.write('\n')
            f.write('inline\n')
            f.write('bool native_to_network(NetString<24> *network, CharName native)\n{\n')
            f.write('    VString<23> tmp = native.to__actual();\n')
            f.write('    bool rv = native_to_network(network, tmp);\n')
            f.write('    return rv;\n')
            f.write('}\n')
            f.write('inline\n')
            f.write('bool network_to_native(CharName *native, NetString<24> network)\n{\n')
            f.write('    VString<23> tmp;\n')
            f.write('    bool rv = network_to_native(&tmp, network);\n')
            f.write('    *native = stringish<CharName>(tmp);\n')
            f.write('    return rv;\n')
            f.write('}\n')
            f.write('\n')
            f.write('inline\n')
            f.write('bool native_to_network(NetString<16> *network, MapName native)\n{\n')
            f.write('    XString tmp = native;\n')
            f.write('    bool rv = native_to_network(network, VString<15>(tmp));\n')
            f.write('    return rv;\n')
            f.write('}\n')
            f.write('inline\n')
            f.write('bool network_to_native(MapName *native, NetString<16> network)\n{\n')
            f.write('    VString<15> tmp;\n')
            f.write('    bool rv = network_to_native(&tmp, network);\n')
            f.write('    *native = stringish<MapName>(tmp);\n')
            f.write('    return rv;\n')
            f.write('}\n')
            f.write('\n')

            f.write('template<class T, size_t N>\n')
            f.write('struct SkewedLength\n{\n')
            f.write('    T data;\n')
            f.write('};\n')
            f.write('template<class T, size_t N, class U>\n')
            f.write('bool native_to_network(SkewedLength<T, N> *network, U native)\n{\n')
            f.write('    native -= N;\n')
            f.write('    return native_to_network(&network->data, native);\n')
            f.write('}\n')
            f.write('template<class T, size_t N, class U>\n')
            f.write('bool network_to_native(U *native, SkewedLength<T, N> network)\n{\n')
            f.write('    bool rv = network_to_native(native, network.data);\n')
            f.write('    *native += N;\n')
            f.write('    return rv;\n')
            f.write('}\n')
            f.write('\n')

            for ty in self._types:
                ty.dump(f)
            f.write('} // namespace tmwa\n')

        for g in glob.glob(os.path.join(outdir, '*.old')):
            print('Obsolete: %s' % g)
            os.remove(g)

    # types

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


def main():

    ## setup

    ctx = Context(outdir='src/proto2/')


    ## headers

    cstdint = ctx.sysinclude('cstdint')

    endians_h = ctx.include('src/ints/little.hpp')

    vstring_h = ctx.include('src/strings/vstring.hpp')

    ip_h = ctx.include('src/net/ip.hpp')
    timer_th = ctx.include('src/net/timer.t.hpp')

    consts_h = ctx.include('src/mmo/consts.hpp')
    enums_h = ctx.include('src/mmo/enums.hpp')
    human_time_diff_h = ctx.include('src/mmo/human_time_diff.hpp')
    ids_h = ctx.include('src/mmo/ids.hpp')
    strs_h = ctx.include('src/mmo/strs.hpp')
    utils_h = ctx.include('src/mmo/utils.hpp')
    version_h = ctx.include('src/mmo/version.hpp')

    login_th = ctx.include('src/login/login.t.hpp')

    clif_th = ctx.include('src/map/clif.t.hpp')
    skill_th = ctx.include('src/map/skill.t.hpp')

    ## primitive types
    char = NeutralType('char')
    bit = NeutralType('bool')

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
    Option = enums_h.native('Option')
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

    TimeT = utils_h.native('TimeT')

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

    VERSION_2 = login_th.native('VERSION_2')


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
    option = ctx.enum(Option, u16)
    epos = ctx.enum(EPOS, u16)
    item_look = ctx.enum(ItemLook, u16)

    species = ctx.wrap(Species, u16)
    account_id = ctx.wrap(AccountId, u32)
    char_id = ctx.wrap(CharId, u32)
    party_id = ctx.wrap(PartyId, u32)
    item_name_id = ctx.wrap(ItemNameId, u16)
    item_name_id4 = ctx.wrap(ItemNameId, u32)
    block_id = ctx.wrap(BlockId, u32)

    time32 = ctx.provided(TimeT, Little32)
    time64 = ctx.provided(TimeT, Little64)

    gm1 = ctx.provided(GmLevel, Byte)
    gm2 = ctx.provided(GmLevel, Little16)
    gm = ctx.provided(GmLevel, Little32)

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

    version_2 = ctx.enum(VERSION_2, u8)

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
                at(105, u8, 'unused2'),
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
    # The element type of an array *may* be of explicit size, in which
    #
    # A string is just an array of characters, except that it may be padded
    # with '\0' bytes even when it is sized.
    # A map is just an array of two-element structs (key, value)
    # However, strings and maps have custom classes used to represent them
    # on the sender and receiver (earray also has this).
    #
    # It would probably be a good idea if everybody parsed network input as
    # padded with '\0's if it is too short, and ignored the extra if it is
    # too long. However, there are efficiency concerns with this, since we
    # don't want to branch everywhere.

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

    # * user
    char_user.r(0x0061, 'change password request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_pass, 'old pass'),
            at(26, account_pass, 'new pass'),
        ],
        fixed_size=50,
        pre=[],
        post=[0x2740],
        desc='''
            Sent by a client to the character server to request a password change.
        ''',
    )
    char_user.s(0x0062, 'change password response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'status'),
        ],
        fixed_size=3,
        pre=[0x2741],
        post=[],
        desc='''
            Sent by the character server with the response of a password change request.

            Status:
                0: The account was not found.
                1: Success.
                2: The old password was incorrect.
                3: The new password was too short.
        ''',
    )
    login_user.r(0x0063, 'update host',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
        pre=[0x0064],
        post=[],
        desc='''
            This packet gives the client the location of the update server URL, such as http://tmwdata.org/updates/

            It is only sent if an update host is specified for the server (there is one in the default configuration) and the client identifies as accepting an update host (which all supported clients do).
        ''',
    )
    login_user.r(0x0064, 'login request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'unknown'),
            at(6, account_name, 'account name'),
            at(30, account_pass, 'account pass'),
            at(54, version_2, 'version 2 flags'),
        ],
        fixed_size=55,
        pre=[0x7531],
        post=[0x006a, 0x0081, 0x0063, 0x0069],
        desc='''
            Registers login credentials.

            All clients must now set both defined version 2 flags.
        ''',
    )
    char_user.r(0x0065, 'tmwa-char connection request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'login id1'),
            at(10, u32, 'login id2'),
            at(14, u16, 'packet tmw version'),
            at(16, sex, 'sex'),
        ],
        fixed_size=17,
        pre=[],
        post=[0x8000, 0x2716, 0x006c, 0x2712, 0x006b],
        desc='''
            Connect request from client.
        ''',
    )
    char_user.r(0x0066, 'select character request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
        pre=[],
        post=[0x0081, 0x0071],
        desc='''
            Request from cilent for character location and map server IP.
        ''',
    )
    char_user.r(0x0067, 'create character request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'char name'),
            at(26, stats6, 'stats'),
            at(32, u8, 'slot'),
            at(33, u16, 'hair color'),
            at(35, u16, 'hair style'),
        ],
        fixed_size=37,
        pre=[],
        post=[0x006d],
        desc='''
            Request from client to create a character.
        ''',
    )
    char_user.r(0x0068, 'delete character request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, account_email, 'email'),
        ],
        fixed_size=46,
        pre=[],
        post=[0x006f, 0x0070, 0x2afe],
        desc='''
            Request from client to delete a character.
        ''',
    )
    login_user.r(0x0069, 'login data',
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
        post=[],
        desc='''
            
        ''',
    )
    login_user.s(0x006a, 'login error',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'error code'),
            at(3, seconds, 'error message'),
        ],
        fixed_size=23,
        pre=[0x0064],
        post=[],
        desc='''
            
        ''',
    )
    char_user.s(0x006b, 'update character list',
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
        post=[],
        desc='''
            Send list of characters to client.
        ''',
    )
    char_user.s(0x006c, 'login error',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
        pre=[0x0065, 0x2713],
        post=[],
        desc='''
            Refuse connection.
            
            Status:
                0: Overpopulated
                0x42: Auth failed
        ''',
    )
    char_user.s(0x006d, 'create character succeeded',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_select, 'char select'),
        ],
        fixed_size=108,
        pre=[0x0067],
        post=[],
        desc='''
            Send new character information to client.
        ''',
    )
    char_user.s(0x006e, 'create character failed',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
    )
    char_user.s(0x006f, 'delete character succeeded',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[0x0068],
        post=[],
        desc='''
            Send deletion success to client.
        ''',
    )
    char_user.s(0x0070, 'delete character failed',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
        pre=[0x0068],
        post=[],
        desc='''
            Send deletion failure to client.
        ''',
    )
    char_user.s(0x0071, 'char-map info',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, map_name, 'map name'),
            at(22, ip4, 'ip'),
            at(26, u16, 'port'),
        ],
        fixed_size=28,
        pre=[0x0066],
        post=[],
        desc='''
            Send character location and IP to client.
        ''',
    )
    map_user.r(0x0072, 'tmwa-map connect',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_id, 'char id'),
            at(10, u32, 'login id1'),
            at(14, u32, 'client tick'),
            at(18, sex, 'sex'),
        ],
        fixed_size=19,
    )
    map_user.s(0x0073, 'map login succeeded',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, tick32, 'tick'),
            at(6, pos1, 'pos'),
            at(9, u8, 'five1'),
            at(10, u8, 'five2'),
        ],
        fixed_size=11,
    )
    map_user.s(0x0078, 'being visibility',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, interval16, 'speed'),
            at(8, opt1, 'opt1'),
            at(10, opt2, 'opt2'),
            at(12, option, 'option'),
            at(14, species, 'species'),
            at(16, u16, 'unused hair style'),
            at(18, u16, 'unused weapon'),
            at(20, u16, 'unused head bottom or species again'),
            at(22, u16, 'unused shield or part of guild emblem'),
            at(24, u16, 'unused head top or unused part of guild emblem'),
            at(26, u16, 'unused head mid or part of guild id'),
            at(28, u16, 'unused hair color or part of guild id'),
            at(30, u16, 'unused clothes color'),
            at(32, u16, 'unused 1'),
            at(34, u16, 'unused 2'),
            at(36, pos1, 'unused pos again'),
            at(39, u8, 'unused 4b'),
            at(40, u16, 'unused 5'),
            at(42, u16, 'unused zero 1'),
            at(44, u8, 'unused zero 2'),
            at(45, u8, 'unused sex'),
            at(46, pos1, 'pos'),
            at(49, u8, 'five1'),
            at(50, u8, 'five2'),
            at(51, u8, 'zero'),
            at(52, u16, 'level'),
        ],
        fixed_size=54,
    )
    map_user.s(0x007b, 'being move',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, interval16, 'speed'),
            at(8, opt1, 'opt1'),
            at(10, opt2, 'opt2'),
            at(12, option, 'option'),
            at(14, species, 'mob class'),
            at(16, u16, 'unused hair style'),
            at(18, u16, 'unused weapon'),
            at(20, u16, 'unused head bottom'),
            at(22, tick32, 'tick and maybe part of guild emblem'),
            at(26, u16, 'unused shield or maybe part of guild emblem'),
            at(28, u16, 'unused head top or maybe part of guild id'),
            at(30, u16, 'unused head mid or maybe part of guild id'),
            at(32, u16, 'unused hair color'),
            at(34, u16, 'unused clothes color'),
            at(36, u16, 'unused 1'),
            at(38, u16, 'unused 2'),
            at(40, u16, 'unused 3'),
            at(42, u16, 'unused 4'),
            at(44, u16, 'unused 5'),
            at(46, u16, 'unused zero 1'),
            at(48, u8, 'unused zero 2'),
            at(49, u8, 'unused sex'),
            at(50, pos2, 'pos2'),
            at(55, u8, 'zero'),
            at(56, u8, 'five1'),
            at(57, u8, 'five2'),
            at(58, u16, 'level'),
        ],
        fixed_size=60,
    )
    map_user.s(0x007c, 'being spawn',
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
    )
    map_user.r(0x007d, 'map loaded',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.r(0x007e, 'client ping',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'client tick'),
        ],
        fixed_size=6,
    )
    map_user.s(0x007f, 'server ping',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, tick32, 'tick'),
        ],
        fixed_size=6,
    )
    map_user.s(0x0080, 'remove being',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, being_remove_why, 'type'),
        ],
        fixed_size=7,
    )
    any_user.s(0x0081, 'connection problem',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'error code'),
        ],
        fixed_size=3,
        pre=[0x0066, 0x2afe, 0x0064],
        post=[],
        desc='''
            
        ''',
    )
    map_user.r(0x0085, 'change player destination',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, pos1, 'pos'),
        ],
        fixed_size=5,
    )
    map_user.s(0x0087, 'walk response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, tick32, 'tick'),
            at(6, pos2, 'pos2'),
            at(11, u8, 'zero'),
        ],
        fixed_size=12,
    )
    map_user.s(0x0088, 'player stop',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u16, 'x'),
            at(8, u16, 'y'),
        ],
        fixed_size=10,
    )
    map_user.r(0x0089, 'player action',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'target id'),
            at(6, damage_type, 'action'),
        ],
        fixed_size=7,
    )
    map_user.s(0x008a, 'being action',
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
    )
    map_user.r(0x008c, 'character chat',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
    )
    map_user.s(0x008d, 'being chat',
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
    )
    map_user.s(0x008e, 'player chat',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
    )
    map_user.r(0x0090, 'chat to npc',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u8, 'unused'),
        ],
        fixed_size=7,
    )
    map_user.s(0x0091, 'warp player',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, map_name, 'map name'),
            at(18, u16, 'x'),
            at(20, u16, 'y'),
        ],
        fixed_size=22,
    )
    map_user.s(0x0092, 'change map server',
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
        post=[],
        desc='''
            Send notification of map server change to client.
        ''',
    )
    map_user.r(0x0094, 'request being name',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x0095, 'being name response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, char_name, 'char name'),
        ],
        fixed_size=30,
    )
    map_user.r(0x0096, 'send whisper',
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
    )
    map_user.s(0x0097, 'receive whisper',
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
    )
    map_user.s(0x0098, 'whisper status',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'flag'),
        ],
        fixed_size=3,
        pre=[0x3802],
        post=[],
        desc='''
            Send Wisp/Page result to client.
        ''',
    )
    map_user.s(0x009a, 'gm announcement',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x3800],
        post=[],
        desc='''
            Broadcast message.
        ''',
    )
    map_user.r(0x009b, 'change player direction',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'unused'),
            at(4, u8, 'client dir'),
        ],
        fixed_size=5,
    )
    map_user.s(0x009c, 'being changed direction',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u16, 'zero'),
            at(8, u8, 'client dir'),
        ],
        fixed_size=9,
    )
    map_user.s(0x009d, 'visible item',
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
    )
    map_user.s(0x009e, 'dropped item',
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
    )
    map_user.r(0x009f, 'pickup item',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'object id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00a0, 'add item to inventory',
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
    )
    map_user.s(0x00a1, 'item removed',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x00a2, 'drop an item',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u16, 'amount'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00a4, 'player equipment',
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
    )
    map_user.s(0x00a6, 'storage equipment',
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
    )
    map_user.r(0x00a7, 'use inventory item',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u32, 'unused id'),
        ],
        fixed_size=8,
    )
    map_user.s(0x00a8, 'item usage response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u16, 'amount'),
            at(6, u8, 'ok'),
        ],
        fixed_size=7,
    )
    map_user.r(0x00a9, 'equip an item request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, epos, 'epos ignored'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00aa, 'item equip ack',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, epos, 'epos'),
            at(6, u8, 'ok'),
        ],
        fixed_size=7,
    )
    map_user.r(0x00ab, 'unequip an item',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
        ],
        fixed_size=4,
    )
    map_user.s(0x00ac, 'unequip item ack',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, epos, 'epos'),
            at(6, u8, 'ok'),
        ],
        fixed_size=7,
    )
    map_user.s(0x00af, 'remove item from inventory',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u16, 'amount'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00b0, 'player stat update 1',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u32, 'value'),
        ],
        fixed_size=8,
    )
    map_user.s(0x00b1, 'player stat update 2',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u32, 'value'),
        ],
        fixed_size=8,
    )
    map_user.r(0x00b2, 'switch or respawn the character',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'flag'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00b3, 'character switch response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'one'),
        ],
        fixed_size=3,
        pre=[0x0x2b03],
        post=[],
        desc='''
            Send character select "OK" to client.
        ''',
    )
    map_user.s(0x00b4, 'npc message',
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
    )
    map_user.s(0x00b5, 'npc message continues',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00b6, 'npc message ends',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00b7, 'npc prompts a choice',
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
    )
    map_user.r(0x00b8, 'send npc response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'npc id'),
            at(6, u8, 'menu entry'),
        ],
        fixed_size=7,
    )
    map_user.r(0x00b9, 'request next npc message',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'npc id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x00bb, 'request a stat update',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'asp'),
            at(4, u8, 'unused'),
        ],
        fixed_size=5,
    )
    map_user.s(0x00bc, 'player stat update 4',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u8, 'ok'),
            at(5, u8, 'val'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00bd, 'player stat update 5',
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
    )
    map_user.s(0x00be, 'player stat update 6',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u8, 'value'),
        ],
        fixed_size=5,
    )
    map_user.r(0x00bf, 'show an emote',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'emote'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00c0, 'show the emote of a being',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u8, 'type'),
        ],
        fixed_size=7,
    )
    map_user.r(0x00c1, 'request online users (unused)',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x00c2, 'online users response (unused)',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'users'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00c4, 'npc shop choice',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x00c5, 'npc shop request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u8, 'type'),
        ],
        fixed_size=7,
    )
    map_user.s(0x00c6, 'npc buy prompt',
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
    )
    map_user.s(0x00c7, 'npc sell prompt',
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
    )
    map_user.r(0x00c8, 'npc buy request',
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
    )
    map_user.r(0x00c9, 'npc sell request',
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
    )
    map_user.s(0x00ca, 'npc buy response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00cb, 'npc sell response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00cd, 'kick status',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x00e4, 'trade request request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00e5, 'incoming trade request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'char name'),
        ],
        fixed_size=26,
    )
    map_user.r(0x00e6, 'incoming trade request response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'type'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00e7, 'trade request response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'type'),
        ],
        fixed_size=3,
    )
    map_user.r(0x00e8, 'trade item add request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'zeny or ioff2'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
    )
    map_user.s(0x00e9, 'trade item add',
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
    )
    map_user.r(0x00eb, 'trade add complete',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x00ec, 'trade ok',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
    )
    map_user.r(0x00ed, 'trace cancel request',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x00ee, 'trade cancel',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.r(0x00ef, 'trade ok request',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x00f0, 'trade complete',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00f2, 'storage status',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'current slots'),
            at(4, u16, 'max slots'),
        ],
        fixed_size=6,
    )
    map_user.r(0x00f3, 'move item to storage request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
    )
    map_user.s(0x00f4, 'move item to storage',
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
    )
    map_user.r(0x00f5, 'move item from storage request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, soff1, 'soff1'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
    )
    map_user.s(0x00f6, 'remove item from storage',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, soff1, 'soff1'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
    )
    map_user.r(0x00f7, 'storage close request',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x00f8, 'storage closed',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.r(0x00f9, 'create party request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_name, 'party name'),
        ],
        fixed_size=26,
    )
    map_user.s(0x00fa, 'create party',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'flag'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00fb, 'party info',
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
    )
    map_user.r(0x00fc, 'party invite request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00fd, 'party invite response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'char name'),
            at(26, u8, 'flag'),
        ],
        fixed_size=27,
    )
    map_user.s(0x00fe, 'party invite succeeded',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, party_name, 'party name'),
        ],
        fixed_size=30,
    )
    map_user.r(0x00ff, 'party join request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'flag'),
        ],
        fixed_size=10,
    )
    map_user.r(0x0100, 'party leave request',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x0101, 'party settings',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'exp'),
            at(4, u16, 'item'),
        ],
        fixed_size=6,
    )
    map_user.r(0x0102, 'party settings request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'exp'),
            at(4, u16, 'item'),
        ],
        fixed_size=6,
    )
    map_user.r(0x0103, 'party kick request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'unused char name'),
        ],
        fixed_size=30,
    )
    map_user.s(0x0105, 'party leave',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'char name'),
            at(30, u8, 'flag'),
        ],
        fixed_size=31,
    )
    map_user.s(0x0106, 'update party member hp',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u16, 'hp'),
            at(8, u16, 'max hp'),
        ],
        fixed_size=10,
    )
    map_user.s(0x0107, 'update party member coords',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u16, 'x'),
            at(8, u16, 'y'),
        ],
        fixed_size=10,
    )
    map_user.r(0x0108, 'party message request',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
    )
    map_user.s(0x0109, 'party message',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1
    )
    map_user.s(0x010c, 'MVP (unused)',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x010e, 'raise a skill',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, skill_id, 'skill id'),
            at(4, u16, 'level'),
            at(6, u16, 'sp'),
            at(8, u16, 'range'),
            at(10, u8, 'can raise'),
        ],
        fixed_size=11,
    )
    map_user.s(0x010f, 'player skills',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, skill_info, 'info'),
        ],
        repeat_size=37,
    )
    map_user.s(0x0110, 'skill failed',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, skill_id, 'skill id'),
            at(4, u16, 'btype'),
            at(6, u16, 'zero1'),
            at(8, u8, 'zero2'),
            at(9, u8, 'type'),
        ],
        fixed_size=10,
    )
    map_user.r(0x0112, 'request a skill lvl up',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, skill_id, 'skill id'),
        ],
        fixed_size=4,
    )
    map_user.r(0x0118, 'stop attack request',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x0119, 'change player status',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, opt1, 'opt1'),
            at(8, opt2, 'opt2'),
            at(10, option, 'option'),
            at(12, u8, 'zero'),
        ],
        fixed_size=13,
    )
    map_user.s(0x0139, 'move player to within attack range',
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
    )
    map_user.s(0x013a, 'player attack range',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'attack range'),
        ],
        fixed_size=4,
    )
    map_user.s(0x013b, 'player arrow message',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'type'),
        ],
        fixed_size=4,
    )
    map_user.s(0x013c, 'player arrow equip',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
        ],
        fixed_size=4,
    )
    map_user.s(0x0141, 'player stat update 3',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u16, 'zero'),
            at(6, u32, 'value status'),
            at(10, u32, 'value b e'),
        ],
        fixed_size=14,
    )
    map_user.s(0x0142, 'npc integer input',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x0143, 'npc integer response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u32, 'input int value'),
        ],
        fixed_size=10,
    )
    map_user.r(0x0146, 'npc close request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x0147, 'single skill info (unused)',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, skill_info, 'info'),
        ],
        fixed_size=39,
    )
    map_user.s(0x0148, 'being resurrected',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u16, 'type'),
        ],
        fixed_size=8,
    )
    map_user.r(0x014d, 'guild check master (unused)',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.r(0x018a, 'client quit',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'unused'),
        ],
        fixed_size=4,
    )
    map_user.s(0x018b, 'map quit response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'okay'),
        ],
        fixed_size=4,
    )
    map_user.s(0x0195, 'guild party info (unused)',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, party_name, 'party name'),
            at(30, str24, 'guild name'),
            at(54, str24, 'guild pos'),
            at(78, str24, 'guild pos again'),
        ],
        fixed_size=102,
    )
    map_user.s(0x0196, 'being status change',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, status_change, 'sc type'),
            at(4, block_id, 'block id'),
            at(8, u8, 'flag'),
        ],
        fixed_size=9,
    )
    map_user.s(0x019b, 'being effect',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u32, 'type'),
        ],
        fixed_size=10,
    )
    map_user.s(0x01b1, 'trade item add response',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, u16, 'amount'),
            at(6, u8, 'fail'),
        ],
        fixed_size=7,
    )
    map_user.s(0x01c8, 'use inventory item succeeded',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, ioff2, 'ioff2'),
            at(4, item_name_id, 'name id'),
            at(6, block_id, 'block id'),
            at(10, u16, 'amount'),
            at(12, u8, 'ok'),
        ],
        fixed_size=13,
    )
    map_user.s(0x01d4, 'npc string input',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x01d5, 'npc string response',
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
    )
    map_user.s(0x01d7, 'change being appearance (unused)',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, look, 'look type'),
            at(7, u16, 'weapon or name id or value'),
            at(9, item_name_id, 'shield'),
        ],
        fixed_size=11,
    )
    # very similar to, but not compatible with, 0x01d9 and 0x01da
    map_user.s(0x01d8, 'player update 1',
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
    )
    # very similar to, but not compatible with, 0x01d8 and 0x01da
    map_user.s(0x01d9, 'player update 2',
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
    )
    # very similar to, but not compatible with, 0x01d8 and 0x01d9
    map_user.s(0x01da, 'player move',
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
    )
    map_user.s(0x01de, 'deal skill damage',
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
    )
    map_user.s(0x01ee, 'player inventory',
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
    )
    map_user.s(0x01f0, 'storage item list',
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
    )
    map_user.s(0x020c, 'set being ip',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, ip4, 'ip'),
        ],
        fixed_size=10,
    )
    map_user.s(0x0212, 'npc command',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'npc id'),
            at(6, u16, 'command'),
            at(8, block_id, 'id'),
            at(12, u16, 'x'),
            at(14, u16, 'y'),
        ],
        fixed_size=16,
    )

    # login char
    login_char.r(0x2709, 'reload gm accounts request',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[0x2af7],
        post=[0x2732],
   	    desc='''
            Request from tmwa-map via tmwa-char to reload GM accounts. (by Yor)
        ''',
    )
    login_char.r(0x2710, 'add char server request',
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
        pre=[],
        post=[0x2711],
        desc='''
            Tmwa-char connection request.
        ''',
    )
    login_char.s(0x2711, 'add char server result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
        pre=[0x2710],
        post=[],
        desc='''
            Tmwa-char connection result.
        ''',
    )
    login_char.r(0x2712, 'account auth request',
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
        post=[0x2729, 0x2713],
        desc='''
            Request from tmwa-char to authenticate account.
        ''',
    )
    login_char.s(0x2713, 'account auth result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'invalid'),
            at(7, account_email, 'email'),
            at(47, time32, 'connect until'),
        ],
        fixed_size=51,
        pre=[0x2712],
        post=[0x006c, 0x006b],
        desc='''
            Send account auth status to tmwa-char.
            
            Status:
                0: good
                1: bad
        ''',
    )
    login_char.r(0x2714, 'online count',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'users'),
        ],
        fixed_size=6,
        pre=[],
        post=[],
        desc='''
            Receive number of users on tmwa-map (every few seconds.)
        ''',
    )
    login_char.r(0x2716, 'email limit request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x0065],
        post=[0x2717],
        desc='''
            Request from tmwa-char to obtain e-mail/time limit.
        ''',
    )
    login_char.s(0x2717, 'email limit result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_email, 'email'),
            at(46, time32, 'connect until'),
        ],
        fixed_size=50,
        pre=[0x2716],
        post=[],
        desc='''
            Send e-mail/time limite to tmwa-char.
        ''',
    )
    # 0x2b0a
    login_char.r(0x2720, 'become gm request',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
        pre=[0x2b0a],
        post=[0x2721],
        desc='''
            Request from tmwa-map via tmwa-char to give GM status to an account.
        ''',
    )
    login_char.s(0x2721, 'become gm reply',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, gm, 'gm level'),
        ],
        fixed_size=10,
        pre=[0x2720],
        post=[0x2b0b],
        desc='''
            Response to tmwa-char of accounts new GM status.
        ''',
    )
    # 0x2b0c
    login_char.r(0x2722, 'account email change request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_email, 'old email'),
            at(46, account_email, 'new email'),
        ],
        fixed_size=86,
        pre=[0x2b0c],
        post=[],
        desc='''
            Request from tmwa-map via tmwa-char to change account email.
        ''',
    )
    login_char.s(0x2723, 'changesex reply',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, sex, 'sex'),
        ],
        fixed_size=7,
        pre=[0x272a],
        post=[0x2b0d],
        desc='''
            Response from tmwa-login about account gender swap.
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
            Request from tmwa-map via tmwa-char to block account.
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
            Request from tmwa-map via tmwa-char to ban account.
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
        desc="""
            Broadcast message to all map servers.
        """,
    )
    login_char.r(0x2727, 'change sex request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x2b0e],
        post=[],
        desc='''
            Request from tmwa-map via tmwa-char to swap account gender.
        ''',
    )
    # 0x2b10, 0x2b11
    for (id, cat) in [
        (0x2728, login_char.r),
        (0x2729, login_char.s),
    ]:
        cat(id, 'update account reg2',
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
        )
    login_char.r(0x272a, 'unban request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x2b0e],
        post=[0x2723],
        desc='''
            Request from tmwa-map via tmwa-char to unblock or unban and account.
        ''',
    )
    login_char.s(0x2730, 'account deleted',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x7932],
        post=[0x2b13],
        desc="""
            Account deletion notification.
        """,
    )
    login_char.s(0x2731, 'status or ban changed',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'ban not status'),
            at(7, time32, 'status or ban until'),
        ],
        fixed_size=11,
        pre=[0x2724, 0x2725, 0x794c, 0x794a],
        post=[0x2b14],
        desc='''
            Response from tmwa-login about account ban/block status.
        ''',
    )
    login_char.s(0x2732, 'gm account list',
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
        pre=[0x2709],
        post=[0x2b15],
        desc='''
            Send GM accounts to all character servers.
        ''',
    )
    login_char.r(0x2740, 'change password request',
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
            Change password request from tmwa-char.
        ''',
    )
    login_char.s(0x2741, 'change password reply',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'status'),
        ],
        fixed_size=7,
        pre=[0x2740],
        post=[0x0062],
        desc='''
            Password change response from tmwa-login.
        ''',
    )

    # char map
    char_map.r(0x2af7, 'reload gm db',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[],
        post=[0x2709],
        desc='''
            Request from tmwa-map to reload GM accounts.
            
            Transmission to tmwa-login. (by Yor)
        ''',
    )
    char_map.r(0x2af8, 'add map server request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'account pass'),
            at(50, u32, 'unused'),
            at(54, ip4, 'ip'),
            at(58, u16, 'port'),
        ],
        fixed_size=60,
        pre=[],
        post=[0x2af9, 0x2b15],
        desc='''
            Attempt to connect from tmwa-map.
        ''',
    )
    char_map.s(0x2af9, 'add map server result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
        pre=[0x2af8],
        post=[0x2afa],
        desc='''
            Acknowledgement to tmwa-map of connection.
        ''',
    )
    # wtf duplicate v
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
            Receive map names list from tmwa-map.
        ''',
    )
    # wtf duplicate ^
    char_map.s(0x2afa, 'itemfrob',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, item_name_id4, 'source item id'),
            at(6, item_name_id4, 'dest item id'),
        ],
        fixed_size=10,
    )
    char_map.s(0x2afb, 'map list ack',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'unknown'),
            at(3, char_name, 'whisper name'),
        ],
        fixed_size=27,
        pre=[0x2afa],
        post=[],
        desc='''
            Acknowledgement to tmwa-map that map names list was received.
        ''',
    )
    char_map.r(0x2afc, 'character auth request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_id, 'char id'),
            at(10, u32, 'login id1'),
            at(14, u32, 'login id2'),
            at(18, ip4, 'ip'),
        ],
        fixed_size=22,
        pre=[],
        post=[0x2afd, 0x2afe],
        desc='''
            Request from tmwa-map to authenticate an account.
        ''',
    )
    char_map.s(0x2afd, 'character auth and data',
        payload=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
            at(8, u32, 'login id2'),
            at(12, time32, 'connect until'),
            at(16, u16, 'packet tmw version'),
            at(18, char_key, 'char key'),
            at(None, char_data, 'char data'),
        ],
        payload_size=None,
        pre=[0x2afc],
        post=[0x3005],
        desc='''
            Send that account authentication succeeded.
        ''',
    )
    char_map.s(0x2afe, 'character auth error',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x0068, 0x2afc],
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
        pre=[],
        post=[],
        desc='''
            Receive list of users from tmwa-map.
        ''',
    )
    char_map.s(0x2b00, 'total users',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'users'),
        ],
        fixed_size=6,
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
        pre=[],
        post=[],
        desc='''
            Receive character save information from tmwa-map.
        ''',
    )
    char_map.r(0x2b02, 'char select req',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'login id1'),
            at(10, u32, 'login id2'),
            at(14, ip4, 'ip'),
        ],
        fixed_size=18,
        pre=[],
        post=[0x2b03],
        desc='''
            Receive character select information from tmwa-map.
        ''',
    )
    char_map.s(0x2b03, 'char select res',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'unknown'),
        ],
        fixed_size=7,
        pre=[0x2b02],
        post=[0x00b3],
        desc='''
            Send character select "OK" to tmwa-map.
        ''',
    )
    char_map.s(0x2b04, 'map list broadcast',
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
        post=[],
        desc='''
            Send map information to all map servers.
        ''',
    )
    char_map.r(0x2b05, 'change map server request',
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
        pre=[],
        post=[0x2b06],
        desc='''
            Request from tmwa-map to change map server.
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
        post=[0x0092],
        desc='''
            Send acknowledgement of map server change to tmwa-map. 
        ''',
    )
    # 0x2720
    char_map.r(0x2b0a, 'become gm request',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
        pre=[],
        post=[0x2720, 0x2b0b],
        desc='''
            Request from tmwa-map to give GM status to an account.
        ''',
    )
    char_map.s(0x2b0b, 'become gm result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, gm, 'gm level'),
        ],
        fixed_size=10,
        pre=[0x2b0a],
        post=[],
        desc='''
            Send notification of accounts GM level to tmwa-map.
        ''',
    )
    # 0x2722
    char_map.r(0x2b0c, 'change email request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_email, 'old email'),
            at(46, account_email, 'new email'),
        ],
        fixed_size=86,
        pre=[],
        post=[0x2722],
        desc='''
            Request from tmwa-map to change account email.
        ''',
    )
    char_map.s(0x2b0d, 'sex changed notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, sex, 'sex'),
        ],
        fixed_size=7,
        pre=[0x2723],
        post=[],
        desc='''
            Response from tmwa-login via tmwa-char about account gender swap.
        ''',
    )
    char_map.r(0x2b0e, 'named char operation request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'char name'),
            at(30, u16, 'operation'),
            at(32, human_time_diff, 'ban add'),
        ],
        fixed_size=44,
        pre=[],
        post=[0x2724, 0x2725, 0x272a, 0x2727, 0x2b0f],
        desc='''
            Request from tmwa-map to change account ban status or gender.
        ''',
    )
    char_map.r(0x2b0f, 'named char operation answer',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'char name'),
            at(30, u16, 'operation'),
            at(32, u16, 'error'),
        ],
        fixed_size=34,
        pre=[0x2b0e],
        post=[],
        desc='''
            Reqponse from tmwa-char about changing account ban/block status or gender.
        ''',
    )
    # 0x2728, 0x2729
    for (id, cat) in [
        (0x2b10, char_map.r),
        (0x2b11, char_map.s),
    ]:
        cat(id, 'account reg2 update',
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
        )
    char_map.s(0x2b12, 'divorce notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, char_id, 'partner id'),
        ],
        fixed_size=10,
        pre=[0x2b12],
        post=[],
        desc='''
            Send notification of character divorce status to tmwa-map.
        ''',
    )
    char_map.s(0x2b13, 'account delete notify',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[0x7930],
        post=[],
        desc="""
            Disconnect player due to deletion.
        """,
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
        post=[],
        desc='''
            Response from tmwa-login via tmwa-char about account ban/block status.
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
        post=[],
        desc='''
            Send GM accounts to all map servers.
        ''',
    )
    char_map.r(0x2b16, 'divorce request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
        ],
        fixed_size=6,
        pre=[],
        post=[0x2b12],
        desc='''
            Request from tmwa-map to divorce a character.
        ''',
    )

    char_map.r(0x3000, 'gm broadcast',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[],
        post=[0x3800],
        desc='''
            Receive message for all GMs from tmwa-map.
        ''',
    )
    char_map.r(0x3001, 'whisper forward',
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
        pre=[0x3001],
        post=[0x3802],
        desc='''
            Receive Wisp/Page from tmwa-map to retransmit.
        ''',
    )
    char_map.r(0x3002, 'whisper forward result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, u8, 'flag'),
        ],
        fixed_size=7,
    )
    # 0x3803
    char_map.r(0x3003, 'wgm forward',
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
    )
    # 0x3804
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
            Request accountreg from tmwa-char.
        ''',
    )
    char_map.r(0x3010, 'want storage',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    char_map.r(0x3011, 'got storage',
        payload=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
            at(8, storage, 'storage'),
        ],
        payload_size=None,
    )
    char_map.r(0x3020, 'create party',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, party_name, 'party name'),
            at(30, char_name, 'char name'),
            at(54, map_name, 'map name'),
            at(70, u16, 'level'),
        ],
        fixed_size=72,
    )
    char_map.r(0x3021, 'request party info',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
        ],
        fixed_size=6,
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
    )
    char_map.r(0x3024, 'party leave',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
        ],
        fixed_size=10,
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
    )
    char_map.r(0x3027, 'party message',
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
    )
    char_map.r(0x3028, 'party check conflict',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, char_name, 'char name'),
        ],
        fixed_size=34,
    )

    char_map.s(0x3800, 'gm broadcast',
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x3800, 0x2726],
        post=[0x009a],
        desc='''
            Broadcast message.
        ''',
    )
    char_map.s(0x3801, 'whisper forward',
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
    )
    char_map.s(0x3802, 'whisper result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'sender char name'),
            at(26, u8, 'flag'),
        ],
        fixed_size=27,
        pre=[0x3001],
        post=[],
        desc='''
            Send Wisp/Page result to tmwa-char.
        ''',
    )
    # 0x3003
    char_map.s(0x3803, 'whisper gm',
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
    )
    # 0x3004
    char_map.s(0x3804, 'broadcast account reg',
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
        pre=[0x3005],
        post=[],
        desc='''
            Send account reg status to tmwa-map.
        ''',
    )
    char_map.s(0x3810, 'load storage',
        payload=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
            at(8, storage, 'storage'),
        ],
        payload_size=None,
    )
    char_map.s(0x3811, 'save storage ack',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'unknown'),
        ],
        fixed_size=7,
    )
    char_map.s(0x3820, 'party created',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'error'),
            at(7, party_id, 'party id'),
            at(11, party_name, 'party name'),
        ],
        fixed_size=35,
    )
    char_map.s(0x3821, 'party info maybe',
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
    )
    char_map.s(0x3822, 'party member added',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, u8, 'flag'),
        ],
        fixed_size=11,
    )
    char_map.s(0x3823, 'party option changed',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, u16, 'exp'),
            at(12, u16, 'item'),
            at(14, u8, 'flag'),
        ],
        fixed_size=15,
    )
    char_map.s(0x3824, 'party member left',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, char_name, 'char name'),
        ],
        fixed_size=34,
    )
    char_map.s(0x3825, 'party member moved',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, map_name, 'map name'),
            at(26, u8, 'online'),
            at(27, u16, 'level'),
        ],
        fixed_size=29,
    )
    char_map.s(0x3826, 'party broken',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, u8, 'flag'),
        ],
        fixed_size=7,
    )
    char_map.s(0x3827, 'party message',
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
    )

    # any client
    any_user.r(0x7530, 'version request',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[],
        post=[0x7531],
        desc='''
            Request from client or ladmin for server version.
        ''',
    )
    any_user.s(0x7531, 'version reply',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, version, 'version'),
        ],
        fixed_size=10,
        pre=[0x7530],
        post=[],
        desc='''
            Response to client's request for server version.
        ''',
    )
    any_user.r(0x7532, 'End of connection',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[],
        post=[],
        desc='''
            Request from client or ladmin to disconnect.
        ''',
    )

    # login admin
    login_admin.r(0x7918, 'admin auth request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'encryption zero'),
            at(4, account_pass, 'account pass'),
        ],
        fixed_size=28,
        pre=[],
        post=[0x7919],
        desc='''
            ladmin connection request.
        ''',
    )
    login_admin.s(0x7919, 'admin auth result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'error'),
        ],
        fixed_size=3,
        pre=[0x7918],
        post=[],
        desc='''
            ladmin connection response.
        ''',
    )
    login_admin.r(0x7920, 'account list request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'start account id'),
            at(6, account_id, 'end account id'),
        ],
        fixed_size=10,
        pre=[],
        post=[0x7921],
        desc="""
            Request accounts list.
        """,
    )
    login_admin.s(0x7921, 'account list reply',
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
        post=[],
        desc="""
            Account list response.
        """,
    )
    login_admin.r(0x7924, 'itemfrob',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, item_name_id4, 'source item id'),
            at(6, item_name_id4, 'dest item id'),
        ],
        fixed_size=10,
        pre=[],
        post=[0x7925],
        desc="""
            Frobnicate item.
        """,
    )
    login_admin.s(0x7925, 'itemfrob ok',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[0x7924],
        post=[],
        desc="""
            Frobnicate OK.
        """,
    )
    login_admin.r(0x7930, 'account create request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'password'),
            at(50, sex_char, 'sex'),
            at(51, account_email, 'email'),
        ],
        fixed_size=91,
        pre=[],
        post=[0x7931],
        desc="""
            Account creation request.
        """,
    )
    login_admin.s(0x7931, 'account create result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7930, 0x7936],
        post=[0x2b14],
        desc="""
            Account creation response.
        """,
    )
    login_admin.r(0x7932, 'account delete request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
        ],
        fixed_size=26,
        pre=[],
        post=[0x7933, 0x2730],
        desc="""
            Account deletion request.
        """,
    )
    login_admin.s(0x7933, 'account delete reply',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7932],
        post=[],
        desc="""
            Account deletion response.
        """,
    )
    login_admin.r(0x7934, 'password change request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'password'),
        ],
        fixed_size=50,
        pre=[],
        post=[0x7935],
        desc="""
            Change password request.
        """,
    )
    login_admin.s(0x7935, 'password change result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7934],
        post=[],
        desc="""
            Change password response.
        """,
    )
    login_admin.r(0x7936, 'account state change request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, u32, 'status'),
            at(30, seconds, 'error message'),
        ],
        fixed_size=50,
        pre=[],
        post=[0x7937, 0x2731],
        desc="""
            Account state change request.
        """,
    )
    login_admin.s(0x7937, 'account state change result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
            at(30, u32, 'status'),
        ],
        fixed_size=34,
        pre=[],
        post=[],
        desc="""
            Account state change response.
        """,
    )
    login_admin.r(0x7938, 'server list request',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[],
        post=[0x7939],
        desc="""
            Server list and player count request.
        """,
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
        post=[],
        desc="""
            Server list and player count response.
        """,
    )
    login_admin.r(0x793a, 'password check request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'password'),
        ],
        fixed_size=50,
        pre=[],
        post=[0x793b],
        desc="""
            Password check request.
        """,
    )
    login_admin.s(0x793b, 'password check result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x793a],
        post=[],
        desc="""
            Password check response.
        """,
    )
    login_admin.r(0x793c, 'change sex request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, sex_char, 'sex'),
        ],
        fixed_size=27,
        pre=[],
        post=[0x793d],
        desc="""
            Modify sex request.
        """,
    )
    login_admin.s(0x793d, 'change sex result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x793c],
        post=[],
        desc="""
            Modify sex response.
        """,
    )
    login_admin.r(0x793e, 'adjust gm level request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, gm1, 'gm level'),
        ],
        fixed_size=27,
        pre=[],
        post=[0x793f],
        desc="""
            Modify GM level request.
        """,
    )
    login_admin.s(0x793f, 'adjust gm level result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x793e],
        post=[],
        desc="""
            Modify GM level response.
        """,
    )
    login_admin.r(0x7940, 'change email request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_email, 'email'),
        ],
        fixed_size=66,
        pre=[],
        post=[0x7941],
        desc="""
            Modify e-mail request.
        """,
    )
    login_admin.s(0x7941, 'change email result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7940],
        post=[],
        desc="""
            Modify e-mail response.
        """,
    )
    # this packet is insane
    login_admin.r(0x7942, 'change memo request',
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
        pre=[],
        post=[0x7943],
        desc="""
            Modify memo request.
        """,
    )
    login_admin.s(0x7943, 'change memo result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7942],
        post=[],
        desc="""
            Modify memo response.
        """,
    )
    login_admin.r(0x7944, 'account id lookup request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
        ],
        fixed_size=26,
        pre=[],
        post=[0x7945],
        desc="""
            Find account id request.
        """,
    )
    login_admin.s(0x7945, 'account id lookup result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7944],
        post=[],
        desc="""
            Find account id response.
        """,
    )
    login_admin.r(0x7946, 'account name lookup request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[],
        post=[0x7947],
        desc="""
            Find account name request.
        """,
    )
    login_admin.s(0x7947, 'account name lookup result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
        pre=[0x7946],
        post=[],
        desc="""
            Find account name response.
        """,
    )
    login_admin.r(0x7948, 'validity absolute request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, time32, 'valid until'),
        ],
        fixed_size=30,
        pre=[],
        post=[0x7949],
        desc="""
            Validity limit change request.
        """,
    )
    login_admin.s(0x7949, 'validity absolute result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
            at(30, time32, 'valid until'),
        ],
        fixed_size=34,
        pre=[0x7948],
        post=[],
        desc="""
            Validity limit change response.
        """,
    )
    login_admin.r(0x794a, 'ban absolute request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, time32, 'ban until'),
        ],
        fixed_size=30,
        pre=[],
        post=[0x794b, 0x2731],
        desc="""
            Ban date end change request.
        """,
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
        post=[],
        desc="""
            Ban date end change response.
        """,
    )
    login_admin.r(0x794c, 'ban relative request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, human_time_diff, 'ban add'),
        ],
        fixed_size=38,
        pre=[],
        post=[0x794d, 0x2731],
        desc="""
            Ban date end change request (2).
        """,
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
        post=[],
        desc="""
            Ban date end change response (2).
        """,
    )
    # evil packet (see also 0x2726)
    login_admin.r(0x794e, 'broadcast message request',
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
        pre=[],
        post=[0x794f, 0x2726],
        desc="""
            Send broadcast message request.
        """,
    )
    login_admin.s(0x794f, 'broadcast message result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'error'),
        ],
        fixed_size=4,
        pre=[0x794e],
        post=[],
        desc="""
            Send broadcast message response.
        """,
    )
    login_admin.r(0x7950, 'validity relative request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, human_time_diff, 'valid add'),
        ],
        fixed_size=38,
        pre=[],
        post=[0x7951],
        desc="""
            Relative validity ilmit change request.
        """,
    )
    login_admin.s(0x7951, 'validity relative result',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
            at(30, time32, 'valid until'),
        ],
        fixed_size=34,
        pre=[0x7950],
        post=[],
        desc="""
            Relative validity limit change response.
        """,
    )
    login_admin.r(0x7952, 'account name info request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
        ],
        fixed_size=26,
        pre=[],
        post=[0x7953],
        desc="""
            Account information by name request.
        """,
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
            at(140, time32, 'connect until'),
            at(144, time32, 'ban until'),
            at(148, SkewLengthType(u16, 150), 'magic packet length'),
        ],
        head_size=150,
        repeat=[
            at(0, u8, 'c'),
        ],
        repeat_size=1,
        pre=[0x7952, 0x7954],
        post=[],
        desc="""
            Account information by name or id response.
        """,
    )
    login_admin.r(0x7954, 'account id info request',
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
        pre=[],
        post=[0x7953],
        desc="""
            Account information by id request.
        """,
    )
    login_admin.r(0x7955, 'reload gm signal',
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
        pre=[],
        post=[],
        desc="""
            Reload GM file request.
        """,
    )

    ## new-style packets
    # general packets
    any_user.x(0x8000, 'special hold',
        payload=[
            at(0, u16, 'packet id'),
            # packet 0x8000 is handled specially
            at(2, u16, 'packet length'),
        ],
        payload_size=4,
        pre=[0x0065],
        post=[],
        desc='''
            
        ''',
    )

    ## teardown
    ctx.dump()

if __name__ == '__main__':
    main()
