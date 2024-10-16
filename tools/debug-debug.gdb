# vim: ft=python
# set auto-load safe-path /
python
try:
    gdb.execute('set auto-load safe-path /')
except:
    pass
gdb.execute('file %s' % file_to_load)
end
set logging file /dev/null
set logging redirect on
set logging enabled off

python
import re
import sys

def hit_breakpoint():
    sys.stdout.write('.')
    value = str(gdb.parse_and_eval('*&value'))
    expected = gdb.parse_and_eval('expected').string()
    if False and expected.startswith('regex:'):
        def compare(value, expected):
            m = re.match(expected[6:], value)
            return m and m.end() == m.endpos
    else:
        def compare(value, expected):
            return value == expected
    if not compare(value, expected):
        print('Error: mismatch, aborting ...')
        print('actual: %r' % value)
        print('expect: %r' % str(expected))
        gdb.execute('bt')
        sys.exit(1)
end

# register a pretty-printer for 'char *' instead
#set print address off
set print static-members off
set print elements 9999
set print frame-arguments none
set python print-stack full

set logging enabled on
# Workaround "Function... not defined in.." (breakpoints not found) (GDB bug)
#   https://sourceware.org/bugzilla/show_bug.cgi?id=15962
# In some gdb versions rbreak works, in some break does.
# This code should work for any.
python
bpoint = gdb.Breakpoint("do_breakpoint")

if bpoint.pending:
    print("`break ...` found no breakpoints, trying `rbreak ...`")
    bpoint.delete()
    gdb.execute("rbreak do_breakpoint")

end
set logging enabled off

commands
silent
python hit_breakpoint()
continue
end
run
