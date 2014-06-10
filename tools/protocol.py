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
    __slots__ = ('id', 'name', 'fields', 'size')

    def __init__(self, id, name, fields, size):
        self.id = id
        self.name = name
        self.fields = fields
        self.size = size

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
        with open(os.path.join(outdir, filename), 'w') as f:
            f.write(self.pp(0))
            f.write(copyright.format(filename=filename, description=desc))
            f.write('\n')
            f.write('#include "%s"\n\n' % poison)

            for t in self._types:
                f.write('using %s = %s;\n' % ('Test_' + ident(t.name), t.name))

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
    __slots__ = ('fixed_struct')

    def __init__(self, fixed_struct):
        self.fixed_struct = fixed_struct

    def dump_fwd(self, fwd):
        self.fixed_struct.dump_fwd(fwd)
        fwd.write('\n')

    def dump_native(self, f):
        self.fixed_struct.dump_native(f)
        f.write('\n')

    def dump_network(self, f):
        self.fixed_struct.dump_network(f)
        f.write('\n')

    def dump_convert(self, f):
        self.fixed_struct.dump_convert(f)
        f.write('\n')

class VarPacket(object):
    __slots__ = ('head_struct', 'repeat_struct')

    def __init__(self, head_struct, repeat_struct):
        self.head_struct = head_struct
        self.repeat_struct = repeat_struct

    def dump_fwd(self, fwd):
        self.head_struct.dump_fwd(fwd)
        self.repeat_struct.dump_fwd(fwd)
        fwd.write('\n')

    def dump_native(self, f):
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

def packet(id,
        fixed=None, fixed_size=None,
        payload=None, payload_size=None,
        head=None, head_size=None,
        repeat=None, repeat_size=None,
        option=None, option_size=None,
):
    assert (fixed is None) <= (fixed_size is None)
    assert (payload is None) <= (payload_size is None)
    assert (head is None) <= (head_size is None)
    assert (repeat is None) <= (repeat_size is None)
    assert (option is None) <= (option_size is None)

    if fixed is not None:
        assert not head and not repeat and not option and not payload
        return FixedPacket(
                StructType(id, 'Packet_Fixed<0x%04x>' % id, fixed, fixed_size))
    elif payload is not None:
        assert not head and not repeat and not option
        return FixedPacket(
                StructType(id, 'Packet_Payload<0x%04x>' % id, payload, payload_size))
    else:
        assert head
        if option:
            return VarPacket(
                    StructType(id, 'Packet_Head<0x%04x>' % id, head, head_size),
                    StructType(id, 'Packet_Option<0x%04x>' % id, option, option_size))
        else:
            assert repeat
            return VarPacket(
                    StructType(id, 'Packet_Head<0x%04x>' % id, head, head_size),
                    StructType(id, 'Packet_Repeat<0x%04x>' % id, repeat, repeat_size))


