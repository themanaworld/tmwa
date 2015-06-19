#include "script-parse-internal.hpp"
//    script-parse.cpp - EAthena script frontend, engine, and library.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011 Chuck Miller
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2013 wushin
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <set>

#include "../generic/array.hpp"
#include "../generic/db.hpp"
#include "../generic/intern-pool.hpp"

#include "../strings/rstring.hpp"

#include "../io/cxxstdio.hpp"

#include "../mmo/cxxstdio_enums.hpp"

#include "../ast/script.hpp"

#include "globals.hpp"
#include "map.t.hpp"
#include "script-buffer.hpp"
#include "script-call.hpp"
#include "script-fun.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
constexpr bool DEBUG_DISP = false;

class ScriptBuffer
{
    typedef ZString::iterator ZSit;

    std::vector<ByteCode> script_buf;
    RString debug_name;
    std::vector<std::pair<ScriptLabel, size_t>> debug_labels;
public:
    ScriptBuffer(RString name) : debug_name(std::move(name)) {}

    // construction methods
    void add_scriptc(ByteCode a);
    void add_scriptb(uint8_t a);
    void add_scripti(uint32_t a);
    void add_scriptl(Borrowed<str_data_t> a);
    void set_label(Borrowed<str_data_t> ld, int pos_);
    ZSit parse_simpleexpr(ZSit p);
    ZSit parse_subexpr(ZSit p, int limit);
    ZSit parse_expr(ZSit p);
    ZSit parse_line(ZSit p, bool *canstep);
    void parse_script(ZString src, int line, bool implicit_end);

    // consumption methods
    ByteCode operator[](size_t i) const { return script_buf[i]; }
    ZString get_str(size_t i) const
    {
        return ZString(strings::really_construct_from_a_pointer, reinterpret_cast<const char *>(&script_buf[i]), nullptr);
    }
};
} // namespace map
} // namespace tmwa

void std::default_delete<const tmwa::map::ScriptBuffer>::operator()(const tmwa::map::ScriptBuffer *sd)
{
    really_delete1 sd;
}

