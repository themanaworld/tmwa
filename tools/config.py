#!/usr/bin/env python
# coding: utf-8

#   config.py - generator for config file parsers
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
import os

from protocol import OpenWrite


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


class AnyHeader(object):
    __slots__ = ('name')

    def __init__(self, name):
        self.name = name

class SystemHeader(AnyHeader):
    __slots__ = ()
    meta = 0

    def relative_to(self, path):
        return '<%s>' % self.name

class Header(AnyHeader):
    __slots__ = ()
    meta = 1

    def relative_to(self, path):
        return '"%s"' % os.path.relpath(self.name, path)


class ConfigType(object):
    __slots__ = ()

class SimpleType(ConfigType):
    __slots__ = ('name', 'headers')

    def __init__(self, name, headers):
        self.name = name
        self.headers = frozenset(headers)

    def __repr__(self):
        return 'SimpleType(%r, %r)' % (self.name, self.headers)

    def type_name(self):
        return self.name

    def dump_extract(self, cpp, var):
        cpp.write(
'''
            if (!extract(value.data, &{var}))
            {{
                value.span.error("Failed to extract value"_s);
                return false;
            }}
'''.lstrip('\n').format(var=var))

class PhonyType(ConfigType):
    __slots__ = ('type', 'name', 'call', 'headers')

    def __init__(self, type, name, call, extra_headers):
        self.type = type
        self.name = name
        self.call = call
        self.headers = type.headers | extra_headers

    def __repr__(self):
        return 'PhonyType(%r, %r, %r, %r)' % (self.type, self.name, self.call, self.headers)

    def type_name(self):
        return '// special %s' % self.type.type_name()

    def dump_extract(self, cpp, var):
        cpp.write('            %s %s;\n' % (self.type.type_name(),  self.name))
        self.type.dump_extract(cpp, self.name)
        cpp.write('            %s\n' % self.call)

class TransformedType(ConfigType):
    __slots__ = ('type', 'transform', 'headers')

    def __init__(self, type, transform, extra_headers=set()):
        self.type = type
        self.transform = transform
        self.headers = type.headers | extra_headers

    def __repr__(self):
        return 'TransformedType(%r, %r)' % (self.type, self.transform)

    def type_name(self):
        return self.type.type_name()

    def dump_extract(self, cpp, var):
        self.type.dump_extract(cpp, var)
        cpp.write('            %s;\n' % self.transform)

class BoundedType(ConfigType):
    __slots__ = ('type', 'low', 'high', 'headers')

    def __init__(self, type, low, high, extra_headers=set()):
        assert isinstance(type, ConfigType)
        self.type = type
        self.low = low
        self.high = high
        self.headers = type.headers | extra_headers

    def __repr__(self):
        return 'BoundedType(%r, %r, %r, %r)' % (self.type, self.low, self.high, self.headers)

    def type_name(self):
        return self.type.type_name()

    def dump_extract(self, cpp, var):
        self.type.dump_extract(cpp, var)
        cpp.write(
'''
            if (!({low} <= {var} && {var} <= {high}))
            {{
                value.span.error("Value of {name} not in range [{low}, {high}]"_s);
                return false;
            }}
'''.format(low=self.low, high=self.high, var=var, name=var.split('.')[-1]))

class MinBoundedType(ConfigType):
    __slots__ = ('type', 'low', 'headers')

    def __init__(self, type, low, extra_headers=set()):
        assert isinstance(type, ConfigType)
        self.type = type
        self.low = low
        self.headers = type.headers | extra_headers

    def __repr__(self):
        return 'MinBoundedType(%r, %r, %r, %r)' % (self.type, self.low, self.headers)

    def type_name(self):
        return self.type.type_name()

    def dump_extract(self, cpp, var):
        self.type.dump_extract(cpp, var)
        cpp.write(
'''
            if (!({low} <= {var}))
            {{
                value.span.error("Value of {name} not at least {low}"_s);
                return false;
            }}
'''.format(low=self.low, var=var, name=var.split('.')[-1]))

