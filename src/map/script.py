class ByteCode:
    ''' print a ByteCode
        (workaround for gcc bug 58150)
    '''
    __slots__ = ('_value')
    name = 'ByteCode'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        val = int(self._value)
        try:
            return 'ByteCode::' + self.types[val]
        except IndexError:
            return 'ByteCode(%x)' % val

    types = [
        'NOP', 'POS', 'INT', 'PARAM', 'FUNC', 'STR', 'CONSTSTR', 'ARG',
        'VARIABLE', 'EOL', 'RETINFO',

        'LOR', 'LAND', 'LE', 'LT', 'GE', 'GT', 'EQ', 'NE',
        'XOR', 'OR', 'AND', 'ADD', 'SUB', 'MUL', 'DIV', 'MOD',
        'NEG', 'LNOT', 'NOT', 'R_SHIFT', 'L_SHIFT',

        'FUNC_REF',
    ]
for i, n in enumerate(ByteCode.types):
    setattr(ByteCode, n, i)
del i, n

class script_data(object):
    ''' print a script_data
    '''
    __slots__ = ('_value')
    name = 'script_data'
    enabled = True

    def __init__(self, value):
        self._value = value

    def children(self):
        v = self._value
        t = v['type']
        yield 'type', ByteCode(t).to_string() # why does this not work?
        v = v['u']
        t = int(t)
        if t == ByteCode.PARAM:
            yield 'reg', v['reg']
        elif t == ByteCode.RETINFO:
            yield 'script', v['script']
        elif t in (ByteCode.STR, ByteCode.CONSTSTR):
            yield 'str', v['str']
        else:
            yield 'numi', v['numi']
