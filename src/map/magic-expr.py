class fun_t(object):
    __slots__ = ('_value')

    name = 'tmwa::magic::fun_t'
    depth = 1
    enabled = True

    def __init__(self, value):
        if not value:
            value = None
        self._value = value

    def to_string(self):
        value = self._value
        if value is None:
            return '(fun_t *) nullptr'
        return '(fun_t *)'

    def children(self):
        value = self._value
        if value is None:
            return
        value = value.dereference()
        yield '->name', value['name']
        yield '->signature', value['signature']
        yield '->ret_ty', value['ret_ty']
        yield '->fun', value['fun']

    test_extra = '''
    using tmwa::operator "" _s;
    '''

    tests = [
            ('static_cast<tmwa::magic::fun_t *>(nullptr)', '(fun_t *) nullptr'),
            ('new tmwa::magic::fun_t{"name"_s, "sig"_s, \'\\0\', nullptr}', '(fun_t *) = {->name = "name", ->signature = "sig", ->ret_ty = 0 \'\\000\', ->fun = 0x0}'),
    ]
