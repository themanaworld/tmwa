class RString(object):
    ''' print a RString
    '''
    __slots__ = ('_value')
    name = 'strings::RString'
    enabled = True

    def __init__(self, value):
        self._value = value

    def children(self):
        yield 'count', self._value['owned'].dereference()['count']

    def to_string(self):
        r = self._value['owned'].dereference()
        b = r['body']
        b = b.cast(b.type.target().pointer())
        d = r['size']
        return b.lazy_string(length=d)
