#include "script.hpp"

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "../strings/mstring.hpp"
#include "../strings/fstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"

#include "../common/config_parse.hpp"
#include "../common/core.hpp"
#include "../common/db.hpp"
#include "../common/extract.hpp"
#include "../common/intern-pool.hpp"
#include "../common/random.hpp"
#include "../common/socket.hpp"
#include "../common/utils.hpp"
#include "../common/timer.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "magic.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "skill.hpp"
#include "storage.hpp"

#include "../poison.hpp"

constexpr bool DEBUG_DISP = false;
constexpr bool DEBUG_RUN = false;

struct str_data_t
{
    ByteCode type;
    FString strs;
    int backpatch;
    int label_;
    int val;
};
static
Map<FString, str_data_t> str_datam;
static
str_data_t LABEL_NEXTLINE_;

static
DMap<SIR, int> mapreg_db;
static
Map<SIR, FString> mapregstr_db;
static
int mapreg_dirty = -1;
FString mapreg_txt = "save/mapreg.txt";
constexpr std::chrono::milliseconds MAPREG_AUTOSAVE_INTERVAL = std::chrono::seconds(10);

Map<ScriptLabel, int> scriptlabel_db;
UPMap<FString, const ScriptBuffer> userfunc_db;

static
const char *pos_str[11] =
{
    "Head",
    "Body",
    "Left hand",
    "Right hand",
    "Robe",
    "Shoes",
    "Accessory 1",
    "Accessory 2",
    "Head 2",
    "Head 3",
    "Not Equipped",
};

static
struct Script_Config
{
    static const
    int warn_func_no_comma = 1;
    static const
    int warn_cmd_no_comma = 1;
    static const
    int warn_func_mismatch_paramnum = 1;
    static const
    int warn_cmd_mismatch_paramnum = 1;
    static const
    int check_cmdcount = 8192;
    static const
    int check_gotocount = 512;
} script_config;

static
int parse_cmd_if = 0;
static
str_data_t *parse_cmdp;

static
void run_func(ScriptState *st);

static
void mapreg_setreg(SIR num, int val);
static
void mapreg_setregstr(SIR num, XString str);

struct BuiltinFunction
{
    void (*func)(ScriptState *);
    ZString name;
    ZString arg;
};
// defined later
extern BuiltinFunction builtin_functions[];

static
InternPool variable_names;

enum class ByteCode : uint8_t
{
    // types and specials
    NOP, POS, INT, PARAM_, FUNC_, STR, CONSTSTR, ARG,
    VARIABLE, EOL, RETINFO,

    // unary and binary operators
    LOR, LAND, LE, LT, GE, GT, EQ, NE,
    XOR, OR, AND, ADD, SUB, MUL, DIV, MOD,
    NEG, LNOT, NOT, R_SHIFT, L_SHIFT,

    // additions
    // needed because FUNC is used for the actual call
    FUNC_REF,
};

static
str_data_t *search_strp(XString p)
{
    return str_datam.search(p);
}