namespace tmwa
{
namespace map
{
// implemented for script-call.hpp because reasons
ByteCode ScriptPointer::peek() const { return (*TRY_UNWRAP(code, abort()))[pos]; }
ByteCode ScriptPointer::pop() { return (*TRY_UNWRAP(code, abort()))[pos++]; }
ZString ScriptPointer::pops()
{
    ZString rv = TRY_UNWRAP(code, abort())->get_str(pos);
    pos += rv.size();
    ++pos;
    return rv;
}

static
struct ScriptConfigParse
{
    static const
    int warn_func_no_comma = 1;
    static const
    int warn_cmd_no_comma = 1;
    static const
    int warn_func_mismatch_paramnum = 1;
    static const
    int warn_cmd_mismatch_paramnum = 1;
} script_config;


Option<Borrowed<str_data_t>> search_strp(XString p)
{
    return str_datam.search(p);
}

Borrowed<str_data_t> add_strp(XString p)
{
    Option<P<str_data_t>> rv_ = search_strp(p);
    OMATCH_BEGIN_SOME (rv, rv_)
    {
        return rv;
    }
    OMATCH_END ();

    RString p2 = p;
    P<str_data_t> datum = str_datam.init(p2);
    datum->type = StringCode::NOP;
    datum->strs = p2;
    datum->backpatch = -1;
    datum->label_ = -1;
    return datum;
}

/*==========================================
 * スクリプトバッファに１バイト書き込む
 *------------------------------------------
 */
void ScriptBuffer::add_scriptc(ByteCode a)
{
    script_buf.push_back(a);
}

/*==========================================
 * スクリプトバッファにデータタイプを書き込む
 *------------------------------------------
 */
void ScriptBuffer::add_scriptb(uint8_t a)
{
    add_scriptc(static_cast<ByteCode>(a));
}

/*==========================================
 * スクリプトバッファに整数を書き込む
 *------------------------------------------
 */
void ScriptBuffer::add_scripti(uint32_t a)
{
    while (a >= 0x40)
    {
        add_scriptb(a | 0xc0);
        a = (a - 0x40) >> 6;
    }
    add_scriptb(a | 0x80);
}

/*==========================================
 * スクリプトバッファにラベル/変数/関数を書き込む
 *------------------------------------------
 */
// 最大16Mまで
void ScriptBuffer::add_scriptl(P<str_data_t> ld)
{
    int backpatch = ld->backpatch;

    switch (ld->type)
    {
        case StringCode::POS:
            add_scriptc(ByteCode::POS);
            add_scriptb(static_cast<uint8_t>(ld->label_));
            add_scriptb(static_cast<uint8_t>(ld->label_ >> 8));
            add_scriptb(static_cast<uint8_t>(ld->label_ >> 16));
            break;
        case StringCode::NOP:
            // need to set backpatch, because it might become a label later
            add_scriptc(ByteCode::VARIABLE);
            ld->backpatch = script_buf.size();
            add_scriptb(static_cast<uint8_t>(backpatch));
            add_scriptb(static_cast<uint8_t>(backpatch >> 8));
            add_scriptb(static_cast<uint8_t>(backpatch >> 16));
            break;
        case StringCode::INT:
            add_scripti(ld->val);
            break;
        case StringCode::FUNC:
            add_scriptc(ByteCode::FUNC_REF);
            add_scriptb(static_cast<uint8_t>(ld->val));
            add_scriptb(static_cast<uint8_t>(ld->val >> 8));
            add_scriptb(static_cast<uint8_t>(ld->val >> 16));
            break;
        case StringCode::PARAM:
            add_scriptc(ByteCode::PARAM);
            add_scriptb(static_cast<uint8_t>(ld->val));
            add_scriptb(static_cast<uint8_t>(ld->val >> 8));
            add_scriptb(static_cast<uint8_t>(ld->val >> 16));
            break;
        default:
            abort();
    }
}

/*==========================================
 * ラベルを解決する
 *------------------------------------------
 */
void ScriptBuffer::set_label(Borrowed<str_data_t> ld, int pos_)
{
    int next;

    ld->type = StringCode::POS;
    ld->label_ = pos_;
    for (int i = ld->backpatch; i >= 0 && i != 0x00ffffff; i = next)
    {
        next = 0;
        // woot! no longer endian-dependent!
        next |= static_cast<uint8_t>(script_buf[i + 0]) << 0;
        next |= static_cast<uint8_t>(script_buf[i + 1]) << 8;
        next |= static_cast<uint8_t>(script_buf[i + 2]) << 16;
        script_buf[i - 1] = ByteCode::POS;
        script_buf[i] = static_cast<ByteCode>(pos_);
        script_buf[i + 1] = static_cast<ByteCode>(pos_ >> 8);
        script_buf[i + 2] = static_cast<ByteCode>(pos_ >> 16);
    }
}

/*==========================================
 * スペース/コメント読み飛ばし
 *------------------------------------------
 */
static
ZString::iterator skip_space(ZString::iterator p)
{
    while (1)
    {
        while (isspace(*p))
            p++;
        if (p[0] == '/' && p[1] == '/')
        {
            while (*p && *p != '\n')
                p++;
        }
        else if (p[0] == '/' && p[1] == '*')
        {
            p++;
            while (*p && (p[-1] != '*' || p[0] != '/'))
                p++;
            if (*p)
                p++;
        }
        else
            break;
    }
    return p;
}

/*==========================================
 * １単語スキップ
 *------------------------------------------
 */
static
ZString::iterator skip_word(ZString::iterator p)
{
    // prefix
    if (*p == '$')
        p++;                    // MAP鯖内共有変数用
    if (*p == '@')
        p++;                    // 一時的変数用(like weiss)
    if (*p == '.')
        p++;                    // npc
    if (*p == '@')
        p++;                    // scope
    if (*p == '#')
        p++;                    // account変数用
    if (*p == '#')
        p++;                    // ワールドaccount変数用

    while (isalnum(*p) || *p == '_')
        p++;

    // postfix
    if (*p == '$')
        p++;                    // 文字列変数

    return p;
}

/*==========================================
 * エラーメッセージ出力
 *------------------------------------------
 */
static
void disp_error_message(ZString mes, ZString::iterator pos_)
{
    script_errors++;

    assert (startptr.begin() <= pos_ && pos_ <= startptr.end());

    int line;
    ZString::iterator p;

    for (line = startline, p = startptr.begin(); p != startptr.end(); line++)
    {
        ZString::iterator linestart = p;
        ZString::iterator lineend = std::find(p, startptr.end(), '\n');
        if (pos_ < lineend)
        {
            PRINTF("\n%s\nline %d : "_fmt, mes, line);
            for (int i = 0; linestart + i != lineend; i++)
            {
                if (linestart + i != pos_)
                    PRINTF("%c"_fmt, linestart[i]);
                else
                    PRINTF("\'%c\'"_fmt, linestart[i]);
            }
            PRINTF("\a\n"_fmt);
            return;
        }
        p = lineend + 1;
    }
}

/*==========================================
 * 項の解析
 *------------------------------------------
 */
ZString::iterator ScriptBuffer::parse_simpleexpr(ZString::iterator p)
{
    p = skip_space(p);

    if (*p == ';' || *p == ',')
    {
        disp_error_message("unexpected expr end"_s, p);
        exit(1);
    }
    if (*p == '(')
    {

        p = parse_subexpr(p + 1, -1);
        p = skip_space(p);
        if ((*p++) != ')')
        {
            disp_error_message("unmatch ')'"_s, p);
            exit(1);
        }
    }
    else if (isdigit(*p) || ((*p == '-' || *p == '+') && isdigit(p[1])))
    {
        char *np;
        int i = strtoul(&*p, &np, 0);
        add_scripti(i);
        p += np - &*p;
    }
    else if (*p == '"')
    {
        add_scriptc(ByteCode::STR);
        p++;
        while (*p && *p != '"')
        {
            if (*p == '\\')
                p++;
            else if (*p == '\n')
            {
                disp_error_message("unexpected newline @ string"_s, p);
                exit(1);
            }
            add_scriptb(*p++);
        }
        if (!*p)
        {
            disp_error_message("unexpected eof @ string"_s, p);
            exit(1);
        }
        add_scriptb(0);
        p++;                    //'"'
    }
    else
    {
        // label , register , function etc
        ZString::iterator p2 = skip_word(p);
        if (p2 == p)
        {
            disp_error_message("unexpected character"_s, p);
            exit(1);
        }
        XString word(&*p, &*p2, nullptr);
        if (word.startswith("On"_s) || word.startswith("L_"_s) || word.startswith("S_"_s))
            probable_labels.insert(stringish<ScriptLabel>(word));
        if (parse_cmd_if && (word == "callsub"_s || word == "callfunc"_s || word == "return"_s))
        {
            disp_error_message("Sorry, callsub/callfunc/return have never worked properly in an if statement."_s, p);
        }
        P<str_data_t> ld = add_strp(word);

        parse_cmdp = Some(ld);          // warn_*_mismatch_paramnumのために必要
        // why not just check l->str == "if"_s or std::string(p, p2) == "if"_s?
        if (Some(ld) == search_strp("if"_s) || Some(ld) == search_strp("elif"_s)
            || Some(ld) == search_strp("else"_s)) // warn_cmd_no_commaのために必要
            parse_cmd_if++;
        p = p2;

        if (ld->type != StringCode::FUNC && *p == '[')
        {
            // array(name[i] => getelementofarray(name,i) )
            add_scriptl(TRY_UNWRAP(search_strp("getelementofarray"_s), abort()));
            add_scriptc(ByteCode::ARG);
            add_scriptl(ld);
            p = parse_subexpr(p + 1, -1);
            p = skip_space(p);
            if (*p != ']')
            {
                disp_error_message("unmatch ']'"_s, p);
                exit(1);
            }
            p++;
            add_scriptc(ByteCode::FUNC);
        }
        else
            add_scriptl(ld);

    }

    return p;
}

/*==========================================
 * 式の解析
 *------------------------------------------
 */
ZString::iterator ScriptBuffer::parse_subexpr(ZString::iterator p, int limit)
{
    ByteCode op;
    int opl, len;

    p = skip_space(p);

    if (*p == '-')
    {
        ZString::iterator tmpp = skip_space(p + 1);
        if (*tmpp == ';' || *tmpp == ',')
        {
            disp_error_message("error: implicit 'next statement' label"_s, p);
            add_scriptl(borrow(LABEL_NEXTLINE_));
            p++;
            return p;
        }
    }
    ZString::iterator tmpp = p;
    if ((op = ByteCode::NEG, *p == '-') || (op = ByteCode::LNOT, *p == '!')
        || (op = ByteCode::NOT, *p == '~'))
    {
        p = parse_subexpr(p + 1, 100);
        add_scriptc(op);
    }
    else
        p = parse_simpleexpr(p);
    p = skip_space(p);
    while (((op = ByteCode::ADD, opl = 6, len = 1, *p == '+') ||
            (op = ByteCode::SUB, opl = 6, len = 1, *p == '-') ||
            (op = ByteCode::MUL, opl = 7, len = 1, *p == '*') ||
            (op = ByteCode::DIV, opl = 7, len = 1, *p == '/') ||
            (op = ByteCode::MOD, opl = 7, len = 1, *p == '%') ||
            (op = ByteCode::FUNC, opl = 8, len = 1, *p == '(') ||
            (op = ByteCode::LAND, opl = 1, len = 2, *p == '&' && p[1] == '&') ||
            (op = ByteCode::AND, opl = 5, len = 1, *p == '&') ||
            (op = ByteCode::LOR, opl = 0, len = 2, *p == '|' && p[1] == '|') ||
            (op = ByteCode::OR, opl = 4, len = 1, *p == '|') ||
            (op = ByteCode::XOR, opl = 3, len = 1, *p == '^') ||
            (op = ByteCode::EQ, opl = 2, len = 2, *p == '=' && p[1] == '=') ||
            (op = ByteCode::NE, opl = 2, len = 2, *p == '!' && p[1] == '=') ||
            (op = ByteCode::R_SHIFT, opl = 5, len = 2, *p == '>' && p[1] == '>') ||
            (op = ByteCode::GE, opl = 2, len = 2, *p == '>' && p[1] == '=') ||
            (op = ByteCode::GT, opl = 2, len = 1, *p == '>') ||
            (op = ByteCode::L_SHIFT, opl = 5, len = 2, *p == '<' && p[1] == '<') ||
            (op = ByteCode::LE, opl = 2, len = 2, *p == '<' && p[1] == '=') ||
            (op = ByteCode::LT, opl = 2, len = 1, *p == '<')) && opl > limit)
    {
        p += len;
        if (op == ByteCode::FUNC)
        {
            int i = 0;
            P<str_data_t> funcp = TRY_UNWRAP(parse_cmdp, abort());
            Array<ZString::iterator, 128> plist;

            if (funcp->type != StringCode::FUNC)
            {
                disp_error_message("expect function"_s, tmpp);
                exit(0);
            }

            add_scriptc(ByteCode::ARG);
            while (*p && *p != ')' && i < 128)
            {
                plist[i] = p;
                p = parse_subexpr(p, -1);
                p = skip_space(p);
                if (*p == ',')
                    p++;
                else if (*p != ')' && script_config.warn_func_no_comma)
                {
                    disp_error_message("expect ',' or ')' at func params"_s,
                                        p);
                }
                p = skip_space(p);
                i++;
            }
            if (i == 128)
            {
                disp_error_message("PANIC: unrecoverable error in function argument list"_s, p);
                abort();
            }
            plist[i] = p;
            if (*p != ')')
            {
                disp_error_message("func request '(' ')'"_s, p);
                exit(1);
            }
            p++;

            if (funcp->type == StringCode::FUNC
                && script_config.warn_func_mismatch_paramnum)
            {
                ZString arg = builtin_functions[funcp->val].arg;
                int j = 0;
                // TODO handle ? and multiple * correctly
                for (j = 0; arg[j]; j++)
                    if (arg[j] == '*' || arg[j] == '?')
                        break;
                if ((arg[j] == 0 && i != j) || ((arg[j] == '*' || arg[j] == '?') && i < j))
                {
                    disp_error_message("illegal number of parameters"_s,
                            plist[std::min(i, j)]);
                }
                if (!builtin_functions[funcp->val].ret)
                {
                    disp_error_message("statement in function context"_s, tmpp);
                }
            }
        }
        else // not op == ByteCode::FUNC
        {
            p = parse_subexpr(p, opl);
        }
        add_scriptc(op);
        p = skip_space(p);
    }
    return p;                   /* return first untreated operator */
}

/*==========================================
 * 式の評価
 *------------------------------------------
 */
ZString::iterator ScriptBuffer::parse_expr(ZString::iterator p)
{
    switch (*p)
    {
        case ')':
        case ';':
        case ':':
        case '[':
        case ']':
        case '}':
            disp_error_message("unexpected char"_s, p);
            exit(1);
    }
    p = parse_subexpr(p, -1);
    return p;
}

/*==========================================
 * 行の解析
 *------------------------------------------
 */
ZString::iterator ScriptBuffer::parse_line(ZString::iterator p, bool *can_step)
{
    int i = 0;
    Array<ZString::iterator, 128> plist;

    p = skip_space(p);
    if (*p == ';')
    {
        disp_error_message("Double semi-colon"_s, p);
        ++p;
        return p;
    }

    parse_cmd_if = 0;           // warn_cmd_no_commaのために必要

    // 最初は関数名
    ZString::iterator p2 = p;
    p = parse_simpleexpr(p);
    p = skip_space(p);

    P<str_data_t> cmd = TRY_UNWRAP(parse_cmdp, abort());
    if (cmd->type != StringCode::FUNC)
    {
        disp_error_message("expect command"_s, p2);
    }

    {
        // TODO should be LString, but no heterogenous lookup yet
        static
        std::set<ZString> terminators =
        {
            "goto"_s,
            "return"_s,
            "close"_s,
            "menu"_s,
            "end"_s,
            "mapexit"_s,
            "shop"_s,
            "destroy"_s,
        };
        *can_step = terminators.count(cmd->strs) == 0;
    }

    add_scriptc(ByteCode::ARG);
    while (*p && *p != ';' && i < 128)
    {
        plist[i] = p;

        p = parse_expr(p);
        p = skip_space(p);
        // 引数区切りの,処理
        if (*p == ',')
            p++;
        else if (*p != ';' && script_config.warn_cmd_no_comma
                 && parse_cmd_if * 2 <= i)
        {
            disp_error_message("expect ',' or ';' at cmd params"_s, p);
        }
        p = skip_space(p);
        i++;
    }
    if (i == 128)
    {
        disp_error_message("PANIC: unknown error in command argument list"_s, p);
        abort();
    }
    plist[i] = p;
    if (*(p++) != ';')
    {
        disp_error_message("need ';'"_s, p);
        exit(1);
    }
    add_scriptc(ByteCode::FUNC);

    if (cmd->type == StringCode::FUNC
        && script_config.warn_cmd_mismatch_paramnum)
    {
        ZString arg = builtin_functions[cmd->val].arg;
        int j = 0;
        // TODO see above
        for (j = 0; arg[j]; j++)
            if (arg[j] == '*' || arg[j] == '?')
                break;
        if ((arg[j] == 0 && i != j) || ((arg[j] == '*' || arg[j] == '?') && i < j))
        {
            disp_error_message("illegal number of parameters"_s,
                    plist[std::min(i, j)]);
        }
        if (builtin_functions[cmd->val].ret)
        {
            disp_error_message("function in statement context"_s, p2);
        }
    }

    return p;
}

/*==========================================
 * 組み込み関数の追加
 *------------------------------------------
 */
static
void add_builtin_functions(void)
{
    for (int i = 0; builtin_functions[i].func; i++)
    {
        P<str_data_t> n = add_strp(builtin_functions[i].name);
        n->type = StringCode::FUNC;
        n->val = i;
    }
}

std::unique_ptr<const ScriptBuffer> compile_script(RString debug_name, const ast::script::ScriptBody& body, bool implicit_end)
{
    auto script_buf = make_unique<ScriptBuffer>(std::move(debug_name));
    script_buf->parse_script(body.braced_body, body.span.begin.line, implicit_end);
    return std::move(script_buf);
}

/*==========================================
 * スクリプトの解析
 *------------------------------------------
 */
void ScriptBuffer::parse_script(ZString src, int line, bool implicit_end)
{
    static int first = 1;

    if (first)
    {
        add_builtin_functions();
    }
    first = 0;
    LABEL_NEXTLINE_.type = StringCode::NOP;
    LABEL_NEXTLINE_.backpatch = -1;
    LABEL_NEXTLINE_.label_ = -1;
    for (auto& pair : str_datam)
    {
        str_data_t& dit = pair.second;
        if (dit.type == StringCode::POS || dit.type == StringCode::VARIABLE)
        {
            dit.type = StringCode::NOP;
            dit.backpatch = -1;
            dit.label_ = -1;
        }
    }

    // 外部用label dbの初期化
    scriptlabel_db.clear();

    // for error message
    startptr = src;
    startline = line;

    bool can_step = true;

    ZString::iterator p = src.begin();
    p = skip_space(p);
    if (*p != '{')
    {
        disp_error_message("not found '{'"_s, p);
        abort();
    }
    for (p++; *p && *p != '}';)
    {
        p = skip_space(p);
        if (*skip_space(skip_word(p)) == ':')
        {
            if (can_step)
            {
                disp_error_message("error: implicit fallthrough"_s, p);
            }
            can_step = true;

            ZString::iterator tmpp = skip_word(p);
            XString str(&*p, &*tmpp, nullptr);
            P<str_data_t> ld = add_strp(str);
            bool e1 = ld->type != StringCode::NOP;
            bool e2 = ld->type == StringCode::POS;
            bool e3 = ld->label_ != -1;
            assert (e1 == e2 && e2 == e3);
            if (e3)
            {
                disp_error_message("dup label "_s, p);
                exit(1);
            }
            set_label(ld, script_buf.size());
            scriptlabel_db.insert(stringish<ScriptLabel>(str), script_buf.size());
            debug_labels.push_back(std::make_pair(stringish<ScriptLabel>(str), script_buf.size()));
            p = tmpp + 1;
            continue;
        }

        if (!can_step)
        {
            disp_error_message("error: unreachable statement"_s, p);
        }
        // 他は全部一緒くた
        p = parse_line(p, &can_step);
        p = skip_space(p);
        add_scriptc(ByteCode::EOL);

        set_label(borrow(LABEL_NEXTLINE_), script_buf.size());
        LABEL_NEXTLINE_.type = StringCode::NOP;
        LABEL_NEXTLINE_.backpatch = -1;
        LABEL_NEXTLINE_.label_ = -1;
    }

    if (can_step && !implicit_end)
    {
        disp_error_message("error: implicit end"_s, p);
    }
    add_scriptc(ByteCode::NOP);

    // resolve the unknown labels
    for (auto& pair : str_datam)
    {
        str_data_t& sit = pair.second;
        if (sit.type == StringCode::NOP)
        {
            sit.type = StringCode::VARIABLE;
            sit.label_ = 0; // anything but -1. Shouldn't matter, but helps asserts.
            size_t pool_index = variable_names.intern(sit.strs);
            for (int next, j = sit.backpatch; j >= 0 && j != 0x00ffffff; j = next)
            {
                next = 0;
                next |= static_cast<uint8_t>(script_buf[j + 0]) << 0;
                next |= static_cast<uint8_t>(script_buf[j + 1]) << 8;
                next |= static_cast<uint8_t>(script_buf[j + 2]) << 16;
                script_buf[j] = static_cast<ByteCode>(pool_index);
                script_buf[j + 1] = static_cast<ByteCode>(pool_index >> 8);
                script_buf[j + 2] = static_cast<ByteCode>(pool_index >> 16);
            }
        }
    }

    for (const auto& pair : scriptlabel_db)
    {
        ScriptLabel key = pair.first;
        if (key.startswith("On"_s))
            continue;
        if (!(key.startswith("L_"_s) || key.startswith("S_"_s)))
            disp_error_message(STRPRINTF("error: ugly label: %s\n"_fmt, key),p);
        else if (!probable_labels.count(key))
            disp_error_message(STRPRINTF("error: unused label: %s\n"_fmt, key),p);
    }
    for (ScriptLabel used : probable_labels)
    {
        if (scriptlabel_db.search(used).is_none())
            disp_error_message(STRPRINTF("error: no such label: %s\n"_fmt, used),p);
    }
    probable_labels.clear();

    if (!DEBUG_DISP)
        return;
    for (size_t i = 0; i < script_buf.size(); i++)
    {
        if ((i & 15) == 0)
            PRINTF("%04zx : "_fmt, i);
        PRINTF("%02x "_fmt, script_buf[i]);
        if ((i & 15) == 15)
            PRINTF("\n"_fmt);
    }
    PRINTF("\n"_fmt);
}
} // namespace map
} // namespace tmwa
