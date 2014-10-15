class AreaUnion(object):
    __slots__ = ('_value')
    name = 'tmwa::magic::AreaUnion'
    enabled = True

    def __init__(self, value):
        self._value = value

    def display_hint(self):
        return 'array'

    def to_string(self):
        return None

    def children(self):
        v = self._value
        for i in [0, 1]:
            yield '[%d]', v['a_union'][i]['impl'].dereference()

    tests = []

class area_t(object):
    enabled = True

    test_extra = '''
    #include "../strings/fwd.hpp"
    using tmwa::operator "" _s;

    inline
    tmwa::Borrowed<tmwa::map_local> fake_map_local_x_dup_for_area_t(tmwa::ZString name)
    {
        auto *p = new tmwa::map_local{};
        p->name_ = tmwa::stringish<tmwa::MapName>(name);
        return tmwa::borrow(*p);
    }
    '''

    tests = [
            ('tmwa::magic::area_t(tmwa::magic::location_t{fake_map_local_x_dup_for_area_t("map"_s), 123, 456})',
                '{<tmwa::sexpr::Variant<tmwa::magic::location_t, tmwa::magic::AreaUnion, tmwa::magic::AreaRect, tmwa::magic::AreaBar>> = {(tmwa::magic::location_t) = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 123, y = 456}}, size = 1}'),
            ('tmwa::magic::area_t(tmwa::magic::AreaUnion{{tmwa::dumb_ptr<tmwa::magic::area_t>::make(tmwa::magic::location_t{fake_map_local_x_dup_for_area_t("map"_s), 123, 456}), tmwa::dumb_ptr<tmwa::magic::area_t>::make(tmwa::magic::location_t{fake_map_local_x_dup_for_area_t("map"_s), 321, 654})}})',
                '{<tmwa::sexpr::Variant<tmwa::magic::location_t, tmwa::magic::AreaUnion, tmwa::magic::AreaRect, tmwa::magic::AreaBar>> = {(tmwa::magic::AreaUnion) = {{<tmwa::sexpr::Variant<tmwa::magic::location_t, tmwa::magic::AreaUnion, tmwa::magic::AreaRect, tmwa::magic::AreaBar>> = {(tmwa::magic::location_t) = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 123, y = 456}}, size = 1}, {<tmwa::sexpr::Variant<tmwa::magic::location_t, tmwa::magic::AreaUnion, tmwa::magic::AreaRect, tmwa::magic::AreaBar>> = {(tmwa::magic::location_t) = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 321, y = 654}}, size = 1}}}, size = 2}'),
            ('tmwa::magic::area_t(tmwa::magic::AreaRect{tmwa::magic::location_t{fake_map_local_x_dup_for_area_t("map"_s), 123, 456}, 789, 102})',
                '{<tmwa::sexpr::Variant<tmwa::magic::location_t, tmwa::magic::AreaUnion, tmwa::magic::AreaRect, tmwa::magic::AreaBar>> = {(tmwa::magic::AreaRect) = {loc = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 123, y = 456}, width = 789, height = 102}}, size = 80478}'),
            ('tmwa::magic::area_t(tmwa::magic::AreaBar{tmwa::magic::location_t{fake_map_local_x_dup_for_area_t("map"_s), 42, 43}, 123, 456, tmwa::DIR::NW})',
                '{<tmwa::sexpr::Variant<tmwa::magic::location_t, tmwa::magic::AreaUnion, tmwa::magic::AreaRect, tmwa::magic::AreaBar>> = {(tmwa::magic::AreaBar) = {loc = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 42, y = 43}, width = 123, depth = 456, dir = tmwa::DIR::NW}}, size = 112632}'),
    ]


