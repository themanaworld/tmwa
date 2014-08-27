class AString(object):
    __slots__ = ('_value')
    name = 'tmwa::strings::AString'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return None

    def children(self):
        b = self._value['data']
        s = self._value['special']
        b = b[0].address
        if s == 255:
            b = b.cast(gdb.lookup_type('tmwa::strings::RString').pointer())
            yield 'allocated', b.dereference()
        else:
            n = 255
            d = n - s
            yield 'contained', b.lazy_string(length=d)

    str256 = '0123456789abcdef' * 16

    test_extra = '''
    using tmwa::operator "" _s;
    #include "../src/strings/zstring.hpp"
    '''

    tests = [
            ('tmwa::AString(""_s)', '{allocated = ""}'),
            ('tmwa::AString(tmwa::ZString(""_s))', '{allocated = ""}'),
            ('tmwa::AString(tmwa::RString(""_s))', '{allocated = ""}'),
            ('tmwa::AString(tmwa::RString(tmwa::ZString(""_s)))', '{allocated = ""}'),
            ('tmwa::AString("Hello"_s)', '{allocated = "Hello"}'),
            ('tmwa::AString(tmwa::ZString("Hello"_s))', '{contained = "Hello"}'),
            ('tmwa::AString(tmwa::RString("Hello"_s))', '{allocated = "Hello"}'),
            ('tmwa::AString(tmwa::RString(tmwa::ZString("Hello"_s)))', '{allocated = "Hello" = {count = 0}}'),
            ('tmwa::AString("' + str256[:-2] + '"_s)', '{allocated = "' + str256[:-2] + '"}'),
            ('tmwa::AString(tmwa::ZString("' + str256[:-2] + '"_s))', '{contained = "' + str256[:-2] + '"}'),
            ('tmwa::AString(tmwa::RString("' + str256[:-2] + '"_s))', '{allocated = "' + str256[:-2] + '"}'),
            ('tmwa::AString(tmwa::RString(tmwa::ZString("' + str256[:-2] + '"_s)))', '{allocated = "' + str256[:-2] + '" = {count = 0}}'),
            ('tmwa::AString("' + str256[:-1] + '"_s)', '{allocated = "' + str256[:-1] + '"}'),
            ('tmwa::AString(tmwa::ZString("' + str256[:-1] + '"_s))', '{contained = "' + str256[:-1] + '"}'),
            ('tmwa::AString(tmwa::RString("' + str256[:-1] + '"_s))', '{allocated = "' + str256[:-1] + '"}'),
            ('tmwa::AString(tmwa::RString(tmwa::ZString("' + str256[:-1] + '"_s)))', '{allocated = "' + str256[:-1] + '" = {count = 0}}'),
            ('tmwa::AString("' + str256 + '"_s)', '{allocated = "' + str256 + '"}'),
            ('tmwa::AString(tmwa::ZString("' + str256 + '"_s))', '{allocated = "' + str256 + '" = {count = 0}}'),
            ('tmwa::AString(tmwa::RString("' + str256 + '"_s))', '{allocated = "' + str256 + '"}'),
            ('tmwa::AString(tmwa::RString(tmwa::ZString("' + str256 + '"_s)))', '{allocated = "' + str256 + '" = {count = 0}}'),
            ('tmwa::AString("' + str256 + 'x"_s)', '{allocated = "' + str256 + 'x"}'),
            ('tmwa::AString(tmwa::ZString("' + str256 + 'x"_s))', '{allocated = "' + str256 + 'x" = {count = 0}}'),
            ('tmwa::AString(tmwa::RString("' + str256 + 'x"_s))', '{allocated = "' + str256 + 'x"}'),
            ('tmwa::AString(tmwa::RString(tmwa::ZString("' + str256 + 'x"_s)))', '{allocated = "' + str256 + 'x" = {count = 0}}'),
    ]