class Option(object):
    __slots__ = ('name', 'type', 'default', 'headers')

    def __init__(self, name, type, default, extra_headers=set()):
        self.name = name
        self.type = type
        self.default = default
        self.headers = type.headers | extra_headers

    def dump1(self, hpp):
        hpp.write('    %s %s = %s;\n' % (self.type.type_name(), self.name, self.default))
    def dump2(self, cpp, x):
        # NOTE about hashing
        # dead simple hash: pack 6 bits of first 5 letters into an int
        y = self.name[:5]
        if x != y:
            if x is not None:
                cpp.write('        break;\n')
            c0 = y[0] if len(y) > 0 else '\\0'
            c1 = y[1] if len(y) > 1 else '\\0'
            c2 = y[2] if len(y) > 2 else '\\0'
            c3 = y[3] if len(y) > 3 else '\\0'
            c4 = y[4] if len(y) > 4 else '\\0'
            assert len(y) >= 3, '<-- change this number in the source file for: %r' % self.name
            cpp.write("    case (('%s' << 24) | ('%s' << 18) | ('%s' << 12) | ('%s' << 6) | ('%s' << 0)):\n" % (c0, c1, c2, c3, c4))
        cpp.write('        if (key.data == "{name}"_s)\n'.format(name=self.name))
        cpp.write('        {\n')
        self.type.dump_extract(cpp, 'conf.%s' % self.name)
        cpp.write('            return true;\n')
        cpp.write('        }\n')
        return y

class Group(object):
    __slots__ = ('name', 'options', 'extra_headers')

    def __init__(self, name):
        self.name = name
        self.options = {}
        self.extra_headers = []

    def extra(self, h):
        self.extra_headers.append(h)

    def opt(self, name, type, default, extra_headers=set(), pre=None, post=None, min=None, max=None):
        assert name not in self.options, 'Duplicate option name: %s' % name
        assert isinstance(default, str)
        if pre is not None:
            type = TransformedType(type, pre)
        if min is not None:
            if max is not None:
                type = BoundedType(type, min, max)
            else:
                type = MinBoundedType(type, min)
        else:
            assert max is None
        if post is not None:
            type = TransformedType(type, post)
        self.options[name] = rv = Option(name, type, default, extra_headers)
        return rv

    def dump_in(self, path, namespace_name):
        if namespace_name == 'char':
            namespace_name += '_'
        var_name = '%s_conf' % self.name
        class_name = var_name.replace('_', ' ').title().replace(' ', '')
        short_hpp_name = '%s.hpp' % var_name
        hpp_name = os.path.join(path, short_hpp_name)
        short_cpp_name = '%s.cpp' % var_name
        cpp_name = os.path.join(path, short_cpp_name)

        values = sorted(self.options.values(), key=lambda o: o.name)

        desc = 'Config for %s::%s' % (namespace_name, self.name)
        with OpenWrite(hpp_name) as hpp, \
                OpenWrite(cpp_name) as cpp:
            hpp.write('#pragma once\n')
            hpp.write(copyright.format(filename=short_hpp_name, description=desc))
            hpp.write('\n')
            hpp.write(generated)
            cpp.write('#include "%s"\n' % short_hpp_name)
            cpp.write(copyright.format(filename=short_cpp_name, description=desc))
            cpp.write('\n')
            cpp.write(generated)
            headers = {
                    Header('src/io/fwd.hpp'),
                    Header('src/strings/fwd.hpp')
            }
            for o in values:
                headers |= o.headers

            hpp.write('\n')
            hpp.write('#include "fwd.hpp"\n')
            for h in sorted(headers, key=lambda h: (h.meta, h.name)):
                hpp.write('#include %s\n' % h.relative_to(path))
            hpp.write('\n')
            cpp.write('\n')
            for h in [
                    SystemHeader('bitset'),
                    Header('src/io/extract.hpp'),
                    Header('src/io/span.hpp'),
                    Header('src/mmo/extract_enums.hpp'),
                    Header('src/high/extract_mmo.hpp'),
            ] + self.extra_headers:
                cpp.write('#include %s\n' % h.relative_to(path))
            cpp.write('\n')
            cpp.write('#include "../poison.hpp"\n')
            cpp.write('\n')

            hpp.write('namespace tmwa\n{\n')
            cpp.write('namespace tmwa\n{\n')
            cpp.write('''
static __attribute__((unused))
bool extract(XString str, bool *v)
{
    if (str == "true"_s || str == "on"_s || str == "yes"_s
        || str == "oui"_s || str == "ja"_s
        || str == "si"_s || str == "1"_s)
    {
        *v = 1;
        return true;
    }
    if (str == "false"_s || str == "off"_s || str == "no"_s
        || str == "non"_s || str == "nein"_s || str == "0"_s)
    {
        *v = 0;
        return true;
    }
    return false;
}

static __attribute__((unused))
bool extract(XString str, std::bitset<256> *v)
{
    if (!str)
    {
        v->reset();
        return true;
    }
    for (uint8_t c : str)
    {
        (*v)[c] = true;
    }
    return true;
}
''')
            hpp.write('namespace %s\n{\n' % namespace_name)
            cpp.write('namespace %s\n{\n' % namespace_name)
            hpp.write('struct %s\n{\n' % class_name)
            for o in values:
                o.dump1(hpp)
            hpp.write('}; // struct %s\n' % class_name)
            hpp.write('bool parse_%s(%s& conf, io::Spanned<XString> key, io::Spanned<ZString> value);\n' % (var_name, class_name))
            hpp.write('} // namespace %s\n' % namespace_name)
            hpp.write('} // namespace tmwa\n')
            cpp.write('bool parse_%s(%s& conf, io::Spanned<XString> key, io::Spanned<ZString> value)\n{\n' % (var_name, class_name))
            # see NOTE about hashing in Option.dump2
            cpp.write('    int key_hash = 0;\n')
            cpp.write('    if (key.data.size() > 0)\n')
            cpp.write('        key_hash |= key.data[0] << 24;\n')
            cpp.write('    if (key.data.size() > 1)\n')
            cpp.write('        key_hash |= key.data[1] << 18;\n')
            cpp.write('    if (key.data.size() > 2)\n')
            cpp.write('        key_hash |= key.data[2] << 12;\n')
            cpp.write('    if (key.data.size() > 3)\n')
            cpp.write('        key_hash |= key.data[3] << 6;\n')
            cpp.write('    if (key.data.size() > 4)\n')
            cpp.write('        key_hash |= key.data[4] << 0;\n')
            cpp.write('    switch (key_hash)\n')
            cpp.write('    {\n')
            x = None
            for o in values:
                x = o.dump2(cpp, x)
            cpp.write('        break;\n')
            cpp.write('    } // switch\n')
            cpp.write('    key.span.error("Unknown config key"_s);\n')
            cpp.write('    return false;\n')
            cpp.write('} // fn parse_%s_conf()\n' % var_name)
            cpp.write('} // namespace %s\n' % namespace_name)
            cpp.write('} // namespace tmwa\n')