class val_t(object):
    enabled = True

    test_extra = '''
    #include "../strings/fwd.hpp"
    using tmwa::operator "" _s;

    inline
    tmwa::Borrowed<tmwa::map_local> fake_map_local_x_dup_for_val_t(tmwa::ZString name)
    {
        auto *p = new tmwa::map_local{};
        p->name_ = tmwa::stringish<tmwa::MapName>(name);
        return tmwa::borrow(*p);
    }
    '''

    tests = [
            ('tmwa::magic::val_t(tmwa::magic::ValUndef{})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValUndef) = {<No data fields>}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValInt{42})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValInt) = {v_int = 42}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValDir{tmwa::DIR::NW})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValDir) = {v_dir = tmwa::DIR::NW}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValString{"Hello"_s})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValString) = {v_string = "Hello"}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValEntityInt{tmwa::wrap<tmwa::BlockId>(123)})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValEntityInt) = {v_eid = 123}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValEntityPtr{tmwa::dumb_ptr<tmwa::block_list>()})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValEntityPtr) = {v_entity = 0x0}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValLocation{tmwa::magic::location_t{fake_map_local_x_dup_for_val_t("map"_s), 42, 123}})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValLocation) = {v_location = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 42, y = 123}}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValArea{tmwa::dumb_ptr<tmwa::magic::area_t>()})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValArea) = {v_area = 0x0}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValSpell{tmwa::dumb_ptr<tmwa::magic::spell_t>()})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValSpell) = {v_spell = 0x0}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValInvocationInt{tmwa::wrap<tmwa::BlockId>(123)})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValInvocationInt) = {v_iid = 123}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValInvocationPtr{})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValInvocationPtr) = {v_invocation = 0x0}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValFail{})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValFail) = {<No data fields>}}, <No data fields>}'),
            ('tmwa::magic::val_t(tmwa::magic::ValNegative1{})',
                '{<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValNegative1) = {<No data fields>}}, <No data fields>}'),
    ]


class ExprAreaUnion(object):
    __slots__ = ('_value')
    name = 'tmwa::magic::ExprAreaUnion'
    enabled = True

    def __init__(self, value):
        self._value = value

    def display_hint(self):
        return 'array'

    def to_string(self):
        return None

    def children(self):
        v = self._value
        for i in [0, 1]:
            yield '[%d]', v['a_union'][i]['impl'].dereference()

    tests = []


class e_area_t(object):
    enabled = True

    tests = [
            ('tmwa::magic::e_area_t(tmwa::magic::e_location_t())',
                '{<tmwa::sexpr::Variant<tmwa::magic::e_location_t, tmwa::magic::ExprAreaUnion, tmwa::magic::ExprAreaRect, tmwa::magic::ExprAreaBar>> = {(tmwa::magic::e_location_t) = {m = 0x0, x = 0x0, y = 0x0}}, <No data fields>}'),
            ('tmwa::magic::e_area_t(tmwa::magic::ExprAreaUnion{{tmwa::dumb_ptr<tmwa::magic::e_area_t>::make(tmwa::magic::e_location_t()), tmwa::dumb_ptr<tmwa::magic::e_area_t>::make(tmwa::magic::e_location_t())}})',
                '{<tmwa::sexpr::Variant<tmwa::magic::e_location_t, tmwa::magic::ExprAreaUnion, tmwa::magic::ExprAreaRect, tmwa::magic::ExprAreaBar>> = {(tmwa::magic::ExprAreaUnion) = {{<tmwa::sexpr::Variant<tmwa::magic::e_location_t, tmwa::magic::ExprAreaUnion, tmwa::magic::ExprAreaRect, tmwa::magic::ExprAreaBar>> = {(tmwa::magic::e_location_t) = {m = 0x0, x = 0x0, y = 0x0}}, <No data fields>}, {<tmwa::sexpr::Variant<tmwa::magic::e_location_t, tmwa::magic::ExprAreaUnion, tmwa::magic::ExprAreaRect, tmwa::magic::ExprAreaBar>> = {(tmwa::magic::e_location_t) = {m = 0x0, x = 0x0, y = 0x0}}, <No data fields>}}}, <No data fields>}'),
            ('tmwa::magic::e_area_t(tmwa::magic::ExprAreaRect{tmwa::magic::e_location_t(), tmwa::dumb_ptr<tmwa::magic::expr_t>(), tmwa::dumb_ptr<tmwa::magic::expr_t>()})',
                '{<tmwa::sexpr::Variant<tmwa::magic::e_location_t, tmwa::magic::ExprAreaUnion, tmwa::magic::ExprAreaRect, tmwa::magic::ExprAreaBar>> = {(tmwa::magic::ExprAreaRect) = {loc = {m = 0x0, x = 0x0, y = 0x0}, width = 0x0, height = 0x0}}, <No data fields>}'),
            ('tmwa::magic::e_area_t(tmwa::magic::ExprAreaBar{tmwa::magic::e_location_t(), tmwa::dumb_ptr<tmwa::magic::expr_t>(), tmwa::dumb_ptr<tmwa::magic::expr_t>(), tmwa::dumb_ptr<tmwa::magic::expr_t>()})',
                '{<tmwa::sexpr::Variant<tmwa::magic::e_location_t, tmwa::magic::ExprAreaUnion, tmwa::magic::ExprAreaRect, tmwa::magic::ExprAreaBar>> = {(tmwa::magic::ExprAreaBar) = {loc = {m = 0x0, x = 0x0, y = 0x0}, width = 0x0, depth = 0x0, dir = 0x0}}, <No data fields>}'),
    ]