static
str_data_t *add_strp(XString p)
{
    if (str_data_t *rv = search_strp(p))
        return rv;

    FString p2 = p;
    str_data_t *datum = str_datam.init(p2);
    datum->type = ByteCode::NOP;
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
void ScriptBuffer::add_scriptl(str_data_t *ld)
{
    int backpatch = ld->backpatch;

    switch (ld->type)
    {
        case ByteCode::POS:
            add_scriptc(ByteCode::POS);
            add_scriptb(static_cast<uint8_t>(ld->label_));
            add_scriptb(static_cast<uint8_t>(ld->label_ >> 8));
            add_scriptb(static_cast<uint8_t>(ld->label_ >> 16));
            break;
        case ByteCode::NOP:
            // need to set backpatch, because it might become a label later
            add_scriptc(ByteCode::VARIABLE);
            ld->backpatch = script_buf.size();
            add_scriptb(static_cast<uint8_t>(backpatch));
            add_scriptb(static_cast<uint8_t>(backpatch >> 8));
            add_scriptb(static_cast<uint8_t>(backpatch >> 16));
            break;
        case ByteCode::INT:
            add_scripti(ld->val);
            break;
        case ByteCode::FUNC_:
            add_scriptc(ByteCode::FUNC_REF);
            add_scriptb(static_cast<uint8_t>(ld->val));
            add_scriptb(static_cast<uint8_t>(ld->val >> 8));
            add_scriptb(static_cast<uint8_t>(ld->val >> 16));
            break;
        case ByteCode::PARAM_:
            add_scriptc(ByteCode::PARAM_);
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
void ScriptBuffer::set_label(str_data_t *ld, int pos_)
{
    int next;

    ld->type = ByteCode::POS;
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
    if (*p == '#')
        p++;                    // account変数用
    if (*p == '#')
        p++;                    // ワールドaccount変数用
    if (*p == 'l')
        p++;                    // 一時的変数用(like weiss)

    while (isalnum(*p) || *p == '_')
        p++;

    // postfix
    if (*p == '$')
        p++;                    // 文字列変数

    return p;
}

// TODO: replace this whole mess with some sort of input stream that works
// a line at a time.
static
ZString startptr;
static
int startline;

int script_errors = 0;
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
            PRINTF("\n%s\nline %d : ", mes, line);
            for (int i = 0; linestart + i != lineend; i++)
            {
                if (linestart + i != pos_)
                    PRINTF("%c", linestart[i]);
                else
                    PRINTF("\'%c\'", linestart[i]);
            }
            PRINTF("\a\n");
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
        disp_error_message("unexpected expr end", p);
        exit(1);
    }
    if (*p == '(')
    {

        p = parse_subexpr(p + 1, -1);
        p = skip_space(p);
        if ((*p++) != ')')
        {
            disp_error_message("unmatch ')'", p);
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
                disp_error_message("unexpected newline @ string", p);
                exit(1);
            }
            add_scriptb(*p++);
        }
        if (!*p)
        {
            disp_error_message("unexpected eof @ string", p);
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
            disp_error_message("unexpected character", p);
            exit(1);
        }
        XString word(&*p, &*p2, nullptr);
        if (parse_cmd_if && (word == "callsub" || word == "callfunc" || word == "return"))
        {
            disp_error_message("Sorry, callsub/callfunc/return have never worked properly in an if statement.", p);
        }
        str_data_t *ld = add_strp(word);

        parse_cmdp = ld;          // warn_*_mismatch_paramnumのために必要
        // why not just check l->str == "if" or std::string(p, p2) == "if"?
        if (ld == search_strp("if")) // warn_cmd_no_commaのために必要
            parse_cmd_if++;
        p = p2;

        if (ld->type != ByteCode::FUNC_ && *p == '[')
        {
            // array(name[i] => getelementofarray(name,i) )
            add_scriptl(search_strp("getelementofarray"));
            add_scriptc(ByteCode::ARG);
            add_scriptl(ld);
            p = parse_subexpr(p + 1, -1);
            p = skip_space(p);
            if (*p != ']')
            {
                disp_error_message("unmatch ']'", p);
                exit(1);
            }
            p++;
            add_scriptc(ByteCode::FUNC_);
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
            add_scriptl(&LABEL_NEXTLINE_);
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
            (op = ByteCode::FUNC_, opl = 8, len = 1, *p == '(') ||
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
        if (op == ByteCode::FUNC_)
        {
            int i = 0;
            str_data_t *funcp = parse_cmdp;
            ZString::iterator plist[128];

            if (funcp->type != ByteCode::FUNC_)
            {
                disp_error_message("expect function", tmpp);
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
                    disp_error_message("expect ',' or ')' at func params",
                                        p);
                }
                p = skip_space(p);
                i++;
            }
            plist[i] = p;
            if (*p != ')')
            {
                disp_error_message("func request '(' ')'", p);
                exit(1);
            }
            p++;

            if (funcp->type == ByteCode::FUNC_
                && script_config.warn_func_mismatch_paramnum)
            {
                ZString arg = builtin_functions[funcp->val].arg;
                int j = 0;
                for (j = 0; arg[j]; j++)
                    if (arg[j] == '*')
                        break;
                if ((arg[j] == 0 && i != j) || (arg[j] == '*' && i < j))
                {
                    disp_error_message("illegal number of parameters",
                            plist[std::min(i, j)]);
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
            disp_error_message("unexpected char", p);
            exit(1);
    }
    p = parse_subexpr(p, -1);
    return p;
}

/*==========================================
 * 行の解析
 *------------------------------------------
 */
ZString::iterator ScriptBuffer::parse_line(ZString::iterator p)
{
    int i = 0;
    ZString::iterator plist[128];

    p = skip_space(p);
    if (*p == ';')
        return p;

    parse_cmd_if = 0;           // warn_cmd_no_commaのために必要

    // 最初は関数名
    ZString::iterator p2 = p;
    p = parse_simpleexpr(p);
    p = skip_space(p);

    str_data_t *cmd = parse_cmdp;
    if (cmd->type != ByteCode::FUNC_)
    {
        disp_error_message("expect command", p2);
//      exit(0);
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
            disp_error_message("expect ',' or ';' at cmd params", p);
        }
        p = skip_space(p);
        i++;
    }
    plist[i] = p;
    if (*(p++) != ';')
    {
        disp_error_message("need ';'", p);
        exit(1);
    }
    add_scriptc(ByteCode::FUNC_);

    if (cmd->type == ByteCode::FUNC_
        && script_config.warn_cmd_mismatch_paramnum)
    {
        ZString arg = builtin_functions[cmd->val].arg;
        int j = 0;
        for (j = 0; arg[j]; j++)
            if (arg[j] == '*')
                break;
        if ((arg[j] == 0 && i != j) || (arg[j] == '*' && i < j))
        {
            disp_error_message("illegal number of parameters",
                    plist[std::min(i, j)]);
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
        str_data_t *n = add_strp(builtin_functions[i].name);
        n->type = ByteCode::FUNC_;
        n->val = i;
    }
}

bool read_constdb(ZString filename)
{
    io::ReadFile in(filename);
    if (!in.is_open())
    {
        PRINTF("can't read %s\n", filename);
        return false;
    }

    bool rv = true;
    FString line;
    while (in.getline(line))
    {
        if (is_comment(line))
            continue;

        FString name;
        int val;
        int type = 0; // if not provided
        // TODO get rid of SSCANF - this is the last serious use
        if (SSCANF(line, "%m[A-Za-z0-9_] %i %i", &name, &val, &type) < 2)
        {
            PRINTF("Bad const line: %s\n", line);
            rv = false;
            continue;
        }
        str_data_t *n = add_strp(name);
        n->type = type ? ByteCode::PARAM_ : ByteCode::INT;
        n->val = val;
    }
    return rv;
}

std::unique_ptr<const ScriptBuffer> parse_script(ZString src, int line)
{
    auto script_buf = make_unique<ScriptBuffer>();
    script_buf->parse_script(src, line);
    return std::move(script_buf);
}

/*==========================================
 * スクリプトの解析
 *------------------------------------------
 */
void ScriptBuffer::parse_script(ZString src, int line)
{
    static int first = 1;

    if (first)
    {
        add_builtin_functions();
    }
    first = 0;
    LABEL_NEXTLINE_.type = ByteCode::NOP;
    LABEL_NEXTLINE_.backpatch = -1;
    LABEL_NEXTLINE_.label_ = -1;
    for (auto& pair : str_datam)
    {
        str_data_t& dit = pair.second;
        if (dit.type == ByteCode::POS || dit.type == ByteCode::VARIABLE)
        {
            dit.type = ByteCode::NOP;
            dit.backpatch = -1;
            dit.label_ = -1;
        }
    }

    // 外部用label dbの初期化
    scriptlabel_db.clear();

    // for error message
    startptr = src;
    startline = line;

    ZString::iterator p = src.begin();
    p = skip_space(p);
    if (*p != '{')
    {
        disp_error_message("not found '{'", p);
        abort();
    }
    for (p++; *p && *p != '}';)
    {
        p = skip_space(p);
        if (*skip_space(skip_word(p)) == ':')
        {
            ZString::iterator tmpp = skip_word(p);
            XString str(&*p, &*tmpp, nullptr);
            str_data_t *ld = add_strp(str);
            bool e1 = ld->type != ByteCode::NOP;
            bool e2 = ld->type == ByteCode::POS;
            bool e3 = ld->label_ != -1;
            assert (e1 == e2 && e2 == e3);
            if (e3)
            {
                disp_error_message("dup label ", p);
                exit(1);
            }
            set_label(ld, script_buf.size());
            scriptlabel_db.insert(stringish<ScriptLabel>(str), script_buf.size());
            p = tmpp + 1;
            continue;
        }

        // 他は全部一緒くた
        p = parse_line(p);
        p = skip_space(p);
        add_scriptc(ByteCode::EOL);

        set_label(&LABEL_NEXTLINE_, script_buf.size());
        LABEL_NEXTLINE_.type = ByteCode::NOP;
        LABEL_NEXTLINE_.backpatch = -1;
        LABEL_NEXTLINE_.label_ = -1;
    }

    add_scriptc(ByteCode::NOP);

    // resolve the unknown labels
    for (auto& pair : str_datam)
    {
        str_data_t& sit = pair.second;
        if (sit.type == ByteCode::NOP)
        {
            sit.type = ByteCode::VARIABLE;
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

    if (!DEBUG_DISP)
        return;
    for (size_t i = 0; i < script_buf.size(); i++)
    {
        if ((i & 15) == 0)
            PRINTF("%04zx : ", i);
        PRINTF("%02x ", script_buf[i]);
        if ((i & 15) == 15)
            PRINTF("\n");
    }
    PRINTF("\n");
}

//
// 実行系
//
enum class ScriptEndState
{
    ZERO,
    STOP,
    END,
    RERUNLINE,
    GOTO,
    RETFUNC,
};

/*==========================================
 * ridからsdへの解決
 *------------------------------------------
 */
static
dumb_ptr<map_session_data> script_rid2sd(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = map_id2sd(st->rid);
    if (!sd)
    {
        PRINTF("script_rid2sd: fatal error ! player not attached!\n");
    }
    return sd;
}

/*==========================================
 * 変数の読み取り
 *------------------------------------------
 */
static
void get_val(ScriptState *st, struct script_data *data)
{
    dumb_ptr<map_session_data> sd = NULL;

    if (data->type == ByteCode::PARAM_)
    {
        if ((sd = script_rid2sd(st)) == NULL)
            PRINTF("get_val error param SP::%d\n", data->u.reg.sp());
        data->type = ByteCode::INT;
        if (sd)
            data->u.numi = pc_readparam(sd, data->u.reg.sp());
    }
    else if (data->type == ByteCode::VARIABLE)
    {
        ZString name_ = variable_names.outtern(data->u.reg.base());
        VarName name = stringish<VarName>(name_);
        char prefix = name.front();
        char postfix = name.back();

        if (prefix != '$')
        {
            if ((sd = script_rid2sd(st)) == NULL)
                PRINTF("get_val error name?:%s\n", name);
        }
        if (postfix == '$')
        {
            data->type = ByteCode::CONSTSTR;
            if (prefix == '@' || prefix == 'l')
            {
                if (sd)
                    data->u.str = dumb_string::fake(pc_readregstr(sd, data->u.reg));
            }
            else if (prefix == '$')
            {
                FString *s = mapregstr_db.search(data->u.reg);
                data->u.str = s ? dumb_string::fake(*s) : dumb_string();
            }
            else
            {
                PRINTF("script: get_val: illegal scope string variable.\n");
                data->u.str = dumb_string::fake("!!ERROR!!");
            }
            if (!data->u.str)
                data->u.str = dumb_string::fake("");
        }
        else
        {
            data->type = ByteCode::INT;
            if (prefix == '@' || prefix == 'l')
            {
                if (sd)
                    data->u.numi = pc_readreg(sd, data->u.reg);
            }
            else if (prefix == '$')
            {
                data->u.numi = mapreg_db.get(data->u.reg);
            }
            else if (prefix == '#')
            {
                if (name[1] == '#')
                {
                    if (sd)
                        data->u.numi = pc_readaccountreg2(sd, name);
                }
                else
                {
                    if (sd)
                        data->u.numi = pc_readaccountreg(sd, name);
                }
            }
            else
            {
                if (sd)
                    data->u.numi = pc_readglobalreg(sd, name);
            }
        }
    }
}

/*==========================================
 * 変数の読み取り2
 *------------------------------------------
 */
static
struct script_data get_val2(ScriptState *st, SIR reg)
{
    struct script_data dat;
    dat.type = ByteCode::VARIABLE;
    dat.u.reg = reg;
    get_val(st, &dat);
    return dat;
}

/*==========================================
 * 変数設定用
 *------------------------------------------
 */
static
void set_reg(dumb_ptr<map_session_data> sd, ByteCode type, SIR reg, struct script_data vd)
{
    if (type == ByteCode::PARAM_)
    {
        assert (vd.type == ByteCode::INT);
        int val = vd.u.numi;
        pc_setparam(sd, reg.sp(), val);
        return;
    }
    assert (type == ByteCode::VARIABLE);

    ZString name_ = variable_names.outtern(reg.base());
    VarName name = stringish<VarName>(name_);
    char prefix = name.front();
    char postfix = name.back();

    if (postfix == '$')
    {
        dumb_string str = vd.u.str;
        if (prefix == '@' || prefix == 'l')
        {
            pc_setregstr(sd, reg, str.str());
        }
        else if (prefix == '$')
        {
            mapreg_setregstr(reg, str.str());
        }
        else
        {
            PRINTF("script: set_reg: illegal scope string variable !");
        }
    }
    else
    {
        // 数値
        int val = vd.u.numi;
        if (prefix == '@' || prefix == 'l')
        {
            pc_setreg(sd, reg, val);
        }
        else if (prefix == '$')
        {
            mapreg_setreg(reg, val);
        }
        else if (prefix == '#')
        {
            if (name[1] == '#')
                pc_setaccountreg2(sd, name, val);
            else
                pc_setaccountreg(sd, name, val);
        }
        else
        {
            pc_setglobalreg(sd, name, val);
        }
    }
}

static
void set_reg(dumb_ptr<map_session_data> sd, ByteCode type, SIR reg, int id)
{
    struct script_data vd;
    vd.type = ByteCode::INT;
    vd.u.numi = id;
    set_reg(sd, type, reg, vd);
}

static
void set_reg(dumb_ptr<map_session_data> sd, ByteCode type, SIR reg, dumb_string zd)
{
    struct script_data vd;
    vd.type = ByteCode::CONSTSTR;
    vd.u.str = zd;
    set_reg(sd, type, reg, vd);
}

/*==========================================
 * 文字列への変換
 *------------------------------------------
 */
static __attribute__((warn_unused_result))
dumb_string conv_str(ScriptState *st, struct script_data *data)
{
    get_val(st, data);
    assert (data->type != ByteCode::RETINFO);
    if (data->type == ByteCode::INT)
    {
        FString buf = STRPRINTF("%d", data->u.numi);
        data->type = ByteCode::STR;
        data->u.str = dumb_string::copys(buf);
    }
    return data->u.str;
}

/*==========================================
 * 数値へ変換
 *------------------------------------------
 */
static __attribute__((warn_unused_result))
int conv_num(ScriptState *st, struct script_data *data)
{
    get_val(st, data);
    assert (data->type != ByteCode::RETINFO);
    if (data->type == ByteCode::STR || data->type == ByteCode::CONSTSTR)
    {
        dumb_string p = data->u.str;
        data->u.numi = atoi(p.c_str());
        if (data->type == ByteCode::STR)
            p.delete_();
        data->type = ByteCode::INT;
    }
    return data->u.numi;
}

static __attribute__((warn_unused_result))
const ScriptBuffer *conv_script(ScriptState *st, struct script_data *data)
{
    get_val(st, data);
    assert (data->type == ByteCode::RETINFO);
    return data->u.script;
}

/*==========================================
 * スタックへ数値をプッシュ
 *------------------------------------------
 */
static
void push_int(struct script_stack *stack, ByteCode type, int val)
{
    assert (type == ByteCode::POS || type == ByteCode::INT || type == ByteCode::ARG || type == ByteCode::FUNC_REF);

    script_data nsd {};
    nsd.type = type;
    nsd.u.numi = val;
    stack->stack_datav.push_back(nsd);
}

static
void push_reg(struct script_stack *stack, ByteCode type, SIR reg)
{
    assert (type == ByteCode::PARAM_ || type == ByteCode::VARIABLE);

    script_data nsd {};
    nsd.type = type;
    nsd.u.reg = reg;
    stack->stack_datav.push_back(nsd);
}

static
void push_script(struct script_stack *stack, ByteCode type, const ScriptBuffer *code)
{
    assert (type == ByteCode::RETINFO);

    script_data nsd {};
    nsd.type = type;
    nsd.u.script = code;
    stack->stack_datav.push_back(nsd);
}

/*==========================================
 * スタックへ文字列をプッシュ
 *------------------------------------------
 */
static
void push_str(struct script_stack *stack, ByteCode type, dumb_string str)
{
    assert (type == ByteCode::STR || type == ByteCode::CONSTSTR);

    script_data nsd {};
    nsd.type = type;
    nsd.u.str = str;
    stack->stack_datav.push_back(nsd);
}

/*==========================================
 * スタックへ複製をプッシュ
 *------------------------------------------
 */
static
void push_copy(struct script_stack *stack, int pos_)
{
    script_data csd = stack->stack_datav[pos_];
    if (csd.type == ByteCode::STR)
        csd.u.str = csd.u.str.dup();
    stack->stack_datav.push_back(csd);
}

/*==========================================
 * スタックからポップ
 *------------------------------------------
 */
static
void pop_stack(struct script_stack *stack, int start, int end)
{
    for (int i = start; i < end; i++)
    {
        if (stack->stack_datav[i].type == ByteCode::STR)
            stack->stack_datav[i].u.str.delete_();
    }
    auto it = stack->stack_datav.begin();
    stack->stack_datav.erase(it + start, it + end);
}

#define AARGO2(n) (st->stack->stack_datav[st->start + (n)])
#define HARGO2(n) (st->end > st->start + (n))

//
// 埋め込み関数
//
/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_mes(ScriptState *st)
{
    dumb_string mes = conv_str(st, &AARGO2(2));
    clif_scriptmes(script_rid2sd(st), st->oid, ZString(mes));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_goto(ScriptState *st)
{
    if (AARGO2(2).type != ByteCode::POS)
    {
        PRINTF("script: goto: not label !\n");
        st->state = ScriptEndState::END;
        return;
    }

    st->scriptp.pos = conv_num(st, &AARGO2(2));
    st->state = ScriptEndState::GOTO;
}

/*==========================================
 * ユーザー定義関数の呼び出し
 *------------------------------------------
 */
static
void builtin_callfunc(ScriptState *st)
{
    dumb_string str = conv_str(st, &AARGO2(2));
    const ScriptBuffer *scr = userfunc_db.get(str.str());

    if (scr)
    {
        int j = 0;
        assert (st->start + 3 == st->end);
#if 0
        for (int i = st->start + 3; i < st->end; i++, j++)
            push_copy(st->stack, i);
#endif

        push_int(st->stack, ByteCode::INT, j); // 引数の数をプッシュ
        push_int(st->stack, ByteCode::INT, st->defsp); // 現在の基準スタックポインタをプッシュ
        push_int(st->stack, ByteCode::INT, st->scriptp.pos);   // 現在のスクリプト位置をプッシュ
        push_script(st->stack, ByteCode::RETINFO, st->scriptp.code);  // 現在のスクリプトをプッシュ

        st->scriptp = ScriptPointer(scr, 0);
        st->defsp = st->start + 4 + j;
        st->state = ScriptEndState::GOTO;
    }
    else
    {
        PRINTF("script:callfunc: function not found! [%s]\n", str);
        st->state = ScriptEndState::END;
    }
}

/*==========================================
 * サブルーティンの呼び出し
 *------------------------------------------
 */
static
void builtin_callsub(ScriptState *st)
{
    int pos_ = conv_num(st, &AARGO2(2));
    int j = 0;
    assert (st->start + 3 == st->end);
#if 0
    for (int i = st->start + 3; i < st->end; i++, j++)
        push_copy(st->stack, i);
#endif

    push_int(st->stack, ByteCode::INT, j); // 引数の数をプッシュ
    push_int(st->stack, ByteCode::INT, st->defsp); // 現在の基準スタックポインタをプッシュ
    push_int(st->stack, ByteCode::INT, st->scriptp.pos);   // 現在のスクリプト位置をプッシュ
    push_script(st->stack, ByteCode::RETINFO, st->scriptp.code);  // 現在のスクリプトをプッシュ

    st->scriptp.pos = pos_;
    st->defsp = st->start + 4 + j;
    st->state = ScriptEndState::GOTO;
}

/*==========================================
 * サブルーチン/ユーザー定義関数の終了
 *------------------------------------------
 */
static
void builtin_return(ScriptState *st)
{
#if 0
    if (HARGO2(2))
    {                           // 戻り値有り
        push_copy(st->stack, st->start + 2);
    }
#endif
    st->state = ScriptEndState::RETFUNC;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_next(ScriptState *st)
{
    st->state = ScriptEndState::STOP;
    clif_scriptnext(script_rid2sd(st), st->oid);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_close(ScriptState *st)
{
    st->state = ScriptEndState::END;
    clif_scriptclose(script_rid2sd(st), st->oid);
}

static
void builtin_close2(ScriptState *st)
{
    st->state = ScriptEndState::STOP;
    clif_scriptclose(script_rid2sd(st), st->oid);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_menu(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    if (sd->state.menu_or_input == 0)
    {
        // First half: show menu.
        st->state = ScriptEndState::RERUNLINE;
        sd->state.menu_or_input = 1;

        MString buf;
        for (int i = st->start + 2; i < st->end; i += 2)
        {
            dumb_string choice_str = conv_str(st, &AARGO2(i - st->start));
            if (!choice_str[0])
                break;
            buf += ZString(choice_str);
            buf += ':';
        }

        clif_scriptmenu(script_rid2sd(st), st->oid, FString(buf));
    }
    else
    {
        // Rerun: item is chosen from menu.
        if (sd->npc_menu == 0xff)
        {
            // cancel
            sd->state.menu_or_input = 0;
            st->state = ScriptEndState::END;
            return;
        }

        // Actually jump to the label.
        // Logic change: menu_choices is the *total* number of labels,
        // not just the displayed number that ends with the "".
        // (Would it be better to pop the stack before rerunning?)
        int menu_choices = (st->end - (st->start + 2)) / 2;
        pc_setreg(sd, SIR::from(variable_names.intern("@menu")), sd->npc_menu);
        sd->state.menu_or_input = 0;
        if (sd->npc_menu > 0 && sd->npc_menu <= menu_choices)
        {
            int arg_index = (sd->npc_menu - 1) * 2 + 1;
            if (AARGO2(arg_index + 2).type != ByteCode::POS)
            {
                st->state = ScriptEndState::END;
                return;
            }
            st->scriptp.pos = conv_num(st, &AARGO2(arg_index + 2));
            st->state = ScriptEndState::GOTO;
        }
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_rand(ScriptState *st)
{
    if (HARGO2(3))
    {
        int min = conv_num(st, &AARGO2(2));
        int max = conv_num(st, &AARGO2(3));
        if (min > max)
            std::swap(max, min);
        push_int(st->stack, ByteCode::INT, random_::in(min, max));
    }
    else
    {
        int range = conv_num(st, &AARGO2(2));
        push_int(st->stack, ByteCode::INT, range <= 0 ? 0 : random_::to(range));
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_pow(ScriptState *st)
{
    int a, b;

    a = conv_num(st, &AARGO2(2));
    b = conv_num(st, &AARGO2(3));

#warning "This is silly"
    push_int(st->stack, ByteCode::INT, static_cast<int>(pow(a * 0.001, b)));

}

/*==========================================
 * Check whether the PC is at the specified location
 *------------------------------------------
 */
static
void builtin_isat(ScriptState *st)
{
    int x, y;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    x = conv_num(st, &AARGO2(3));
    y = conv_num(st, &AARGO2(4));

    if (!sd)
        return;

    push_int(st->stack, ByteCode::INT,
            (x == sd->bl_x) && (y == sd->bl_y)
            && (str == sd->bl_m->name_));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_warp(ScriptState *st)
{
    int x, y;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    x = conv_num(st, &AARGO2(3));
    y = conv_num(st, &AARGO2(4));
    if (str == "Random")
        pc_randomwarp(sd, BeingRemoveWhy::WARPED);
    else if (str == "SavePoint")
    {
        if (sd->bl_m->flag.noreturn)    // 蝶禁止
            return;

        pc_setpos(sd, sd->status.save_point.map_, sd->status.save_point.x, sd->status.save_point.y,
                BeingRemoveWhy::WARPED);
    }
    else if (str == "Save")
    {
        if (sd->bl_m->flag.noreturn)    // 蝶禁止
            return;

        pc_setpos(sd, sd->status.save_point.map_, sd->status.save_point.x, sd->status.save_point.y,
                BeingRemoveWhy::WARPED);
    }
    else
        pc_setpos(sd, str, x, y, BeingRemoveWhy::GONE);
}

/*==========================================
 * エリア指定ワープ
 *------------------------------------------
 */
static
void builtin_areawarp_sub(dumb_ptr<block_list> bl, MapName mapname, int x, int y)
{
    dumb_ptr<map_session_data> sd = bl->is_player();
    if (mapname == "Random")
        pc_randomwarp(sd, BeingRemoveWhy::WARPED);
    else
        pc_setpos(sd, mapname, x, y, BeingRemoveWhy::GONE);
}

static
void builtin_areawarp(ScriptState *st)
{
    int x, y;
    int x0, y0, x1, y1;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    x0 = conv_num(st, &AARGO2(3));
    y0 = conv_num(st, &AARGO2(4));
    x1 = conv_num(st, &AARGO2(5));
    y1 = conv_num(st, &AARGO2(6));
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(7))));
    x = conv_num(st, &AARGO2(8));
    y = conv_num(st, &AARGO2(9));

    map_local *m = map_mapname2mapid(mapname);
    if (m == nullptr)
        return;

    map_foreachinarea(std::bind(builtin_areawarp_sub, ph::_1, str, x, y),
            m,
            x0, y0,
            x1, y1,
            BL::PC);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_heal(ScriptState *st)
{
    int hp, sp;

    hp = conv_num(st, &AARGO2(2));
    sp = conv_num(st, &AARGO2(3));
    pc_heal(script_rid2sd(st), hp, sp);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_itemheal(ScriptState *st)
{
    int hp, sp;

    hp = conv_num(st, &AARGO2(2));
    sp = conv_num(st, &AARGO2(3));
    pc_itemheal(script_rid2sd(st), hp, sp);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_percentheal(ScriptState *st)
{
    int hp, sp;

    hp = conv_num(st, &AARGO2(2));
    sp = conv_num(st, &AARGO2(3));
    pc_percentheal(script_rid2sd(st), hp, sp);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_input(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = NULL;
    script_data& scrd = AARGO2(2);
    ByteCode type = scrd.type;
    assert (type == ByteCode::VARIABLE);

    SIR reg = scrd.u.reg;
    ZString name = variable_names.outtern(reg.base());
//  char prefix = name.front();
    char postfix = name.back();

    sd = script_rid2sd(st);
    if (sd->state.menu_or_input)
    {
        // Second time (rerun)
        sd->state.menu_or_input = 0;
        if (postfix == '$')
        {
            set_reg(sd, type, reg, dumb_string::fake(sd->npc_str));
        }
        else
        {
            //commented by Lupus (check Value Number Input fix in clif.c)
            //** Fix by fritz :X keeps people from abusing old input bugs
            // wtf?
            if (sd->npc_amount < 0) //** If input amount is less then 0
            {
                clif_tradecancelled(sd);   // added "Deal has been cancelled" message by Valaris
                builtin_close(st); //** close
            }

            set_reg(sd, type, reg, sd->npc_amount);
        }
    }
    else
    {
        // First time - send prompt to client, then wait
        st->state = ScriptEndState::RERUNLINE;
        if (postfix == '$')
            clif_scriptinputstr(sd, st->oid);
        else
            clif_scriptinput(sd, st->oid);
        sd->state.menu_or_input = 1;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_if (ScriptState *st)
{
    int sel, i;

    sel = conv_num(st, &AARGO2(2));
    if (!sel)
        return;

    // 関数名をコピー
    push_copy(st->stack, st->start + 3);
    // 間に引数マーカを入れて
    push_int(st->stack, ByteCode::ARG, 0);
    // 残りの引数をコピー
    for (i = st->start + 4; i < st->end; i++)
    {
        push_copy(st->stack, i);
    }
    run_func(st);
}

/*==========================================
 * 変数設定
 *------------------------------------------
 */
static
void builtin_set(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = NULL;
    SIR reg = AARGO2(2).u.reg;
    if (AARGO2(2).type == ByteCode::PARAM_)
    {
        sd = script_rid2sd(st);

        int val = conv_num(st, &AARGO2(3));
        set_reg(sd, ByteCode::PARAM_, reg, val);
        return;
    }
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();
    char postfix = name.back();

    assert (AARGO2(2).type == ByteCode::VARIABLE);

    if (prefix != '$')
        sd = script_rid2sd(st);

    if (postfix == '$')
    {
        // 文字列
        dumb_string str = conv_str(st, &AARGO2(3));
        set_reg(sd, ByteCode::VARIABLE, reg, str);
    }
    else
    {
        // 数値
        int val = conv_num(st, &AARGO2(3));
        set_reg(sd, ByteCode::VARIABLE, reg, val);
    }

}

/*==========================================
 * 配列変数設定
 *------------------------------------------
 */
static
void builtin_setarray(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = NULL;
    assert (AARGO2(2).type == ByteCode::VARIABLE);
    SIR reg = AARGO2(2).u.reg;
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();
    char postfix = name.back();

    if (prefix != '$' && prefix != '@')
    {
        PRINTF("builtin_setarray: illegal scope !\n");
        return;
    }
    if (prefix != '$')
        sd = script_rid2sd(st);

    for (int j = 0, i = st->start + 3; i < st->end && j < 256; i++, j++)
    {
        if (postfix == '$')
            set_reg(sd, ByteCode::VARIABLE, reg.iplus(j), conv_str(st, &AARGO2(i - st->start)));
        else
            set_reg(sd, ByteCode::VARIABLE, reg.iplus(j), conv_num(st, &AARGO2(i - st->start)));
    }
}

/*==========================================
 * 配列変数クリア
 *------------------------------------------
 */
static
void builtin_cleararray(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = NULL;
    assert (AARGO2(2).type == ByteCode::VARIABLE);
    SIR reg = AARGO2(2).u.reg;
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();
    char postfix = name.back();
    int sz = conv_num(st, &AARGO2(4));

    if (prefix != '$' && prefix != '@')
    {
        PRINTF("builtin_cleararray: illegal scope !\n");
        return;
    }
    if (prefix != '$')
        sd = script_rid2sd(st);

    for (int i = 0; i < sz; i++)
    {
        if (postfix == '$')
            set_reg(sd, ByteCode::VARIABLE, reg.iplus(i), conv_str(st, &AARGO2(3)));
        else
            set_reg(sd, ByteCode::VARIABLE, reg.iplus(i), conv_num(st, &AARGO2(3)));
    }

}

/*==========================================
 * 配列変数のサイズ所得
 *------------------------------------------
 */
static
int getarraysize(ScriptState *st, SIR reg, bool is_string)
{
    int i = reg.index(), c = i;
    for (; i < 256; i++)
    {
        // This is obviously not what was intended
        struct script_data vd = get_val2(st, reg.iplus(i));
        if (is_string ? bool(vd.u.str[0]) : bool(vd.u.numi))
            c = i;
    }
    return c + 1;
}

static
void builtin_getarraysize(ScriptState *st)
{
    assert (AARGO2(2).type == ByteCode::VARIABLE);
    SIR reg = AARGO2(2).u.reg;
    ZString name = variable_names.outtern(reg.base());
    char prefix = name.front();
    char postfix = name.back();

    if (prefix != '$' && prefix != '@')
    {
        PRINTF("builtin_copyarray: illegal scope !\n");
        return;
    }

    push_int(st->stack, ByteCode::INT, getarraysize(st, reg, postfix == '$'));
}

/*==========================================
 * 指定要素を表す値(キー)を所得する
 *------------------------------------------
 */
static
void builtin_getelementofarray(ScriptState *st)
{
    if (AARGO2(2).type == ByteCode::VARIABLE)
    {
        int i = conv_num(st, &AARGO2(3));
        if (i > 255 || i < 0)
        {
            PRINTF("script: getelementofarray (operator[]): param2 illegal number %d\n",
                 i);
            push_int(st->stack, ByteCode::INT, 0);
        }
        else
        {
            push_reg(st->stack, ByteCode::VARIABLE,
                    AARGO2(2).u.reg.iplus(i));
        }
    }
    else
    {
        PRINTF("script: getelementofarray (operator[]): param1 not name !\n");
        push_int(st->stack, ByteCode::INT, 0);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_setlook(ScriptState *st)
{
    LOOK type = LOOK(conv_num(st, &AARGO2(2)));
    int val = conv_num(st, &AARGO2(3));

    pc_changelook(script_rid2sd(st), type, val);

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_countitem(ScriptState *st)
{
    int nameid = 0, count = 0, i;
    dumb_ptr<map_session_data> sd;

    struct script_data *data;

    sd = script_rid2sd(st);

    data = &AARGO2(2);
    get_val(st, data);
    if (data->type == ByteCode::STR || data->type == ByteCode::CONSTSTR)
    {
        ItemName name = stringish<ItemName>(ZString(conv_str(st, data)));
        struct item_data *item_data = itemdb_searchname(name);
        if (item_data != NULL)
            nameid = item_data->nameid;
    }
    else
        nameid = conv_num(st, data);

    if (nameid >= 500)          //if no such ID then skip this iteration
        for (i = 0; i < MAX_INVENTORY; i++)
        {
            if (sd->status.inventory[i].nameid == nameid)
                count += sd->status.inventory[i].amount;
        }
    else
    {
        if (battle_config.error_log)
            PRINTF("wrong item ID : countitem (%i)\n", nameid);
    }
    push_int(st->stack, ByteCode::INT, count);

}

/*==========================================
 * 重量チェック
 *------------------------------------------
 */
static
void builtin_checkweight(ScriptState *st)
{
    int nameid = 0, amount;
    dumb_ptr<map_session_data> sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &AARGO2(2);
    get_val(st, data);
    if (data->type == ByteCode::STR || data->type == ByteCode::CONSTSTR)
    {
        ItemName name = stringish<ItemName>(ZString(conv_str(st, data)));
        struct item_data *item_data = itemdb_searchname(name);
        if (item_data)
            nameid = item_data->nameid;
    }
    else
        nameid = conv_num(st, data);

    amount = conv_num(st, &AARGO2(3));
    if (amount <= 0 || nameid < 500)
    {
        //if get wrong item ID or amount<=0, don't count weight of non existing items
        push_int(st->stack, ByteCode::INT, 0);
        return;
    }

    if (itemdb_weight(nameid) * amount + sd->weight > sd->max_weight)
    {
        push_int(st->stack, ByteCode::INT, 0);
    }
    else
    {
        push_int(st->stack, ByteCode::INT, 1);
    }

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_getitem(ScriptState *st)
{
    int nameid, amount;
    dumb_ptr<map_session_data> sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &AARGO2(2);
    get_val(st, data);
    if (data->type == ByteCode::STR || data->type == ByteCode::CONSTSTR)
    {
        ItemName name = stringish<ItemName>(ZString(conv_str(st, data)));
        struct item_data *item_data = itemdb_searchname(name);
        nameid = 727;           //Default to iten
        if (item_data != NULL)
            nameid = item_data->nameid;
    }
    else
        nameid = conv_num(st, data);

    if ((amount =
         conv_num(st, &AARGO2(3))) <= 0)
    {
        return;               //return if amount <=0, skip the useles iteration
    }

    if (nameid > 0)
    {
        struct item item_tmp {};
        item_tmp.nameid = nameid;
        item_tmp.identify = 1;
        if (HARGO2(5))    //アイテムを指定したIDに渡す
            sd = map_id2sd(conv_num(st, &AARGO2(5)));
        if (sd == NULL)         //アイテムを渡す相手がいなかったらお帰り
            return;
        PickupFail flag;
        if ((flag = pc_additem(sd, &item_tmp, amount)) != PickupFail::OKAY)
        {
            clif_additem(sd, 0, 0, flag);
            map_addflooritem(&item_tmp, amount,
                    sd->bl_m, sd->bl_x, sd->bl_y,
                    NULL, NULL, NULL);
        }
    }

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_makeitem(ScriptState *st)
{
    int nameid, amount, flag = 0;
    int x, y;
    dumb_ptr<map_session_data> sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &AARGO2(2);
    get_val(st, data);
    if (data->type == ByteCode::STR || data->type == ByteCode::CONSTSTR)
    {
        ItemName name = stringish<ItemName>(ZString(conv_str(st, data)));
        struct item_data *item_data = itemdb_searchname(name);
        nameid = 512;           //Apple Item ID
        if (item_data)
            nameid = item_data->nameid;
    }
    else
        nameid = conv_num(st, data);

    amount = conv_num(st, &AARGO2(3));
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARGO2(4))));
    x = conv_num(st, &AARGO2(5));
    y = conv_num(st, &AARGO2(6));

    map_local *m;
    if (sd && mapname == MOB_THIS_MAP)
        m = sd->bl_m;
    else
        m = map_mapname2mapid(mapname);

    if (nameid > 0)
    {
        struct item item_tmp {};
        item_tmp.nameid = nameid;
        if (!flag)
            item_tmp.identify = 1;
        else
            item_tmp.identify = !itemdb_isequip3(nameid);

        map_addflooritem(&item_tmp, amount, m, x, y, NULL, NULL, NULL);
    }

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_delitem(ScriptState *st)
{
    int nameid = 0, amount, i;
    dumb_ptr<map_session_data> sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &AARGO2(2);
    get_val(st, data);
    if (data->type == ByteCode::STR || data->type == ByteCode::CONSTSTR)
    {
        ItemName name = stringish<ItemName>(ZString(conv_str(st, data)));
        struct item_data *item_data = itemdb_searchname(name);
        //nameid=512;
        if (item_data)
            nameid = item_data->nameid;
    }
    else
        nameid = conv_num(st, data);

    amount = conv_num(st, &AARGO2(3));

    if (nameid < 500 || amount <= 0)
    {
        //by Lupus. Don't run FOR if u got wrong item ID or amount<=0
        //PRINTF("wrong item ID or amount<=0 : delitem %i,\n",nameid,amount);
        return;
    }

    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid <= 0
            || sd->inventory_data[i] == NULL
            || sd->inventory_data[i]->type != ItemType::_7
            || sd->status.inventory[i].amount <= 0)
            continue;
    }
    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid == nameid)
        {
            if (sd->status.inventory[i].amount >= amount)
            {
                pc_delitem(sd, i, amount, 0);
                break;
            }
            else
            {
                amount -= sd->status.inventory[i].amount;
                if (amount == 0)
                    amount = sd->status.inventory[i].amount;
                pc_delitem(sd, i, amount, 0);
                break;
            }
        }
    }

}

/*==========================================
 *キャラ関係のパラメータ取得
 *------------------------------------------
 */
static
void builtin_readparam(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    SP type = SP(conv_num(st, &AARGO2(2)));
    if (HARGO2(3))
        sd = map_nick2sd(stringish<CharName>(ZString(conv_str(st, &AARGO2(3)))));
    else
        sd = script_rid2sd(st);

    if (sd == NULL)
    {
        push_int(st->stack, ByteCode::INT, -1);
        return;
    }

    push_int(st->stack, ByteCode::INT, pc_readparam(sd, type));

}

/*==========================================
 *キャラ関係のID取得
 *------------------------------------------
 */
static
void builtin_getcharid(ScriptState *st)
{
    int num;
    dumb_ptr<map_session_data> sd;

    num = conv_num(st, &AARGO2(2));
    if (HARGO2(3))
        sd = map_nick2sd(stringish<CharName>(ZString(conv_str(st, &AARGO2(3)))));
    else
        sd = script_rid2sd(st);
    if (sd == NULL)
    {
        push_int(st->stack, ByteCode::INT, -1);
        return;
    }
    if (num == 0)
        push_int(st->stack, ByteCode::INT, sd->status.char_id);
    if (num == 1)
        push_int(st->stack, ByteCode::INT, sd->status.party_id);
    if (num == 2)
        push_int(st->stack, ByteCode::INT, 0/*guild_id*/);
    if (num == 3)
        push_int(st->stack, ByteCode::INT, sd->status.account_id);
}

/*==========================================
 *指定IDのPT名取得
 *------------------------------------------
 */
static
dumb_string builtin_getpartyname_sub(int party_id)
{
    struct party *p = party_search(party_id);

    if (p)
        return dumb_string::copys(p->name);

    return dumb_string();
}

/*==========================================
 * キャラクタの名前
 *------------------------------------------
 */
static
void builtin_strcharinfo(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;
    int num;

    sd = script_rid2sd(st);
    num = conv_num(st, &AARGO2(2));
    if (num == 0)
    {
        dumb_string buf = dumb_string::copys(sd->status.name.to__actual());
        push_str(st->stack, ByteCode::STR, buf);
    }
    if (num == 1)
    {
        dumb_string buf = builtin_getpartyname_sub(sd->status.party_id);
        if (buf)
            push_str(st->stack, ByteCode::STR, buf);
        else
            push_str(st->stack, ByteCode::CONSTSTR, dumb_string::fake(""));
    }
    if (num == 2)
    {
        // was: guild name
        push_str(st->stack, ByteCode::CONSTSTR, dumb_string::fake(""));
    }

}

// indexed by the equip_* in db/const.txt
// TODO change to use EQUIP
static
EPOS equip[11] =
{
    EPOS::HAT,
    EPOS::MISC1,
    EPOS::SHIELD,
    EPOS::WEAPON,
    EPOS::GLOVES,
    EPOS::SHOES,
    EPOS::CAPE,
    EPOS::MISC2,
    EPOS::TORSO,
    EPOS::LEGS,
    EPOS::ARROW,
};

/*==========================================
 * GetEquipID(Pos);     Pos: 1-10
 *------------------------------------------
 */
static
void builtin_getequipid(ScriptState *st)
{
    int i, num;
    dumb_ptr<map_session_data> sd;
    struct item_data *item;

    sd = script_rid2sd(st);
    if (sd == NULL)
    {
        PRINTF("getequipid: sd == NULL\n");
        return;
    }
    num = conv_num(st, &AARGO2(2));
    i = pc_checkequip(sd, equip[num - 1]);
    if (i >= 0)
    {
        item = sd->inventory_data[i];
        if (item)
            push_int(st->stack, ByteCode::INT, item->nameid);
        else
            push_int(st->stack, ByteCode::INT, 0);
    }
    else
    {
        push_int(st->stack, ByteCode::INT, -1);
    }
}

/*==========================================
 * 装備名文字列（精錬メニュー用）
 *------------------------------------------
 */
static
void builtin_getequipname(ScriptState *st)
{
    int i, num;
    dumb_ptr<map_session_data> sd;
    struct item_data *item;

    FString buf;

    sd = script_rid2sd(st);
    num = conv_num(st, &AARGO2(2));
    i = pc_checkequip(sd, equip[num - 1]);
    if (i >= 0)
    {
        item = sd->inventory_data[i];
        if (item)
            buf = STRPRINTF("%s-[%s]", pos_str[num - 1], item->jname);
        else
            buf = STRPRINTF("%s-[%s]", pos_str[num - 1], pos_str[10]);
    }
    else
    {
        buf = STRPRINTF("%s-[%s]", pos_str[num - 1], pos_str[10]);
    }
    push_str(st->stack, ByteCode::STR, dumb_string::copys(buf));

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_statusup2(ScriptState *st)
{
    SP type = SP(conv_num(st, &AARGO2(2)));
    int val = conv_num(st, &AARGO2(3));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    pc_statusup2(sd, type, val);

}

/*==========================================
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
static
void builtin_bonus(ScriptState *st)
{
    SP type = SP(conv_num(st, &AARGO2(2)));
    int val = conv_num(st, &AARGO2(3));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    pc_bonus(sd, type, val);

}

/*==========================================
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
static
void builtin_bonus2(ScriptState *st)
{
    SP type = SP(conv_num(st, &AARGO2(2)));
    int type2 = conv_num(st, &AARGO2(3));
    int val = conv_num(st, &AARGO2(4));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    pc_bonus2(sd, type, type2, val);

}

/*==========================================
 * スキル所得
 *------------------------------------------
 */
static
void builtin_skill(ScriptState *st)
{
    int level, flag = 1;
    dumb_ptr<map_session_data> sd;

    SkillID id = SkillID(conv_num(st, &AARGO2(2)));
    level = conv_num(st, &AARGO2(3));
    if (HARGO2(4))
        flag = conv_num(st, &AARGO2(4));
    sd = script_rid2sd(st);
    pc_skill(sd, id, level, flag);
    clif_skillinfoblock(sd);

}

/*==========================================
 * [Fate] Sets the skill level permanently
 *------------------------------------------
 */
static
void builtin_setskill(ScriptState *st)
{
    int level;
    dumb_ptr<map_session_data> sd;

    SkillID id = static_cast<SkillID>(conv_num(st, &AARGO2(2)));
    level = conv_num(st, &AARGO2(3));
    sd = script_rid2sd(st);

    sd->status.skill[id].lv = level;
    clif_skillinfoblock(sd);
}

/*==========================================
 * スキルレベル所得
 *------------------------------------------
 */
static
void builtin_getskilllv(ScriptState *st)
{
    SkillID id = SkillID(conv_num(st, &AARGO2(2)));
    push_int(st->stack, ByteCode::INT, pc_checkskill(script_rid2sd(st), id));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_getgmlevel(ScriptState *st)
{
    push_int(st->stack, ByteCode::INT, pc_isGM(script_rid2sd(st)));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_end(ScriptState *st)
{
    st->state = ScriptEndState::END;
}

/*==========================================
 * [Freeyorp] Return the current opt2
 *------------------------------------------
 */

static
void builtin_getopt2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);

    push_int(st->stack, ByteCode::INT, uint16_t(sd->opt2));

}

/*==========================================
 * [Freeyorp] Sets opt2
 *------------------------------------------
 */

static
void builtin_setopt2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    Opt2 new_opt2 = Opt2(conv_num(st, &AARGO2(2)));
    sd = script_rid2sd(st);
    if (new_opt2 == sd->opt2)
        return;
    sd->opt2 = new_opt2;
    clif_changeoption(sd);
    pc_calcstatus(sd, 0);

}

/*==========================================
 *      セーブポイントの保存
 *------------------------------------------
 */
static
void builtin_savepoint(ScriptState *st)
{
    int x, y;

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    x = conv_num(st, &AARGO2(3));
    y = conv_num(st, &AARGO2(4));
    pc_setsavepoint(script_rid2sd(st), str, x, y);
}

/*==========================================
 * gettimetick(type)
 *
 * type The type of time measurement.
 *  Specify 0 for the system tick, 1 for
 *  seconds elapsed today, or 2 for seconds
 *  since Unix epoch. Defaults to 0 for any
 *  other value.
 *------------------------------------------
 */
static
void builtin_gettimetick(ScriptState *st)   /* Asgard Version */
{
    int type;
    type = conv_num(st, &AARGO2(2));

    switch (type)
    {
        /* Number of seconds elapsed today(0-86399, 00:00:00-23:59:59). */
        case 1:
        {
            struct tm t = TimeT::now();
            push_int(st->stack, ByteCode::INT,
                    t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec);
            break;
        }
        /* Seconds since Unix epoch. */
        case 2:
            push_int(st->stack, ByteCode::INT, static_cast<time_t>(TimeT::now()));
            break;
        /* System tick(unsigned int, and yes, it will wrap). */
        case 0:
        default:
            push_int(st->stack, ByteCode::INT, gettick().time_since_epoch().count());
            break;
    }
}

/*==========================================
 * GetTime(Type);
 * 1: Sec     2: Min     3: Hour
 * 4: WeekDay     5: MonthDay     6: Month
 * 7: Year
 *------------------------------------------
 */
static
void builtin_gettime(ScriptState *st)   /* Asgard Version */
{
    int type = conv_num(st, &AARGO2(2));

    struct tm t = TimeT::now();

    switch (type)
    {
        case 1:                //Sec(0~59)
            push_int(st->stack, ByteCode::INT, t.tm_sec);
            break;
        case 2:                //Min(0~59)
            push_int(st->stack, ByteCode::INT, t.tm_min);
            break;
        case 3:                //Hour(0~23)
            push_int(st->stack, ByteCode::INT, t.tm_hour);
            break;
        case 4:                //WeekDay(0~6)
            push_int(st->stack, ByteCode::INT, t.tm_wday);
            break;
        case 5:                //MonthDay(01~31)
            push_int(st->stack, ByteCode::INT, t.tm_mday);
            break;
        case 6:                //Month(01~12)
            push_int(st->stack, ByteCode::INT, t.tm_mon + 1);
            break;
        case 7:                //Year(20xx)
            push_int(st->stack, ByteCode::INT, t.tm_year + 1900);
            break;
        default:               //(format error)
            push_int(st->stack, ByteCode::INT, -1);
            break;
    }
}

/*==========================================
 * カプラ倉庫を開く
 *------------------------------------------
 */
static
void builtin_openstorage(ScriptState *st)
{
//  int sync = 0;
//  if (st->end >= 3) sync = conv_num(st,& (st->stack->stack_data[st->start+2]));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

//  if (sync) {
    st->state = ScriptEndState::STOP;
    sd->npc_flags.storage = 1;
//  } else st->state = ScriptEndState::END;

    storage_storageopen(sd);
}

/*==========================================
 * NPCで経験値上げる
 *------------------------------------------
 */
static
void builtin_getexp(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int base = 0, job = 0;

    base = conv_num(st, &AARGO2(2));
    job = conv_num(st, &AARGO2(3));
    if (base < 0 || job < 0)
        return;
    if (sd)
        pc_gainexp_reason(sd, base, job, PC_GAINEXP_REASON::SCRIPT);

}

/*==========================================
 * モンスター発生
 *------------------------------------------
 */
static
void builtin_monster(ScriptState *st)
{
    int mob_class, amount, x, y;
    NpcEvent event;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    x = conv_num(st, &AARGO2(3));
    y = conv_num(st, &AARGO2(4));
    MobName str = stringish<MobName>(ZString(conv_str(st, &AARGO2(5))));
    mob_class = conv_num(st, &AARGO2(6));
    amount = conv_num(st, &AARGO2(7));
    if (HARGO2(8))
        extract(ZString(conv_str(st, &AARGO2(8))), &event);

    mob_once_spawn(map_id2sd(st->rid), mapname, x, y, str, mob_class, amount,
            event);
}

/*==========================================
 * モンスター発生
 *------------------------------------------
 */
static
void builtin_areamonster(ScriptState *st)
{
    int mob_class, amount, x0, y0, x1, y1;
    NpcEvent event;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    x0 = conv_num(st, &AARGO2(3));
    y0 = conv_num(st, &AARGO2(4));
    x1 = conv_num(st, &AARGO2(5));
    y1 = conv_num(st, &AARGO2(6));
    MobName str = stringish<MobName>(ZString(conv_str(st, &AARGO2(7))));
    mob_class = conv_num(st, &AARGO2(8));
    amount = conv_num(st, &AARGO2(9));
    if (HARGO2(10))
        extract(ZString(conv_str(st, &AARGO2(10))), &event);

    mob_once_spawn_area(map_id2sd(st->rid), mapname, x0, y0, x1, y1, str, mob_class,
            amount, event);
}

/*==========================================
 * モンスター削除
 *------------------------------------------
 */
static
void builtin_killmonster_sub(dumb_ptr<block_list> bl, NpcEvent event)
{
    dumb_ptr<mob_data> md = bl->is_mob();
    if (event)
    {
        if (event == md->npc_event)
            mob_delete(md);
        return;
    }
    else if (!event)
    {
        if (md->spawn.delay1 == static_cast<interval_t>(-1)
            && md->spawn.delay2 == static_cast<interval_t>(-1))
            mob_delete(md);
        return;
    }
}

static
void builtin_killmonster(ScriptState *st)
{
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    ZString event_ = ZString(conv_str(st, &AARGO2(3)));
    NpcEvent event;
    if (event_ != "All")
        extract(event_, &event);

    map_local *m = map_mapname2mapid(mapname);
    if (m == nullptr)
        return;
    map_foreachinarea(std::bind(builtin_killmonster_sub, ph::_1, event),
            m,
            0, 0,
            m->xs, m->ys,
            BL::MOB);
}

static
void builtin_killmonsterall_sub(dumb_ptr<block_list> bl)
{
    mob_delete(bl->is_mob());
}

static
void builtin_killmonsterall(ScriptState *st)
{
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));

    map_local *m = map_mapname2mapid(mapname);
    if (m == nullptr)
        return;
    map_foreachinarea(builtin_killmonsterall_sub,
            m,
            0, 0,
            m->xs, m->ys,
            BL::MOB);
}

/*==========================================
 * NPC主体イベント実行
 *------------------------------------------
 */
static
void builtin_donpcevent(ScriptState *st)
{
    ZString event_ = ZString(conv_str(st, &AARGO2(2)));
    NpcEvent event;
    extract(event_, &event);
    npc_event_do(event);
}

/*==========================================
 * イベントタイマー追加
 *------------------------------------------
 */
static
void builtin_addtimer(ScriptState *st)
{
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARGO2(2)));
    ZString event_ = ZString(conv_str(st, &AARGO2(3)));
    NpcEvent event;
    extract(event_, &event);
    pc_addeventtimer(script_rid2sd(st), tick, event);
}

/*==========================================
 * NPCタイマー初期化
 *------------------------------------------
 */
static
void builtin_initnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    if (HARGO2(2))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARGO2(2)))));
    else
        nd_ = map_id_is_npc(st->oid);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_settimerevent_tick(nd, interval_t::zero());
    npc_timerevent_start(nd);
}

/*==========================================
 * NPCタイマー開始
 *------------------------------------------
 */
static
void builtin_startnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    if (HARGO2(2))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARGO2(2)))));
    else
        nd_ = map_id_is_npc(st->oid);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_timerevent_start(nd);
}

/*==========================================
 * NPCタイマー停止
 *------------------------------------------
 */
static
void builtin_stopnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    if (HARGO2(2))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARGO2(2)))));
    else
        nd_ = map_id_is_npc(st->oid);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_timerevent_stop(nd);
}

