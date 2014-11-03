#pragma once
//    ast/script.hpp - Structure of tmwa-script
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "fwd.hpp"

#include "../compat/result.hpp"

#include "../io/span.hpp"


namespace tmwa
{
namespace ast
{
namespace script
{
    using io::Spanned;

    struct ScriptBody
    {
        RString braced_body;
        io::LineSpan span;
    };

    struct ScriptOptions
    {
        // don't require a label at the beginning
        bool implicit_start = false;
        // label to generate at the beginning if not already present
        RString default_label;
        // beginning must be only 'end;'
        bool no_start;
        // don't requite an 'end;' at the end
        bool implicit_end = false;
        // forbid newlines anywhere between { and }
        bool one_line = false;
        // forbid the OnTouch event
        bool no_touch = false;
        // forbid all events
        bool no_event = false;
    };

    Result<ScriptBody> parse_script_body(io::LineCharReader& lr, ScriptOptions opt);

    /*
    (-- First bare-body-chunk only allowed for npcs, items, magic, functions.
        It is not allowed for events. Basically it's an implicit label at times.
        Last normal-lines is only permitted on item and magic scripts. --)
    { script-body }: "{" bare-body-chunk? body-chunk* normal-lines? "}"
    body-chunk: (comment* labelname ":")+ bare-body-chunk
    bare-body-chunk: normal-lines terminator-line
    normal-lines: normal-line*
    any-line: normal-line
    any-line: terminator-line
    normal-line: "if" "(" expr ")" any-line
    normal-line: normal-command ((expr ",")* expr)? ";"
    terminator-line: "menu" (expr, labelname)* expr, labelname ";"
    terminator-line: "goto" labelname ";"
    terminator-line: terminator ((expr ",")* expr)? ";"
    terminator: "return"
    terminator: "close"
    terminator: "end"
    terminator: "mapexit"
    terminator: "shop"

    expr: subexpr_-1
    subexpr_N: ("+" | "-" | "!" | "~") subexpr_7
    subexpr_N: simple-expr (op_M subexpr_M | "(" ((expr ",")+ expr)? ")")*            if N < M; function call only if N < 8 and preceding simple-expr (op sub)* is a known function
    op_0: "||"
    op_1: "&&"
    op_2: "=="
    op_2: "!="
    op_2: ">="
    op_2: ">"
    op_2: "<="
    op_2: "<"
    op_3: "^"
    op_4: "|"
    op_5: "&"
    op_5: ">>"
    op_5: "<<"
    op_6: "+"
    op_6: "-"
    op_7: "*"
    op_7: "/"
    op_7: "%"
    simple-expr: "(" expr ")"
    simple-expr: integer
    simple-expr: string
    simple-expr: variable ("[" expr "]")?
    simple-expr: function // no longer command/label though
    */
} // namespace script
} // namespace ast
} // namespace tmwa