class expr_t(object):
    enabled = True

    tests = [
            ('tmwa::magic::expr_t(tmwa::magic::val_t(tmwa::magic::ValUndef()))',
                '{<tmwa::sexpr::Variant<tmwa::magic::val_t, tmwa::magic::e_location_t, tmwa::magic::e_area_t, tmwa::magic::ExprFunApp, tmwa::magic::ExprId, tmwa::magic::ExprField>> = {(tmwa::magic::val_t) = {<tmwa::sexpr::Variant<tmwa::magic::ValUndef, tmwa::magic::ValInt, tmwa::magic::ValDir, tmwa::magic::ValString, tmwa::magic::ValEntityInt, tmwa::magic::ValEntityPtr, tmwa::magic::ValLocation, tmwa::magic::ValArea, tmwa::magic::ValSpell, tmwa::magic::ValInvocationInt, tmwa::magic::ValInvocationPtr, tmwa::magic::ValFail, tmwa::magic::ValNegative1>> = {(tmwa::magic::ValUndef) = {<No data fields>}}, <No data fields>}}, <No data fields>}'),
            ('tmwa::magic::expr_t(tmwa::magic::e_location_t())',
                '{<tmwa::sexpr::Variant<tmwa::magic::val_t, tmwa::magic::e_location_t, tmwa::magic::e_area_t, tmwa::magic::ExprFunApp, tmwa::magic::ExprId, tmwa::magic::ExprField>> = {(tmwa::magic::e_location_t) = {m = 0x0, x = 0x0, y = 0x0}}, <No data fields>}'),
            ('tmwa::magic::expr_t(tmwa::magic::e_area_t(tmwa::magic::e_location_t()))',
                '{<tmwa::sexpr::Variant<tmwa::magic::val_t, tmwa::magic::e_location_t, tmwa::magic::e_area_t, tmwa::magic::ExprFunApp, tmwa::magic::ExprId, tmwa::magic::ExprField>> = {(tmwa::magic::e_area_t) = {<tmwa::sexpr::Variant<tmwa::magic::e_location_t, tmwa::magic::ExprAreaUnion, tmwa::magic::ExprAreaRect, tmwa::magic::ExprAreaBar>> = {(tmwa::magic::e_location_t) = {m = 0x0, x = 0x0, y = 0x0}}, <No data fields>}}, <No data fields>}'),
            ('tmwa::magic::expr_t(tmwa::magic::ExprFunApp())',
                '{<tmwa::sexpr::Variant<tmwa::magic::val_t, tmwa::magic::e_location_t, tmwa::magic::e_area_t, tmwa::magic::ExprFunApp, tmwa::magic::ExprId, tmwa::magic::ExprField>> = {(tmwa::magic::ExprFunApp) = {funp = (fun_t *) nullptr, line_nr = 0, column = 0, args_nr = 0, args = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}}, <No data fields>}'),
            ('tmwa::magic::expr_t(tmwa::magic::ExprId{123})',
                '{<tmwa::sexpr::Variant<tmwa::magic::val_t, tmwa::magic::e_location_t, tmwa::magic::e_area_t, tmwa::magic::ExprFunApp, tmwa::magic::ExprId, tmwa::magic::ExprField>> = {(tmwa::magic::ExprId) = {e_id = 123}}, <No data fields>}'),
            ('tmwa::magic::expr_t(tmwa::magic::ExprField{tmwa::dumb_ptr<tmwa::magic::expr_t>(), 42})',
                '{<tmwa::sexpr::Variant<tmwa::magic::val_t, tmwa::magic::e_location_t, tmwa::magic::e_area_t, tmwa::magic::ExprFunApp, tmwa::magic::ExprId, tmwa::magic::ExprField>> = {(tmwa::magic::ExprField) = {expr = 0x0, id = 42}}, <No data fields>}'),
    ]