/*==========================================
 * NPCタイマー情報所得
 *------------------------------------------
 */
static
void builtin_getnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    int type = conv_num(st, &AARGO2(2));
    int val = 0;
    if (HARGO2(3))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARGO2(3)))));
    else
        nd_ = map_id_is_npc(st->oid);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    switch (type)
    {
        case 0:
            val = npc_gettimerevent_tick(nd).count();
            break;
        case 1:
            val = nd->scr.timer_active;
            break;
        case 2:
            val = nd->scr.timer_eventv.size();
            break;
    }
    push_int(st->stack, ByteCode::INT, val);
}

/*==========================================
 * NPCタイマー値設定
 *------------------------------------------
 */
static
void builtin_setnpctimer(ScriptState *st)
{
    dumb_ptr<npc_data> nd_;
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARGO2(2)));
    if (HARGO2(3))
        nd_ = npc_name2id(stringish<NpcName>(ZString(conv_str(st, &AARGO2(3)))));
    else
        nd_ = map_id_is_npc(st->oid);
    assert (nd_ && nd_->npc_subtype == NpcSubtype::SCRIPT);
    dumb_ptr<npc_data_script> nd = nd_->is_script();

    npc_settimerevent_tick(nd, tick);
}

/*==========================================
 * 天の声アナウンス
 *------------------------------------------
 */
