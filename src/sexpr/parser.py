class SExpr(object):
    __slots__ = ('_value')
    name = 'tmwa::sexpr::SExpr'
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

    test_extra = '''
    #include "../strings/fwd.hpp"
    using tmwa::operator "" _s;
    '''

    tests = [
            ('tmwa::sexpr::SExpr(); value._type = tmwa::sexpr::LIST; value._list = {tmwa::sexpr::SExpr(), tmwa::sexpr::SExpr()}',
                '{(list) = std::vector of length 2, capacity 2 = {{(list) = std::vector of length 0, capacity 0, _span = {begin = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}, end = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}}}, {(list) = std::vector of length 0, capacity 0, _span = {begin = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}, end = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}}}}, _span = {begin = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}, end = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}}}'),
            ('tmwa::sexpr::SExpr(); value._type = tmwa::sexpr::INT; value._int = 42',
                '{(int) = 42, _span = {begin = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}, end = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}}}'),
            ('tmwa::sexpr::SExpr(); value._type = tmwa::sexpr::STRING; value._str = "Hello"_s',
                '{(str) = "Hello", _span = {begin = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}, end = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}}}'),
            ('tmwa::sexpr::SExpr(); value._type = tmwa::sexpr::TOKEN; value._str = "Hello"_s',
                '{(token) = "Hello", _span = {begin = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}, end = {<tmwa::io::Line> = {text = "", filename = "", line = 0, column = 0}, <No data fields>}}}'),
    ]
