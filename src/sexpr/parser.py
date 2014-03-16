class SExpr(object):
    ''' print a SExpr
    '''
    __slots__ = ('_value')
    name = 'sexpr::SExpr'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return None

    def children(self):
        v = self._value
        t = v['_type']
        if t == 0:
            yield '(list)', v['_list']
        if t == 1:
            yield '(int)', v['_int']
        if t == 2:
            yield '(str)', v['_str']
        if t == 3:
            yield '(token)', v['_str']
        yield '_span', v['_span']