static
void builtin_announce(ScriptState *st)
{
    int flag;
    ZString str = ZString(conv_str(st, &AARGO2(2)));
    flag = conv_num(st, &AARGO2(3));

    if (flag & 0x0f)
    {
        dumb_ptr<block_list> bl;
        if (flag & 0x08)
            bl = map_id2bl(st->oid);
        else
            bl = script_rid2sd(st);
        clif_GMmessage(bl, str, flag);
    }
    else
        intif_GMmessage(str);
}

/*==========================================
 * 天の声アナウンス（特定マップ）
 *------------------------------------------
 */
static
void builtin_mapannounce_sub(dumb_ptr<block_list> bl, XString str, int flag)
{
    clif_GMmessage(bl, str, flag | 3);
}

static
void builtin_mapannounce(ScriptState *st)
{
    int flag;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    ZString str = ZString(conv_str(st, &AARGO2(3)));
    flag = conv_num(st, &AARGO2(4));

    map_local *m = map_mapname2mapid(mapname);
    if (m == nullptr)
        return;
    map_foreachinarea(std::bind(builtin_mapannounce_sub, ph::_1, str, flag & 0x10),
            m,
            0, 0,
            m->xs, m->ys,
            BL::PC);
}

/*==========================================
 * ユーザー数所得
 *------------------------------------------
 */
