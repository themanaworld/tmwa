class Wrapped(object):
    __slots__ = ('_value')
    name = 'tmwa::ints::wrapped::Wrapped'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return self._value['_value']

    # tests are in src/mmo/ids.py instead
    tests = []