class Channel(object):
    __slots__ = ('server', 'client', 'packets')

    def __init__(self, server, client):
        self.server = server
        self.client = client
        self.packets = []

    def x(self, id, **kwargs):
        self.packets.append(packet(id, **kwargs))
    r = x
    s = x

    def dump(self, outdir, fwd):
        server = self.server
        client = self.client
        header = '%s-%s.hpp' % (server, client)
        test = '%s-%s_test.cpp' % (server, client)
        desc = 'TMWA network protocol: %s/%s' % (server, client)
        with open(os.path.join(outdir, header), 'w') as f:
            proto2 = relpath(outdir, 'src')
            define = ('TMWA_%s_%s_%s_HPP' % (proto2, server, client)).upper()
            f.write('#ifndef %s\n' % define)
            f.write('#define %s\n' % define)
            f.write(copyright.format(filename=header, description=desc))
            f.write('\n')
            f.write(generated)
            f.write('\n')
            f.write('# include "fwd.hpp"\n\n')
            f.write('# include "types.hpp"\n')
            f.write('\n')
            if client == 'user':
                f.write('// This is a public protocol, and changes require client cooperation\n')
            else:
                f.write('// This is an internal protocol, and can be changed without notice\n')
            f.write('\n')
            f.write('// this is only needed for the payload packet right now, and that needs to die\n')
            f.write('# pragma pack(push, 1)\n')
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
            f.write('\n')
            f.write('# pragma pack(pop)\n')
            f.write('\n')
            f.write('#endif // %s\n' % define)

        with open(os.path.join(outdir, test), 'w') as f:
            poison = relpath('src/poison.hpp', outdir)
            f.write('#include "%s"\n' % header)
            f.write(copyright.format(filename=test, description=desc))
            f.write('\n')
            f.write(generated)
            f.write('\n')
            f.write('#include "{}"\n'.format(poison))


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
            os.remove(g)
        proto2 = relpath(outdir, 'src')
        with open(os.path.join(outdir, 'fwd.hpp'), 'w') as f:
            header = '%s/fwd.hpp' % proto2
            desc = 'Forward declarations of network packets'
            sanity = relpath('src/sanity.hpp', outdir)
            define = ('TMWA_%s_FWD_HPP' % proto2).upper()
            f.write('#ifndef %s\n' % define)
            f.write('#define %s\n' % define)
            f.write(copyright.format(filename=header, description=desc))
            f.write('\n')
            f.write('# include "%s"\n\n' % sanity)
            f.write('# include <cstdint>\n\n')
            for b in ['Fixed', 'Payload', 'Head', 'Repeat', 'Option']:
                c = 'Packet_' + b
                f.write('template<uint16_t PACKET_ID> class %s;\n' % c)
                f.write('template<uint16_t PACKET_ID> class Net%s;\n' % c)
            f.write('\n')

            for ch in self._channels:
                ch.dump(outdir, f)

            f.write('\n')
            f.write('#endif // %s\n' % define)

        with open(os.path.join(outdir, 'types.hpp'), 'w') as f:
            header = '%s/types.hpp' % proto2
            desc = 'Forward declarations of packet component types'
            define = ('TMWA_%s_TYPES_HPP' % proto2).upper()
            f.write('#ifndef %s\n' % define)
            f.write('#define %s\n' % define)
            f.write(copyright.format(filename=header, description=desc))
            f.write('\n')
            f.write('# include "fwd.hpp"\n\n')
            f.write('//TODO split the includes\n')
            for inc in self._includes:
                f.write(inc.pp(1))
                # this is writing another file
                inc.testcase(outdir)

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
            f.write('#endif // %s\n' % define)


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

    def struct(self, name, body, size):
        rv = StructType(None, name, body, size)
        self._types.append(rv)
        return rv

    def partial_struct(self, native, body):
        rv = PartialStructType(native, body)
        self._types.append(rv)
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

    enums_h = ctx.include('src/mmo/enums.hpp')
    human_time_diff_h = ctx.include('src/mmo/human_time_diff.hpp')
    ids_h = ctx.include('src/mmo/ids.hpp')
    mmo_h = ctx.include('src/mmo/mmo.hpp')
    strs_h = ctx.include('src/mmo/strs.hpp')
    utils_h = ctx.include('src/mmo/utils.hpp')
    version_h = ctx.include('src/mmo/version.hpp')

    login_th = ctx.include('src/login/login.t.hpp')

    clif_th = ctx.include('src/map/clif.t.hpp')
    skill_th = ctx.include('src/map/skill.t.hpp')

    ## included types

    uint8_t = cstdint.native('uint8_t')
    uint16_t = cstdint.native('uint16_t')
    uint32_t = cstdint.native('uint32_t')
    uint64_t = cstdint.native('uint64_t')

    Byte = endians_h.neutral('Byte')
    Little16 = endians_h.network('Little16')
    Little32 = endians_h.network('Little32')
    Little64 = endians_h.network('Little64')

    SEX = enums_h.native('SEX')
    Option = enums_h.native('Option')
    EPOS = enums_h.native('EPOS')

    Species = ids_h.native('Species')
    AccountId = ids_h.native('AccountId')
    CharId = ids_h.native('CharId')
    PartyId = ids_h.native('PartyId')
    ItemNameId = ids_h.native('ItemNameId')
    BlockId = ids_h.native('BlockId')
    GmLevel = ids_h.native('GmLevel')

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


    # TODO: fix LIES
    char_key = mmo_h.neutral('CharKey')
    char_data = mmo_h.neutral('CharData')
    party_most = mmo_h.neutral('PartyMost')
    storage = mmo_h.neutral('Storage')


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

    sex_char = ctx.provided(SEX, NeutralType('char'))

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

    skill_id = ctx.enum(SkillID, u16)
    status_change = ctx.enum(StatusChange, u16)
    skill_flags = ctx.enum(SkillFlags, u16)

    tick32 = ctx.provided(tick_t, Little32)
    interval32 = ctx.provided(interval_t, Little32)
    interval16 = ctx.provided(interval_t, Little16)

    sex = ctx.enum(SEX, u8)
    option = ctx.enum(Option, u16)
    epos = ctx.enum(EPOS, u16)

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
    #account_crypt = ctx.string(AccountCrypt)
    account_email = ctx.string(AccountEmail)
    server_name = ctx.string(ServerName)
    party_name = ctx.string(PartyName)
    var_name = ctx.string(VarName)
    char_name = ctx.string(CharName)
    map_name = ctx.string(MapName)
    #mob_name = ctx.string(MobName)
    #npc_name = ctx.string(NpcName)
    #script_label = ctx.string(ScriptLabel)
    #item_name = ctx.string(ItemName)
    #md5_string = ctx.string(md5_string)
    #salt_string = ctx.string(SaltString)

    # this will be *so* useful when I do the party copy!
    human_time_diff = ctx.partial_struct(
            HumanTimeDiff,
            [
                ('year', u16),
                ('month', u16),
                ('day', u16),
                ('hour', u16),
                ('minute', u16),
                ('second', u16),
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

    ## packet channels

    # this is a somewhat simplistic view. For packets that get forwarded,
    # it may be worth pretending something like admin->char, map->login ...
    # that does break the tree format though

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
    char_user.r(0x0061,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_pass, 'old pass'),
            at(26, account_pass, 'new pass'),
        ],
        fixed_size=50,
    )
    char_user.s(0x0062,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'status'),
        ],
        fixed_size=3,
    )
    login_user.r(0x0063,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
    )
    login_user.r(0x0064,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'unknown'),
            at(6, account_name, 'account name'),
            at(30, account_pass, 'account pass'),
            at(54, version_2, 'version 2 flags'),
        ],
        fixed_size=55,
    )
    char_user.r(0x0065,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'login id1'),
            at(10, u32, 'login id2'),
            at(14, u16, 'packet tmw version'),
            at(16, sex, 'sex'),
        ],
        fixed_size=17,
    )
    char_user.r(0x0066,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
    )
    char_user.r(0x0067,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'char name'),
            at(26, stats6, 'stats'),
            at(32, u8, 'slot'),
            at(33, u16, 'hair color'),
            at(35, u16, 'hair style'),
        ],
        fixed_size=37,
    )
    char_user.r(0x0068,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, account_email, 'email'),
        ],
        fixed_size=46,
    )
    login_user.r(0x0069,
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
    )
    login_user.s(0x006a,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'error code'),
            at(3, seconds, 'error message'),
        ],
        fixed_size=23,
    )
    char_user.s(0x006b,
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
    )
    char_user.s(0x006c,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
    )
    char_user.s(0x006d,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_select, 'char select'),
        ],
        fixed_size=108,
    )
    char_user.s(0x006e,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
    )
    char_user.s(0x006f,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    char_user.s(0x0070,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
    )
    char_user.s(0x0071,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, map_name, 'map name'),
            at(22, ip4, 'ip'),
            at(26, u16, 'port'),
        ],
        fixed_size=28,
    )
    map_user.r(0x0072,
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
    map_user.s(0x0073,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, tick32, 'tick'),
            at(6, pos1, 'pos'),
            at(9, u8, 'five1'),
            at(10, u8, 'five2'),
        ],
        fixed_size=11,
    )
    map_user.s(0x0078,
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
    map_user.s(0x007b,
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
    map_user.s(0x007c,
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
    map_user.r(0x007d,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.r(0x007e,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'client tick'),
        ],
        fixed_size=6,
    )
    map_user.s(0x007f,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, tick32, 'tick'),
        ],
        fixed_size=6,
    )
    map_user.s(0x0080,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, being_remove_why, 'type'),
        ],
        fixed_size=7,
    )
    any_user.s(0x0081,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'error code'),
        ],
        fixed_size=3,
    )
    map_user.r(0x0085,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, pos1, 'pos'),
        ],
        fixed_size=5,
    )
    map_user.s(0x0087,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, tick32, 'tick'),
            at(6, pos2, 'pos2'),
            at(11, u8, 'zero'),
        ],
        fixed_size=12,
    )
    map_user.s(0x0088,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u16, 'x'),
            at(8, u16, 'y'),
        ],
        fixed_size=10,
    )
    map_user.r(0x0089,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'target id'),
            at(6, damage_type, 'action'),
        ],
        fixed_size=7,
    )
    map_user.s(0x008a,
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
    map_user.r(0x008c,
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
    map_user.s(0x008d,
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
    map_user.s(0x008e,
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
    map_user.r(0x0090,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u8, 'unused'),
        ],
        fixed_size=7,
    )
    map_user.s(0x0091,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, map_name, 'map name'),
            at(18, u16, 'x'),
            at(20, u16, 'y'),
        ],
        fixed_size=22,
    )
    map_user.s(0x0092,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, map_name, 'map name'),
            at(18, u16, 'x'),
            at(20, u16, 'y'),
            at(22, ip4, 'ip'),
            at(26, u16, 'port'),
        ],
        fixed_size=28,
    )
    map_user.r(0x0094,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x0095,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, char_name, 'char name'),
        ],
        fixed_size=30,
    )
    map_user.r(0x0096,
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
    map_user.s(0x0097,
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
    map_user.s(0x0098,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'flag'),
        ],
        fixed_size=3,
    )
    map_user.s(0x009a,
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
    map_user.r(0x009b,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'unused'),
            at(4, u8, 'client dir'),
        ],
        fixed_size=5,
    )
    map_user.s(0x009c,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u16, 'zero'),
            at(8, u8, 'client dir'),
        ],
        fixed_size=9,
    )
    map_user.s(0x009d,
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
    map_user.s(0x009e,
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
    map_user.r(0x009f,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'object id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00a0,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
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
    map_user.s(0x00a1,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x00a2,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
            at(4, u16, 'amount'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00a4,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u16, 'ioff2'),
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
    map_user.s(0x00a6,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u16, 'soff1'),
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
    map_user.r(0x00a7,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
            at(4, u32, 'unused id'),
        ],
        fixed_size=8,
    )
    map_user.s(0x00a8,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
            at(4, u16, 'amount'),
            at(6, u8, 'ok'),
        ],
        fixed_size=7,
    )
    map_user.r(0x00a9,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
            at(4, epos, 'epos ignored'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00aa,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
            at(4, epos, 'epos'),
            at(6, u8, 'ok'),
        ],
        fixed_size=7,
    )
    map_user.r(0x00ab,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
        ],
        fixed_size=4,
    )
    map_user.s(0x00ac,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
            at(4, epos, 'epos'),
            at(6, u8, 'ok'),
        ],
        fixed_size=7,
    )
    map_user.s(0x00af,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
            at(4, u16, 'amount'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00b0,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u32, 'value'),
        ],
        fixed_size=8,
    )
    map_user.s(0x00b1,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u32, 'value'),
        ],
        fixed_size=8,
    )
    map_user.r(0x00b2,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'flag'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00b3,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'one'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00b4,
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
    map_user.s(0x00b5,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00b6,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00b7,
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
    map_user.r(0x00b8,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'npc id'),
            at(6, u8, 'menu entry'),
        ],
        fixed_size=7,
    )
    map_user.r(0x00b9,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'npc id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x00bb,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'asp'),
            at(4, u8, 'unused'),
        ],
        fixed_size=5,
    )
    map_user.s(0x00bc,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u8, 'ok'),
            at(5, u8, 'val'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00bd,
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
    map_user.s(0x00be,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u8, 'value'),
        ],
        fixed_size=5,
    )
    map_user.r(0x00bf,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'emote'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00c0,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u8, 'type'),
        ],
        fixed_size=7,
    )
    map_user.r(0x00c1,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x00c2,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'users'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00c4,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x00c5,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u8, 'type'),
        ],
        fixed_size=7,
    )
    map_user.s(0x00c6,
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
    map_user.s(0x00c7,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u16, 'ioff2'),
            at(2, u32, 'base price'),
            at(6, u32, 'actual price'),
        ],
        repeat_size=10,
    )
    map_user.r(0x00c8,
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
    map_user.r(0x00c9,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u16, 'ioff2'),
            at(2, u16, 'count'),
        ],
        repeat_size=4,
    )
    map_user.s(0x00ca,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00cb,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00cd,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x00e4,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00e5,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'char name'),
        ],
        fixed_size=26,
    )
    map_user.r(0x00e6,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'type'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00e7,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'type'),
        ],
        fixed_size=3,
    )
    map_user.r(0x00e8,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'zeny or ioff2'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
    )
    map_user.s(0x00e9,
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
    map_user.r(0x00eb,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x00ec,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
    )
    map_user.r(0x00ed,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x00ee,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.r(0x00ef,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x00f0,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'fail'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00f2,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'current slots'),
            at(4, u16, 'max slots'),
        ],
        fixed_size=6,
    )
    map_user.r(0x00f3,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
    )
    map_user.s(0x00f4,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'soff1'),
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
    map_user.r(0x00f5,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'soff1'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
    )
    map_user.s(0x00f6,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'soff1'),
            at(4, u32, 'amount'),
        ],
        fixed_size=8,
    )
    map_user.r(0x00f7,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x00f8,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.r(0x00f9,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_name, 'party name'),
        ],
        fixed_size=26,
    )
    map_user.s(0x00fa,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'flag'),
        ],
        fixed_size=3,
    )
    map_user.s(0x00fb,
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
    map_user.r(0x00fc,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x00fd,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'char name'),
            at(26, u8, 'flag'),
        ],
        fixed_size=27,
    )
    map_user.s(0x00fe,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, party_name, 'party name'),
        ],
        fixed_size=30,
    )
    map_user.r(0x00ff,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'flag'),
        ],
        fixed_size=10,
    )
    map_user.r(0x0100,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x0101,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'exp'),
            at(4, u16, 'item'),
        ],
        fixed_size=6,
    )
    map_user.r(0x0102,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'exp'),
            at(4, u16, 'item'),
        ],
        fixed_size=6,
    )
    map_user.r(0x0103,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'unused char name'),
        ],
        fixed_size=30,
    )
    map_user.s(0x0105,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'char name'),
            at(30, u8, 'flag'),
        ],
        fixed_size=31,
    )
    map_user.s(0x0106,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u16, 'hp'),
            at(8, u16, 'max hp'),
        ],
        fixed_size=10,
    )
    map_user.s(0x0107,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u16, 'x'),
            at(8, u16, 'y'),
        ],
        fixed_size=10,
    )
    map_user.r(0x0108,
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
    map_user.s(0x0109,
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
    map_user.s(0x010c,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x010e,
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
    map_user.s(0x010f,
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
    map_user.s(0x0110,
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
    map_user.r(0x0112,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, skill_id, 'skill id'),
        ],
        fixed_size=4,
    )
    map_user.r(0x0118,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.s(0x0119,
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
    map_user.s(0x0139,
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
    map_user.s(0x013a,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'attack range'),
        ],
        fixed_size=4,
    )
    map_user.s(0x013b,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'type'),
        ],
        fixed_size=4,
    )
    map_user.s(0x013c,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
        ],
        fixed_size=4,
    )
    map_user.s(0x0141,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, sp, 'sp type'),
            at(4, u16, 'zero'),
            at(6, u32, 'value status'),
            at(10, u32, 'value b e'),
        ],
        fixed_size=14,
    )
    map_user.s(0x0142,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x0143,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u32, 'input int value'),
        ],
        fixed_size=10,
    )
    map_user.r(0x0146,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.s(0x0147,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, skill_info, 'info'),
        ],
        fixed_size=39,
    )
    map_user.s(0x0148,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u16, 'type'),
        ],
        fixed_size=8,
    )
    map_user.r(0x014d,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    map_user.r(0x018a,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'unused'),
        ],
        fixed_size=4,
    )
    map_user.s(0x018b,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'okay'),
        ],
        fixed_size=4,
    )
    map_user.s(0x0195,
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
    map_user.s(0x0196,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, status_change, 'sc type'),
            at(4, block_id, 'block id'),
            at(8, u8, 'flag'),
        ],
        fixed_size=9,
    )
    map_user.s(0x019b,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, u32, 'type'),
        ],
        fixed_size=10,
    )
    map_user.s(0x01b1,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
            at(4, u16, 'amount'),
            at(6, u8, 'fail'),
        ],
        fixed_size=7,
    )
    map_user.s(0x01c8,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'ioff2'),
            at(4, item_name_id, 'name id'),
            at(6, block_id, 'block id'),
            at(10, u16, 'amount'),
            at(12, u8, 'ok'),
        ],
        fixed_size=13,
    )
    map_user.s(0x01d4,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
        ],
        fixed_size=6,
    )
    map_user.r(0x01d5,
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
    map_user.s(0x01d7,
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
    map_user.s(0x01d8,
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
    map_user.s(0x01d9,
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
    map_user.s(0x01da,
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
    map_user.s(0x01de,
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
    map_user.s(0x01ee,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u16, 'ioff2'),
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
    map_user.s(0x01f0,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u16, 'soff1'),
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
    map_user.s(0x020c,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, block_id, 'block id'),
            at(6, ip4, 'ip'),
        ],
        fixed_size=10,
    )
    map_user.s(0x0212,
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
    login_char.r(0x2709,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    login_char.r(0x2710,
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
    )
    login_char.s(0x2711,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
    )
    login_char.r(0x2712,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'login id1'),
            at(10, u32, 'login id2'),
            at(14, sex, 'sex'),
            at(15, ip4, 'ip'),
        ],
        fixed_size=19,
    )
    login_char.s(0x2713,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'invalid'),
            at(7, account_email, 'email'),
            at(47, time32, 'connect until'),
        ],
        fixed_size=51,
    )
    login_char.r(0x2714,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'users'),
        ],
        fixed_size=6,
    )
    login_char.r(0x2715,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_email, 'email'),
        ],
        fixed_size=46,
    )
    login_char.r(0x2716,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    login_char.s(0x2717,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_email, 'email'),
            at(46, time32, 'connect until'),
        ],
        fixed_size=50,
    )
    # 0x2b0a
    login_char.r(0x2720,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
    )
    login_char.s(0x2721,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, gm, 'gm level'),
        ],
        fixed_size=10,
    )
    # 0x2b0c
    login_char.r(0x2722,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_email, 'old email'),
            at(46, account_email, 'new email'),
        ],
        fixed_size=86,
    )
    login_char.s(0x2723,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, sex, 'sex'),
        ],
        fixed_size=7,
    )
    login_char.r(0x2724,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'status'),
        ],
        fixed_size=10,
    )
    login_char.r(0x2725,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, human_time_diff, 'ban add'),
        ],
        fixed_size=18,
    )
    # evil packet, see also 0x794e
    login_admin.s(0x2726,
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
    )
    login_char.r(0x2727,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    # 0x2b10, 0x2b11
    for (id, cat) in [
        (0x2728, login_char.r),
        (0x2729, login_char.s),
    ]:
        cat(id,
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
    login_char.r(0x272a,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    login_char.s(0x2730,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    login_char.s(0x2731,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'ban not status'),
            at(7, time32, 'status or ban until'),
        ],
        fixed_size=11,
    )
    login_char.s(0x2732,
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
    )
    login_char.r(0x2740,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_pass, 'old pass'),
            at(30, account_pass, 'new pass'),
        ],
        fixed_size=54,
    )
    login_char.s(0x2741,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'status'),
        ],
        fixed_size=7,
    )

    # char map
    char_map.r(0x2af7,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    char_map.r(0x2af8,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'account pass'),
            at(50, u32, 'unused'),
            at(54, ip4, 'ip'),
            at(58, u16, 'port'),
        ],
        fixed_size=60,
    )
    char_map.s(0x2af9,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'code'),
        ],
        fixed_size=3,
    )
    # wtf duplicate v
    char_map.r(0x2afa,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, map_name, 'map name'),
        ],
        repeat_size=16,
    )
    # wtf duplicate ^
    char_map.s(0x2afa,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, item_name_id4, 'source item id'),
            at(6, item_name_id4, 'dest item id'),
        ],
        fixed_size=10,
    )
    char_map.s(0x2afb,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'unknown'),
            at(3, char_name, 'whisper name'),
        ],
        fixed_size=27,
    )
    char_map.r(0x2afc,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_id, 'char id'),
            at(10, u32, 'login id1'),
            at(14, u32, 'login id2'),
            at(18, ip4, 'ip'),
        ],
        fixed_size=22,
    )
    char_map.s(0x2afd,
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
    )
    char_map.s(0x2afe,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    char_map.r(0x2aff,
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
    )
    char_map.s(0x2b00,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'users'),
        ],
        fixed_size=6,
    )
    char_map.r(0x2b01,
        payload=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
            at(8, char_id, 'char id'),
            at(12, char_key, 'char key'),
            at(None, char_data, 'char data'),
        ],
        payload_size=None,
    )
    char_map.r(0x2b02,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u32, 'login id1'),
            at(10, u32, 'login id2'),
            at(14, ip4, 'ip'),
        ],
        fixed_size=18,
    )
    char_map.s(0x2b03,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'unknown'),
        ],
        fixed_size=7,
    )
    char_map.s(0x2b04,
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
    )
    char_map.r(0x2b05,
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
    )
    char_map.s(0x2b06,
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
    )
    # 0x2720
    char_map.r(0x2b0a,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
        ],
        head_size=8,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
    )
    char_map.s(0x2b0b,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, gm, 'gm level'),
        ],
        fixed_size=10,
    )
    # 0x2722
    char_map.r(0x2b0c,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_email, 'old email'),
            at(46, account_email, 'new email'),
        ],
        fixed_size=86,
    )
    char_map.s(0x2b0d,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, sex, 'sex'),
        ],
        fixed_size=7,
    )
    char_map.r(0x2b0e,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'char name'),
            at(30, u16, 'operation'),
            at(32, human_time_diff, 'ban add'),
        ],
        fixed_size=44,
    )
    char_map.r(0x2b0f,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, char_name, 'char name'),
            at(30, u16, 'operation'),
            at(32, u16, 'error'),
        ],
        fixed_size=34,
    )
    # 0x2728, 0x2729
    for (id, cat) in [
        (0x2b10, char_map.r),
        (0x2b11, char_map.s),
    ]:
        cat(id,
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
    char_map.s(0x2b12,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, char_id, 'partner id'),
        ],
        fixed_size=10,
    )
    char_map.s(0x2b13,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    char_map.s(0x2b14,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'ban not status'),
            at(7, time32, 'status or ban until'),
        ],
        fixed_size=11,
    )
    char_map.s(0x2b15,
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
    )
    char_map.r(0x2b16,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
        ],
        fixed_size=6,
    )

    char_map.r(0x3000,
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
    char_map.r(0x3001,
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
    )
    char_map.r(0x3002,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_id, 'char id'),
            at(6, u8, 'flag'),
        ],
        fixed_size=7,
    )
    # 0x3803
    char_map.r(0x3003,
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
    char_map.r(0x3004,
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
    char_map.r(0x3005,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    char_map.r(0x3010,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    char_map.r(0x3011,
        payload=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
            at(8, storage, 'storage'),
        ],
        payload_size=None,
    )
    char_map.r(0x3020,
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
    char_map.r(0x3021,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
        ],
        fixed_size=6,
    )
    char_map.r(0x3022,
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
    char_map.r(0x3023,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, u16, 'exp'),
            at(12, u16, 'item'),
        ],
        fixed_size=14,
    )
    char_map.r(0x3024,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
        ],
        fixed_size=10,
    )
    char_map.r(0x3025,
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
    char_map.r(0x3026,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
        ],
        fixed_size=6,
    )
    char_map.r(0x3027,
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
    char_map.r(0x3028,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, char_name, 'char name'),
        ],
        fixed_size=34,
    )

    char_map.s(0x3800,
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
    char_map.s(0x3801,
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
    char_map.s(0x3802,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, char_name, 'sender char name'),
            at(26, u8, 'flag'),
        ],
        fixed_size=27,
    )
    # 0x3003
    char_map.s(0x3803,
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
    char_map.s(0x3804,
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
    char_map.s(0x3810,
        payload=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, account_id, 'account id'),
            at(8, storage, 'storage'),
        ],
        payload_size=None,
    )
    char_map.s(0x3811,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'unknown'),
        ],
        fixed_size=7,
    )
    char_map.s(0x3820,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, u8, 'error'),
            at(7, party_id, 'party id'),
            at(11, party_name, 'party name'),
        ],
        fixed_size=35,
    )
    char_map.s(0x3821,
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
    char_map.s(0x3822,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, u8, 'flag'),
        ],
        fixed_size=11,
    )
    char_map.s(0x3823,
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
    char_map.s(0x3824,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, account_id, 'account id'),
            at(10, char_name, 'char name'),
        ],
        fixed_size=34,
    )
    char_map.s(0x3825,
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
    char_map.s(0x3826,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, party_id, 'party id'),
            at(6, u8, 'flag'),
        ],
        fixed_size=7,
    )
    char_map.s(0x3827,
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
    any_user.r(0x7530,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    any_user.s(0x7531,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, version, 'version'),
        ],
        fixed_size=10,
    )
    any_user.r(0x7532,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )

    # login admin
    login_admin.r(0x7918,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'encryption zero'),
            at(4, account_pass, 'account pass'),
        ],
        fixed_size=28,
    )
    login_admin.s(0x7919,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u8, 'error'),
        ],
        fixed_size=3,
    )
    login_admin.r(0x7920,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'start account id'),
            at(6, account_id, 'end account id'),
        ],
        fixed_size=10,
    )
    login_admin.s(0x7921,
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
    )
    login_admin.r(0x7924,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, item_name_id4, 'source item id'),
            at(6, item_name_id4, 'dest item id'),
        ],
        fixed_size=10,
    )
    login_admin.s(0x7925,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    login_admin.r(0x7930,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'password'),
            at(50, sex_char, 'sex'),
            at(51, account_email, 'email'),
        ],
        fixed_size=91,
    )
    login_admin.s(0x7931,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    login_admin.r(0x7932,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
        ],
        fixed_size=26,
    )
    login_admin.s(0x7933,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    login_admin.r(0x7934,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'password'),
        ],
        fixed_size=50,
    )
    login_admin.s(0x7935,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    login_admin.r(0x7936,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, u32, 'status'),
            at(30, seconds, 'error message'),
        ],
        fixed_size=50,
    )
    login_admin.s(0x7937,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
            at(30, u32, 'status'),
        ],
        fixed_size=34,
    )
    login_admin.r(0x7938,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    login_admin.s(0x7939,
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
    )
    login_admin.r(0x793a,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_pass, 'password'),
        ],
        fixed_size=50,
    )
    login_admin.s(0x793b,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    login_admin.r(0x793c,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, sex_char, 'sex'),
        ],
        fixed_size=27,
    )
    login_admin.s(0x793d,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    login_admin.r(0x793e,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, gm1, 'gm level'),
        ],
        fixed_size=27,
    )
    login_admin.s(0x793f,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    login_admin.r(0x7940,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, account_email, 'email'),
        ],
        fixed_size=66,
    )
    login_admin.s(0x7941,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    # this packet is insane
    login_admin.r(0x7942,
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
    )
    login_admin.s(0x7943,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    login_admin.r(0x7944,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
        ],
        fixed_size=26,
    )
    login_admin.s(0x7945,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    login_admin.r(0x7946,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    login_admin.s(0x7947,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    login_admin.r(0x7948,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, time32, 'valid until'),
        ],
        fixed_size=30,
    )
    login_admin.s(0x7949,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
            at(30, time32, 'valid until'),
        ],
        fixed_size=34,
    )
    login_admin.r(0x794a,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, time32, 'ban until'),
        ],
        fixed_size=30,
    )
    login_admin.s(0x794b,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
            at(30, time32, 'ban until'),
        ],
        fixed_size=34,
    )
    login_admin.r(0x794c,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, human_time_diff, 'ban add'),
        ],
        fixed_size=38,
    )
    login_admin.s(0x794d,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
            at(30, time32, 'ban until'),
        ],
        fixed_size=34,
    )
    # evil packet (see also 0x2726)
    login_admin.r(0x794e,
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
    )
    login_admin.s(0x794f,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u16, 'error'),
        ],
        fixed_size=4,
    )
    login_admin.r(0x7950,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, human_time_diff, 'valid add'),
        ],
        fixed_size=38,
    )
    login_admin.s(0x7951,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
            at(6, account_name, 'account name'),
            at(30, time32, 'valid until'),
        ],
        fixed_size=34,
    )
    login_admin.r(0x7952,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
        ],
        fixed_size=26,
    )
    # this packet is insane
    login_admin.s(0x7953,
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
    )
    login_admin.r(0x7954,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_id, 'account id'),
        ],
        fixed_size=6,
    )
    login_admin.r(0x7955,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )

    ## new-style packets
    # general packets
    any_user.x(0x8000,
        payload=[
            at(0, u16, 'packet id'),
            # packet 0x8000 is handled specially
            at(2, u16, 'packet length'),
        ],
        payload_size=4,
    )

    ## teardown
    ctx.dump()

if __name__ == '__main__':
    main()
