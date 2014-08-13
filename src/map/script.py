class script_data(object):
    enabled = True

    tests = [
    ('tmwa::script_data(tmwa::ScriptDataPos{42})',
                '{<tmwa::sexpr::Variant<tmwa::ScriptDataPos, tmwa::ScriptDataInt, tmwa::ScriptDataParam, tmwa::ScriptDataStr, tmwa::ScriptDataArg, tmwa::ScriptDataVariable, tmwa::ScriptDataRetInfo, tmwa::ScriptDataFuncRef>> = {(tmwa::ScriptDataPos) = {numi = 42}}, <No data fields>}'),
    ('tmwa::script_data(tmwa::ScriptDataInt{123})',
                '{<tmwa::sexpr::Variant<tmwa::ScriptDataPos, tmwa::ScriptDataInt, tmwa::ScriptDataParam, tmwa::ScriptDataStr, tmwa::ScriptDataArg, tmwa::ScriptDataVariable, tmwa::ScriptDataRetInfo, tmwa::ScriptDataFuncRef>> = {(tmwa::ScriptDataInt) = {numi = 123}}, <No data fields>}'),
    ('tmwa::script_data(tmwa::ScriptDataParam{tmwa::SIR()})',
                '{<tmwa::sexpr::Variant<tmwa::ScriptDataPos, tmwa::ScriptDataInt, tmwa::ScriptDataParam, tmwa::ScriptDataStr, tmwa::ScriptDataArg, tmwa::ScriptDataVariable, tmwa::ScriptDataRetInfo, tmwa::ScriptDataFuncRef>> = {(tmwa::ScriptDataParam) = {reg = {impl = 0}}}, <No data fields>}'),
    ('tmwa::script_data(tmwa::ScriptDataStr{"Hello"_s})',
                '{<tmwa::sexpr::Variant<tmwa::ScriptDataPos, tmwa::ScriptDataInt, tmwa::ScriptDataParam, tmwa::ScriptDataStr, tmwa::ScriptDataArg, tmwa::ScriptDataVariable, tmwa::ScriptDataRetInfo, tmwa::ScriptDataFuncRef>> = {(tmwa::ScriptDataStr) = {str = "Hello"}}, <No data fields>}'),
    ('tmwa::script_data(tmwa::ScriptDataArg{0})',
                '{<tmwa::sexpr::Variant<tmwa::ScriptDataPos, tmwa::ScriptDataInt, tmwa::ScriptDataParam, tmwa::ScriptDataStr, tmwa::ScriptDataArg, tmwa::ScriptDataVariable, tmwa::ScriptDataRetInfo, tmwa::ScriptDataFuncRef>> = {(tmwa::ScriptDataArg) = {numi = 0}}, <No data fields>}'),
    ('tmwa::script_data(tmwa::ScriptDataVariable{tmwa::SIR()})',
                '{<tmwa::sexpr::Variant<tmwa::ScriptDataPos, tmwa::ScriptDataInt, tmwa::ScriptDataParam, tmwa::ScriptDataStr, tmwa::ScriptDataArg, tmwa::ScriptDataVariable, tmwa::ScriptDataRetInfo, tmwa::ScriptDataFuncRef>> = {(tmwa::ScriptDataVariable) = {reg = {impl = 0}}}, <No data fields>}'),
    ('tmwa::script_data(tmwa::ScriptDataRetInfo{static_cast<const tmwa::ScriptBuffer *>(nullptr)})',
                '{<tmwa::sexpr::Variant<tmwa::ScriptDataPos, tmwa::ScriptDataInt, tmwa::ScriptDataParam, tmwa::ScriptDataStr, tmwa::ScriptDataArg, tmwa::ScriptDataVariable, tmwa::ScriptDataRetInfo, tmwa::ScriptDataFuncRef>> = {(tmwa::ScriptDataRetInfo) = {script = 0x0}}, <No data fields>}'),
    ('tmwa::script_data(tmwa::ScriptDataFuncRef{404})',
                '{<tmwa::sexpr::Variant<tmwa::ScriptDataPos, tmwa::ScriptDataInt, tmwa::ScriptDataParam, tmwa::ScriptDataStr, tmwa::ScriptDataArg, tmwa::ScriptDataVariable, tmwa::ScriptDataRetInfo, tmwa::ScriptDataFuncRef>> = {(tmwa::ScriptDataFuncRef) = {numi = 404}}, <No data fields>}'),
    ]
