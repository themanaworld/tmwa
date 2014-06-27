class area_t(object):
    ''' print an area_t
    '''
    __slots__ = ('_value')
    name = 'tmwa::area_t'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return None

    def children(self):
        v = self._value
        yield 'size', v['size']
        ty = v['ty']
        yield 'ty', ty
        a = v['a']
        if ty == 0:
            yield 'a.a_loc', a['a_loc']
        elif ty == 1:
            yield 'a.a_union', a['a_union']
        elif ty == 2:
            yield 'a.a_rect', a['a_rect']
        elif ty == 3:
            yield 'a.a_bar', a['a_bar']


class val_t(object):
    ''' print a val_t
    '''
    __slots__ = ('_value')
    name = 'tmwa::val_t'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return None

    def children(self):
        v = self._value
        ty = v['ty']
        yield 'ty', ty
        u = v['v']
        if ty == 1:
            yield 'v.v_int', u['v_int']
        elif ty == 2:
            yield 'v.v_dir', u['v_dir']
        elif ty == 3:
            yield 'v.v_string', u['v_string']
        elif ty == 4:
            yield 'v.v_int', u['v_int']
            yield 'v.v_entity', u['v_entity']
        elif ty == 5:
            yield 'v.v_location', u['v_location']
        elif ty == 6:
            yield 'v.v_area', u['v_area']
        elif ty == 7:
            yield 'v.v_spell', u['v_spell']
        elif ty == 8:
            yield 'v.v_int', u['v_int']
            yield 'v.v_invocation', u['v_invocation']


class e_area_t(object):
    ''' print an e_area_t
    '''
    __slots__ = ('_value')
    name = 'tmwa::e_area_t'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return None

    def children(self):
        v = self._value
        ty = v['ty']
        yield 'ty', ty
        a = v['a']
        if ty == 0:
            yield 'a.a_loc', a['a_loc']
        elif ty == 1:
            yield 'a.a_union', a['a_union']
        elif ty == 2:
            yield 'a.a_rect', a['a_rect']
        elif ty == 3:
            yield 'a.a_bar', a['a_bar']


class expr_t(object):
    ''' print an expr_t
    '''
    __slots__ = ('_value')
    name = 'tmwa::expr_t'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return None

    def children(self):
        v = self._value
        ty = v['ty']
        yield 'ty', ty
        u = v['e']
        if ty == 0:
            yield 'e.e_val', u['e_val']
        elif ty == 1:
            yield 'e.e_location', u['e_location']
        elif ty == 2:
            yield 'e.e_area', u['e_area']
        elif ty == 3:
            yield 'e.e_funapp', u['e_funapp']
        elif ty == 4:
            yield 'e.e_id', u['e_id']
        elif ty == 5:
            yield 'e.e_field', u['e_field']


class effect_t(object):
    ''' print an effect_t
    '''
    __slots__ = ('_value')
    name = 'tmwa::effect_t'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return None

    def children(self):
        v = self._value
        yield 'next', v['next']
        ty = v['ty']
        yield 'ty', ty
        u = v['e']
        if ty == 2:
            yield 'e.e_assign', u['e_assign']
        elif ty == 3:
            yield 'e.e_foreach', u['e_foreach']
        elif ty == 4:
            yield 'e.e_for', u['e_for']
        elif ty == 5:
            yield 'e.e_if', u['e_if']
        elif ty == 6:
            yield 'e.e_sleep', u['e_sleep']
        elif ty == 7:
            yield 'e.e_script', u['e_script']
        elif ty == 9:
            yield 'e.e_op', u['e_op']
        elif ty == 11:
            yield 'e.e_call', u['e_call']


class spellguard_t(object):
    ''' print a spellguard_t
    '''
    __slots__ = ('_value')
    name = 'tmwa::spellguard_t'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return None

    def children(self):
        v = self._value
        yield 'next', v['next']
        ty = v['ty']
        yield 'ty', ty
        u = v['s']
        if ty == 0:
            yield 's.s_condition', u['s_condition']
        elif ty == 1:
            yield 's.s_components', u['s_components']
        elif ty == 2:
            yield 's.s_catalysts', u['s_catalysts']
        elif ty == 3:
            yield 's.s_alt', u['s_alt']
        elif ty == 4:
            yield 's.s_mana', u['s_mana']
        elif ty == 5:
            yield 's.s_casttime', u['s_casttime']
        elif ty == 6:
            yield 's.s_effect', u['s_effect']


class cont_activation_record_t(object):
    ''' print a cont_activation_record_t
    '''
    __slots__ = ('_value')
    name = 'tmwa::cont_activation_record_t'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return None

    def children(self):
        v = self._value
        yield 'return_location', v['return_location']
        ty = v['ty']
        yield 'ty', ty
        u = v['c']
        if ty == 0:
            yield 'c.c_foreach', u['c_foreach']
        elif ty == 1:
            yield 'c.c_for', u['c_for']
        elif ty == 2:
            yield 'c.c_proc', u['c_proc']