static
void builtin_getusers(ScriptState *st)
{
    int flag = conv_num(st, &AARGO2(2));
    dumb_ptr<block_list> bl = map_id2bl((flag & 0x08) ? st->oid : st->rid);
    int val = 0;
    switch (flag & 0x07)
    {
        case 0:
            val = bl->bl_m->users;
            break;
        case 1:
            val = map_getusers();
            break;
    }
    push_int(st->stack, ByteCode::INT, val);
}

/*==========================================
 * マップ指定ユーザー数所得
 *------------------------------------------
 */
static
void builtin_getmapusers(ScriptState *st)
{
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    map_local *m = map_mapname2mapid(str);
    if (m == nullptr)
    {
        push_int(st->stack, ByteCode::INT, -1);
        return;
    }
    push_int(st->stack, ByteCode::INT, m->users);
}

/*==========================================
 * エリア指定ユーザー数所得
 *------------------------------------------
 */
static
void builtin_getareausers_sub(dumb_ptr<block_list> bl, int *users)
{
    if (bool(bl->is_player()->status.option & Option::HIDE))
        return;
    (*users)++;
}

static
void builtin_getareausers_living_sub(dumb_ptr<block_list> bl, int *users)
{
    if (bool(bl->is_player()->status.option & Option::HIDE))
        return;
    if (!pc_isdead(bl->is_player()))
        (*users)++;
}

static
void builtin_getareausers(ScriptState *st)
{
    int x0, y0, x1, y1, users = 0;
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    x0 = conv_num(st, &AARGO2(3));
    y0 = conv_num(st, &AARGO2(4));
    x1 = conv_num(st, &AARGO2(5));
    y1 = conv_num(st, &AARGO2(6));

    int living = 0;
    if (HARGO2(7))
    {
        living = conv_num(st, &AARGO2(7));
    }
    map_local *m = map_mapname2mapid(str);
    if (m == nullptr)
    {
        push_int(st->stack, ByteCode::INT, -1);
        return;
    }
    map_foreachinarea(std::bind(living ? builtin_getareausers_living_sub: builtin_getareausers_sub, ph::_1, &users),
            m,
            x0, y0,
            x1, y1,
            BL::PC);
    push_int(st->stack, ByteCode::INT, users);
}

/*==========================================
 * エリア指定ドロップアイテム数所得
 *------------------------------------------
 */
static
void builtin_getareadropitem_sub(dumb_ptr<block_list> bl, int item, int *amount)
{
    dumb_ptr<flooritem_data> drop = bl->is_item();

    if (drop->item_data.nameid == item)
        (*amount) += drop->item_data.amount;

}

static
void builtin_getareadropitem_sub_anddelete(dumb_ptr<block_list> bl, int item, int *amount)
{
    dumb_ptr<flooritem_data> drop = bl->is_item();

    if (drop->item_data.nameid == item)
    {
        (*amount) += drop->item_data.amount;
        clif_clearflooritem(drop, 0);
        map_delobject(drop->bl_id, drop->bl_type);
    }
}

static
void builtin_getareadropitem(ScriptState *st)
{
    int x0, y0, x1, y1, item, amount = 0, delitems = 0;
    struct script_data *data;

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    x0 = conv_num(st, &AARGO2(3));
    y0 = conv_num(st, &AARGO2(4));
    x1 = conv_num(st, &AARGO2(5));
    y1 = conv_num(st, &AARGO2(6));

    data = &AARGO2(7);
    get_val(st, data);
    if (data->type == ByteCode::STR || data->type == ByteCode::CONSTSTR)
    {
        ItemName name = stringish<ItemName>(ZString(conv_str(st, data)));
        struct item_data *item_data = itemdb_searchname(name);
        item = 512;
        if (item_data)
            item = item_data->nameid;
    }
    else
        item = conv_num(st, data);

    if (HARGO2(8))
        delitems = conv_num(st, &AARGO2(8));

    map_local *m = map_mapname2mapid(str);
    if (m == nullptr)
    {
        push_int(st->stack, ByteCode::INT, -1);
        return;
    }
    if (delitems)
        map_foreachinarea(std::bind(builtin_getareadropitem_sub_anddelete, ph::_1, item, &amount),
                m,
                x0, y0,
                x1, y1,
                BL::ITEM);
    else
        map_foreachinarea(std::bind(builtin_getareadropitem_sub, ph::_1, item, &amount),
                m,
                x0, y0,
                x1, y1,
                BL::ITEM);

    push_int(st->stack, ByteCode::INT, amount);
}

/*==========================================
 * NPCの有効化
 *------------------------------------------
 */
static
void builtin_enablenpc(ScriptState *st)
{
    NpcName str = stringish<NpcName>(ZString(conv_str(st, &AARGO2(2))));
    npc_enable(str, 1);
}

/*==========================================
 * NPCの無効化
 *------------------------------------------
 */
static
void builtin_disablenpc(ScriptState *st)
{
    NpcName str = stringish<NpcName>(ZString(conv_str(st, &AARGO2(2))));
    npc_enable(str, 0);
}

/*==========================================
 * 状態異常にかかる
 *------------------------------------------
 */
static
void builtin_sc_start(ScriptState *st)
{
    dumb_ptr<block_list> bl;
    int val1;
    StatusChange type = static_cast<StatusChange>(conv_num(st, &AARGO2(2)));
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARGO2(3)));
    if (tick < std::chrono::seconds(1))
        // work around old behaviour of:
        // speed potion
        // atk potion
        // matk potion
        //
        // which used to use seconds
        // all others used milliseconds
        tick *= 1000;
    val1 = conv_num(st, &AARGO2(4));
    if (HARGO2(5))    //指定したキャラを状態異常にする
        bl = map_id2bl(conv_num(st, &AARGO2(5)));
    else
        bl = map_id2bl(st->rid);
    skill_status_change_start(bl, type, val1, tick);
}

/*==========================================
 * 状態異常が直る
 *------------------------------------------
 */
static
void builtin_sc_end(ScriptState *st)
{
    dumb_ptr<block_list> bl;
    StatusChange type = StatusChange(conv_num(st, &AARGO2(2)));
    bl = map_id2bl(st->rid);
    skill_status_change_end(bl, type, nullptr);
}

static
void builtin_sc_check(ScriptState *st)
{
    dumb_ptr<block_list> bl;
    StatusChange type = StatusChange(conv_num(st, &AARGO2(2)));
    bl = map_id2bl(st->rid);

    push_int(st->stack, ByteCode::INT, skill_status_change_active(bl, type));

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_debugmes(ScriptState *st)
{
    dumb_string mes = conv_str(st, &AARGO2(2));
    PRINTF("script debug : %d %d : %s\n",
            st->rid, st->oid, mes);
}

/*==========================================
 * ステータスリセット
 *------------------------------------------
 */
static
void builtin_resetstatus(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;
    sd = script_rid2sd(st);
    pc_resetstate(sd);
}

/*==========================================
 * 性別変換
 *------------------------------------------
 */
static
void builtin_changesex(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = NULL;
    sd = script_rid2sd(st);

    chrif_char_ask_name(-1, sd->status.name, 5, HumanTimeDiff()); // type: 5 - changesex
    chrif_save(sd);
}

/*==========================================
 * RIDのアタッチ
 *------------------------------------------
 */
static
void builtin_attachrid(ScriptState *st)
{
    st->rid = conv_num(st, &AARGO2(2));
    push_int(st->stack, ByteCode::INT, (map_id2sd(st->rid) != NULL));
}

/*==========================================
 * RIDのデタッチ
 *------------------------------------------
 */
static
void builtin_detachrid(ScriptState *st)
{
    st->rid = 0;
}

/*==========================================
 * 存在チェック
 *------------------------------------------
 */
static
void builtin_isloggedin(ScriptState *st)
{
    push_int(st->stack, ByteCode::INT,
              map_id2sd(conv_num(st,
                          &AARGO2(2))) != NULL);
}

/*==========================================
 *
 *------------------------------------------
 */
enum
{
    MF_NOMEMO = 0,
    MF_NOTELEPORT = 1,
    MF_NOSAVE = 2,
    MF_NOBRANCH = 3,
    MF_NOPENALTY = 4,
    MF_NOZENYPENALTY = 5,
    MF_PVP = 6,
    MF_PVP_NOPARTY = 7,
    //MF_PVP_NOGUILD = 8,
    //MF_GVG = 9,
    //MF_GVG_NOPARTY = 10,
    MF_NOTRADE = 11,
    //MF_NOSKILL = 12,
    MF_NOWARP = 13,
    MF_NOPVP = 14,
    MF_NOICEWALL = 15,
    MF_SNOW = 16,
    MF_FOG = 17,
    MF_SAKURA = 18,
    MF_LEAVES = 19,
    MF_RAIN = 20,
    MF_RESAVE = 21,
};

static
void builtin_setmapflag(ScriptState *st)
{
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    int i = conv_num(st, &AARGO2(3));
    map_local *m = map_mapname2mapid(str);
    if (m != nullptr)
    {
        switch (i)
        {
            case MF_NOMEMO:
                m->flag.nomemo = 1;
                break;
            case MF_NOTELEPORT:
                m->flag.noteleport = 1;
                break;
            case MF_NOBRANCH:
                m->flag.nobranch = 1;
                break;
            case MF_NOPENALTY:
                m->flag.nopenalty = 1;
                break;
            case MF_PVP_NOPARTY:
                m->flag.pvp_noparty = 1;
                break;
            case MF_NOZENYPENALTY:
                m->flag.nozenypenalty = 1;
                break;
            case MF_NOTRADE:
                m->flag.notrade = 1;
                break;
            case MF_NOWARP:
                m->flag.nowarp = 1;
                break;
            case MF_NOPVP:
                m->flag.nopvp = 1;
                break;
            case MF_NOICEWALL: // [Valaris]
                m->flag.noicewall = 1;
                break;
            case MF_SNOW:      // [Valaris]
                m->flag.snow = 1;
                break;
            case MF_FOG:       // [Valaris]
                m->flag.fog = 1;
                break;
            case MF_SAKURA:    // [Valaris]
                m->flag.sakura = 1;
                break;
            case MF_LEAVES:    // [Valaris]
                m->flag.leaves = 1;
                break;
            case MF_RAIN:      // [Valaris]
                m->flag.rain = 1;
                break;
        }
    }

}

static
void builtin_removemapflag(ScriptState *st)
{
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    int i = conv_num(st, &AARGO2(3));
    map_local *m = map_mapname2mapid(str);
    if (m != nullptr)
    {
        switch (i)
        {
            case MF_NOMEMO:
                m->flag.nomemo = 0;
                break;
            case MF_NOTELEPORT:
                m->flag.noteleport = 0;
                break;
            case MF_NOSAVE:
                m->flag.nosave = 0;
                break;
            case MF_RESAVE:
                m->flag.resave = 0;
                break;
            case MF_NOBRANCH:
                m->flag.nobranch = 0;
                break;
            case MF_NOPENALTY:
                m->flag.nopenalty = 0;
                break;
            case MF_PVP_NOPARTY:
                m->flag.pvp_noparty = 0;
                break;
            case MF_NOZENYPENALTY:
                m->flag.nozenypenalty = 0;
                break;
            case MF_NOWARP:
                m->flag.nowarp = 0;
                break;
            case MF_NOPVP:
                m->flag.nopvp = 0;
                break;
            case MF_NOICEWALL: // [Valaris]
                m->flag.noicewall = 0;
                break;
            case MF_SNOW:      // [Valaris]
                m->flag.snow = 0;
                break;
            case MF_FOG:       // [Valaris]
                m->flag.fog = 0;
                break;
            case MF_SAKURA:    // [Valaris]
                m->flag.sakura = 0;
                break;
            case MF_LEAVES:    // [Valaris]
                m->flag.leaves = 0;
                break;
            case MF_RAIN:      // [Valaris]
                m->flag.rain = 0;
                break;
        }
    }
}

static
void builtin_getmapflag(ScriptState *st)
{
    int r = -1;

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    int i = conv_num(st, &AARGO2(3));
    map_local *m = map_mapname2mapid(str);
    if (m != nullptr)
    {
        switch (i)
        {
            case MF_NOMEMO:
                r = m->flag.nomemo;
                break;
            case MF_NOTELEPORT:
                r = m->flag.noteleport;
                break;
            case MF_NOSAVE:
                r = m->flag.nosave;
                break;
            case MF_RESAVE:
                r = m->flag.resave;
                break;
            case MF_NOBRANCH:
                r = m->flag.nobranch;
                break;
            case MF_NOPENALTY:
                r = m->flag.nopenalty;
                break;
            case MF_PVP_NOPARTY:
                r = m->flag.pvp_noparty;
                break;
            case MF_NOZENYPENALTY:
                r = m->flag.nozenypenalty;
                break;
            case MF_NOWARP:
                r = m->flag.nowarp;
                break;
            case MF_NOPVP:
                r = m->flag.nopvp;
                break;
            case MF_NOICEWALL: // [Valaris]
                r = m->flag.noicewall;
                break;
            case MF_SNOW:      // [Valaris]
                r = m->flag.snow;
                break;
            case MF_FOG:       // [Valaris]
                r = m->flag.fog;
                break;
            case MF_SAKURA:    // [Valaris]
                r = m->flag.sakura;
                break;
            case MF_LEAVES:    // [Valaris]
                r = m->flag.leaves;
                break;
            case MF_RAIN:      // [Valaris]
                r = m->flag.rain;
                break;
        }
    }

    push_int(st->stack, ByteCode::INT, r);
}

static
void builtin_pvpon(ScriptState *st)
{
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    map_local *m = map_mapname2mapid(str);
    if (m != nullptr && !m->flag.pvp && !m->flag.nopvp)
    {
        m->flag.pvp = 1;

        if (battle_config.pk_mode)  // disable ranking functions if pk_mode is on [Valaris]
            return;

        for (int i = 0; i < fd_max; i++)
        {
            if (!session[i])
                continue;
            map_session_data *pl_sd = static_cast<map_session_data *>(session[i]->session_data.get());
            if (pl_sd && pl_sd->state.auth)
            {
                if (m == pl_sd->bl_m && !pl_sd->pvp_timer)
                {
                    pl_sd->pvp_timer = Timer(gettick() + std::chrono::milliseconds(200),
                            std::bind(pc_calc_pvprank_timer, ph::_1, ph::_2,
                                pl_sd->bl_id));
                    pl_sd->pvp_rank = 0;
                    pl_sd->pvp_lastusers = 0;
                    pl_sd->pvp_point = 5;
                }
            }
        }
    }

}

static
void builtin_pvpoff(ScriptState *st)
{
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    map_local *m = map_mapname2mapid(str);
    if (m != nullptr && m->flag.pvp && m->flag.nopvp)
    {
        m->flag.pvp = 0;

        if (battle_config.pk_mode)  // disable ranking options if pk_mode is on [Valaris]
            return;

        for (int i = 0; i < fd_max; i++)
        {
            if (!session[i])
                continue;
            map_session_data *pl_sd = static_cast<map_session_data *>(session[i]->session_data.get());
            if (pl_sd && pl_sd->state.auth)
            {
                if (m == pl_sd->bl_m)
                {
                    pl_sd->pvp_timer.cancel();
                }
            }
        }
    }

}

/*==========================================
 *      NPCエモーション
 *------------------------------------------
 */

static
void builtin_emotion(ScriptState *st)
{
    int type;
    type = conv_num(st, &AARGO2(2));
    if (type < 0 || type > 100)
        return;
    clif_emotion(map_id2bl(st->oid), type);
}

static
void builtin_mapwarp(ScriptState *st)   // Added by RoVeRT
{
    int x, y;
    int x0, y0, x1, y1;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    x0 = 0;
    y0 = 0;
    map_local *m = map_mapname2mapid(mapname);
    x1 = m->xs;
    y1 = m->ys;
    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(3))));
    x = conv_num(st, &AARGO2(4));
    y = conv_num(st, &AARGO2(5));

    if (m == nullptr)
        return;

    map_foreachinarea(std::bind(builtin_areawarp_sub, ph::_1, str, x, y),
            m,
            x0, y0,
            x1, y1,
            BL::PC);
}

