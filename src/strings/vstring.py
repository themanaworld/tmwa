class VString(object):
    __slots__ = ('_value')
    name = 'tmwa::strings::VString'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        b = self._value['_data']
        b = b.cast(b.type.target().pointer())
        n = self._value.type.template_argument(0)
        s = self._value['_special']
        d = n - s
        return b.lazy_string(length=d)

    str256 = '0123456789abcdef' * 16

    test_extra = '''
    using tmwa::operator "" _s;
    '''

    tests = [
            ('tmwa::VString<255>(""_s)', '""'),
            ('tmwa::VString<255>("Hello"_s)', '"Hello"'),
            ('tmwa::VString<255>("' + str256[:-2] + '"_s)', '"' + str256[:-2] + '"'),
            ('tmwa::VString<255>("' + str256[:-1] + '"_s)', '"' + str256[:-1] + '"'),
            ('tmwa::VString<255>("' + str256 + '"_s)', '"' + str256[:-1] + '"'),
            ('tmwa::VString<255>("' + str256 + 'x"_s)', '"' + str256[:-1] + '"'),
    ]
