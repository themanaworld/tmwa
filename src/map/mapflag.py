class MapFlags(object):
    ''' print a set of map flags
    '''
    __slots__ = ('_value')
    name = 'MapFlags'
    enabled = True

    def __init__(self, value):
        self._value = value['flags']

    def to_string(self):
        i = int(self._value)
        s = []
        for n, v in [
            ('ALIAS', 21),
            ('NOMEMO', 0),
            ('NOTELEPORT', 1),
            ('NORETURN', 22),
            ('MONSTER_NOTELEPORT', 23),
            ('NOSAVE', 2),
            ('NOBRANCH', 3),
            ('NOPENALTY', 4),
            ('PVP', 6),
            ('PVP_NOPARTY', 7),
            ('PVP_NOGUILD', 8),
            ('PVP_NIGHTMAREDROP', 24),
            ('PVP_NOCALCRANK', 25),
            ('GVG', 9),
            ('GVG_NOPARTY', 10),
            ('NOZENYPENALTY', 5),
            ('NOTRADE', 11),
            ('NOSKILL', 12),
            ('NOWARP', 13),
            ('NOWARPTO', 26),
            ('NOPVP', 14),
            ('NOICEWALL', 15),
            ('SNOW', 16),
            ('FOG', 17),
            ('SAKURA', 18),
            ('LEAVES', 19),
            ('RAIN', 20),
            ('NO_PLAYER_DROPS', 27),
            ('TOWN', 28),
            ('OUTSIDE', 29),
            ('RESAVE', 30),
        ]:
            v = 1 << v
            if i & v:
                i -= v
                s.append(n)
        if i or not s:
            s.append('%#08x' % i)
        return 'MapFlags(%s)' % (' | '.join(s))
