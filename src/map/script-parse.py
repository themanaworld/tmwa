class ScriptBuffer(object):
    __slots__ = ('_value')
    name = 'tmwa::ScriptBuffer'
    enabled = True

    def __init__(self, value):
        self._value = value

    def to_string(self):
        return self._value['debug_name']

    def get_com(self, b, i, r, labels_dict):
        # see script-parse-internal.hpp:ByteCode and script-call.cpp:get_com
        ops = '''
    NOP, POS, INT, PARAM, FUNC, STR, ARG,
    VARIABLE, EOL,
    LOR, LAND, LE, LT, GE, GT, EQ, NE,
    XOR, OR, AND, ADD, SUB, MUL, DIV, MOD,
    NEG, LNOT, NOT, R_SHIFT, L_SHIFT,
    FUNC_REF,
        '''.replace(',', '').split()
        ci = int(b[i])
        if ci >= 0x80:
            rv = 0
            sh = 0
            # Because of how the python iterator up a frame consumed the i already,
            # have to manually unroll the first iteration of the C loop.
            # TODO itertools.chain([i], r) or something?
            if True:
                if True:
                    rv += (ci & 0x7f) << sh
                    sh += 6
            if ci >= 0xc0:
                while True:
                    i = next(r)
                    ci = int(b[i])

                    rv += (ci & 0x7f) << sh
                    sh += 6
                    if not (ci >= 0xc0):
                        break
            return 'INT %d' % rv

        cs = ops[ci]
        if cs in {'POS', 'VARIABLE', 'FUNC_REF', 'PARAM'}:
            ai = 0
            ai |= (int(b[next(r)]) << 0)
            ai |= (int(b[next(r)]) << 8)
            ai |= (int(b[next(r)]) << 16)
            if cs == 'POS':
                ln = labels_dict.get(ai)
                if ln is not None:
                    return 'POS %d %s' % (ai, ln[0])
            elif cs == 'VARIABLE':
                global rstring_disable_children
                rstring_disable_children = True
                try:
                    rv = 'VARIABLE %s' % gdb.parse_and_eval('tmwa::variable_names.names._M_impl._M_start[{ai}]'.format(ai=ai))
                finally:
                    rstring_disable_children = False
                return rv
            elif cs == 'FUNC_REF':
                return 'FUNC_REF %s' % gdb.parse_and_eval('tmwa::builtin_functions[{ai}].name'.format(ai=ai))
            elif cs == 'PARAM':
                # https://sourceware.org/bugzilla/show_bug.cgi?id=17568
                try:
                    # this should work
                    SP = gdb.lookup_type('tmwa::SP')
                except gdb.error:
                    # this should not work
                    SP = gdb.parse_and_eval('tmwa::SP').type
                return 'PARAM %s' % gdb.Value(ai).cast(SP)
            else:
                assert False
            return '%s %d' % (cs, ai)
        elif cs == 'STR':
            buf = bytearray()
            while True:
                i = next(r)
                ci = int(b[i])
                if ci == 0:
                    break
                buf.append(ci)
            return 'STR "%s"' % str(buf).replace('\\', '\\\\').replace('"', '\\"')
        elif cs == 'EOL':
            return cs + '\n'
        return cs

    def children(self):
        labels = self._value['debug_labels']
        labels_begin = labels['_M_impl']['_M_start']
        labels_end = labels['_M_impl']['_M_finish']
        labels_size = int(labels_end - labels_begin)
        labels_list = [labels_begin[i] for i in range(labels_size)]
        labels_dict = {}
        char_ptr = gdb.lookup_type('char').pointer()
        for e in labels_list:
            offset = int(e['second'])
            label_name = str(e['first'].address.cast(char_ptr))
            labels_dict.setdefault(offset, []).append(label_name)
        code = self._value['script_buf']
        code_begin = code['_M_impl']['_M_start']
        code_end = code['_M_impl']['_M_finish']
        code_size = int(code_end - code_begin)
        r = iter(range(code_size))
        for i in r:
            buf = []
            for label in labels_dict.get(i, []):
                if label.startswith('On'):
                    yield 'blah', 'event %s:' % label
                else:
                    yield 'blah', 'label %s:' % label
            c = self.get_com(code_begin, i, r, labels_dict)
            yield 'blah', '%6d: %s' % (i, c)


    def display_hint(self):
        return 'array'

    src = '''
    {
        end;
    S_Sub:
        return;
    OnFoo:
        callsub S_Sub;
        setarray @a,
            -1, 0, 1,
            0x0, 0x1, 0x2,
            0x1, 0x2, 0x3,
            0x3, 0x4, 0x5,
            0x7, 0x8, 0x9,
            0xf, 0x10, 0x11,
            0x1f, 0x20, 0x21,
            0x3f, 0x40, 0x41,
            0x7f, 0x80, 0x81,
            0xff, 0x100, 0x101,
            0x1ff, 0x200, 0x201,
            0x3ff, 0x400, 0x401,
            0x7ff, 0x800, 0x801,
            0xfff, 0x1000, 0x1001,
            0x1fff, 0x2000, 0x2001,
            0x3fff, 0x4000, 0x4001,
            0x7fff, 0x8000, 0x8001,
            0xffff, 0x10000, 0x10001,
            0x1ffff, 0x20000, 0x20001,
            0x3ffff, 0x40000, 0x40001,
            0x7ffff, 0x80000, 0x80001,
            0xfffff, 0x100000, 0x100001,
            0x1fffff, 0x200000, 0x200001,
            0x3fffff, 0x400000, 0x400001,
            0x7fffff, 0x800000, 0x800001,
            0xffffff, 0x1000000, 0x1000001,
            0x1ffffff, 0x2000000, 0x2000001,
            0x3ffffff, 0x4000000, 0x4000001,
            0x7ffffff, 0x8000000, 0x8000001,
            0xfffffff, 0x10000000, 0x10000001,
            0x1fffffff, 0x20000000, 0x20000001,
            0x3fffffff, 0x40000000, 0x40000001,
            0x7fffffff, 0x80000000, 0x80000001,
            0xffffffff;
        set TEST_FAKE_PARAM_BASELEVEL, TEST_FAKE_CONSTANT;
        set @s$, "hello";
        set @i, a || b;
        set @i, a && b;
        set @i, a <= b;
        set @i, a < b;
        set @i, a >= b;
        set @i, a > b;
        set @i, a == b;
        set @i, a != b;
        set @i, a ^ b;
        set @i, a | b;
        set @i, a & b;
        set @i, a + b;
        set @i, a - b;
        set @i, a * b;
        set @i, a / b;
        set @i, a % b;
        set @i, - b;
        set @i, ! b;
        set @i, ~ b;
        set @i, a >> b;
        set @i, a << b;
    }
    '''.replace('\n', ' ')

    asm = ''\
    +'''     0: FUNC_REF "end",
             4: ARG,
             5: FUNC,
             6: EOL
        ,
        label "S_Sub":,
             7: FUNC_REF "return",
            11: ARG,
            12: FUNC,
            13: EOL
        ,
        label "OnFoo":,
            14: FUNC_REF "callsub",
            18: ARG,
            19: POS 7 "S_Sub",
            23: FUNC,
            24: EOL
        ,
            25: FUNC_REF "setarray",
            29: ARG,
            30: VARIABLE "@a",
            34: INT 1,
            35: NEG,
            36: INT 0,
            37: INT 1,
            38: INT 0,
            39: INT 1,
            40: INT 2,
            41: INT 1,
            42: INT 2,
            43: INT 3,
            44: INT 3,
            45: INT 4,
            46: INT 5,
            47: INT 7,
            48: INT 8,
            49: INT 9,
            50: INT 15,
            51: INT 16,
            52: INT 17,
            53: INT 31,
            54: INT 32,
            55: INT 33,
            56: INT 63,
            57: INT 64,
            59: INT 65,
            61: INT 127,
            63: INT 128,
            65: INT 129,
            67: INT 255,
            69: INT 256,
            71: INT 257,
            73: INT 511,
            75: INT 512,
            77: INT 513,
            79: INT 1023,
            81: INT 1024,
            83: INT 1025,
            85: INT 2047,
            87: INT 2048,
            89: INT 2049,
            91: INT 4095,
            93: INT 4096,
            95: INT 4097,
            97: INT 8191,
           100: INT 8192,
           103: INT 8193,
           106: INT 16383,
           109: INT 16384,
           112: INT 16385,
           115: INT 32767,
           118: INT 32768,
           121: INT 32769,
           124: INT 65535,
           127: INT 65536,
           130: INT 65537,
           133: INT 131071,
           136: INT 131072,
           139: INT 131073,
           142: INT 262143,
           145: INT 262144,
           148: INT 262145,
           151: INT 524287,
           155: INT 524288,
           159: INT 524289,
           163: INT 1048575,
           167: INT 1048576,
           171: INT 1048577,
           175: INT 2097151,
           179: INT 2097152,
           183: INT 2097153,
           187: INT 4194303,
           191: INT 4194304,
           195: INT 4194305,
           199: INT 8388607,
           203: INT 8388608,
           207: INT 8388609,
           211: INT 16777215,
           215: INT 16777216,
           219: INT 16777217,
           223: INT 33554431,
           228: INT 33554432,
           233: INT 33554433,
           238: INT 67108863,
           243: INT 67108864,
           248: INT 67108865,
           253: INT 134217727,
           258: INT 134217728,
           263: INT 134217729,
           268: INT 268435455,
           273: INT 268435456,
           278: INT 268435457,
           283: INT 536870911,
           288: INT 536870912,
           293: INT 536870913,
           298: INT 1073741823,
           303: INT 1073741824,
           308: INT 1073741825,
           313: INT 2147483647,
           319: INT 2147483648,
           325: INT 2147483649,
           331: INT 4294967295,
           337: FUNC,
           338: EOL
        ,
           339: FUNC_REF "set",
           343: ARG,
           344: PARAM tmwa::SP::BASELEVEL,
           348: INT 42,
           349: FUNC,
           350: EOL
        ,
           351: FUNC_REF "set",
           355: ARG,
           356: VARIABLE "@s$",
           360: STR "hello",
           367: FUNC,
           368: EOL
        ,
           369: FUNC_REF "set",
           373: ARG,
           374: VARIABLE "@i",
           378: VARIABLE "a",
           382: VARIABLE "b",
           386: LOR,
           387: FUNC,
           388: EOL
        ,
           389: FUNC_REF "set",
           393: ARG,
           394: VARIABLE "@i",
           398: VARIABLE "a",
           402: VARIABLE "b",
           406: LAND,
           407: FUNC,
           408: EOL
        ,
           409: FUNC_REF "set",
           413: ARG,
           414: VARIABLE "@i",
           418: VARIABLE "a",
           422: VARIABLE "b",
           426: LE,
           427: FUNC,
           428: EOL
        ,
           429: FUNC_REF "set",
           433: ARG,
           434: VARIABLE "@i",
           438: VARIABLE "a",
           442: VARIABLE "b",
           446: LT,
           447: FUNC,
           448: EOL
        ,
           449: FUNC_REF "set",
           453: ARG,
           454: VARIABLE "@i",
           458: VARIABLE "a",
           462: VARIABLE "b",
           466: GE,
           467: FUNC,
           468: EOL
        ,
           469: FUNC_REF "set",
           473: ARG,
           474: VARIABLE "@i",
           478: VARIABLE "a",
           482: VARIABLE "b",
           486: GT,
           487: FUNC,
           488: EOL
        ,
           489: FUNC_REF "set",
           493: ARG,
           494: VARIABLE "@i",
           498: VARIABLE "a",
           502: VARIABLE "b",
           506: EQ,
           507: FUNC,
           508: EOL
        ,
           509: FUNC_REF "set",
           513: ARG,
           514: VARIABLE "@i",
           518: VARIABLE "a",
           522: VARIABLE "b",
           526: NE,
           527: FUNC,
           528: EOL
        ,
           529: FUNC_REF "set",
           533: ARG,
           534: VARIABLE "@i",
           538: VARIABLE "a",
           542: VARIABLE "b",
           546: XOR,
           547: FUNC,
           548: EOL
        ,
           549: FUNC_REF "set",
           553: ARG,
           554: VARIABLE "@i",
           558: VARIABLE "a",
           562: VARIABLE "b",
           566: OR,
           567: FUNC,
           568: EOL
        ,
           569: FUNC_REF "set",
           573: ARG,
           574: VARIABLE "@i",
           578: VARIABLE "a",
           582: VARIABLE "b",
           586: AND,
           587: FUNC,
           588: EOL
        ,
           589: FUNC_REF "set",
           593: ARG,
           594: VARIABLE "@i",
           598: VARIABLE "a",
           602: VARIABLE "b",
           606: ADD,
           607: FUNC,
           608: EOL
        ,
           609: FUNC_REF "set",
           613: ARG,
           614: VARIABLE "@i",
           618: VARIABLE "a",
           622: VARIABLE "b",
           626: SUB,
           627: FUNC,
           628: EOL
        ,
           629: FUNC_REF "set",
           633: ARG,
           634: VARIABLE "@i",
           638: VARIABLE "a",
           642: VARIABLE "b",
           646: MUL,
           647: FUNC,
           648: EOL
        ,
           649: FUNC_REF "set",
           653: ARG,
           654: VARIABLE "@i",
           658: VARIABLE "a",
           662: VARIABLE "b",
           666: DIV,
           667: FUNC,
           668: EOL
        ,
           669: FUNC_REF "set",
           673: ARG,
           674: VARIABLE "@i",
           678: VARIABLE "a",
           682: VARIABLE "b",
           686: MOD,
           687: FUNC,
           688: EOL
        ,
           689: FUNC_REF "set",
           693: ARG,
           694: VARIABLE "@i",
           698: VARIABLE "b",
           702: NEG,
           703: FUNC,
           704: EOL
        ,
           705: FUNC_REF "set",
           709: ARG,
           710: VARIABLE "@i",
           714: VARIABLE "b",
           718: LNOT,
           719: FUNC,
           720: EOL
        ,
           721: FUNC_REF "set",
           725: ARG,
           726: VARIABLE "@i",
           730: VARIABLE "b",
           734: NOT,
           735: FUNC,
           736: EOL
        ,
           737: FUNC_REF "set",
           741: ARG,
           742: VARIABLE "@i",
           746: VARIABLE "a",
           750: VARIABLE "b",
           754: R_SHIFT,
           755: FUNC,
           756: EOL
        ,
           757: FUNC_REF "set",
           761: ARG,
           762: VARIABLE "@i",
           766: VARIABLE "a",
           770: VARIABLE "b",
           774: L_SHIFT,
           775: FUNC,
           776: EOL
        ,
           777: NOP'''.replace('''
        ''', '\n')

    test_extra = ('''
    #include "../compat/borrow.hpp"
    #include "../io/line.hpp"
    #include "../mmo/clif.t.hpp"
    #include "../ast/script.hpp"
    #include "../map/script-parse-internal.hpp"

    using tmwa::operator "" _s;

    static
    const tmwa::ScriptBuffer& test_script_buffer(tmwa::LString source)
    {
        auto p = tmwa::add_strp("TEST_FAKE_PARAM_BASELEVEL"_s);
        p->type = tmwa::StringCode::PARAM;
        p->val = static_cast<uint16_t>(tmwa::SP::BASELEVEL);
        p = tmwa::add_strp("TEST_FAKE_CONSTANT"_s);
        p->type = tmwa::StringCode::INT;
        p->val = 42;

        tmwa::io::LineCharReader lr(tmwa::io::from_string, "<script debug print test>"_s, source);
        tmwa::ast::script::ScriptOptions opt;
        opt.implicit_start = true;
        opt.implicit_end = true;
        auto code_res = tmwa::ast::script::parse_script_body(lr, opt);
        auto code = TRY_UNWRAP(code_res.get_success(), abort());
        auto ups = tmwa::compile_script("script debug print test"_s, code, opt.implicit_end);
        assert(ups);
        return *ups.release();
    }
    ''')

    # the pretty-printer is designed to be run with 'set print array on'
    # but the testsuite runs with all settings default, in this case off.
    tests = [
            ('test_script_buffer("' + src.replace('\\', '\\\\').replace('"', '\\"') + '"_s)',
                '"script debug print test" = {' + asm.replace('\n', ' ').replace('EOL ,', 'EOL\n,') + '}'),
    ]
