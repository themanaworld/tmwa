class Variant(object):
    __slots__ = ('_value')
    name = 'tmwa::sexpr::Variant'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return None

    def children(self):
        value = self._value
        data = value['data']
        state = value['state']
        ty = value.type.template_argument(state)
        yield '(%s)' % ty, data.address.cast(ty.pointer()).dereference()

    tests = [
            ('tmwa::sexpr::Variant<int, const char *>(42)', '{(int) = 42}'),
            ('tmwa::sexpr::Variant<int, const char *>("Hello, World!")', '{(const char *) = "Hello, World!"}'),
    ]
