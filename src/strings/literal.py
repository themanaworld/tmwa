class LString(object):
    __slots__ = ('_value')
    name = 'tmwa::strings::LString'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        v = self._value
        b = v['_b']['_ptr']
        e = v['_e']['_ptr']
        d = e - b
        return b.lazy_string(length=d)

    test_extra = '''
    using tmwa::operator "" _s;
    '''

    tests = [
            ('""_s', '""'),
            ('"Hello, World!"_s', '"Hello, World!"'),
    ]