static
void builtin_cmdothernpc(ScriptState *st)   // Added by RoVeRT
{
    NpcName npc = stringish<NpcName>(ZString(conv_str(st, &AARGO2(2))));
    ZString command = ZString(conv_str(st, &AARGO2(3)));

    npc_command(map_id2sd(st->rid), npc, command);
}

static
void builtin_mobcount_sub(dumb_ptr<block_list> bl, NpcEvent event, int *c)
{
    if (event == bl->is_mob()->npc_event)
        (*c)++;
}

static
void builtin_mobcount(ScriptState *st)  // Added by RoVeRT
{
    int c = 0;
    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    ZString event_ = ZString(conv_str(st, &AARGO2(3)));
    NpcEvent event;
    extract(event_, &event);

    map_local *m = map_mapname2mapid(mapname);
    if (m == nullptr)
    {
        push_int(st->stack, ByteCode::INT, -1);
        return;
    }
    map_foreachinarea(std::bind(builtin_mobcount_sub, ph::_1, event, &c),
            m,
            0, 0,
            m->xs, m->ys,
            BL::MOB);

    push_int(st->stack, ByteCode::INT, (c - 1));

}

static
void builtin_marriage(ScriptState *st)
{
    CharName partner = stringish<CharName>(ZString(conv_str(st, &AARGO2(2))));
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    dumb_ptr<map_session_data> p_sd = map_nick2sd(partner);

    if (sd == NULL || p_sd == NULL || pc_marriage(sd, p_sd) < 0)
    {
        push_int(st->stack, ByteCode::INT, 0);
        return;
    }
    push_int(st->stack, ByteCode::INT, 1);
}

static
void builtin_divorce(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    st->state = ScriptEndState::STOP;           // rely on pc_divorce to restart

    sd->npc_flags.divorce = 1;

    if (sd == NULL || pc_divorce(sd) < 0)
    {
        push_int(st->stack, ByteCode::INT, 0);
        return;
    }

    push_int(st->stack, ByteCode::INT, 1);
}

/*==========================================
 * IDからItem名
 *------------------------------------------
 */
static
void builtin_getitemname(ScriptState *st)
{
    struct item_data *i_data;
    struct script_data *data;

    data = &AARGO2(2);
    get_val(st, data);
    if (data->type == ByteCode::STR || data->type == ByteCode::CONSTSTR)
    {
        ItemName name = stringish<ItemName>(ZString(conv_str(st, data)));
        i_data = itemdb_searchname(name);
    }
    else
    {
        int item_id = conv_num(st, data);
        i_data = itemdb_search(item_id);
    }

    dumb_string item_name;
    if (i_data)
        item_name = dumb_string::copys(i_data->jname);
    else
        item_name = dumb_string::copys("Unknown Item");

    push_str(st->stack, ByteCode::STR, item_name);
}

static
void builtin_getspellinvocation(ScriptState *st)
{
    dumb_string name = conv_str(st, &AARGO2(2));

    FString invocation = magic_find_invocation(name.str());
    if (!invocation)
        invocation = "...";

    push_str(st->stack, ByteCode::STR, dumb_string::copys(invocation));
}

static
void builtin_getpartnerid2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    push_int(st->stack, ByteCode::INT, sd->status.partner_id);
}

/*==========================================
 * PCの所持品情報読み取り
 *------------------------------------------
 */
static
void builtin_getinventorylist(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int i, j = 0;
    if (!sd)
        return;
    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid > 0
            && sd->status.inventory[i].amount > 0)
        {
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_id"), j),
                       sd->status.inventory[i].nameid);
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_amount"), j),
                       sd->status.inventory[i].amount);
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_equip"), j),
                    static_cast<uint16_t>(sd->status.inventory[i].equip));
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_refine"), j),
                       sd->status.inventory[i].refine);
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_identify"), j),
                       sd->status.inventory[i].identify);
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_attribute"), j),
                       sd->status.inventory[i].attribute);
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_card1"), j),
                       sd->status.inventory[i].card[0]);
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_card2"), j),
                       sd->status.inventory[i].card[1]);
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_card3"), j),
                       sd->status.inventory[i].card[2]);
            pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_card4"), j),
                       sd->status.inventory[i].card[3]);
            j++;
        }
    }
    pc_setreg(sd, SIR::from(variable_names.intern("@inventorylist_count")), j);
}

static
void builtin_getactivatedpoolskilllist(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    SkillID pool_skills[MAX_SKILL_POOL];
    int skill_pool_size = skill_pool(sd, pool_skills);
    int i, count = 0;

    if (!sd)
        return;

    for (i = 0; i < skill_pool_size; i++)
    {
        SkillID skill_id = pool_skills[i];

        if (sd->status.skill[skill_id].lv)
        {
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_id"), count),
                    static_cast<uint16_t>(skill_id));
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_lv"), count),
                    sd->status.skill[skill_id].lv);
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_flag"), count),
                    static_cast<uint16_t>(sd->status.skill[skill_id].flags));
            pc_setregstr(sd, SIR::from(variable_names.intern("@skilllist_name$"), count),
                    skill_name(skill_id));
            ++count;
        }
    }
    pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_count")), count);

}

static
void builtin_getunactivatedpoolskilllist(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    int i, count = 0;

    if (!sd)
        return;

    for (i = 0; i < skill_pool_skills_size; i++)
    {
        SkillID skill_id = skill_pool_skills[i];

        if (sd->status.skill[skill_id].lv
            && !bool(sd->status.skill[skill_id].flags & SkillFlags::POOL_ACTIVATED))
        {
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_id"), count),
                    static_cast<uint16_t>(skill_id));
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_lv"), count),
                    sd->status.skill[skill_id].lv);
            pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_flag"), count),
                    static_cast<uint16_t>(sd->status.skill[skill_id].flags));
            pc_setregstr(sd, SIR::from(variable_names.intern("@skilllist_name$"), count),
                    skill_name(skill_id));
            ++count;
        }
    }
    pc_setreg(sd, SIR::from(variable_names.intern("@skilllist_count")), count);
}

static
void builtin_poolskill(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    SkillID skill_id = SkillID(conv_num(st, &AARGO2(2)));

    skill_pool_activate(sd, skill_id);
    clif_skillinfoblock(sd);

}

static
void builtin_unpoolskill(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    SkillID skill_id = SkillID(conv_num(st, &AARGO2(2)));

    skill_pool_deactivate(sd, skill_id);
    clif_skillinfoblock(sd);

}

/*==========================================
 * NPCから発生するエフェクト
 * misceffect(effect, [target])
 *
 * effect The effect type/ID.
 * target The player name or being ID on
 *  which to display the effect. If not
 *  specified, it attempts to default to
 *  the current NPC or invoking PC.
 *------------------------------------------
 */
static
void builtin_misceffect(ScriptState *st)
{
    int type;
    int id = 0;
    CharName name;
    dumb_ptr<block_list> bl = NULL;

    type = conv_num(st, &AARGO2(2));

    if (HARGO2(3))
    {
        struct script_data *sdata = &AARGO2(3);

        get_val(st, sdata);

        if (sdata->type == ByteCode::STR || sdata->type == ByteCode::CONSTSTR)
            name = stringish<CharName>(ZString(conv_str(st, sdata)));
        else
            id = conv_num(st, sdata);
    }

    if (name.to__actual())
    {
        dumb_ptr<map_session_data> sd = map_nick2sd(name);
        if (sd)
            bl = sd;
    }
    else if (id)
        bl = map_id2bl(id);
    else if (st->oid)
        bl = map_id2bl(st->oid);
    else
    {
        dumb_ptr<map_session_data> sd = script_rid2sd(st);
        if (sd)
            bl = sd;
    }

    if (bl)
        clif_misceffect(bl, type);

}

/*==========================================
 * Special effects [Valaris]
 *------------------------------------------
 */
static
void builtin_specialeffect(ScriptState *st)
{
    dumb_ptr<block_list> bl = map_id2bl(st->oid);

    if (bl == NULL)
        return;

    clif_specialeffect(bl,
                        conv_num(st,
                                  &AARGO2(2)),
                        0);

}

static
void builtin_specialeffect2(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    if (sd == NULL)
        return;

    clif_specialeffect(sd,
                        conv_num(st,
                                  &AARGO2(2)),
                        0);

}

/*==========================================
 * Nude [Valaris]
 *------------------------------------------
 */

static
void builtin_nude(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    if (sd == NULL)
        return;

    for (EQUIP i : EQUIPs)
        if (sd->equip_index[i] >= 0)
            pc_unequipitem(sd, sd->equip_index[i], CalcStatus::LATER);
    pc_calcstatus(sd, 0);

}

/*==========================================
 * UnequipById [Freeyorp]
 *------------------------------------------
 */

static
void builtin_unequipbyid(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    if (sd == NULL)
        return;

    EQUIP slot_id = EQUIP(conv_num(st, &AARGO2(2)));

    if (slot_id >= EQUIP() && slot_id < EQUIP::COUNT
        && sd->equip_index[slot_id] >= 0)
        pc_unequipitem(sd, sd->equip_index[slot_id], CalcStatus::LATER);

    pc_calcstatus(sd, 0);

}

/*==========================================
 * gmcommand [MouseJstr]
 *
 * suggested on the forums...
 *------------------------------------------
 */

static
void builtin_gmcommand(ScriptState *st)
{
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);
    dumb_string cmd = conv_str(st, &AARGO2(2));

    is_atcommand(sd->fd, sd, cmd, 99);

}

/*==========================================
 * npcwarp [remoitnane]
 * Move NPC to a new position on the same map.
 *------------------------------------------
 */
static
void builtin_npcwarp(ScriptState *st)
{
    int x, y;
    dumb_ptr<npc_data> nd = NULL;

    x = conv_num(st, &AARGO2(2));
    y = conv_num(st, &AARGO2(3));
    NpcName npc = stringish<NpcName>(ZString(conv_str(st, &AARGO2(4))));
    nd = npc_name2id(npc);

    if (!nd)
    {
        PRINTF("builtin_npcwarp: no such npc: %s\n", npc);
        return;
    }

    map_local *m = nd->bl_m;

    /* Crude sanity checks. */
    if (m == nullptr || !nd->bl_prev
            || x < 0 || x > m->xs -1
            || y < 0 || y > m->ys - 1)
        return;

    npc_enable(npc, 0);
    map_delblock(nd); /* [Freeyorp] */
    nd->bl_x = x;
    nd->bl_y = y;
    map_addblock(nd);
    npc_enable(npc, 1);

}

/*==========================================
 * message [MouseJstr]
 *------------------------------------------
 */

static
void builtin_message(ScriptState *st)
{
    CharName player = stringish<CharName>(ZString(conv_str(st, &AARGO2(2))));
    ZString msg = ZString(conv_str(st, &AARGO2(3)));

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(player);
    if (pl_sd == NULL)
        return;
    clif_displaymessage(pl_sd->fd, msg);

}

/*==========================================
 * npctalk (sends message to surrounding
 * area) [Valaris]
 *------------------------------------------
 */

static
void builtin_npctalk(ScriptState *st)
{
    dumb_ptr<npc_data> nd = map_id_is_npc(st->oid);
    dumb_string str = conv_str(st, &AARGO2(2));

    if (nd)
    {
        MString message;
        message += nd->name;
        message += " : ";
        message += ZString(str);
        clif_message(nd, FString(message));
    }
}

/*==========================================
  * getlook char info. getlook(arg)
  *------------------------------------------
  */
static
void builtin_getlook(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    LOOK type = LOOK(conv_num(st, &AARGO2(2)));
    int val = -1;
    switch (type)
    {
        case LOOK::HAIR:        //1
            val = sd->status.hair;
            break;
        case LOOK::WEAPON:      //2
            val = static_cast<uint16_t>(sd->status.weapon);
            break;
        case LOOK::HEAD_BOTTOM: //3
            val = sd->status.head_bottom;
            break;
        case LOOK::HEAD_TOP:    //4
            val = sd->status.head_top;
            break;
        case LOOK::HEAD_MID:    //5
            val = sd->status.head_mid;
            break;
        case LOOK::HAIR_COLOR:  //6
            val = sd->status.hair_color;
            break;
        case LOOK::CLOTHES_COLOR:   //7
            val = sd->status.clothes_color;
            break;
        case LOOK::SHIELD:      //8
            val = sd->status.shield;
            break;
        case LOOK::SHOES:       //9
            break;
    }

    push_int(st->stack, ByteCode::INT, val);
}

/*==========================================
  *     get char save point. argument: 0- map name, 1- x, 2- y
  *------------------------------------------
*/
static
void builtin_getsavepoint(ScriptState *st)
{
    int x, y, type;
    dumb_ptr<map_session_data> sd;

    sd = script_rid2sd(st);

    type = conv_num(st, &AARGO2(2));

    x = sd->status.save_point.x;
    y = sd->status.save_point.y;
    switch (type)
    {
        case 0:
        {
            dumb_string mapname = dumb_string::copys(sd->status.save_point.map_);
            push_str(st->stack, ByteCode::STR, mapname);
        }
            break;
        case 1:
            push_int(st->stack, ByteCode::INT, x);
            break;
        case 2:
            push_int(st->stack, ByteCode::INT, y);
            break;
    }
}

/*==========================================
 *     areatimer
 *------------------------------------------
 */
static
void builtin_areatimer_sub(dumb_ptr<block_list> bl, interval_t tick, NpcEvent event)
{
    pc_addeventtimer(bl->is_player(), tick, event);
}

static
void builtin_areatimer(ScriptState *st)
{
    int x0, y0, x1, y1;

    MapName mapname = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    x0 = conv_num(st, &AARGO2(3));
    y0 = conv_num(st, &AARGO2(4));
    x1 = conv_num(st, &AARGO2(5));
    y1 = conv_num(st, &AARGO2(6));
    interval_t tick = static_cast<interval_t>(conv_num(st, &AARGO2(7)));
    ZString event_ = ZString(conv_str(st, &AARGO2(8)));
    NpcEvent event;
    extract(event_, &event);

    map_local *m = map_mapname2mapid(mapname);
    if (m == nullptr)
        return;

    map_foreachinarea(std::bind(builtin_areatimer_sub, ph::_1, tick, event),
            m,
            x0, y0,
            x1, y1,
            BL::PC);
}

/*==========================================
 * Check whether the PC is in the specified rectangle
 *------------------------------------------
 */
static
void builtin_isin(ScriptState *st)
{
    int x1, y1, x2, y2;
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    MapName str = stringish<MapName>(ZString(conv_str(st, &AARGO2(2))));
    x1 = conv_num(st, &AARGO2(3));
    y1 = conv_num(st, &AARGO2(4));
    x2 = conv_num(st, &AARGO2(5));
    y2 = conv_num(st, &AARGO2(6));

    if (!sd)
        return;

    push_int(st->stack, ByteCode::INT,
              (sd->bl_x >= x1 && sd->bl_x <= x2)
              && (sd->bl_y >= y1 && sd->bl_y <= y2)
              && (str == sd->bl_m->name_));
}

