class AreaUnion(object):
    __slots__ = ('_value')
    name = 'tmwa::map::magic::AreaUnion'
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
    tmwa::Borrowed<tmwa::map::map_local> fake_map_local_x_dup_for_area_t(tmwa::ZString name)
    {
        auto *p = new tmwa::map::map_local{};
        p->name_ = tmwa::stringish<tmwa::MapName>(name);
        return tmwa::borrow(*p);
    }
    '''

    tests = [
            ('tmwa::map::magic::area_t(tmwa::map::magic::location_t{fake_map_local_x_dup_for_area_t("map"_s), 123, 456})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::location_t, tmwa::map::magic::AreaUnion, tmwa::map::magic::AreaRect, tmwa::map::magic::AreaBar>> = {(tmwa::map::magic::location_t) = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 123, y = 456}}, size = 1}'),
            ('tmwa::map::magic::area_t(tmwa::map::magic::AreaUnion{{tmwa::dumb_ptr<tmwa::map::magic::area_t>::make(tmwa::map::magic::location_t{fake_map_local_x_dup_for_area_t("map"_s), 123, 456}), tmwa::dumb_ptr<tmwa::map::magic::area_t>::make(tmwa::map::magic::location_t{fake_map_local_x_dup_for_area_t("map"_s), 321, 654})}})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::location_t, tmwa::map::magic::AreaUnion, tmwa::map::magic::AreaRect, tmwa::map::magic::AreaBar>> = {(tmwa::map::magic::AreaUnion) = {{<tmwa::sexpr::Variant<tmwa::map::magic::location_t, tmwa::map::magic::AreaUnion, tmwa::map::magic::AreaRect, tmwa::map::magic::AreaBar>> = {(tmwa::map::magic::location_t) = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 123, y = 456}}, size = 1}, {<tmwa::sexpr::Variant<tmwa::map::magic::location_t, tmwa::map::magic::AreaUnion, tmwa::map::magic::AreaRect, tmwa::map::magic::AreaBar>> = {(tmwa::map::magic::location_t) = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 321, y = 654}}, size = 1}}}, size = 2}'),
            ('tmwa::map::magic::area_t(tmwa::map::magic::AreaRect{tmwa::map::magic::location_t{fake_map_local_x_dup_for_area_t("map"_s), 123, 456}, 789, 102})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::location_t, tmwa::map::magic::AreaUnion, tmwa::map::magic::AreaRect, tmwa::map::magic::AreaBar>> = {(tmwa::map::magic::AreaRect) = {loc = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 123, y = 456}, width = 789, height = 102}}, size = 80478}'),
            ('tmwa::map::magic::area_t(tmwa::map::magic::AreaBar{tmwa::map::magic::location_t{fake_map_local_x_dup_for_area_t("map"_s), 42, 43}, 123, 456, tmwa::DIR::NW})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::location_t, tmwa::map::magic::AreaUnion, tmwa::map::magic::AreaRect, tmwa::map::magic::AreaBar>> = {(tmwa::map::magic::AreaBar) = {loc = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 42, y = 43}, width = 123, depth = 456, dir = tmwa::DIR::NW}}, size = 112632}'),
    ]


class val_t(object):
    enabled = True

    test_extra = '''
    #include "../strings/fwd.hpp"
    using tmwa::operator "" _s;

    inline
    tmwa::Borrowed<tmwa::map::map_local> fake_map_local_x_dup_for_val_t(tmwa::ZString name)
    {
        auto *p = new tmwa::map::map_local{};
        p->name_ = tmwa::stringish<tmwa::MapName>(name);
        return tmwa::borrow(*p);
    }
    '''

    tests = [
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValUndef{})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValUndef) = {<No data fields>}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValInt{42})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValInt) = {v_int = 42}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValDir{tmwa::DIR::NW})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValDir) = {v_dir = tmwa::DIR::NW}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValString{"Hello"_s})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValString) = {v_string = "Hello"}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValEntityInt{tmwa::wrap<tmwa::BlockId>(123)})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValEntityInt) = {v_eid = 123}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValEntityPtr{tmwa::dumb_ptr<tmwa::map::block_list>()})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValEntityPtr) = {v_entity = 0x0}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValLocation{tmwa::map::magic::location_t{fake_map_local_x_dup_for_val_t("map"_s), 42, 123}})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValLocation) = {v_location = {m = (map_local *) = {->name = "map", ->xs = 0, ->ys = 0}, x = 42, y = 123}}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValArea{tmwa::dumb_ptr<tmwa::map::magic::area_t>()})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValArea) = {v_area = 0x0}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValSpell{tmwa::dumb_ptr<tmwa::map::magic::spell_t>()})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValSpell) = {v_spell = 0x0}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValInvocationInt{tmwa::wrap<tmwa::BlockId>(123)})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValInvocationInt) = {v_iid = 123}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValInvocationPtr{})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValInvocationPtr) = {v_invocation = 0x0}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValFail{})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValFail) = {<No data fields>}}, <No data fields>}'),
            ('tmwa::map::magic::val_t(tmwa::map::magic::ValNegative1{})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValNegative1) = {<No data fields>}}, <No data fields>}'),
    ]


class ExprAreaUnion(object):
    __slots__ = ('_value')
    name = 'tmwa::map::magic::ExprAreaUnion'
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
            ('tmwa::map::magic::e_area_t(tmwa::map::magic::e_location_t())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::e_location_t, tmwa::map::magic::ExprAreaUnion, tmwa::map::magic::ExprAreaRect, tmwa::map::magic::ExprAreaBar>> = {(tmwa::map::magic::e_location_t) = {m = 0x0, x = 0x0, y = 0x0}}, <No data fields>}'),
            ('tmwa::map::magic::e_area_t(tmwa::map::magic::ExprAreaUnion{{tmwa::dumb_ptr<tmwa::map::magic::e_area_t>::make(tmwa::map::magic::e_location_t()), tmwa::dumb_ptr<tmwa::map::magic::e_area_t>::make(tmwa::map::magic::e_location_t())}})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::e_location_t, tmwa::map::magic::ExprAreaUnion, tmwa::map::magic::ExprAreaRect, tmwa::map::magic::ExprAreaBar>> = {(tmwa::map::magic::ExprAreaUnion) = {{<tmwa::sexpr::Variant<tmwa::map::magic::e_location_t, tmwa::map::magic::ExprAreaUnion, tmwa::map::magic::ExprAreaRect, tmwa::map::magic::ExprAreaBar>> = {(tmwa::map::magic::e_location_t) = {m = 0x0, x = 0x0, y = 0x0}}, <No data fields>}, {<tmwa::sexpr::Variant<tmwa::map::magic::e_location_t, tmwa::map::magic::ExprAreaUnion, tmwa::map::magic::ExprAreaRect, tmwa::map::magic::ExprAreaBar>> = {(tmwa::map::magic::e_location_t) = {m = 0x0, x = 0x0, y = 0x0}}, <No data fields>}}}, <No data fields>}'),
            ('tmwa::map::magic::e_area_t(tmwa::map::magic::ExprAreaRect{tmwa::map::magic::e_location_t(), tmwa::dumb_ptr<tmwa::map::magic::expr_t>(), tmwa::dumb_ptr<tmwa::map::magic::expr_t>()})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::e_location_t, tmwa::map::magic::ExprAreaUnion, tmwa::map::magic::ExprAreaRect, tmwa::map::magic::ExprAreaBar>> = {(tmwa::map::magic::ExprAreaRect) = {loc = {m = 0x0, x = 0x0, y = 0x0}, width = 0x0, height = 0x0}}, <No data fields>}'),
            ('tmwa::map::magic::e_area_t(tmwa::map::magic::ExprAreaBar{tmwa::map::magic::e_location_t(), tmwa::dumb_ptr<tmwa::map::magic::expr_t>(), tmwa::dumb_ptr<tmwa::map::magic::expr_t>(), tmwa::dumb_ptr<tmwa::map::magic::expr_t>()})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::e_location_t, tmwa::map::magic::ExprAreaUnion, tmwa::map::magic::ExprAreaRect, tmwa::map::magic::ExprAreaBar>> = {(tmwa::map::magic::ExprAreaBar) = {loc = {m = 0x0, x = 0x0, y = 0x0}, width = 0x0, depth = 0x0, dir = 0x0}}, <No data fields>}'),
    ]



class expr_t(object):
    enabled = True

    tests = [
            ('tmwa::map::magic::expr_t(tmwa::map::magic::val_t(tmwa::map::magic::ValUndef()))',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::val_t, tmwa::map::magic::e_location_t, tmwa::map::magic::e_area_t, tmwa::map::magic::ExprFunApp, tmwa::map::magic::ExprId, tmwa::map::magic::ExprField>> = {(tmwa::map::magic::val_t) = {<tmwa::sexpr::Variant<tmwa::map::magic::ValUndef, tmwa::map::magic::ValInt, tmwa::map::magic::ValDir, tmwa::map::magic::ValString, tmwa::map::magic::ValEntityInt, tmwa::map::magic::ValEntityPtr, tmwa::map::magic::ValLocation, tmwa::map::magic::ValArea, tmwa::map::magic::ValSpell, tmwa::map::magic::ValInvocationInt, tmwa::map::magic::ValInvocationPtr, tmwa::map::magic::ValFail, tmwa::map::magic::ValNegative1>> = {(tmwa::map::magic::ValUndef) = {<No data fields>}}, <No data fields>}}, <No data fields>}'),
            ('tmwa::map::magic::expr_t(tmwa::map::magic::e_location_t())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::val_t, tmwa::map::magic::e_location_t, tmwa::map::magic::e_area_t, tmwa::map::magic::ExprFunApp, tmwa::map::magic::ExprId, tmwa::map::magic::ExprField>> = {(tmwa::map::magic::e_location_t) = {m = 0x0, x = 0x0, y = 0x0}}, <No data fields>}'),
            ('tmwa::map::magic::expr_t(tmwa::map::magic::e_area_t(tmwa::map::magic::e_location_t()))',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::val_t, tmwa::map::magic::e_location_t, tmwa::map::magic::e_area_t, tmwa::map::magic::ExprFunApp, tmwa::map::magic::ExprId, tmwa::map::magic::ExprField>> = {(tmwa::map::magic::e_area_t) = {<tmwa::sexpr::Variant<tmwa::map::magic::e_location_t, tmwa::map::magic::ExprAreaUnion, tmwa::map::magic::ExprAreaRect, tmwa::map::magic::ExprAreaBar>> = {(tmwa::map::magic::e_location_t) = {m = 0x0, x = 0x0, y = 0x0}}, <No data fields>}}, <No data fields>}'),
            ('tmwa::map::magic::expr_t(tmwa::map::magic::ExprFunApp())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::val_t, tmwa::map::magic::e_location_t, tmwa::map::magic::e_area_t, tmwa::map::magic::ExprFunApp, tmwa::map::magic::ExprId, tmwa::map::magic::ExprField>> = {(tmwa::map::magic::ExprFunApp) = {funp = (fun_t *) nullptr, line_nr = 0, column = 0, args_nr = 0, args = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}}, <No data fields>}'),
            ('tmwa::map::magic::expr_t(tmwa::map::magic::ExprId{123})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::val_t, tmwa::map::magic::e_location_t, tmwa::map::magic::e_area_t, tmwa::map::magic::ExprFunApp, tmwa::map::magic::ExprId, tmwa::map::magic::ExprField>> = {(tmwa::map::magic::ExprId) = {e_id = 123}}, <No data fields>}'),
            ('tmwa::map::magic::expr_t(tmwa::map::magic::ExprField{tmwa::dumb_ptr<tmwa::map::magic::expr_t>(), 42})',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::val_t, tmwa::map::magic::e_location_t, tmwa::map::magic::e_area_t, tmwa::map::magic::ExprFunApp, tmwa::map::magic::ExprId, tmwa::map::magic::ExprField>> = {(tmwa::map::magic::ExprField) = {expr = 0x0, id = 42}}, <No data fields>}'),
    ]


class effect_t(object):
    enabled = True

    tests = [
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectSkip{}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectSkip) = {<No data fields>}}, next = 0x0}'),
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectAbort{}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectAbort) = {<No data fields>}}, next = 0x0}'),
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectAssign{42, tmwa::dumb_ptr<tmwa::map::magic::expr_t>()}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectAssign) = {id = 42, expr = 0x0}}, next = 0x0}'),
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectForEach{123, tmwa::dumb_ptr<tmwa::map::magic::expr_t>(), tmwa::dumb_ptr<tmwa::map::magic::effect_t>(), tmwa::map::magic::FOREACH_FILTER::PC}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectForEach) = {id = 123, area = 0x0, body = 0x0, filter = tmwa::map::magic::FOREACH_FILTER::PC}}, next = 0x0}'),
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectFor{42, tmwa::dumb_ptr<tmwa::map::magic::expr_t>(), tmwa::dumb_ptr<tmwa::map::magic::expr_t>(), tmwa::dumb_ptr<tmwa::map::magic::effect_t>()}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectFor) = {id = 42, start = 0x0, stop = 0x0, body = 0x0}}, next = 0x0}'),
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectIf{tmwa::dumb_ptr<tmwa::map::magic::expr_t>(), tmwa::dumb_ptr<tmwa::map::magic::effect_t>(), tmwa::dumb_ptr<tmwa::map::magic::effect_t>()}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectIf) = {cond = 0x0, true_branch = 0x0, false_branch = 0x0}}, next = 0x0}'),
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectSleep{tmwa::dumb_ptr<tmwa::map::magic::expr_t>()}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectSleep) = {e_sleep = 0x0}}, next = 0x0}'),
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectScript{tmwa::dumb_ptr<const tmwa::map::ScriptBuffer>()}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectScript) = {e_script = 0x0}}, next = 0x0}'),
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectBreak{}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectBreak) = {<No data fields>}}, next = 0x0}'),
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectOp(), tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectOp) = {opp = (op_t *) nullptr, args_nr = 0, line_nr = 0, column = 0, args = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}}, next = 0x0}'),
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectEnd{}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectEnd) = {<No data fields>}}, next = 0x0}'),
            ('tmwa::map::magic::effect_t(tmwa::map::magic::EffectCall{nullptr, tmwa::dumb_ptr<std::vector<tmwa::dumb_ptr<tmwa::map::magic::expr_t>>>(), tmwa::dumb_ptr<tmwa::map::magic::effect_t>()}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::EffectSkip, tmwa::map::magic::EffectAbort, tmwa::map::magic::EffectAssign, tmwa::map::magic::EffectForEach, tmwa::map::magic::EffectFor, tmwa::map::magic::EffectIf, tmwa::map::magic::EffectSleep, tmwa::map::magic::EffectScript, tmwa::map::magic::EffectBreak, tmwa::map::magic::EffectOp, tmwa::map::magic::EffectEnd, tmwa::map::magic::EffectCall>> = {(tmwa::map::magic::EffectCall) = {formalv = nullptr, actualvp = 0x0, body = 0x0}}, next = 0x0}'),
    ]


class spellguard_t(object):
    enabled = True

    tests = [
            ('tmwa::map::magic::spellguard_t(tmwa::map::magic::GuardCondition{tmwa::dumb_ptr<tmwa::map::magic::expr_t>()}, tmwa::dumb_ptr<tmwa::map::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::GuardCondition, tmwa::map::magic::GuardMana, tmwa::map::magic::GuardCastTime, tmwa::map::magic::GuardComponents, tmwa::map::magic::GuardCatalysts, tmwa::map::magic::GuardChoice, tmwa::map::magic::effect_set_t>> = {(tmwa::map::magic::GuardCondition) = {s_condition = 0x0}}, next = 0x0}'),
            ('tmwa::map::magic::spellguard_t(tmwa::map::magic::GuardMana{tmwa::dumb_ptr<tmwa::map::magic::expr_t>()}, tmwa::dumb_ptr<tmwa::map::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::GuardCondition, tmwa::map::magic::GuardMana, tmwa::map::magic::GuardCastTime, tmwa::map::magic::GuardComponents, tmwa::map::magic::GuardCatalysts, tmwa::map::magic::GuardChoice, tmwa::map::magic::effect_set_t>> = {(tmwa::map::magic::GuardMana) = {s_mana = 0x0}}, next = 0x0}'),
            ('tmwa::map::magic::spellguard_t(tmwa::map::magic::GuardCastTime{tmwa::dumb_ptr<tmwa::map::magic::expr_t>()}, tmwa::dumb_ptr<tmwa::map::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::GuardCondition, tmwa::map::magic::GuardMana, tmwa::map::magic::GuardCastTime, tmwa::map::magic::GuardComponents, tmwa::map::magic::GuardCatalysts, tmwa::map::magic::GuardChoice, tmwa::map::magic::effect_set_t>> = {(tmwa::map::magic::GuardCastTime) = {s_casttime = 0x0}}, next = 0x0}'),
            ('tmwa::map::magic::spellguard_t(tmwa::map::magic::GuardComponents{tmwa::dumb_ptr<tmwa::map::magic::component_t>()}, tmwa::dumb_ptr<tmwa::map::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::GuardCondition, tmwa::map::magic::GuardMana, tmwa::map::magic::GuardCastTime, tmwa::map::magic::GuardComponents, tmwa::map::magic::GuardCatalysts, tmwa::map::magic::GuardChoice, tmwa::map::magic::effect_set_t>> = {(tmwa::map::magic::GuardComponents) = {s_components = 0x0}}, next = 0x0}'),
            ('tmwa::map::magic::spellguard_t(tmwa::map::magic::GuardCatalysts{tmwa::dumb_ptr<tmwa::map::magic::component_t>()}, tmwa::dumb_ptr<tmwa::map::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::GuardCondition, tmwa::map::magic::GuardMana, tmwa::map::magic::GuardCastTime, tmwa::map::magic::GuardComponents, tmwa::map::magic::GuardCatalysts, tmwa::map::magic::GuardChoice, tmwa::map::magic::effect_set_t>> = {(tmwa::map::magic::GuardCatalysts) = {s_catalysts = 0x0}}, next = 0x0}'),
            ('tmwa::map::magic::spellguard_t(tmwa::map::magic::GuardChoice{tmwa::dumb_ptr<tmwa::map::magic::spellguard_t>()}, tmwa::dumb_ptr<tmwa::map::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::GuardCondition, tmwa::map::magic::GuardMana, tmwa::map::magic::GuardCastTime, tmwa::map::magic::GuardComponents, tmwa::map::magic::GuardCatalysts, tmwa::map::magic::GuardChoice, tmwa::map::magic::effect_set_t>> = {(tmwa::map::magic::GuardChoice) = {s_alt = 0x0}}, next = 0x0}'),
            ('tmwa::map::magic::spellguard_t(tmwa::map::magic::effect_set_t{tmwa::dumb_ptr<tmwa::map::magic::effect_t>(), tmwa::dumb_ptr<tmwa::map::magic::effect_t>(), tmwa::dumb_ptr<tmwa::map::magic::effect_t>()}, tmwa::dumb_ptr<tmwa::map::magic::spellguard_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::GuardCondition, tmwa::map::magic::GuardMana, tmwa::map::magic::GuardCastTime, tmwa::map::magic::GuardComponents, tmwa::map::magic::GuardCatalysts, tmwa::map::magic::GuardChoice, tmwa::map::magic::effect_set_t>> = {(tmwa::map::magic::effect_set_t) = {effect = 0x0, at_trigger = 0x0, at_end = 0x0}}, next = 0x0}'),
    ]


class cont_activation_record_t(object):
    enabled = True

    tests = [
            ('tmwa::map::magic::cont_activation_record_t(tmwa::map::magic::CarForEach{42, true, tmwa::dumb_ptr<tmwa::map::magic::effect_t>(), tmwa::dumb_ptr<std::vector<tmwa::BlockId>>(), 123}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::CarForEach, tmwa::map::magic::CarFor, tmwa::map::magic::CarProc>> = {(tmwa::map::magic::CarForEach) = {id = 42, ty_is_spell_not_entity = true, body = 0x0, entities_vp = 0x0, index = 123}}, return_location = 0x0}'),
            ('tmwa::map::magic::cont_activation_record_t(tmwa::map::magic::CarFor{42, tmwa::dumb_ptr<tmwa::map::magic::effect_t>(), 123, 456}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::CarForEach, tmwa::map::magic::CarFor, tmwa::map::magic::CarProc>> = {(tmwa::map::magic::CarFor) = {id = 42, body = 0x0, current = 123, stop = 456}}, return_location = 0x0}'),
            ('tmwa::map::magic::cont_activation_record_t(tmwa::map::magic::CarProc{123, nullptr, tmwa::dumb_ptr<tmwa::map::magic::val_t[]>()}, tmwa::dumb_ptr<tmwa::map::magic::effect_t>())',
                '{<tmwa::sexpr::Variant<tmwa::map::magic::CarForEach, tmwa::map::magic::CarFor, tmwa::map::magic::CarProc>> = {(tmwa::map::magic::CarProc) = {args_nr = 123, formalap = nullptr, old_actualpa = 0x0 = {sz = 0}}}, return_location = 0x0}'),
    ]
