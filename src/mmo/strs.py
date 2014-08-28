for s in [
        'AccountName',
        'AccountPass',
        'AccountCrypt',
        'AccountEmail',
        'ServerName',
        'PartyName',
        'VarName',
        'MapName',
        'CharName',
]:
    class OtherString(object):
        __slots__ = ('_value')
        name = 'tmwa::%s' % s
        enabled = True

        def __init__(self, value):
            self._value = value

        def to_string(self):
            value = self._value
            fields = value.type.fields()
            field0 = fields[-1]
            if field0.is_base_class:
                w = value.cast(field0.type)
            else:
                w = value[field0.name]
            return '%s' % w

        test_extra = '''
        #include "../strings/fwd.hpp"
        using tmwa::operator "" _s;
        '''

        tests = [
                ('tmwa::stringish<tmwa::%s>("Hello"_s)' % s, '"Hello"'),
        ]
    globals()[s] = OtherString
    del OtherString