// Trigger the shop on a (hopefully) nearby shop NPC
static
void builtin_shop(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);
    dumb_ptr<npc_data> nd;

    if (!sd)
        return;

    NpcName name = stringish<NpcName>(ZString(conv_str(st, &AARGO2(2))));
    nd = npc_name2id(name);
    if (!nd)
    {
        PRINTF("builtin_shop: no such npc: %s\n", name);
        return;
    }

    builtin_close(st);
    clif_npcbuysell(sd, nd->bl_id);
}

/*==========================================
 * Check whether the PC is dead
 *------------------------------------------
 */
static
void builtin_isdead(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    push_int(st->stack, ByteCode::INT, pc_isdead(sd));
}

/*========================================
 * Changes a NPC name, and sprite
 *----------------------------------------
 */
static
void builtin_fakenpcname(ScriptState *st)
{
    NpcName name = stringish<NpcName>(ZString(conv_str(st, &AARGO2(2))));
    NpcName newname = stringish<NpcName>(ZString(conv_str(st, &AARGO2(3))));
    int newsprite = conv_num(st, &AARGO2(4));
    dumb_ptr<npc_data> nd = npc_name2id(name);
    if (!nd)
    {
        PRINTF("builtin_fakenpcname: no such npc: %s\n", name);
        return;
    }
    nd->name = newname;
    nd->npc_class = newsprite;

    // Refresh this npc
    npc_enable(name, 0);
    npc_enable(name, 1);

}

/*============================
 * Gets the PC's x pos
 *----------------------------
 */
static
void builtin_getx(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    push_int(st->stack, ByteCode::INT, sd->bl_x);
}

/*============================
 * Gets the PC's y pos
 *----------------------------
 */
static
void builtin_gety(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    push_int(st->stack, ByteCode::INT, sd->bl_y);
}

/*
 * Get the PC's current map's name
 */
static
void builtin_getmap(ScriptState *st)
{
    dumb_ptr<map_session_data> sd = script_rid2sd(st);

    // A map_data lives essentially forever.
    push_str(st->stack, ByteCode::CONSTSTR, dumb_string::fake(sd->bl_m->name_));
}

static
void builtin_mapexit(ScriptState *)
{
    runflag = 0;
}


//
// 実行部main
//
/*==========================================
 * コマンドの読み取り
 *------------------------------------------
 */
static
ByteCode get_com(ScriptPointer *script)
{
    if (static_cast<uint8_t>(script->peek()) >= 0x80)
    {
        // synthetic! Does not advance pos yet.
        return ByteCode::INT;
    }
    return script->pop();
}

/*==========================================
 * 数値の所得
 *------------------------------------------
 */
static
int get_num(ScriptPointer *scr)
{
    int i = 0;
    int j = 0;
    uint8_t val;
    do
    {
        val = static_cast<uint8_t>(scr->pop());
        i += (val & 0x7f) << j;
        j += 6;
    }
    while (val >= 0xc0);
    return i;
}

/*==========================================
 * スタックから値を取り出す
 *------------------------------------------
 */
static
int pop_val(ScriptState *st)
{
    if (st->stack->stack_datav.empty())
        return 0;
    script_data& back = st->stack->stack_datav.back();
    get_val(st, &back);
    int rv = 0;
    if (back.type == ByteCode::INT)
        rv = back.u.numi;
    st->stack->stack_datav.pop_back();
    return rv;
}

static
bool isstr(struct script_data& c)
{
    return c.type == ByteCode::STR
        || c.type == ByteCode::CONSTSTR;
}

/*==========================================
 * 加算演算子
 *------------------------------------------
 */
static
void op_add(ScriptState *st)
{
    get_val(st, &st->stack->stack_datav.back());
    script_data back = st->stack->stack_datav.back();
    st->stack->stack_datav.pop_back();

    script_data& back1 = st->stack->stack_datav.back();
    get_val(st, &back1);

    if (!(isstr(back) || isstr(back1)))
    {
        back1.u.numi += back.u.numi;
    }
    else
    {
        dumb_string sb = conv_str(st, &back);
        dumb_string sb1 = conv_str(st, &back1);
        MString buf;
        buf += ZString(sb1);
        buf += ZString(sb);
        if (back1.type == ByteCode::STR)
            back1.u.str.delete_();
        if (back.type == ByteCode::STR)
            back.u.str.delete_();
        back1.type = ByteCode::STR;
        back1.u.str = dumb_string::copys(FString(buf));
    }
}

/*==========================================
 * 二項演算子(文字列)
 *------------------------------------------
 */
static
void op_2str(ScriptState *st, ByteCode op, dumb_string s1_, dumb_string s2_)
{
    ZString s1 = ZString(s1_);
    ZString s2 = ZString(s2_);
    int a = 0;

    switch (op)
    {
        case ByteCode::EQ:
            a = s1 == s2;
            break;
        case ByteCode::NE:
            a = s1 != s2;
            break;
        case ByteCode::GT:
            a = s1 > s2;
            break;
        case ByteCode::GE:
            a = s1 >= s2;
            break;
        case ByteCode::LT:
            a = s1 < s2;
            break;
        case ByteCode::LE:
            a = s1 <= s2;
            break;
        default:
            PRINTF("illegal string operater\n");
            break;
    }

    push_int(st->stack, ByteCode::INT, a);
}

/*==========================================
 * 二項演算子(数値)
 *------------------------------------------
 */
static
void op_2num(ScriptState *st, ByteCode op, int i1, int i2)
{
    switch (op)
    {
        case ByteCode::SUB:
            i1 -= i2;
            break;
        case ByteCode::MUL:
            i1 *= i2;
            break;
        case ByteCode::DIV:
            i1 /= i2;
            break;
        case ByteCode::MOD:
            i1 %= i2;
            break;
        case ByteCode::AND:
            i1 &= i2;
            break;
        case ByteCode::OR:
            i1 |= i2;
            break;
        case ByteCode::XOR:
            i1 ^= i2;
            break;
        case ByteCode::LAND:
            i1 = i1 && i2;
            break;
        case ByteCode::LOR:
            i1 = i1 || i2;
            break;
        case ByteCode::EQ:
            i1 = i1 == i2;
            break;
        case ByteCode::NE:
            i1 = i1 != i2;
            break;
        case ByteCode::GT:
            i1 = i1 > i2;
            break;
        case ByteCode::GE:
            i1 = i1 >= i2;
            break;
        case ByteCode::LT:
            i1 = i1 < i2;
            break;
        case ByteCode::LE:
            i1 = i1 <= i2;
            break;
        case ByteCode::R_SHIFT:
            i1 = i1 >> i2;
            break;
        case ByteCode::L_SHIFT:
            i1 = i1 << i2;
            break;
    }
    push_int(st->stack, ByteCode::INT, i1);
}

/*==========================================
 * 二項演算子
 *------------------------------------------
 */
static
void op_2(ScriptState *st, ByteCode op)
{
    // pop_val has unfortunate implications here
    script_data d2 = st->stack->stack_datav.back();
    st->stack->stack_datav.pop_back();
    get_val(st, &d2);
    script_data d1 = st->stack->stack_datav.back();
    st->stack->stack_datav.pop_back();
    get_val(st, &d1);

    if (isstr(d1) && isstr(d2))
    {
        // ss => op_2str
        op_2str(st, op, d1.u.str, d2.u.str);
        if (d1.type == ByteCode::STR)
            d1.u.str.delete_();
        if (d2.type == ByteCode::STR)
            d2.u.str.delete_();
    }
    else if (!(isstr(d1) || isstr(d2)))
    {
        // ii => op_2num
        op_2num(st, op, d1.u.numi, d2.u.numi);
    }
    else
    {
        // si,is => error
        PRINTF("script: op_2: int&str, str&int not allow.\n");
        push_int(st->stack, ByteCode::INT, 0);
    }
}

/*==========================================
 * 単項演算子
 *------------------------------------------
 */
static
void op_1num(ScriptState *st, ByteCode op)
{
    int i1;
    i1 = pop_val(st);
    switch (op)
    {
        case ByteCode::NEG:
            i1 = -i1;
            break;
        case ByteCode::NOT:
            i1 = ~i1;
            break;
        case ByteCode::LNOT:
            i1 = !i1;
            break;
    }
    push_int(st->stack, ByteCode::INT, i1);
}

/*==========================================
 * 関数の実行
 *------------------------------------------
 */
void run_func(ScriptState *st)
{
    size_t end_sp = st->stack->stack_datav.size();
    size_t start_sp = end_sp - 1;
    while (st->stack->stack_datav[start_sp].type != ByteCode::ARG)
    {
        start_sp--;
        if (start_sp == 0)
        {
            if (battle_config.error_log)
                PRINTF("function not found\n");
            st->state = ScriptEndState::END;
            return;
        }
    }
    // the func is before the arg
    start_sp--;
    st->start = start_sp;
    st->end = end_sp;

    size_t func = st->stack->stack_datav[st->start].u.numi;
    if (st->stack->stack_datav[st->start].type != ByteCode::FUNC_REF)
    {
        PRINTF("run_func: not function and command! \n");
        st->state = ScriptEndState::END;
        return;
    }

    if (DEBUG_RUN && battle_config.etc_log)
    {
        PRINTF("run_func : %s\n",
                builtin_functions[func].name);
        PRINTF("stack dump :");
        for (script_data& d : st->stack->stack_datav)
        {
            switch (d.type)
            {
                case ByteCode::INT:
                    PRINTF(" int(%d)", d.u.numi);
                    break;
                case ByteCode::RETINFO:
                    PRINTF(" retinfo(%p)", static_cast<const void *>(d.u.script));
                    break;
                case ByteCode::PARAM_:
                    PRINTF(" param(%d)", d.u.reg.sp());
                    break;
                case ByteCode::VARIABLE:
                    PRINTF(" name(%s)[%d]", variable_names.outtern(d.u.reg.base()), d.u.reg.index());
                    break;
                case ByteCode::ARG:
                    PRINTF(" arg");
                    break;
                case ByteCode::POS:
                    PRINTF(" pos(%d)", d.u.numi);
                    break;
                case ByteCode::STR:
                    PRINTF(" str(%s)", d.u.str);
                    break;
                case ByteCode::CONSTSTR:
                    PRINTF(" cstr(%s)", d.u.str);
                    break;
                case ByteCode::FUNC_REF:
                    PRINTF(" func(%s)", builtin_functions[d.u.numi].name);
                    break;
                default:
                    PRINTF(" %d,%d", d.type, d.u.numi);
            }
        }
        PRINTF("\n");
    }
    builtin_functions[func].func(st);

    pop_stack(st->stack, start_sp, end_sp);

    if (st->state == ScriptEndState::RETFUNC)
    {
        // ユーザー定義関数からの復帰
        int olddefsp = st->defsp;

        pop_stack(st->stack, st->defsp, start_sp); // 復帰に邪魔なスタック削除
        if (st->defsp < 4
            || st->stack->stack_datav[st->defsp - 1].type != ByteCode::RETINFO)
        {
            PRINTF("script:run_func (return) return without callfunc or callsub!\n");
            st->state = ScriptEndState::END;
            return;
        }
        assert (olddefsp == st->defsp); // pretty sure it hasn't changed yet
        st->scriptp.code = conv_script(st, &st->stack->stack_datav[olddefsp - 1]);   // スクリプトを復元
        st->scriptp.pos = conv_num(st, &st->stack->stack_datav[olddefsp - 2]);   // スクリプト位置の復元
        st->defsp = conv_num(st, &st->stack->stack_datav[olddefsp - 3]); // 基準スタックポインタを復元
        // Number of arguments.
        int i = conv_num(st, &st->stack->stack_datav[olddefsp - 4]); // 引数の数所得
        assert (i == 0);

        pop_stack(st->stack, olddefsp - 4 - i, olddefsp);  // 要らなくなったスタック(引数と復帰用データ)削除

        st->state = ScriptEndState::GOTO;
    }
}

// pretend it's external so this can be called in the debugger
void dump_script(const ScriptBuffer *script);
void dump_script(const ScriptBuffer *script)
{
    ScriptPointer scriptp(script, 0);
    while (scriptp.pos < reinterpret_cast<const std::vector<ByteCode> *>(script)->size())
    {
        PRINTF("%6zu: ", scriptp.pos);
        switch (ByteCode c = get_com(&scriptp))
        {
            case ByteCode::EOL:
                PRINTF("EOL\n"); // extra newline between functions
                break;
            case ByteCode::INT:
                // synthesized!
                PRINTF("INT %d", get_num(&scriptp));
                break;

            case ByteCode::POS:
            case ByteCode::VARIABLE:
            case ByteCode::FUNC_REF:
            case ByteCode::PARAM_:
            {
                int arg = 0;
                arg |= static_cast<uint8_t>(scriptp.pop()) << 0;
                arg |= static_cast<uint8_t>(scriptp.pop()) << 8;
                arg |= static_cast<uint8_t>(scriptp.pop()) << 16;
                switch(c)
                {
                case ByteCode::POS:
                    PRINTF("POS %d", arg);
                    break;
                case ByteCode::VARIABLE:
                    PRINTF("VARIABLE %s", variable_names.outtern(arg));
                    break;
                case ByteCode::FUNC_REF:
                    PRINTF("FUNC_REF %s", builtin_functions[arg].name);
                    break;
                case ByteCode::PARAM_:
                    PRINTF("PARAM SP::#%d (sorry)", arg);
                    break;
                }
            }
                break;
            case ByteCode::ARG:
                PRINTF("ARG");
                break;
            case ByteCode::STR:
                PRINTF("STR \"%s\"", scriptp.pops());
                break;
            case ByteCode::FUNC_:
                PRINTF("FUNC_");
                break;

            case ByteCode::ADD:
                PRINTF("ADD");
                break;
            case ByteCode::SUB:
                PRINTF("SUB");
                break;
            case ByteCode::MUL:
                PRINTF("MUL");
                break;
            case ByteCode::DIV:
                PRINTF("DIV");
                break;
            case ByteCode::MOD:
                PRINTF("MOD");
                break;
            case ByteCode::EQ:
                PRINTF("EQ");
                break;
            case ByteCode::NE:
                PRINTF("NE");
                break;
            case ByteCode::GT:
                PRINTF("GT");
                break;
            case ByteCode::GE:
                PRINTF("GE");
                break;
            case ByteCode::LT:
                PRINTF("LT");
                break;
            case ByteCode::LE:
                PRINTF("LE");
                break;
            case ByteCode::AND:
                PRINTF("AND");
                break;
            case ByteCode::OR:
                PRINTF("OR");
                break;
            case ByteCode::XOR:
                PRINTF("XOR");
                break;
            case ByteCode::LAND:
                PRINTF("LAND");
                break;
            case ByteCode::LOR:
                PRINTF("LOR");
                break;
            case ByteCode::R_SHIFT:
                PRINTF("R_SHIFT");
                break;
            case ByteCode::L_SHIFT:
                PRINTF("L_SHIFT");
                break;
            case ByteCode::NEG:
                PRINTF("NEG");
                break;
            case ByteCode::NOT:
                PRINTF("NOT");
                break;
            case ByteCode::LNOT:
                PRINTF("LNOT");
                break;

            case ByteCode::NOP:
                PRINTF("NOP");
                break;

            default:
                PRINTF("??? %d", c);
                break;
        }
        PRINTF("\n");
    }
}