class effect_t(object):
    enabled = True

    tests = [
            ('tmwa::magic::effect_t(tmwa::magic::EffectSkip{}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectSkip) = {<No data fields>}}, next = 0x0}'),
            ('tmwa::magic::effect_t(tmwa::magic::EffectAbort{}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectAbort) = {<No data fields>}}, next = 0x0}'),
            ('tmwa::magic::effect_t(tmwa::magic::EffectAssign{42, tmwa::dumb_ptr<tmwa::magic::expr_t>()}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectAssign) = {id = 42, expr = 0x0}}, next = 0x0}'),
            ('tmwa::magic::effect_t(tmwa::magic::EffectForEach{123, tmwa::dumb_ptr<tmwa::magic::expr_t>(), tmwa::dumb_ptr<tmwa::magic::effect_t>(), tmwa::magic::FOREACH_FILTER::PC}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectForEach) = {id = 123, area = 0x0, body = 0x0, filter = tmwa::magic::FOREACH_FILTER::PC}}, next = 0x0}'),
            ('tmwa::magic::effect_t(tmwa::magic::EffectFor{42, tmwa::dumb_ptr<tmwa::magic::expr_t>(), tmwa::dumb_ptr<tmwa::magic::expr_t>(), tmwa::dumb_ptr<tmwa::magic::effect_t>()}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectFor) = {id = 42, start = 0x0, stop = 0x0, body = 0x0}}, next = 0x0}'),
            ('tmwa::magic::effect_t(tmwa::magic::EffectIf{tmwa::dumb_ptr<tmwa::magic::expr_t>(), tmwa::dumb_ptr<tmwa::magic::effect_t>(), tmwa::dumb_ptr<tmwa::magic::effect_t>()}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectIf) = {cond = 0x0, true_branch = 0x0, false_branch = 0x0}}, next = 0x0}'),
            ('tmwa::magic::effect_t(tmwa::magic::EffectSleep{tmwa::dumb_ptr<tmwa::magic::expr_t>()}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectSleep) = {e_sleep = 0x0}}, next = 0x0}'),
            ('tmwa::magic::effect_t(tmwa::magic::EffectScript{tmwa::dumb_ptr<const tmwa::ScriptBuffer>()}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectScript) = {e_script = 0x0}}, next = 0x0}'),
            ('tmwa::magic::effect_t(tmwa::magic::EffectBreak{}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectBreak) = {<No data fields>}}, next = 0x0}'),
            ('tmwa::magic::effect_t(tmwa::magic::EffectOp(), tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectOp) = {opp = (op_t *) nullptr, args_nr = 0, line_nr = 0, column = 0, args = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}}, next = 0x0}'),
            ('tmwa::magic::effect_t(tmwa::magic::EffectEnd{}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectEnd) = {<No data fields>}}, next = 0x0}'),
            ('tmwa::magic::effect_t(tmwa::magic::EffectCall{nullptr, tmwa::dumb_ptr<std::vector<tmwa::dumb_ptr<tmwa::magic::expr_t>>>(), tmwa::dumb_ptr<tmwa::magic::effect_t>()}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::EffectSkip, tmwa::magic::EffectAbort, tmwa::magic::EffectAssign, tmwa::magic::EffectForEach, tmwa::magic::EffectFor, tmwa::magic::EffectIf, tmwa::magic::EffectSleep, tmwa::magic::EffectScript, tmwa::magic::EffectBreak, tmwa::magic::EffectOp, tmwa::magic::EffectEnd, tmwa::magic::EffectCall>> = {(tmwa::magic::EffectCall) = {formalv = nullptr, actualvp = 0x0, body = 0x0}}, next = 0x0}'),
    ]


