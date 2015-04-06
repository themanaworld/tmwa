for s in [
        'Species',
        'AccountId',
        'CharId',
        'PartyId',
        'ItemNameId',
        'BlockId',
        'QuestId',
]:
    class OtherId(object):
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

        tests = [
                ('tmwa::wrap<tmwa::%s>(123)' % s, '123'),
        ]
    globals()[s] = OtherId
    del OtherId

class GmLevel(object):
    pass
