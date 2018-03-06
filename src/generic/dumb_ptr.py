class dumb_ptr(object):
    __slots__ = ('_value')
    name = 'tmwa::dumb_ptr'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return '0x%x' % int(self._value['impl'].cast(gdb.parse_and_eval('(long *)0').type))

    def children(self):
        try:
            sz = self._value['sz']
            yield 'sz', sz
        except gdb.error:
            pass

    tests = [
            ('tmwa::dumb_ptr<int>()', '0x0'),
            ('tmwa::dumb_ptr<int[]>()', '0x0 = {sz = 0}'),
    ]
