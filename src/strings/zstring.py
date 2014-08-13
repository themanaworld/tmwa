class ZString(object):
    __slots__ = ('_value')
    name = 'tmwa::strings::ZString'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        b = self._value['_b']['_ptr']
        e = self._value['_e']['_ptr']
        d = e - b
        return b.lazy_string(length=d)

    def children(self):
        yield 'base', self._value['_base']

    str256 = '0123456789abcdef' * 16

    tests = [
            ('tmwa::ZString(""_s)', '"" = {base = 0x0}'),
            ('tmwa::ZString("Hello"_s)', '"Hello" = {base = 0x0}'),
            ('tmwa::ZString("' + str256[:-2] + '"_s)', '"' + str256[:-2] + '" = {base = 0x0}'),
            ('tmwa::ZString("' + str256[:-1] + '"_s)', '"' + str256[:-1] + '" = {base = 0x0}'),
            ('tmwa::ZString("' + str256 + '"_s)', '"' + str256 + '" = {base = 0x0}'),
            ('tmwa::ZString("' + str256 + 'x"_s)', '"' + str256 + 'x" = {base = 0x0}'),
    ]
