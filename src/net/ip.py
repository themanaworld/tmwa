class IP4Address(object):
    ''' print an IP4Address
    '''
    __slots__ = ('_value')
    name = 'IP4Address'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        addr = self._value['_addr']
        addr = tuple(int(addr[i]) for i in range(4))
        return '%d.%d.%d.%d' % addr
