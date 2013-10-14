# Work around awkwardness in gdb's python printers:
# 1. In src/main-gdb-head.py, define the printer mechanism.
# 2. In src/*/*.py, define all the printer classes.
# 3. In src/main-gdb-tail.py, reflect to actually add the printers.

# gdb sticks everything in one scope.
# This lets us enumerate what *we* added.
initial_globals = set(globals())

def finish():
    diff = set(globals()) - initial_globals
    fp = FastPrinters()
    # After this, don't access any more globals in this function.
    global finish, initial_globals, FastPrinters
    del finish, initial_globals, FastPrinters

    for k in diff:
        v = globals()[k]
        if hasattr(v, 'children') or hasattr(v, 'to_string'):
            fp.add_printer(v)

    gdb.current_objfile().pretty_printers.append(fp)

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
        self.printers[cls.name] = cls

    @property
    def subprinters(self):
        return self.printers.values()

    def strip_templates(self, name, __pattern=re.compile('<[^<>]>')):
        # TODO what about '<' and '>' as non-type template parameters?
        changed = 1
        while changed:
            name, changed = __pattern.subn('', name)
        return name

    def __call__(self, value):
        stype = gdb.types.get_basic_type(value.type).tag
        #dtype = gdb.types.get_basic_type(value.dynamic_type).tag
        if stype is None:
            return

        stype = self.strip_templates(stype)
        p = self.printers.get(stype)
        if p is not None and p.enabled:
            return p(value)
        return None
