class VString(object):
    ''' print a VString
    '''
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
