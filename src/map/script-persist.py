class script_data(object):
    enabled = True

    test_extra = '''
    #include "../strings/literal.hpp"
    #include <vector>
    using tmwa::operator "" _s;

    #include "../map/script-parse-internal.hpp"

    static
    tmwa::Borrowed<const tmwa::map::ScriptBuffer> fake_script()
    {
        static
        const std::vector<tmwa::map::ByteCode> buffer;
        return tmwa::borrow(reinterpret_cast<const tmwa::map::ScriptBuffer&>(buffer));
    }

    '''

    tests = [
    ('tmwa::map::script_data(tmwa::map::ScriptDataPos{42})',
                '{<tmwa::sexpr::Variant<tmwa::map::ScriptDataPos, tmwa::map::ScriptDataInt, tmwa::map::ScriptDataParam, tmwa::map::ScriptDataStr, tmwa::map::ScriptDataArg, tmwa::map::ScriptDataVariable, tmwa::map::ScriptDataRetInfo, tmwa::map::ScriptDataFuncRef>> = {(tmwa::map::ScriptDataPos) = {numi = 42}}, <No data fields>}'),
    ('tmwa::map::script_data(tmwa::map::ScriptDataInt{123})',
                '{<tmwa::sexpr::Variant<tmwa::map::ScriptDataPos, tmwa::map::ScriptDataInt, tmwa::map::ScriptDataParam, tmwa::map::ScriptDataStr, tmwa::map::ScriptDataArg, tmwa::map::ScriptDataVariable, tmwa::map::ScriptDataRetInfo, tmwa::map::ScriptDataFuncRef>> = {(tmwa::map::ScriptDataInt) = {numi = 123}}, <No data fields>}'),
    ('tmwa::map::script_data(tmwa::map::ScriptDataParam{tmwa::map::SIR()})',
                '{<tmwa::sexpr::Variant<tmwa::map::ScriptDataPos, tmwa::map::ScriptDataInt, tmwa::map::ScriptDataParam, tmwa::map::ScriptDataStr, tmwa::map::ScriptDataArg, tmwa::map::ScriptDataVariable, tmwa::map::ScriptDataRetInfo, tmwa::map::ScriptDataFuncRef>> = {(tmwa::map::ScriptDataParam) = {reg = {impl = 0}}}, <No data fields>}'),
    ('tmwa::map::script_data(tmwa::map::ScriptDataStr{"Hello"_s})',
                '{<tmwa::sexpr::Variant<tmwa::map::ScriptDataPos, tmwa::map::ScriptDataInt, tmwa::map::ScriptDataParam, tmwa::map::ScriptDataStr, tmwa::map::ScriptDataArg, tmwa::map::ScriptDataVariable, tmwa::map::ScriptDataRetInfo, tmwa::map::ScriptDataFuncRef>> = {(tmwa::map::ScriptDataStr) = {str = "Hello"}}, <No data fields>}'),
    ('tmwa::map::script_data(tmwa::map::ScriptDataArg{0})',
                '{<tmwa::sexpr::Variant<tmwa::map::ScriptDataPos, tmwa::map::ScriptDataInt, tmwa::map::ScriptDataParam, tmwa::map::ScriptDataStr, tmwa::map::ScriptDataArg, tmwa::map::ScriptDataVariable, tmwa::map::ScriptDataRetInfo, tmwa::map::ScriptDataFuncRef>> = {(tmwa::map::ScriptDataArg) = {numi = 0}}, <No data fields>}'),
    ('tmwa::map::script_data(tmwa::map::ScriptDataVariable{tmwa::map::SIR()})',
                '{<tmwa::sexpr::Variant<tmwa::map::ScriptDataPos, tmwa::map::ScriptDataInt, tmwa::map::ScriptDataParam, tmwa::map::ScriptDataStr, tmwa::map::ScriptDataArg, tmwa::map::ScriptDataVariable, tmwa::map::ScriptDataRetInfo, tmwa::map::ScriptDataFuncRef>> = {(tmwa::map::ScriptDataVariable) = {reg = {impl = 0}}}, <No data fields>}'),
    ('tmwa::map::script_data(tmwa::map::ScriptDataRetInfo{fake_script()})',
        '{<tmwa::sexpr::Variant<tmwa::map::ScriptDataPos, tmwa::map::ScriptDataInt, tmwa::map::ScriptDataParam, tmwa::map::ScriptDataStr, tmwa::map::ScriptDataArg, tmwa::map::ScriptDataVariable, tmwa::map::ScriptDataRetInfo, tmwa::map::ScriptDataFuncRef>> = {(tmwa::map::ScriptDataRetInfo) = {script = <fake_script()::buffer>}}, <No data fields>}'),
    ('tmwa::map::script_data(tmwa::map::ScriptDataFuncRef{404})',
                '{<tmwa::sexpr::Variant<tmwa::map::ScriptDataPos, tmwa::map::ScriptDataInt, tmwa::map::ScriptDataParam, tmwa::map::ScriptDataStr, tmwa::map::ScriptDataArg, tmwa::map::ScriptDataVariable, tmwa::map::ScriptDataRetInfo, tmwa::map::ScriptDataFuncRef>> = {(tmwa::map::ScriptDataFuncRef) = {numi = 404}}, <No data fields>}'),
    ]