/*==========================================
 * スクリプトの実行メイン部分
 *------------------------------------------
 */
static
void run_script_main(ScriptState *st, const ScriptBuffer *rootscript)
{
    int cmdcount = script_config.check_cmdcount;
    int gotocount = script_config.check_gotocount;
    struct script_stack *stack = st->stack;

    st->defsp = stack->stack_datav.size();

    int rerun_pos = st->scriptp.pos;
    st->state = ScriptEndState::ZERO;
    while (st->state == ScriptEndState::ZERO)
    {
        switch (ByteCode c = get_com(&st->scriptp))
        {
            case ByteCode::EOL:
                if (stack->stack_datav.size() != st->defsp)
                {
                    if (battle_config.error_log)
                        PRINTF("stack.sp (%zu) != default (%d)\n",
                                stack->stack_datav.size(),
                                st->defsp);
                    stack->stack_datav.resize(st->defsp);
                }
                rerun_pos = st->scriptp.pos;
                break;
            case ByteCode::INT:
                // synthesized!
                push_int(stack, ByteCode::INT, get_num(&st->scriptp));
                break;

            case ByteCode::POS:
            case ByteCode::VARIABLE:
            case ByteCode::FUNC_REF:
            case ByteCode::PARAM_:
                // Note that these 3 have *very* different meanings,
                // despite being encoded similarly.
            {
                int arg = 0;
                arg |= static_cast<uint8_t>(st->scriptp.pop()) << 0;
                arg |= static_cast<uint8_t>(st->scriptp.pop()) << 8;
                arg |= static_cast<uint8_t>(st->scriptp.pop()) << 16;
                switch(c)
                {
                case ByteCode::POS:
                    push_int(stack, ByteCode::POS, arg);
                    break;
                case ByteCode::VARIABLE:
                    push_reg(stack, ByteCode::VARIABLE, SIR::from(arg));
                    break;
                case ByteCode::FUNC_REF:
                    push_int(stack, ByteCode::FUNC_REF, arg);
                    break;
                case ByteCode::PARAM_:
                    SP arg_sp = static_cast<SP>(arg);
                    push_reg(stack, ByteCode::PARAM_, SIR::from(arg_sp));
                    break;
                }
            }
                break;
            case ByteCode::ARG:
                push_int(stack, ByteCode::ARG, 0);
                break;
            case ByteCode::STR:
                push_str(stack, ByteCode::CONSTSTR, dumb_string::fake(st->scriptp.pops()));
                break;
            case ByteCode::FUNC_:
                run_func(st);
                if (st->state == ScriptEndState::GOTO)
                {
                    rerun_pos = st->scriptp.pos;
                    st->state = ScriptEndState::ZERO;
                    if (gotocount > 0 && (--gotocount) <= 0)
                    {
                        PRINTF("run_script: infinity loop !\n");
                        st->state = ScriptEndState::END;
                    }
                }
                break;

            case ByteCode::ADD:
                op_add(st);
                break;

            case ByteCode::SUB:
            case ByteCode::MUL:
            case ByteCode::DIV:
            case ByteCode::MOD:
            case ByteCode::EQ:
            case ByteCode::NE:
            case ByteCode::GT:
            case ByteCode::GE:
            case ByteCode::LT:
            case ByteCode::LE:
            case ByteCode::AND:
            case ByteCode::OR:
            case ByteCode::XOR:
            case ByteCode::LAND:
            case ByteCode::LOR:
            case ByteCode::R_SHIFT:
            case ByteCode::L_SHIFT:
                op_2(st, c);
                break;

            case ByteCode::NEG:
            case ByteCode::NOT:
            case ByteCode::LNOT:
                op_1num(st, c);
                break;

            case ByteCode::NOP:
                st->state = ScriptEndState::END;
                break;

            default:
                if (battle_config.error_log)
                    PRINTF("unknown command : %d @ %zu\n",
                            c, st->scriptp.pos);
                st->state = ScriptEndState::END;
                break;
        }
        if (cmdcount > 0 && (--cmdcount) <= 0)
        {
            PRINTF("run_script: infinity loop !\n");
            st->state = ScriptEndState::END;
        }
    }
    switch (st->state)
    {
        case ScriptEndState::STOP:
            break;
        case ScriptEndState::END:
        {
            dumb_ptr<map_session_data> sd = map_id2sd(st->rid);
            st->scriptp.code = nullptr;
            st->scriptp.pos = -1;
            if (sd && sd->npc_id == st->oid)
                npc_event_dequeue(sd);
        }
            break;
        case ScriptEndState::RERUNLINE:
            st->scriptp.pos = rerun_pos;
            break;
    }

    if (st->state != ScriptEndState::END)
    {
        // 再開するためにスタック情報を保存
        dumb_ptr<map_session_data> sd = map_id2sd(st->rid);
        if (sd)
        {
            sd->npc_stackbuf = stack->stack_datav;
            sd->npc_script = st->scriptp.code;
            // sd->npc_pos is set later ... ???
            sd->npc_scriptroot = rootscript;
        }
    }
}

/*==========================================
 * スクリプトの実行
 *------------------------------------------
 */
int run_script(ScriptPointer sp, int rid, int oid)
{
    return run_script_l(sp, rid, oid, 0, NULL);
}

int run_script_l(ScriptPointer sp, int rid, int oid,
        int args_nr, argrec_t *args)
{
    struct script_stack stack;
    ScriptState st;
    dumb_ptr<map_session_data> sd = map_id2sd(rid);
    const ScriptBuffer *rootscript = sp.code;
    int i;
    if (sp.code == NULL || sp.pos >> 24)
        return -1;

    if (sd && !sd->npc_stackbuf.empty() && sd->npc_scriptroot == rootscript)
    {
        // 前回のスタックを復帰
        sp.code = sd->npc_script;
        stack.stack_datav = std::move(sd->npc_stackbuf);
    }
    st.stack = &stack;
    st.scriptp = sp;
    st.rid = rid;
    st.oid = oid;
    for (i = 0; i < args_nr; i++)
    {
        if (args[i].name.back() == '$')
            pc_setregstr(sd, SIR::from(variable_names.intern(args[i].name)), args[i].v.s);
        else
            pc_setreg(sd, SIR::from(variable_names.intern(args[i].name)), args[i].v.i);
    }
    run_script_main(&st, rootscript);

    stack.stack_datav.clear();
    return st.scriptp.pos;
}

/*==========================================
 * マップ変数の変更
 *------------------------------------------
 */
void mapreg_setreg(SIR reg, int val)
{
    mapreg_db.put(reg, val);

    mapreg_dirty = 1;
}

/*==========================================
 * 文字列型マップ変数の変更
 *------------------------------------------
 */
void mapreg_setregstr(SIR reg, XString str)
{
    if (!str)
        mapregstr_db.erase(reg);
    else
        mapregstr_db.insert(reg, str);

    mapreg_dirty = 1;
}

/*==========================================
 * 永続的マップ変数の読み込み
 *------------------------------------------
 */
static
void script_load_mapreg(void)
{
    io::ReadFile in(mapreg_txt);

    if (!in.is_open())
        return;

    FString line;
    while (in.getline(line))
    {
        XString buf1, buf2;
        int index = 0;
        if (extract(line,
                    record<'\t'>(
                        record<','>(&buf1, &index),
                        &buf2))
            || extract(line,
                    record<'\t'>(
                        record<','>(&buf1),
                        &buf2)))
        {
            int s = variable_names.intern(buf1);
            SIR key = SIR::from(s, index);
            if (buf1.back() == '$')
            {
                mapregstr_db.insert(key, buf2);
            }
            else
            {
                int v;
                if (!extract(buf2, &v))
                    goto borken;
                mapreg_db.put(key, v);
            }
        }
        else
        {
        borken:
            PRINTF("%s: %s broken data !\n", mapreg_txt, FString(buf1));
            continue;
        }
    }
    mapreg_dirty = 0;
}

/*==========================================
 * 永続的マップ変数の書き込み
 *------------------------------------------
 */
static
void script_save_mapreg_intsub(SIR key, int data, io::WriteFile& fp)
{
    int num = key.base(), i = key.index();
    ZString name = variable_names.outtern(num);
    if (name[1] != '@')
    {
        if (i == 0)
            FPRINTF(fp, "%s\t%d\n", name, data);
        else
            FPRINTF(fp, "%s,%d\t%d\n", name, i, data);
    }
}

static
void script_save_mapreg_strsub(SIR key, ZString data, io::WriteFile& fp)
{
    int num = key.base(), i = key.index();
    ZString name = variable_names.outtern(num);
    if (name[1] != '@')
    {
        if (i == 0)
            FPRINTF(fp, "%s\t%s\n", name, data);
        else
            FPRINTF(fp, "%s,%d\t%s\n", name, i, data);
    }
}

static
void script_save_mapreg(void)
{
    io::WriteLock fp(mapreg_txt);
    if (!fp.is_open())
        return;
    for (auto& pair : mapreg_db)
        script_save_mapreg_intsub(pair.first, pair.second, fp);
    for (auto& pair : mapregstr_db)
        script_save_mapreg_strsub(pair.first, pair.second, fp);
    mapreg_dirty = 0;
}

static
void script_autosave_mapreg(TimerData *, tick_t)
{
    if (mapreg_dirty)
        script_save_mapreg();
}

void do_final_script(void)
{
    if (mapreg_dirty >= 0)
        script_save_mapreg();

    mapreg_db.clear();
    mapregstr_db.clear();
    scriptlabel_db.clear();
    userfunc_db.clear();

    str_datam.clear();
}

/*==========================================
 * 初期化
 *------------------------------------------
 */
void do_init_script(void)
{
    script_load_mapreg();

    Timer(gettick() + MAPREG_AUTOSAVE_INTERVAL,
            script_autosave_mapreg,
            MAPREG_AUTOSAVE_INTERVAL
    ).detach();
}

#define BUILTIN(func, args) \
{builtin_##func, {#func}, {args}}

BuiltinFunction builtin_functions[] =
{
    BUILTIN(mes, "s"),
    BUILTIN(next, ""),
    BUILTIN(close, ""),
    BUILTIN(close2, ""),
    BUILTIN(menu, "sL*"),
    BUILTIN(goto, "L"),
    BUILTIN(callsub, "L"),
    BUILTIN(callfunc, "F"),
    BUILTIN(return, ""),
    BUILTIN(input, "N"),
    BUILTIN(warp, "Mxy"),
    BUILTIN(isat, "Mxy"),
    BUILTIN(areawarp, "MxyxyMxy"),
    BUILTIN(setlook, "ii"),
    BUILTIN(set, "Ne"),
    BUILTIN(setarray, "Ne*"),
    BUILTIN(cleararray, "Nei"),
    BUILTIN(getarraysize, "N"),
    BUILTIN(getelementofarray, "Ni"),
    BUILTIN(if, "iF*"),
    BUILTIN(getitem, "Ii**"),
    BUILTIN(makeitem, "IiMxy"),
    BUILTIN(delitem, "Ii"),
    BUILTIN(heal, "ii"),
    BUILTIN(itemheal, "ii"),
    BUILTIN(percentheal, "ii"),
    BUILTIN(rand, "i*"),
    BUILTIN(pow, "ii"),
    BUILTIN(countitem, "I"),
    BUILTIN(checkweight, "Ii"),
    BUILTIN(readparam, "i*"),
    BUILTIN(getcharid, "i*"),
    BUILTIN(strcharinfo, "i"),
    BUILTIN(getequipid, "i"),
    BUILTIN(getequipname, "i"),
    BUILTIN(statusup2, "ii"),
    BUILTIN(bonus, "ii"),
    BUILTIN(bonus2, "iii"),
    BUILTIN(skill, "ii*"),
    BUILTIN(setskill, "ii"),
    BUILTIN(getskilllv, "i"),
    BUILTIN(getgmlevel, ""),
    BUILTIN(end, ""),
    BUILTIN(getopt2, ""),
    BUILTIN(setopt2, "i"),
    BUILTIN(savepoint, "Mxy"),
    BUILTIN(gettimetick, "i"),
    BUILTIN(gettime, "i"),
    BUILTIN(openstorage, "*"),
    BUILTIN(monster, "Mxysmi*"),
    BUILTIN(areamonster, "Mxyxysmi*"),
    BUILTIN(killmonster, "ME"),
    BUILTIN(killmonsterall, "M"),
    BUILTIN(donpcevent, "E"),
    BUILTIN(addtimer, "tE"),
    BUILTIN(initnpctimer, ""),
    BUILTIN(stopnpctimer, ""),
    BUILTIN(startnpctimer, "*"),
    BUILTIN(setnpctimer, "i"),
    BUILTIN(getnpctimer, "i"),
    BUILTIN(announce, "si"),
    BUILTIN(mapannounce, "Msi"),
    BUILTIN(getusers, "i"),
    BUILTIN(getmapusers, "M"),
    BUILTIN(getareausers, "Mxyxy*"),
    BUILTIN(getareadropitem, "Mxyxyi*"),
    BUILTIN(enablenpc, "s"),
    BUILTIN(disablenpc, "s"),
    BUILTIN(sc_start, "iTi*"),
    BUILTIN(sc_end, "i"),
    BUILTIN(sc_check, "i"),
    BUILTIN(debugmes, "s"),
    BUILTIN(resetstatus, ""),
    BUILTIN(changesex, ""),
    BUILTIN(attachrid, "i"),
    BUILTIN(detachrid, ""),
    BUILTIN(isloggedin, "i"),
    BUILTIN(setmapflag, "Mi"),
    BUILTIN(removemapflag, "Mi"),
    BUILTIN(getmapflag, "Mi"),
    BUILTIN(pvpon, "M"),
    BUILTIN(pvpoff, "M"),
    BUILTIN(emotion, "i"),
    BUILTIN(marriage, "P"),
    BUILTIN(divorce, ""),
    BUILTIN(getitemname, "I"),
    BUILTIN(getspellinvocation, "s"),
    BUILTIN(getpartnerid2, ""),
    BUILTIN(getexp, "ii"),
    BUILTIN(getinventorylist, ""),
    BUILTIN(getactivatedpoolskilllist, ""),
    BUILTIN(getunactivatedpoolskilllist, ""),
    BUILTIN(poolskill, "i"),
    BUILTIN(unpoolskill, "i"),
    BUILTIN(misceffect, "i*"),
    BUILTIN(specialeffect, "i"),
    BUILTIN(specialeffect2, "i"),
    BUILTIN(nude, ""),
    BUILTIN(mapwarp, "MMxy"),
    BUILTIN(cmdothernpc, "ss"),
    BUILTIN(gmcommand, "s"),
    BUILTIN(npcwarp, "xys"),
    BUILTIN(message, "Ps"),
    BUILTIN(npctalk, "s"),
    BUILTIN(mobcount, "ME"),
    BUILTIN(getlook, "i"),
    BUILTIN(getsavepoint, "i"),
    BUILTIN(areatimer, "MxyxytE"),
    BUILTIN(isin, "Mxyxy"),
    BUILTIN(shop, "s"),
    BUILTIN(isdead, ""),
    BUILTIN(unequipbyid, "i"),
    BUILTIN(fakenpcname, "ssi"),
    BUILTIN(getx, ""),
    BUILTIN(gety, ""),
    BUILTIN(getmap, ""),
    BUILTIN(mapexit, ""),
    {nullptr, ZString(), ZString()},
};
