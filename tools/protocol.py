#!/usr/bin/env python
# coding: utf-8

#   protocol.py - generator for entire TMWA network protocol
#
#   Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
#
#   This file is part of The Mana World (Athena server)
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

import glob
import os
from posixpath import relpath

# The following code should be relatively easy to understand, but please
# keep your sanity fastened and your arms and legs inside at all times.

generated = '// This is a generated file, edit %s instead\n' % __file__

copyright = '''//    {filename} - {description}
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
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

class StructType(Type):
    __slots__ = ('name', 'fields', 'size')

    def __init__(self, name, fields, size):
        self.name = name
        self.fields = fields
        self.size = size

    def dump(self, f):
        self.dump_native(f)
        self.dump_network(f)
        self.dump_convert(f)
        f.write('\n')

    def dump_native(self, f):
        name = self.name
        f.write('struct %s\n{\n' % name)
        for (o, l, n) in self.fields:
            f.write('    %s %s;\n' % (l.native_tag(), n))
        f.write('};\n')

    def dump_network(self, f):
        name = 'Net%s' % self.name
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
        filename = 'include_%s_test.cpp' % root
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

    def dump(self, f):
        self.fixed_struct.dump(f)

class VarPacket(object):
    __slots__ = ('head_struct', 'repeat_struct')

    def __init__(self, head_struct, repeat_struct):
        self.head_struct = head_struct
        self.repeat_struct = repeat_struct

    def dump(self, f):
        self.head_struct.dump(f)
        self.repeat_struct.dump(f)

def packet(name,
        fixed=None, fixed_size=None,
        head=None, head_size=None,
        repeat=None, repeat_size=None,
):
    assert (fixed is None) <= (fixed_size is None)
    assert (head is None) <= (head_size is None)
    assert (repeat is None) <= (repeat_size is None)

    if fixed is not None:
        assert not head and not repeat
        return FixedPacket(
                StructType('%s_Fixed' % name, fixed, fixed_size))
    else:
        assert head and repeat
        return VarPacket(
                StructType('%s_Head' % name, head, head_size),
                StructType('%s_Repeat' % name, repeat, repeat_size))


class Channel(object):
    __slots__ = ('server', 'client', 'packets')

    def __init__(self, server, client):
        self.server = server
        self.client = client
        self.packets = []

    def s(self, id, **kwargs):
        name = 'SPacket0x%04x' % id
        self.packets.append(packet(name, **kwargs))

    def r(self, id, **kwargs):
        name = 'RPacket0x%04x' % id
        self.packets.append(packet(name, **kwargs))

    def dump(self, outdir):
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
            for p in self.packets:
                p.dump(f)
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
            f.write('// TODO put stuff here\n')
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

            f.write('template <class T>\n')
            f.write('bool native_to_network(T *network, T native)\n{\n')
            f.write('    *network = native;\n')
            f.write('    return true;\n')
            f.write('}\n')
            f.write('template <class T>\n')
            f.write('bool network_to_native(T *native, T network)\n{\n')
            f.write('    *native = network;\n')
            f.write('    return true;\n')
            f.write('}\n')

            f.write('template <size_t N>\n')
            f.write('struct NetString\n{\n')
            f.write('    char data[N];\n')
            f.write('};\n')
            f.write('template <size_t N>\n')
            f.write('bool native_to_network(NetString<N> *network, VString<N-1> native)\n{\n')
            f.write('    // basically WBUF_STRING\n')
            f.write('    char *const begin = network->data;\n')
            f.write('    char *const end = begin + N;\n')
            f.write('    char *const mid = std::copy(native.begin(), native.end(), begin);\n')
            f.write('    std::fill(mid, end, \'\\0\');\n')
            f.write('    return true;\n')
            f.write('}\n')
            f.write('template <size_t N>\n')
            f.write('bool network_to_native(VString<N-1> *native, NetString<N> network)\n{\n')
            f.write('    // basically RBUF_STRING\n')
            f.write('    const char *const begin = network.data;\n')
            f.write('    const char *const end = begin + N;\n')
            f.write('    const char *const mid = std::find(begin, end, \'\\0\');\n')
            f.write('    *native = XString(begin, mid, nullptr);\n')
            f.write('    return true;\n')
            f.write('}\n')
            f.write('\n')
            for ty in self._types:
                ty.dump(f)
            f.write('#endif // %s\n' % define)
        for ch in self._channels:
            ch.dump(outdir)


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

    def struct(self, name, body):
        rv = StructType(name, body)
        self._types.append(rv)
        return rv

    def partial_struct(self, native, body):
        rv = PartialStructType(native, body)
        self._types.append(rv)
        return rv


def main():

    # setup

    ctx = Context(outdir='src/proto2/')


    # headers

    cstdint = ctx.sysinclude('cstdint')

    endians_h = ctx.include('src/ints/little.hpp')

    vstring_h = ctx.include('src/strings/vstring.hpp')

    ip_h = ctx.include('src/net/ip.hpp')

    enums_h = ctx.include('src/mmo/enums.hpp')
    human_time_diff_h = ctx.include('src/mmo/human_time_diff.hpp')
    ids_h = ctx.include('src/mmo/ids.hpp')
    strs_h = ctx.include('src/mmo/strs.hpp')
    utils_h = ctx.include('src/mmo/utils.hpp')
    version_h = ctx.include('src/mmo/version.hpp')


    # included types

    uint8_t = cstdint.native('uint8_t')
    uint16_t = cstdint.native('uint16_t')
    uint32_t = cstdint.native('uint32_t')
    uint64_t = cstdint.native('uint64_t')

    Byte = endians_h.neutral('Byte')
    Little16 = endians_h.network('Little16')
    Little32 = endians_h.network('Little32')
    Little64 = endians_h.network('Little64')

    SEX = enums_h.native('SEX')

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
    #PartyName = strs_h.native('PartyName')
    VarName = strs_h.native('VarName')
    #MobName = map_t_h.native('MobName')
    #NpcName = map_t_h.native('NpcName')
    #ScriptLabel = map_t_h.native('ScriptLabel')
    #ItemName = map_t_h.native('ItemName')
    #md5_native = md5_h.native('md5_native')
    #SaltString = md5_h.native('SaltString')

    # generated types

    u8 = ctx.provided(uint8_t, Byte)
    u16 = ctx.provided(uint16_t, Little16)
    u32 = ctx.provided(uint32_t, Little32)
    u64 = ctx.provided(uint64_t, Little64)

    sex = ctx.enum(SEX, u8)
    species = ctx.wrap(Species, u16)
    account_id = ctx.wrap(AccountId, u32)
    char_id = ctx.wrap(CharId, u32)
    party_id = ctx.wrap(PartyId, u32)
    item_name_id = ctx.wrap(ItemNameId, u16)
    block_id = ctx.wrap(BlockId, u32)

    time32 = ctx.provided(TimeT, Little32)
    time64 = ctx.provided(TimeT, Little64)

    gm1 = ctx.provided(GmLevel, Byte)
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
    #party_name = ctx.string(PartyName)
    var_name = ctx.string(VarName)
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

    # packets

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

    # map user
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
    login_char.r(0x2712,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
            at(6, account_email, 'email'),
        ],
        fixed_size=46,
    )
    login_char.r(0x2716,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
        ],
        fixed_size=6,
    )
    login_char.s(0x2717,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
            at(6, account_email, 'email'),
            at(46, time32, 'connect until'),
        ],
        fixed_size=50,
    )
    login_char.r(0x2720,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
            at(4, u32, 'account id'),
        ],
        head_size=8,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
    )
    login_char.s(0x2721,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
            at(6, gm, 'gm level'),
        ],
        fixed_size=10,
    )
    login_char.r(0x2722,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
            at(6, account_email, 'old email'),
            at(46, account_email, 'new email'),
        ],
        fixed_size=86,
    )
    login_char.s(0x2723,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
            at(6, sex, 'sex'),
        ],
        fixed_size=7,
    )
    login_char.r(0x2724,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
            at(6, u32, 'status'),
        ],
        fixed_size=10,
    )
    login_char.r(0x2725,
        head=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
            at(6, human_time_diff, 'deltas'),
        ],
        head_size=18,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
    )
    # evil packet, see also 0x794e
    login_admin.r(0x2726,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'unused'),
            at(4, u32, 'string length'),
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
            at(2, u32, 'account id'),
        ],
        fixed_size=6,
    )
    for (id, cat) in [
        (0x2728, login_char.r),
        (0x2729, login_char.s),
    ]:
        cat(id,
            head=[
                at(0, u16, 'packet id'),
                at(2, u16, 'packet length'),
                at(4, u32, 'account id'),
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
            at(2, u32, 'account id'),
        ],
        fixed_size=6,
    )
    login_char.s(0x2730,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )
    login_char.s(0x2731,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
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
            at(0, u32, 'account id'),
            at(4, gm1, 'gm level'),
        ],
        repeat_size=5,
    )
    login_char.r(0x2740,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
            at(6, account_pass, 'old pass'),
            at(30, account_pass, 'new pass'),
        ],
        fixed_size=54,
    )
    login_char.s(0x2741,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
            at(6, u8, 'status'),
        ],
        fixed_size=7,
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
    login_admin.r(0x7920,
        head=[
            at(0, u16, 'packet id'),
            at(2, u32, 'start account id'),
            at(6, u32, 'end account id'),
        ],
        head_size=10,
        repeat=[at(0, u8, 'c')],
        repeat_size=1,
    )
    login_admin.s(0x7921,
        head=[
            at(0, u16, 'packet id'),
            at(2, u16, 'packet length'),
        ],
        head_size=4,
        repeat=[
            at(0, u32, 'account id'),
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
            at(2, u32, 'source item id'),
            at(6, u32, 'dest item id'),
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
            at(50, sex, 'sex'),
            at(51, account_email, 'email'),
        ],
        fixed_size=91,
    )
    login_admin.s(0x7931,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
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
            at(30, u16, 'is_new'),
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
            at(2, u32, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    login_admin.r(0x793c,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, sex, 'sex'),
        ],
        fixed_size=27,
    )
    login_admin.s(0x793d,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    # this packet is insane
    login_admin.r(0x7942,
        head=[
            at(0, u16, 'packet id'),
            at(2, account_name, 'account name'),
            at(26, u16, 'string length'),
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
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
            at(6, account_name, 'account name'),
        ],
        fixed_size=30,
    )
    login_admin.r(0x7946,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
        ],
        fixed_size=6,
    )
    login_admin.s(0x7947,
        fixed=[
            at(0, u16, 'packet id'),
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
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
            at(4, u32, 'string length'),
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
            at(2, u32, 'account id'),
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
            at(2, u32, 'account id'),
            at(6, gm1, 'gm level'),
            at(7, account_name, 'account name'),
            at(31, sex, 'id'),
            at(32, u32, 'login count'),
            at(36, u32, 'state'),
            at(40, seconds, 'error message'),
            at(60, millis, 'last login string'),
            at(84, str16, 'ip string'),
            at(100, account_email, 'email'),
            at(140, time32, 'connect until'),
            at(144, time32, 'ban until'),
            at(148, u16, 'string length'),
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
            at(2, u32, 'account id'),
        ],
        fixed_size=6,
    )
    login_admin.r(0x7955,
        fixed=[
            at(0, u16, 'packet id'),
        ],
        fixed_size=2,
    )


    # teardown
    ctx.dump()

if __name__ == '__main__':
    main()
