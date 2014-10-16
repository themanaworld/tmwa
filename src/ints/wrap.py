class Wrapped(object):
    __slots__ = ('_value')
    name = 'tmwa::ints::wrapped::Wrapped'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return self._value['_value']

    test_extra = '''
    void do_breakpoint();
    void do_breakpoint() {}
    '''

    # tests are in src/mmo/ids.py instead
    tests = []
