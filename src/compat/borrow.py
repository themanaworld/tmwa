class Borrowed(object):
    __slots__ = ('_value')
    name = 'tmwa::Borrowed'
    enabled = True

    def __init__(self, value):
        self._value = value['stupid']

    def to_string(self):
        return self._value

    test_extra = '''
    static int borrow_thingy;
    '''

    tests = [
            ('tmwa::borrow(borrow_thingy)', '<borrow_thingy>'),
    ]
