class duration(object):
    __slots__ = ('_whole', '_frac', '_units')
    name = 'std::chrono::duration'
    enabled = True

    def __init__(self, value):
        from decimal import Decimal

        rep = int(value['__r'])
        ratio = value.type.template_argument(1)
        num = int(ratio.template_argument(0))
        den = int(ratio.template_argument(1))
        # this will fail on duration<float>
        value = Decimal(rep) * num / den
        whole = int(value)
        self._whole = whole
        self._frac = value - whole
        units = {
                (1, 1000*1000*1000): ('nanoseconds', '_ns'),
                (1, 1000*1000): ('microseconds', '_us'),
                (1, 1000): ('milliseconds', '_ms'),
                (1, 1): ('seconds', '_s'),
                (60, 1): ('minutes', '_min'),
                (60*60, 1): ('hours', '_h'),
                (24*60*60, 1): ('duration<int, ratio<%d, %d>>', '_d'),
                # days don't exist (probably because of leap seconds)
        }
        self._units = units.get((num, den)) or ('duration<???, ratio<%d, %d>>' % (num, den), '_?')

    def to_string(self):
        whole = self._whole
        frac = self._frac
        cu, su = self._units
        if not whole and not frac:
            return '0%s' % su
        s = whole
        min = s // 60
        s %= 60
        h = min // 60
        min %= 60
        d = h // 24
        h %= 24
        msx = frac * 1000
        ms = int(msx)
        usx = (msx - ms) * 1000
        us = int(usx)
        nsx = (usx - us) * 1000
        ns = int(nsx)
        bits = [
            '%d_d' % d if d else None,
            '%d_h' % h if h else None,
            '%d_min' % min if min else None,
            '%d_s' % s if s else None,
            '%d_ms' % ms if ms else None,
            '%d_us' % us if us else None,
            '%d_ns' % ns if ns else None,
        ]
        body = ' + '.join(b for b in bits if b is not None)
        if not body.endswith(su):
            body = '%s(%s)' % (cu, body)
        elif '+' in body:
            body = '(%s)' % body
        return body

    tests = [
            ('std::chrono::nanoseconds(0)', '0_ns'),
            ('std::chrono::microseconds(0)', '0_us'),
            ('std::chrono::milliseconds(0)', '0_ms'),
            ('std::chrono::seconds(0)', '0_s'),
            ('std::chrono::minutes(0)', '0_min'),
            ('std::chrono::hours(0)', '0_h'),
            ('std::chrono::duration<int, std::ratio<60*60*24>>(0)', '0_d'),

            ('std::chrono::nanoseconds(1)', '1_ns'),
            ('std::chrono::microseconds(1)', '1_us'),
            ('std::chrono::milliseconds(1)', '1_ms'),
            ('std::chrono::seconds(1)', '1_s'),
            ('std::chrono::minutes(1)', '1_min'),
            ('std::chrono::hours(1)', '1_h'),
            ('std::chrono::duration<int, std::ratio<60*60*24>>(1)', '1_d'),

            ('std::chrono::nanoseconds(1)', '1_ns'),
            ('std::chrono::microseconds(1)', '1_us'),
            ('std::chrono::milliseconds(1)', '1_ms'),
            ('std::chrono::seconds(1)', '1_s'),
            ('std::chrono::minutes(1)', '1_min'),
            ('std::chrono::hours(1)', '1_h'),
            ('std::chrono::duration<int, std::ratio<60*60*24>>(1)', '1_d'),

            ('std::chrono::nanoseconds(3)', '3_ns'),
            ('std::chrono::microseconds(3)', '3_us'),
            ('std::chrono::milliseconds(3)', '3_ms'),
            ('std::chrono::seconds(3)', '3_s'),
            ('std::chrono::minutes(3)', '3_min'),
            ('std::chrono::hours(3)', '3_h'),
            ('std::chrono::duration<int, std::ratio<60*60*24>>(3)', '3_d'),

            ('std::chrono::nanoseconds(1000)', 'nanoseconds(1_us)'),
            ('std::chrono::microseconds(1000)', 'microseconds(1_ms)'),
            ('std::chrono::milliseconds(1000)', 'milliseconds(1_s)'),
            ('std::chrono::seconds(60)', 'seconds(1_min)'),
            ('std::chrono::minutes(60)', 'minutes(1_h)'),
            ('std::chrono::hours(24)', 'hours(1_d)'),

            ('std::chrono::nanoseconds(1001)', '(1_us + 1_ns)'),
            ('std::chrono::microseconds(1001)', '(1_ms + 1_us)'),
            ('std::chrono::milliseconds(1001)', '(1_s + 1_ms)'),
            ('std::chrono::seconds(61)', '(1_min + 1_s)'),
            ('std::chrono::minutes(61)', '(1_h + 1_min)'),
            ('std::chrono::hours(25)', '(1_d + 1_h)'),

            ('std::chrono::nanoseconds(1001*1000)', 'nanoseconds(1_ms + 1_us)'),
            ('std::chrono::microseconds(1001*1000)', 'microseconds(1_s + 1_ms)'),
            ('std::chrono::milliseconds(61*1000)', 'milliseconds(1_min + 1_s)'),
            ('std::chrono::seconds(61*60)', 'seconds(1_h + 1_min)'),
            ('std::chrono::minutes(25*60)', 'minutes(1_d + 1_h)'),
    ]
