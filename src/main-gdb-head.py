# Work around awkwardness in gdb's python printers:
# 1. In src/main-gdb-head.py, define the printer mechanism.
# 2. In src/*/*.py, define all the printer classes.
# 3. In src/main-gdb-tail.py, reflect to actually add the printers.

# gdb sticks everything in one scope.
# This lets us enumerate what *we* added.
initial_globals = {id(v):v for v in list(globals().values())}

import re

# copied from gdb/types.py for compatibility with old gdb
def get_basic_type(type_):
    """Return the "basic" type of a type.

    Arguments:
        type_: The type to reduce to its basic type.

    Returns:
        type_ with const/volatile is stripped away,
        and typedefs/references converted to the underlying type.
    """

    while (type_.code == gdb.TYPE_CODE_REF or
           type_.code == gdb.TYPE_CODE_TYPEDEF):
        if type_.code == gdb.TYPE_CODE_REF:
            type_ = type_.target()
        else:
            type_ = type_.strip_typedefs()
    return type_.unqualified()

def info_symbol(addr):
    ''' returns (symbol, offset, section, lib or None) or None?
    '''
    info = gdb.execute('info symbol %d' % addr, to_string=True)
    try:
        sym_and_off, sec_and_lib = info.split(' in section ')
    except ValueError:
        return None
    try:
        sym, off = sym_and_off.split(' + ')
    except ValueError:
        sym = sym_and_off
        off = 0
    else:
        off = int(off, 10)
    try:
        sec, lib = sec_and_lib.split(' of ')
    except ValueError:
        sec = sec_and_lib
        lib = None
    return (sym, off, sec, lib)

def finish():
    global finish, initial_globals, FastPrinters, EnumPrinter, PointerPrinter

    final_globals = {id(v):v for v in globals().values()}
    diff = set(final_globals.keys()) - set(initial_globals.keys()) \
            - {
                    'finish',
                    'initial_globals',
                    'FastPrinters',
                    'EnumPrinter',
                    'PointerPrinter',
            }
    fp = FastPrinters()
    ep = EnumPrinter
    ptrp = PointerPrinter

    # After this, don't access any more globals in this function.
    del finish, initial_globals, FastPrinters, EnumPrinter, PointerPrinter

    for i in diff:
        v = final_globals[i]
        if hasattr(v, 'children') or hasattr(v, 'to_string'):
            fp.add_printer(v)
        # TODO see if there's a way to detect the top-level printers too
        # the problem is that some of them collide and the order *is*
        # important, but sets and dicts don't preserve order.
        # Particularly, 'PointerPrinter' must come after 'FastPrinters'.

    obj = gdb.current_objfile()
    if obj is None:
        obj = gdb
        filename = '<unknown>'
    else:
        filename = obj.filename
    obj.pretty_printers.append(fp)
    n = len(obj.pretty_printers)
    obj.pretty_printers.append(ep)
    obj.pretty_printers.append(ptrp)
    n = len(obj.pretty_printers) - n
    print('Added %d+%d custom printers for %s'
            % (len(fp.printers), n, filename))

class EnumPrinter(object):
    __slots__ = ('_value')
    name = 'enum-class'
    enabled = True

    def __new__(cls, v):
        type = get_basic_type(v.type)
        if type.code != gdb.TYPE_CODE_ENUM:
            return None
        return object.__new__(cls)

    def __init__(self, v):
        self._value = v

    def to_string(self):
        v = self._value
        self.__class__.enabled = False
        try:
            name = str(v)
        finally:
            self.__class__.enabled = True
        name = name.split('::')[-1]
        scope = get_basic_type(v.type).tag
        return '%s::%s' % (scope, name)

class PointerPrinter(object):
    __slots__ = ('_value')
    name = 'any-symbol-pointer'
    enabled = True

    def __new__(cls, v):
        type = get_basic_type(v.type)
        if type.code != gdb.TYPE_CODE_PTR:
            return None
        return object.__new__(cls)

    def __init__(self, v):
        self._value = v

    def to_string(self):
        v = self._value
        uptr = gdb.lookup_type('uintptr_t')
        addr = int(v.cast(uptr))
        if not addr:
            s = 'nullptr'
        else:
            try:
                sym, off, sec, lib = info_symbol(addr)
            except TypeError:
                sp = gdb.parse_and_eval('$sp')
                sp = int(sp.cast(uptr))
                LOTS = 8 * 1024 * 1024
                diff = addr - sp
                if +diff >= 0 and +diff <= LOTS:
                    a = '<$sp+0x%x>' % +diff
                elif -diff >= 0 and -diff <= LOTS:
                    a = '<$sp-0x%x>' % -diff
                else:
                    a = '<heap 0x%x>' % addr
                s = '(%s)%s' % (v.type, a)
            else:
                if off:
                    s = '<%s+%d>' % off
                else:
                    s = '<%s>' % sym
        return s


class FastPrinters(object):
    ''' printer dispatch the way gdb *should* have done it
    '''
    __slots__ = ('name', 'enabled', 'printers')

    def __init__(self):
        self.name = 'tmwa'
        self.enabled = True
        self.printers = {}

    def add_printer(self, cls):
        assert hasattr(cls, 'enabled')
        # TODO: check if the class name exists
        # this is really hard since templates are involved
        self.printers[(cls.name, getattr(cls, 'depth', 0))] = cls

    @property
    def subprinters(self):
        return list(self.printers.values())

    def strip_templates(self, name, __pattern=re.compile('<[^<>]*>')):
        # TODO what about '<' and '>' as non-type template parameters?
        changed = 1
        while changed:
            name, changed = __pattern.subn('', name)
        return name

    def get_tag_and_depth(self, type):
        depth = 0
        while True:
            type = get_basic_type(type)
            if type.code != gdb.TYPE_CODE_PTR:
                break
            type = type.target()
            depth += 1
        return (str(type), depth)

    def __call__(self, value):
        (stype, depth) = self.get_tag_and_depth(value.type)
        #(dtype, _) = self.get_tag_and_depth(value.dynamic_type)
        if stype is None:
            return

        stype = self.strip_templates(stype)
        p = self.printers.get((stype, depth))
        if p is not None and p.enabled:
            return p(value)
        return None

class char(object):
    __slots__ = ('_value')
    name = 'char'
    depth = 1
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return self._value.lazy_string()
