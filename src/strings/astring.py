class AString(object):
    ''' print an AString
    '''
    __slots__ = ('_value')
    name = 'strings::AString'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return None

    def children(self):
        b = self._value['data']
        s = self._value['special']
        if s == 255:
            b = b.cast(gdb.lookup_type('strings::RString').pointer())
            yield 'allocated', b.dereference()
        else:
            b = b.cast(b.type.target().pointer())
            n = 255
            d = n - s
            yield 'contained', b.lazy_string(length=d)
