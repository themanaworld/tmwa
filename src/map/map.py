class map_local(object):
    __slots__ = ('_value')

    name = 'tmwa::map_local'
    depth = 1
    enabled = True

    def __init__(self, value):
        if not value:
            value = None
        self._value = value

    def to_string(self):
        value = self._value
        if value is None:
            return '(map_local *) nullptr'
        return '(map_local *)'

    def children(self):
        value = self._value
        if value is None:
            return
        value = value.dereference()
        yield '->name', value['name_']
        yield '->xs', value['xs']
        yield '->ys', value['ys']

    tests = [
            ('static_cast<tmwa::map_local *>(nullptr)', '(map_local *) nullptr'),
            ('fake_map_local("map"_s, 42, 404)', '(map_local *) = {->name = "map", ->xs = 42, ->ys = 404}'),
    ]

class map_remote(object):
    __slots__ = ('_value')

    name = 'tmwa::map_remote'
    depth = 1
    enabled = True

    def __init__(self, value):
        if not value:
            value = None
        self._value = value

    def to_string(self):
        value = self._value
        if value is None:
            return '(map_remote *) nullptr'
        return '(map_remote *)'

    def children(self):
        value = self._value
        if value is None:
            return
        value = value.dereference()
        yield '->name', value['name_']
        yield '->ip', value['ip']
        yield '->port', value['port']

    tests = [
            ('static_cast<tmwa::map_remote *>(nullptr)', '(map_remote *) nullptr'),
            ('fake_map_remote("map"_s, tmwa::IP4Address({8, 8, 8, 8}), 6667)', '(map_remote *) = {->name = "map", ->ip = 8.8.8.8, ->port = 6667}'),
    ]

class map_abstract(object):
    __slots__ = ('_value')

    name = 'tmwa::map_abstract'
    depth = 1
    enabled = True

    def __init__(self, value):
        if not value:
            value = None
        self._value = value

    def to_string(self):
        value = self._value
        if value is None:
            return '(map_abstract *) nullptr'
        gat = value.dereference()['gat']
        gat = gat.address.cast(gdb.lookup_type('tmwa::map_abstract').pointer().pointer()).dereference()
        if gat:
            return value.cast(gdb.lookup_type('tmwa::map_local').pointer())
        else:
            return value.cast(gdb.lookup_type('tmwa::map_remote').pointer())

    tests = [
            ('static_cast<tmwa::map_abstract *>(nullptr)', '(map_abstract *) nullptr'),
    ] + [
            ('static_cast<tmwa::map_abstract *>(%s); value->gat.reset(new tmwa::MapCell[1])' % expr, expected)
            for (expr, expected) in map_local.tests[1:]
    ] + [
            ('static_cast<tmwa::map_abstract *>(%s)' % expr, expected)
            for (expr, expected) in map_remote.tests[1:]
    ]

    test_extra = '''
    using tmwa::operator ""_s;

    inline
    tmwa::map_local *fake_map_local(tmwa::ZString name, int xs, int ys)
    {
        auto *p = new tmwa::map_local{};
        p->name_ = tmwa::stringish<tmwa::MapName>(name);
        p->xs = xs;
        p->ys = ys;
        return p;
    }

    inline
    tmwa::map_remote *fake_map_remote(tmwa::ZString name, tmwa::IP4Address ip, uint16_t port)
    {
        auto *p = new tmwa::map_remote{};
        p->name_ = tmwa::stringish<tmwa::MapName>(name);
        p->ip = ip;
        p->port = port;
        return p;
    }

    void fake_delete(tmwa::map_abstract *);
    void fake_delete(tmwa::map_abstract *map)
    {
        delete map;
    }

    '''
