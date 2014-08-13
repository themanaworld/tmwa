class RString(object):
    __slots__ = ('_value')
    name = 'tmwa::strings::RString'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        v = self._value
        e = v['maybe_end']
        if e:
            b = v['u']['begin']
            d = e - b
            return b.lazy_string(length=d)
        else:
            r = v['u']['owned'].dereference()
            b = r['body']
            b = b[0].address
            d = r['size']
            return b.lazy_string(length=d)

    def children(self):
        v = self._value
        if v['maybe_end']:
            pass
        else:
            yield 'count', v['u']['owned'].dereference()['count']

    str256 = '0123456789abcdef' * 16

    tests = [
            ('tmwa::RString(""_s)', '""'),
            ('tmwa::RString(tmwa::ZString(""_s))', '""'),
            ('tmwa::RString("Hello"_s)', '"Hello"'),
            ('tmwa::RString(tmwa::ZString("Hello"_s))', '"Hello" = {count = 0}'),
            ('tmwa::RString("' + str256[:-2] + '"_s)', '"' + str256[:-2] + '"'),
            ('tmwa::RString(tmwa::ZString("' + str256[:-2] + '"_s))', '"' + str256[:-2] + '" = {count = 0}'),
            ('tmwa::RString("' + str256[:-1] + '"_s)', '"' + str256[:-1] + '"'),
            ('tmwa::RString(tmwa::ZString("' + str256[:-1] + '"_s))', '"' + str256[:-1] + '" = {count = 0}'),
            ('tmwa::RString("' + str256 + '"_s)', '"' + str256 + '"'),
            ('tmwa::RString(tmwa::ZString("' + str256 + '"_s))', '"' + str256 + '" = {count = 0}'),
            ('tmwa::RString("' + str256 + 'x"_s)', '"' + str256 + 'x"'),
            ('tmwa::RString(tmwa::ZString("' + str256 + 'x"_s))', '"' + str256 + 'x" = {count = 0}'),
    ]
