# Work around awkwardness in gdb's python printers:
# 1. In src/main-gdb-head.py, define the printer mechanism.
# 2. In src/*/*.py, define all the printer classes.
# 3. In src/main-gdb-tail.py, reflect to actually add the printers.

# gdb sticks everything in one scope.
# This lets us enumerate what *we* added.
initial_globals = set(globals())

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

def finish():
    global finish, initial_globals, FastPrinters

    diff = set(globals()) - initial_globals \
            - {'finish', 'initial_globals', 'FastPrinters'}
    fp = FastPrinters()

    # After this, don't access any more globals in this function.
    del finish, initial_globals, FastPrinters

    for k in diff:
        v = globals()[k]
        if hasattr(v, 'children') or hasattr(v, 'to_string'):
            fp.add_printer(v)

    gdb.current_objfile().pretty_printers.append(fp)
    print('Added %d custom printers for %s'
            % (len(fp.printers), gdb.current_objfile().filename))

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
        stype = get_basic_type(value.type).tag
        #dtype = get_basic_type(value.dynamic_type).tag
        if stype is None:
            return

        stype = self.strip_templates(stype)
        p = self.printers.get(stype)
        if p is not None and p.enabled:
            return p(value)
        return None
