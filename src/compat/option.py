class Option(object):
    __slots__ = ('_value')
    name = 'tmwa::option::Option'
    enabled = True

    def __init__(self, value):
        self._value = value['repr']

    def to_string(self):
        value = self._value
        ty = value.type.template_argument(0)
        try:
            some = bool(value['_some'])
        except gdb.error:
            stupider = value['stupider']
            if stupider:
                return 'Some<%s>(%s)' % (ty, stupider)
            else:
                return 'None<%s>' % ty
        else:
            if some:
                data = value['_data']
                data = data.address.cast(ty.pointer()).dereference()
                return 'Some<%s>(%s)' % (ty, data)
            else:
                return 'None<%s>' % ty

    test_extra = '''
    static int option_borrow_thingy;
    '''

    tests = [
            ('tmwa::None<int>()', 'None<int>'),
            ('tmwa::Some(1)', 'Some<int>(1)'),
            ('tmwa::Option<tmwa::Borrowed<int>>(tmwa::None)', 'None<tmwa::Borrowed<int>>'),
            ('tmwa::Some(tmwa::borrow(option_borrow_thingy))', 'Some<tmwa::Borrowed<int>>(<option_borrow_thingy>)'),
    ]