class spellguard_t(object):
    enabled = True

    tests = [
            ('tmwa::magic::spellguard_t(tmwa::magic::GuardCondition{tmwa::dumb_ptr<tmwa::magic::expr_t>()}, tmwa::dumb_ptr<tmwa::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::GuardCondition, tmwa::magic::GuardMana, tmwa::magic::GuardCastTime, tmwa::magic::GuardComponents, tmwa::magic::GuardCatalysts, tmwa::magic::GuardChoice, tmwa::magic::effect_set_t>> = {(tmwa::magic::GuardCondition) = {s_condition = 0x0}}, next = 0x0}'),
            ('tmwa::magic::spellguard_t(tmwa::magic::GuardMana{tmwa::dumb_ptr<tmwa::magic::expr_t>()}, tmwa::dumb_ptr<tmwa::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::GuardCondition, tmwa::magic::GuardMana, tmwa::magic::GuardCastTime, tmwa::magic::GuardComponents, tmwa::magic::GuardCatalysts, tmwa::magic::GuardChoice, tmwa::magic::effect_set_t>> = {(tmwa::magic::GuardMana) = {s_mana = 0x0}}, next = 0x0}'),
            ('tmwa::magic::spellguard_t(tmwa::magic::GuardCastTime{tmwa::dumb_ptr<tmwa::magic::expr_t>()}, tmwa::dumb_ptr<tmwa::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::GuardCondition, tmwa::magic::GuardMana, tmwa::magic::GuardCastTime, tmwa::magic::GuardComponents, tmwa::magic::GuardCatalysts, tmwa::magic::GuardChoice, tmwa::magic::effect_set_t>> = {(tmwa::magic::GuardCastTime) = {s_casttime = 0x0}}, next = 0x0}'),
            ('tmwa::magic::spellguard_t(tmwa::magic::GuardComponents{tmwa::dumb_ptr<tmwa::magic::component_t>()}, tmwa::dumb_ptr<tmwa::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::GuardCondition, tmwa::magic::GuardMana, tmwa::magic::GuardCastTime, tmwa::magic::GuardComponents, tmwa::magic::GuardCatalysts, tmwa::magic::GuardChoice, tmwa::magic::effect_set_t>> = {(tmwa::magic::GuardComponents) = {s_components = 0x0}}, next = 0x0}'),
            ('tmwa::magic::spellguard_t(tmwa::magic::GuardCatalysts{tmwa::dumb_ptr<tmwa::magic::component_t>()}, tmwa::dumb_ptr<tmwa::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::GuardCondition, tmwa::magic::GuardMana, tmwa::magic::GuardCastTime, tmwa::magic::GuardComponents, tmwa::magic::GuardCatalysts, tmwa::magic::GuardChoice, tmwa::magic::effect_set_t>> = {(tmwa::magic::GuardCatalysts) = {s_catalysts = 0x0}}, next = 0x0}'),
            ('tmwa::magic::spellguard_t(tmwa::magic::GuardChoice{tmwa::dumb_ptr<tmwa::magic::spellguard_t>()}, tmwa::dumb_ptr<tmwa::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::GuardCondition, tmwa::magic::GuardMana, tmwa::magic::GuardCastTime, tmwa::magic::GuardComponents, tmwa::magic::GuardCatalysts, tmwa::magic::GuardChoice, tmwa::magic::effect_set_t>> = {(tmwa::magic::GuardChoice) = {s_alt = 0x0}}, next = 0x0}'),
            ('tmwa::magic::spellguard_t(tmwa::magic::effect_set_t{tmwa::dumb_ptr<tmwa::magic::effect_t>(), tmwa::dumb_ptr<tmwa::magic::effect_t>(), tmwa::dumb_ptr<tmwa::magic::effect_t>()}, tmwa::dumb_ptr<tmwa::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::GuardCondition, tmwa::magic::GuardMana, tmwa::magic::GuardCastTime, tmwa::magic::GuardComponents, tmwa::magic::GuardCatalysts, tmwa::magic::GuardChoice, tmwa::magic::effect_set_t>> = {(tmwa::magic::effect_set_t) = {effect = 0x0, at_trigger = 0x0, at_end = 0x0}}, next = 0x0}'),
    ]


class cont_activation_record_t(object):
    enabled = True

    tests = [
            ('tmwa::magic::cont_activation_record_t(tmwa::magic::CarForEach{42, true, tmwa::dumb_ptr<tmwa::magic::effect_t>(), tmwa::dumb_ptr<std::vector<tmwa::BlockId>>(), 123}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::CarForEach, tmwa::magic::CarFor, tmwa::magic::CarProc>> = {(tmwa::magic::CarForEach) = {id = 42, ty_is_spell_not_entity = true, body = 0x0, entities_vp = 0x0, index = 123}}, return_location = 0x0}'),
            ('tmwa::magic::cont_activation_record_t(tmwa::magic::CarFor{42, tmwa::dumb_ptr<tmwa::magic::effect_t>(), 123, 456}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::CarForEach, tmwa::magic::CarFor, tmwa::magic::CarProc>> = {(tmwa::magic::CarFor) = {id = 42, body = 0x0, current = 123, stop = 456}}, return_location = 0x0}'),
            ('tmwa::magic::cont_activation_record_t(tmwa::magic::CarProc{123, nullptr, tmwa::dumb_ptr<tmwa::magic::val_t[]>()}, tmwa::dumb_ptr<tmwa::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::magic::CarForEach, tmwa::magic::CarFor, tmwa::magic::CarProc>> = {(tmwa::magic::CarProc) = {args_nr = 123, formalap = nullptr, old_actualpa = 0x0 = {sz = 0}}}, return_location = 0x0}'),
    ]
