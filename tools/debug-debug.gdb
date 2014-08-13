# vim: ft=python
set logging file /dev/null
set logging redirect on
set logging off

python
import sys

def hit_breakpoint():
    sys.stdout.write('.')
    value = str(gdb.parse_and_eval('*&value'))
    expected = gdb.parse_and_eval('expected').string()
    if value != expected:
        print 'Error: mismatch, aborting ...'
        print 'actual: %r' % value
        print 'expect: %r' % str(expected)
        gdb.execute('bt')
        sys.exit(1)
end

# register a pretty-printer for 'char *' instead
#set print address off
set print static-members off
set print elements unlimited
set print frame-arguments none

set logging on
rbreak do_breakpoint
set logging off
commands
silent
python hit_breakpoint()
continue
end
run