class Realm(object):
    __slots__ = ('path', 'groups')

    def __init__(self, path):
        self.path = path
        self.groups = {}

    def conf(self, name=None):
        if not name:
            name = self.path.split('/')[-1]
        assert name not in self.groups, 'Duplicate group name: %s' % name
        self.groups[name] = rv = Group(name)
        return rv

    def dump(self):
        for g in self.groups.values():
            g.dump_in(self.path, self.path.split('/')[-1])

class Everything(object):
    __slots__ = ('realms')

    def __init__(self):
        self.realms = {}

    def realm(self, path):
        assert path not in self.realms, 'Duplicate realm path: %s' % path
        self.realms[path] = rv = Realm(path)
        return rv

    def dump(self):
        for g in glob.glob('src/*/*_conf.[ch]pp'):
            os.rename(g, g + '.old')
        for v in self.realms.values():
            v.dump()
        for g in glob.glob('src/*/*_conf.[ch]pp.old'):
            print('Obsolete: %s' % g)
            os.remove(g)


def lit(s):
    return '"%s"_s' % s.replace('\\', '\\\\').replace('"', '\\"')

def build_config():
    rv = Everything()

    # realms
    login_realm = rv.realm('src/login')
    admin_realm = rv.realm('src/admin')
    char_realm = rv.realm('src/char')
    map_realm = rv.realm('src/map')

    # confs
    login_conf = login_realm.conf()
    login_lan_conf = login_realm.conf('login_lan')

    admin_conf = admin_realm.conf()

    char_conf = char_realm.conf()
    char_lan_conf = char_realm.conf('char_lan')
    inter_conf = char_realm.conf('inter')

    map_conf = map_realm.conf()
    battle_conf = map_realm.conf('battle')

    # headers
    cstdint_sys = SystemHeader('cstdint')
    vector_sys = SystemHeader('vector')
    bitset_sys = SystemHeader('bitset')

    ip_h = Header('src/net/ip.hpp')
    rstring_h = Header('src/strings/rstring.hpp')
    literal_h = Header('src/strings/literal.hpp')
    ids_h = Header('src/mmo/ids.hpp')
    strs_h = Header('src/mmo/strs.hpp')
    timer_th = Header('src/net/timer.t.hpp')
    login_th = Header('src/login/login.t.hpp')
    udl_h = Header('src/ints/udl.hpp')
    net_point_h = Header('src/proto2/net-Point.hpp')
    char_h = Header('src/char/char.hpp')
    map_h = Header('src/map/map.hpp')
    map_th = Header('src/map/map.t.hpp')
    npc_h = Header('src/map/npc.hpp')
    npc_parse_h = Header('src/map/npc-parse.hpp')

    map_conf.extra(npc_parse_h)

    # types
    bool = SimpleType('bool', set())
    #double = SimpleType('double', set())
    u8 = SimpleType('uint8_t', {cstdint_sys})
    u16 = SimpleType('uint16_t', {cstdint_sys})
    u32 = SimpleType('uint32_t', {cstdint_sys})
    u64 = SimpleType('uint64_t', {cstdint_sys})
    i8 = SimpleType('int8_t', {cstdint_sys})
    i16 = SimpleType('int16_t', {cstdint_sys})
    i32 = SimpleType('int32_t', {cstdint_sys})
    i64 = SimpleType('int64_t', {cstdint_sys})

    percent = i32
    perk = i32
    per10k = i32
    #per10kd = double
    per10kd = i32

    IP4Address = SimpleType('IP4Address', {ip_h})
    IP4Mask = SimpleType('IP4Mask', {ip_h})
    IpSet = SimpleType('std::vector<IP4Mask>', {vector_sys, ip_h})
    RString = SimpleType('RString', {rstring_h, literal_h})
    GmLevel = SimpleType('GmLevel', {ids_h})
    hours = SimpleType('std::chrono::hours', {timer_th})
    seconds = SimpleType('std::chrono::seconds', {timer_th})
    milliseconds = SimpleType('std::chrono::milliseconds', {timer_th})
    ACO = SimpleType('ACO', {login_th})
    ServerName = SimpleType('ServerName', {strs_h})
    AccountName = SimpleType('AccountName', {strs_h})
    AccountPass = SimpleType('AccountPass', {strs_h})
    Point = SimpleType('Point', {net_point_h})
    CharName = SimpleType('CharName', {strs_h})
    CharBitset = SimpleType('std::bitset<256>', {bitset_sys})
    MapName = SimpleType('MapName', {strs_h})
    ATK = SimpleType('ATK', {map_th})

    addmap = PhonyType(MapName, 'name', 'map_addmap(name);', {map_h})
    delmap = PhonyType(MapName, 'name', 'map_delmap(name);', {map_h})
    addnpc = PhonyType(RString, 'npc', 'npc_addsrcfile(npc);', {npc_h})
    delnpc = PhonyType(RString, 'npc', 'npc_delsrcfile(npc);', {npc_h})


    # options
    login_lan_conf.opt('lan_char_ip', IP4Address, 'IP4_LOCALHOST')
    login_lan_conf.opt('lan_subnet', IP4Mask, 'IP4Mask(IP4_LOCALHOST, IP4_BROADCAST)')

    login_conf.opt('admin_state', bool, 'false')
    login_conf.opt('admin_pass', AccountPass, '{}')
    login_conf.opt('ladminallowip', IpSet, '{}')
    login_conf.opt('new_account', bool, 'false')
    login_conf.opt('login_port', u16, '6901', min='1024')
    login_conf.opt('account_filename', RString, lit('save/account.txt'))
    login_conf.opt('gm_account_filename', RString, lit('save/gm_account.txt'))
    login_conf.opt('gm_account_filename_check_timer', seconds, '15_s')
    login_conf.opt('login_log_filename', RString, lit('log/login.log'))
    login_conf.opt('display_parse_login', bool, 'false')
    login_conf.opt('display_parse_admin', bool, 'false')
    login_conf.opt('display_parse_fromchar', i32, '0', min='0', max='2')
    login_conf.opt('min_level_to_connect', GmLevel, 'GmLevel::from(0_u32)', {udl_h})
    login_conf.opt('order', ACO, 'ACO::DENY_ALLOW')
    login_conf.opt('allow', IpSet, '{}')
    login_conf.opt('deny', IpSet, '{}')
    login_conf.opt('anti_freeze_enable', bool, 'false')
    login_conf.opt('anti_freeze_interval', seconds, '15_s')
    login_conf.opt('update_host', RString, '{}')
    login_conf.opt('main_server', ServerName, '{}')
    login_conf.opt('userid', AccountName, '{}')
    login_conf.opt('passwd', AccountPass, '{}')


    admin_conf.opt('login_ip', IP4Address, 'IP4_LOCALHOST')
    admin_conf.opt('login_port', u16, '6901', min='1024')
    admin_conf.opt('admin_pass', AccountPass, 'stringish<AccountPass>("admin"_s)')
    admin_conf.opt('ladmin_log_filename', RString, lit('log/ladmin.log'))


    char_lan_conf.opt('lan_map_ip', IP4Address, 'IP4_LOCALHOST')
    char_lan_conf.opt('lan_subnet', IP4Mask, 'IP4Mask(IP4_LOCALHOST, IP4_BROADCAST)')

    char_conf.opt('userid', AccountName, '{}')
    char_conf.opt('passwd', AccountPass, '{}')
    char_conf.opt('server_name', ServerName, '{}')
    char_conf.opt('login_ip', IP4Address, '{}')
    char_conf.opt('login_port', u16, '6901', min='1024')
    char_conf.opt('char_ip', IP4Address, '{}')
    char_conf.opt('char_port', u16, '6121', min='1024')
    char_conf.opt('char_txt', RString, '{}')
    char_conf.opt('max_connect_user', u32, '0')
    char_conf.opt('autosave_time', seconds, 'DEFAULT_AUTOSAVE_INTERVAL', {char_h}, min='1_s')
    char_conf.opt('start_point', Point, '{ {"001-1.gat"_s}, 273, 354 }')
    char_conf.opt('unknown_char_name', CharName, 'stringish<CharName>("Unknown"_s)')
    char_conf.opt('char_log_filename', RString, lit('log/char.log'))
    char_conf.opt('char_name_letters', CharBitset, '{}')
    char_conf.opt('online_txt_filename', RString, lit('online.txt'))
    char_conf.opt('online_html_filename', RString, lit('online.html'))
    char_conf.opt('online_gm_display_min_level', GmLevel, 'GmLevel::from(20_u32)', {udl_h})
    char_conf.opt('online_refresh_html', u32, '20', min=1)
    char_conf.opt('max_hair_style', u16, '20', min=1)
    char_conf.opt('max_hair_color', u16, '11', min=1)
    char_conf.opt('min_stat_value', u16, '1')
    char_conf.opt('max_stat_value', u16, '9', min=1)
    char_conf.opt('total_stat_sum', u16, '30', min=6)
    char_conf.opt('min_name_length', u16, '4', min=4)
    char_conf.opt('char_slots', u16, '9', min=1)
    char_conf.opt('anti_freeze_enable', bool, 'false')
    char_conf.opt('anti_freeze_interval', seconds, '6_s', min='5_s')

    inter_conf.opt('storage_txt', RString, lit('save/storage.txt'))
    inter_conf.opt('party_txt', RString, lit('save/party.txt'))
    inter_conf.opt('accreg_txt', RString, lit('save/accreg.txt'))
    inter_conf.opt('party_share_level', u32, '10')

    map_conf.opt('userid', AccountName, '{}')
    map_conf.opt('passwd', AccountPass, '{}')
    map_conf.opt('char_ip', IP4Address, '{}')
    map_conf.opt('char_port', u16, '6121', min='1024')
    map_conf.opt('map_ip', IP4Address, '{}')
    map_conf.opt('map_port', u16, '5121', min='1024')
    map_conf.opt('map', addmap, '{}')
    map_conf.opt('delmap', delmap, '{}')
    map_conf.opt('npc', addnpc, '{}')
    map_conf.opt('delnpc', delnpc, '{}')
    map_conf.opt('autosave_time', seconds, 'DEFAULT_AUTOSAVE_INTERVAL', {map_h})
    map_conf.opt('mapreg_txt', RString, lit('save/mapreg.txt'))
    map_conf.opt('gm_log', RString, '{}')
    map_conf.opt('log_file', RString, '{}')

    battle_conf.opt('enemy_critical', bool, 'false')
    battle_conf.opt('enemy_critical_rate', percent, '100')
    battle_conf.opt('enemy_str', bool, 'true')
    battle_conf.opt('enemy_perfect_flee', bool, 'false')
    battle_conf.opt('casting_rate', percent, '100')
    battle_conf.opt('delay_rate', percent, '100')
    battle_conf.opt('delay_dependon_dex', bool, 'false')
    battle_conf.opt('skill_delay_attack_enable', bool, 'false')
    battle_conf.opt('monster_skill_add_range', i32, '0')
    battle_conf.opt('player_damage_delay', bool, '1')
    battle_conf.opt('flooritem_lifetime', milliseconds, 'LIFETIME_FLOORITEM', min='1_s')
    battle_conf.opt('item_auto_get', bool, 'false')
    battle_conf.opt('item_first_get_time', milliseconds, '3_s')
    battle_conf.opt('item_second_get_time', milliseconds, '1_s')
    battle_conf.opt('item_third_get_time', milliseconds, '1_s')
    battle_conf.opt('base_exp_rate', percent, '100')
    battle_conf.opt('job_exp_rate', percent, '100')
    battle_conf.opt('death_penalty_type', i32, '0', min='0', max='2')
    battle_conf.opt('death_penalty_base', per10kd, '0')
    battle_conf.opt('death_penalty_job', per10kd, '0')
    battle_conf.opt('restart_hp_rate', percent, '0', min='0', max='100')
    battle_conf.opt('restart_sp_rate', percent, '0', min='0', max='100')
    battle_conf.opt('monster_hp_rate', percent, '0')
    battle_conf.opt('monster_max_aspd', milliseconds, '199_ms', pre='conf.monster_max_aspd = 2000_ms - conf.monster_max_aspd * 10;', min='10_ms', max='1000_ms')
    battle_conf.opt('atcommand_gm_only', bool, '0')
    battle_conf.opt('atcommand_spawn_quantity_limit', i32, '{}')
    battle_conf.opt('gm_all_equipment', GmLevel, 'GmLevel::from(0_u32)', {udl_h})
    battle_conf.opt('monster_active_enable', bool, 'true')
    battle_conf.opt('mob_skill_use', bool, 'true')
    battle_conf.opt('mob_count_rate', percent, '100')
    battle_conf.opt('basic_skill_check', bool, 'true')
    battle_conf.opt('player_invincible_time', milliseconds, '5_s')
    battle_conf.opt('player_pvp_time', milliseconds, '5_s')
    battle_conf.opt('skill_min_damage', bool, 'false')
    battle_conf.opt('natural_healhp_interval', milliseconds, '6_s', {map_h}, min='NATURAL_HEAL_INTERVAL')
    battle_conf.opt('natural_healsp_interval', milliseconds, '8_s', {map_h}, min='NATURAL_HEAL_INTERVAL')
    battle_conf.opt('natural_heal_weight_rate', percent, '50', min='50', max='101')
    battle_conf.opt('arrow_decrement', bool, 'true')
    battle_conf.opt('max_aspd', milliseconds, '199_ms', pre='conf.max_aspd = 2000_ms - conf.max_aspd * 10;', min='10_ms', max='1000_ms')
    battle_conf.opt('max_hp', i32, '32500', min='100', max='1000000')
    battle_conf.opt('max_sp', i32, '32500', min='100', max='1000000')
    battle_conf.opt('max_lv', i32, '99')
    battle_conf.opt('max_parameter', i32, '99', min='10', max='10000')
    battle_conf.opt('monster_skill_log', bool, 'false')
    battle_conf.opt('battle_log', bool, 'false')
    battle_conf.opt('save_log', bool, 'false')
    battle_conf.opt('error_log', bool, 'true')
    battle_conf.opt('etc_log', bool, 'true')
    battle_conf.opt('save_clothcolor', bool, 'false')
    battle_conf.opt('undead_detect_type', i32, '0', min='0', max='2')
    battle_conf.opt('agi_penaly_type', i32, '0', min='0', max='2')
    battle_conf.opt('agi_penaly_count', i32, '3', min='2')
    battle_conf.opt('agi_penaly_num', i32, '0')
    battle_conf.opt('vit_penaly_type', i32, '0', min='0', max='2')
    battle_conf.opt('vit_penaly_count', i32, '3', min='2')
    battle_conf.opt('vit_penaly_num', i32, '0')
    battle_conf.opt('mob_changetarget_byskill', bool, 'false')
    battle_conf.opt('player_attack_direction_change', bool, 'true')
    battle_conf.opt('monster_attack_direction_change', bool, 'true')
    battle_conf.opt('display_delay_skill_fail', bool, 'true')
    battle_conf.opt('prevent_logout', bool, 'true')
    battle_conf.opt('alchemist_summon_reward', bool, '{}')
    battle_conf.opt('maximum_level', i32, '255')
    battle_conf.opt('drops_by_luk', percent, '0')
    battle_conf.opt('monsters_ignore_gm', bool, '{}')
    battle_conf.opt('multi_level_up', bool, 'false')
    battle_conf.opt('pk_mode', bool, 'false')
    battle_conf.opt('agi_penaly_count_lv', ATK, 'ATK::FLEE')
    battle_conf.opt('vit_penaly_count_lv', ATK, 'ATK::DEF')
    battle_conf.opt('hide_GM_session', bool, 'false')
    battle_conf.opt('invite_request_check', bool, 'true')
    battle_conf.opt('disp_experience', bool, '0')
    battle_conf.opt('hack_info_GM_level', GmLevel, 'GmLevel::from(60_u32)', {udl_h})
    battle_conf.opt('any_warp_GM_min_level', GmLevel, 'GmLevel::from(20_u32)', {udl_h})
    battle_conf.opt('min_hair_style', i32, '0')
    battle_conf.opt('max_hair_style', i32, '20')
    battle_conf.opt('min_hair_color', i32, '0')
    battle_conf.opt('max_hair_color', i32, '9')
    battle_conf.opt('min_cloth_color', i32, '0')
    battle_conf.opt('max_cloth_color', i32, '4')
    battle_conf.opt('castrate_dex_scale', i32, '150')
    battle_conf.opt('area_size', i32, '14')
    battle_conf.opt('chat_lame_penalty', i32, '2')
    battle_conf.opt('chat_spam_threshold', seconds, '10_s', min='0_s', max='32767_s')
    battle_conf.opt('chat_spam_flood', i32, '10', min='0', max='32767')
    battle_conf.opt('chat_spam_ban', hours, '1_h', min='0_h', max='32767_h')
    battle_conf.opt('chat_spam_warn', i32, '8', min='0', max='32767')
    battle_conf.opt('chat_maxline', i32, '255', min='1', max='512')
    battle_conf.opt('packet_spam_threshold', seconds, '2_s', min='0_s', max='32767_s')
    battle_conf.opt('packet_spam_flood', i32, '30', min='0', max='32767')
    battle_conf.opt('packet_spam_kick', bool, 'true')
    battle_conf.opt('mask_ip_gms', bool, 'true')
    battle_conf.opt('drop_pickup_safety_zone', i32, '20')
    battle_conf.opt('itemheal_regeneration_factor', i32, '1')
    battle_conf.opt('mob_splash_radius', i32, '-1', min='-1')

    return rv

def main():
    cfg = build_config()
    cfg.dump()

if __name__ == '__main__':
    main()
