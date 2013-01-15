#include "script.hpp"

#include <sys/time.h>

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <fstream>

#include "../common/db.hpp"
#include "../common/extract.hpp"
#include "../common/lock.hpp"
#include "../common/mt_rand.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "chat.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "skill.hpp"
#include "storage.hpp"

//#define DEBUG_FUNCIN
//#define DEBUG_DISP
//#define DEBUG_RUN

#define SCRIPT_BLOCK_SIZE 256
enum
{ LABEL_NEXTLINE = 1, LABEL_START };
static
ScriptCode *script_buf;
static
int script_pos, script_size;

static
char *str_buf;
static
int str_pos, str_size;
static
struct str_data_t
{
    ScriptCode type;
    int str;
    int backpatch;
    int label;
    void(*func)(ScriptState *);
    int val;
    int next;
}   *str_data;
static
int str_num = LABEL_START, str_data_size;
static
int str_hash[16];

static
struct dbt *mapreg_db = NULL;
static
struct dbt *mapregstr_db = NULL;
static
int mapreg_dirty = -1;
char mapreg_txt[256] = "save/mapreg.txt";
#define MAPREG_AUTOSAVE_INTERVAL        (10*1000)

static
struct dbt *scriptlabel_db = NULL;
static
struct dbt *userfunc_db = NULL;

struct dbt *script_get_label_db(void)
{
    return scriptlabel_db;
}

struct dbt *script_get_userfunc_db(void)
{
    if (!userfunc_db)
        userfunc_db = strdb_init(50);
    return userfunc_db;
}

static
const char *pos[11] =
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
    int warn_func_no_comma;
    int warn_cmd_no_comma;
    int warn_func_mismatch_paramnum;
    int warn_cmd_mismatch_paramnum;
    int check_cmdcount;
    int check_gotocount;
} script_config;
static
int parse_cmd_if = 0;
static
int parse_cmd;

/*==========================================
 * ローカルプロトタイプ宣言 (必要な物のみ)
 *------------------------------------------
 */
static
const char *parse_subexpr(const char *, int);

static
void run_func(ScriptState *st);

static
void mapreg_setreg(int num, int val);
static
void mapreg_setregstr(int num, const char *str);

struct BuiltinFunction
{
    void(*func)(ScriptState *);
    const char *name;
    const char *arg;
};
// defined later
extern BuiltinFunction builtin_functions[];


enum class ScriptCode : uint8_t
{
    // tyoes and specials
    NOP, POS, INT, PARAM, FUNC, STR, CONSTSTR, ARG,
    NAME, EOL, RETINFO,

    // unary and binary operators
    LOR, LAND, LE, LT, GE, GT, EQ, NE,
    XOR, OR, AND, ADD, SUB, MUL, DIV, MOD, NEG, LNOT,
    NOT, R_SHIFT, L_SHIFT
};

/*==========================================
 * 文字列のハッシュを計算
 *------------------------------------------
 */
static
int calc_hash(const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    int h = 0;
    while (*p)
    {
        h = (h << 1) + (h >> 3) + (h >> 5) + (h >> 8);
        h += *p++;
    }
    return h & 15;
}

/*==========================================
 * str_dataの中に名前があるか検索する
 *------------------------------------------
 */
// 既存のであれば番号、無ければ-1
static
int search_str(const char *p)
{
    int i;
    i = str_hash[calc_hash(p)];
    while (i)
    {
        if (strcmp(str_buf + str_data[i].str, p) == 0)
        {
            return i;
        }
        i = str_data[i].next;
    }
    return -1;
}

/*==========================================
 * str_dataに名前を登録
 *------------------------------------------
 */
// 既存のであれば番号、無ければ登録して新規番号
static
int add_str(const char *p)
{
    int i;
    char *lowcase;

    lowcase = strdup(p);
    for (i = 0; lowcase[i]; i++)
        lowcase[i] = tolower(lowcase[i]);
    if ((i = search_str(lowcase)) >= 0)
    {
        free(lowcase);
        return i;
    }
    free(lowcase);

    i = calc_hash(p);
    if (str_hash[i] == 0)
    {
        str_hash[i] = str_num;
    }
    else
    {
        i = str_hash[i];
        for (;;)
        {
            if (strcmp(str_buf + str_data[i].str, p) == 0)
            {
                return i;
            }
            if (str_data[i].next == 0)
                break;
            i = str_data[i].next;
        }
        str_data[i].next = str_num;
    }
    if (str_num >= str_data_size)
    {
        str_data_size += 128;
        RECREATE(str_data, struct str_data_t, str_data_size);
        memset(str_data + (str_data_size - 128), '\0', 128);
    }
    while (str_pos + strlen(p) + 1 >= str_size)
    {
        str_size += 256;
        str_buf = (char *) realloc(str_buf, str_size);
        memset(str_buf + (str_size - 256), '\0', 256);
    }
    strcpy(str_buf + str_pos, p);
    str_data[str_num].type = ScriptCode::NOP;
    str_data[str_num].str = str_pos;
    str_data[str_num].next = 0;
    str_data[str_num].func = NULL;
    str_data[str_num].backpatch = -1;
    str_data[str_num].label = -1;
    str_pos += strlen(p) + 1;
    return str_num++;
}

/*==========================================
 * スクリプトバッファサイズの確認と拡張
 *------------------------------------------
 */
static
void check_script_buf(int size)
{
    if (script_pos + size >= script_size)
    {
        script_size += SCRIPT_BLOCK_SIZE;
        script_buf = (ScriptCode *) realloc(script_buf, script_size);
        memset(script_buf + script_size - SCRIPT_BLOCK_SIZE, '\0',
                SCRIPT_BLOCK_SIZE);
    }
}

/*==========================================
 * スクリプトバッファに１バイト書き込む
 *------------------------------------------
 */
static
void add_scriptc(ScriptCode a)
{
    check_script_buf(1);
    script_buf[script_pos++] = a;
}

/*==========================================
 * スクリプトバッファにデータタイプを書き込む
 *------------------------------------------
 */
static
void add_scriptb(uint8_t a)
{
    add_scriptc(static_cast<ScriptCode>(a));
}

/*==========================================
 * スクリプトバッファに整数を書き込む
 *------------------------------------------
 */
static
void add_scripti(unsigned int a)
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
static
void add_scriptl(int l)
{
    int backpatch = str_data[l].backpatch;

    switch (str_data[l].type)
    {
        case ScriptCode::POS:
            add_scriptc(ScriptCode::POS);
            add_scriptb({uint8_t(str_data[l].label)});
            add_scriptb({uint8_t(str_data[l].label >> 8)});
            add_scriptb({uint8_t(str_data[l].label >> 16)});
            break;
        case ScriptCode::NOP:
            // ラベルの可能性があるのでbackpatch用データ埋め込み
            add_scriptc(ScriptCode::NAME);
            str_data[l].backpatch = script_pos;
            add_scriptb({uint8_t(backpatch)});
            add_scriptb({uint8_t(backpatch >> 8)});
            add_scriptb({uint8_t(backpatch >> 16)});
            break;
        case ScriptCode::INT:
            add_scripti(str_data[l].val);
            break;
        default:
            // もう他の用途と確定してるので数字をそのまま
            add_scriptc(ScriptCode::NAME);
            add_scriptb({uint8_t(l)});
            add_scriptb({uint8_t(l >> 8)});
            add_scriptb({uint8_t(l >> 16)});
            break;
    }
}

/*==========================================
 * ラベルを解決する
 *------------------------------------------
 */
static
void set_label(int l, int pos_)
{
    int i, next;

    str_data[l].type = ScriptCode::POS;
    str_data[l].label = pos_;
    for (i = str_data[l].backpatch; i >= 0 && i != 0x00ffffff;)
    {
        next = (*(int *)(script_buf + i)) & 0x00ffffff;
        script_buf[i - 1] = ScriptCode::POS;
        script_buf[i] = static_cast<ScriptCode>(pos_);
        script_buf[i + 1] = static_cast<ScriptCode>(pos_ >> 8);
        script_buf[i + 2] = static_cast<ScriptCode>(pos_ >> 16);
        i = next;
    }
}

/*==========================================
 * スペース/コメント読み飛ばし
 *------------------------------------------
 */
static
const char *skip_space(const char *p)
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
const char *skip_word(const char *p)
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

static
const char *startptr;
static
int startline;

/*==========================================
 * エラーメッセージ出力
 *------------------------------------------
 */
static
void disp_error_message(const char *mes, const char *pos_)
{
    int line;
    const char *p;

    for (line = startline, p = startptr; p && *p; line++)
    {
        const char *linestart = p;
        char *lineend = const_cast<char *>(strchr(p, '\n'));
        // always initialized, but clang is not smart enough
        char c = '\0';
        if (lineend)
        {
            c = *lineend;
            *lineend = 0;
        }
        if (lineend == NULL || pos_ < lineend)
        {
            PRINTF("%s line %d : ", mes, line);
            for (int i = 0;
                 (linestart[i] != '\r') && (linestart[i] != '\n')
                 && linestart[i]; i++)
            {
                if (linestart + i != pos_)
                    PRINTF("%c", linestart[i]);
                else
                    PRINTF("\'%c\'", linestart[i]);
            }
            PRINTF("\a\n");
            if (lineend)
                *lineend = c;
            return;
        }
        *lineend = c;
        p = lineend + 1;
    }
}

/*==========================================
 * 項の解析
 *------------------------------------------
 */
static
const char *parse_simpleexpr(const char *p)
{
    int i;
    p = skip_space(p);

#ifdef DEBUG_FUNCIN
    if (battle_config.etc_log)
        PRINTF("parse_simpleexpr %s\n", p);
#endif
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
        i = strtoul(p, &np, 0);
        add_scripti(i);
        p = np;
    }
    else if (*p == '"')
    {
        add_scriptc(ScriptCode::STR);
        p++;
        while (*p && *p != '"')
        {
            if (p[-1] <= 0x7e && *p == '\\')
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
        int l;
        // label , register , function etc
        if (skip_word(p) == p)
        {
            disp_error_message("unexpected character", p);
            exit(1);
        }
        char *p2 = const_cast<char *>(skip_word(p));
        char c = *p2;
        *p2 = 0;                // 名前をadd_strする
        l = add_str(p);

        parse_cmd = l;          // warn_*_mismatch_paramnumのために必要
        if (l == search_str("if")) // warn_cmd_no_commaのために必要
            parse_cmd_if++;
/*
                // 廃止予定のl14/l15,およびプレフィックスｌの警告
                if (    strcmp(str_buf+str_data[l].str,"l14")==0 ||
                        strcmp(str_buf+str_data[l].str,"l15")==0 ){
                        disp_error_message("l14 and l15 is DEPRECATED. use @menu instead of l15.",p);
                }else if (str_buf[str_data[l].str]=='l'){
                        disp_error_message("prefix 'l' is DEPRECATED. use prefix '@' instead.",p2);
                }
*/
        *p2 = c;
        p = p2;

        if (str_data[l].type != ScriptCode::FUNC && c == '[')
        {
            // array(name[i] => getelementofarray(name,i) )
            add_scriptl(search_str("getelementofarray"));
            add_scriptc(ScriptCode::ARG);
            add_scriptl(l);
            p = parse_subexpr(p + 1, -1);
            p = skip_space(p);
            if ((*p++) != ']')
            {
                disp_error_message("unmatch ']'", p);
                exit(1);
            }
            add_scriptc(ScriptCode::FUNC);
        }
        else
            add_scriptl(l);

    }

#ifdef DEBUG_FUNCIN
    if (battle_config.etc_log)
        PRINTF("parse_simpleexpr end %s\n", p);
#endif
    return p;
}

/*==========================================
 * 式の解析
 *------------------------------------------
 */
const char *parse_subexpr(const char *p, int limit)
{
    ScriptCode op;
    int opl, len;

#ifdef DEBUG_FUNCIN
    if (battle_config.etc_log)
        PRINTF("parse_subexpr %s\n", p);
#endif
    p = skip_space(p);

    if (*p == '-')
    {
        const char *tmpp = skip_space(p + 1);
        if (*tmpp == ';' || *tmpp == ',')
        {
            add_scriptl(LABEL_NEXTLINE);
            p++;
            return p;
        }
    }
    const char *tmpp = p;
    if ((op = ScriptCode::NEG, *p == '-') || (op = ScriptCode::LNOT, *p == '!')
        || (op = ScriptCode::NOT, *p == '~'))
    {
        p = parse_subexpr(p + 1, 100);
        add_scriptc(op);
    }
    else
        p = parse_simpleexpr(p);
    p = skip_space(p);
    while (((op = ScriptCode::ADD, opl = 6, len = 1, *p == '+') ||
            (op = ScriptCode::SUB, opl = 6, len = 1, *p == '-') ||
            (op = ScriptCode::MUL, opl = 7, len = 1, *p == '*') ||
            (op = ScriptCode::DIV, opl = 7, len = 1, *p == '/') ||
            (op = ScriptCode::MOD, opl = 7, len = 1, *p == '%') ||
            (op = ScriptCode::FUNC, opl = 8, len = 1, *p == '(') ||
            (op = ScriptCode::LAND, opl = 1, len = 2, *p == '&' && p[1] == '&') ||
            (op = ScriptCode::AND, opl = 5, len = 1, *p == '&') ||
            (op = ScriptCode::LOR, opl = 0, len = 2, *p == '|' && p[1] == '|') ||
            (op = ScriptCode::OR, opl = 4, len = 1, *p == '|') ||
            (op = ScriptCode::XOR, opl = 3, len = 1, *p == '^') ||
            (op = ScriptCode::EQ, opl = 2, len = 2, *p == '=' && p[1] == '=') ||
            (op = ScriptCode::NE, opl = 2, len = 2, *p == '!' && p[1] == '=') ||
            (op = ScriptCode::R_SHIFT, opl = 5, len = 2, *p == '>' && p[1] == '>') ||
            (op = ScriptCode::GE, opl = 2, len = 2, *p == '>' && p[1] == '=') ||
            (op = ScriptCode::GT, opl = 2, len = 1, *p == '>') ||
            (op = ScriptCode::L_SHIFT, opl = 5, len = 2, *p == '<' && p[1] == '<') ||
            (op = ScriptCode::LE, opl = 2, len = 2, *p == '<' && p[1] == '=') ||
            (op = ScriptCode::LT, opl = 2, len = 1, *p == '<')) && opl > limit)
    {
        p += len;
        if (op == ScriptCode::FUNC)
        {
            int i = 0, func = parse_cmd;
            const char *plist[128];

            if (str_data[func].type != ScriptCode::FUNC)
            {
                disp_error_message("expect function", tmpp);
                exit(0);
            }

            add_scriptc(ScriptCode::ARG);
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
            if (*(p++) != ')')
            {
                disp_error_message("func request '(' ')'", p);
                exit(1);
            }

            if (str_data[func].type == ScriptCode::FUNC
                && script_config.warn_func_mismatch_paramnum)
            {
                const char *arg = builtin_functions[str_data[func].val].arg;
                int j = 0;
                for (j = 0; arg[j]; j++)
                    if (arg[j] == '*')
                        break;
                if ((arg[j] == 0 && i != j) || (arg[j] == '*' && i < j))
                {
                    disp_error_message("illegal number of parameters",
                                        plist[(i < j) ? i : j]);
                }
            }
        }
        else // not op == ScriptCode::FUNC
        {
            p = parse_subexpr(p, opl);
        }
        add_scriptc(op);
        p = skip_space(p);
    }
#ifdef DEBUG_FUNCIN
    if (battle_config.etc_log)
        PRINTF("parse_subexpr end %s\n", p);
#endif
    return p;                   /* return first untreated operator */
}

/*==========================================
 * 式の評価
 *------------------------------------------
 */
static
const char *parse_expr(const char *p)
{
#ifdef DEBUG_FUNCIN
    if (battle_config.etc_log)
        PRINTF("parse_expr %s\n", p);
#endif
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
#ifdef DEBUG_FUNCIN
    if (battle_config.etc_log)
        PRINTF("parse_expr end %s\n", p);
#endif
    return p;
}

/*==========================================
 * 行の解析
 *------------------------------------------
 */
static
const char *parse_line(const char *p)
{
    int i = 0, cmd;
    const char *plist[128];

    p = skip_space(p);
    if (*p == ';')
        return p;

    parse_cmd_if = 0;           // warn_cmd_no_commaのために必要

    // 最初は関数名
    const char *p2 = p;
    p = parse_simpleexpr(p);
    p = skip_space(p);

    cmd = parse_cmd;
    if (str_data[cmd].type != ScriptCode::FUNC)
    {
        disp_error_message("expect command", p2);
//      exit(0);
    }

    add_scriptc(ScriptCode::ARG);
    while (p && *p && *p != ';' && i < 128)
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
    if (!p || *(p++) != ';')
    {
        disp_error_message("need ';'", p);
        exit(1);
    }
    add_scriptc(ScriptCode::FUNC);

    if (str_data[cmd].type == ScriptCode::FUNC
        && script_config.warn_cmd_mismatch_paramnum)
    {
        const char *arg = builtin_functions[str_data[cmd].val].arg;
        int j = 0;
        for (j = 0; arg[j]; j++)
            if (arg[j] == '*')
                break;
        if ((arg[j] == 0 && i != j) || (arg[j] == '*' && i < j))
        {
            disp_error_message("illegal number of parameters",
                                plist[(i < j) ? i : j]);
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
    int i, n;
    for (i = 0; builtin_functions[i].func; i++)
    {
        n = add_str(builtin_functions[i].name);
        str_data[n].type = ScriptCode::FUNC;
        str_data[n].val = i;
        str_data[n].func = builtin_functions[i].func;
    }
}

/*==========================================
 * 定数データベースの読み込み
 *------------------------------------------
 */
static
void read_constdb(void)
{
    std::ifstream in("db/const.txt");
    if (!in.is_open())
    {
        PRINTF("can't read db/const.txt\n");
        return;
    }

    std::string line;
    while (std::getline(in, line))
    {
        if (line[0] == '/' && line[1] == '/')
            continue;

        char *name = nullptr;
        int val;
        int type = 0; // if not provided
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
        if (sscanf(line.c_str(), "%m[A-Za-z0-9_] %i %i", &name, &val, &type) < 2)
            {
                free(name);
                continue;
            }
#pragma GCC diagnostic pop
        for (char *p = name; *p; ++p)
            *p = tolower(*p);
        int n = add_str(name);
        free(name);
        str_data[n].type = type ? ScriptCode::PARAM : ScriptCode::INT;
        str_data[n].val = val;
    }
}

/*==========================================
 * スクリプトの解析
 *------------------------------------------
 */
const ScriptCode *parse_script(const char *src, int line)
{
    const char *p;
    int i;
    static int first = 1;

    if (first)
    {
        add_builtin_functions();
        read_constdb();
    }
    first = 0;
    script_buf = (ScriptCode *) calloc(SCRIPT_BLOCK_SIZE, 1);
    script_pos = 0;
    script_size = SCRIPT_BLOCK_SIZE;
    str_data[LABEL_NEXTLINE].type = ScriptCode::NOP;
    str_data[LABEL_NEXTLINE].backpatch = -1;
    str_data[LABEL_NEXTLINE].label = -1;
    for (i = LABEL_START; i < str_num; i++)
    {
        if (str_data[i].type == ScriptCode::POS || str_data[i].type == ScriptCode::NAME)
        {
            str_data[i].type = ScriptCode::NOP;
            str_data[i].backpatch = -1;
            str_data[i].label = -1;
        }
    }

    // 外部用label dbの初期化
    if (scriptlabel_db != NULL)
        strdb_final(scriptlabel_db, NULL);
    scriptlabel_db = strdb_init(50);

    // for error message
    startptr = src;
    startline = line;

    p = src;
    p = skip_space(p);
    if (*p != '{')
    {
        disp_error_message("not found '{'", p);
        return NULL;
    }
    for (p++; p && *p && *p != '}';)
    {
        p = skip_space(p);
        // labelだけ特殊処理
        if (*skip_space(skip_word(p)) == ':')
        {
            char *tmpp = const_cast<char *>(skip_word(p));
            char c = *tmpp;
            *tmpp = '\0';
            int l = add_str(p);
            if (str_data[l].label != -1)
            {
                *tmpp = c;
                disp_error_message("dup label ", p);
                exit(1);
            }
            set_label(l, script_pos);
            strdb_insert(scriptlabel_db, p, script_pos);   // 外部用label db登録
            *tmpp = c;
            p = tmpp + 1;
            continue;
        }

        // 他は全部一緒くた
        p = parse_line(p);
        p = skip_space(p);
        add_scriptc(ScriptCode::EOL);

        set_label(LABEL_NEXTLINE, script_pos);
        str_data[LABEL_NEXTLINE].type = ScriptCode::NOP;
        str_data[LABEL_NEXTLINE].backpatch = -1;
        str_data[LABEL_NEXTLINE].label = -1;
    }

    add_scriptc(ScriptCode::NOP);

    script_size = script_pos;
    script_buf = (ScriptCode *) realloc(script_buf, script_pos + 1);

    // 未解決のラベルを解決
    for (i = LABEL_START; i < str_num; i++)
    {
        if (str_data[i].type == ScriptCode::NOP)
        {
            int j, next;
            str_data[i].type = ScriptCode::NAME;
            str_data[i].label = i;
            for (j = str_data[i].backpatch; j >= 0 && j != 0x00ffffff;)
            {
                next = (*(int *)(script_buf + j)) & 0x00ffffff;
                script_buf[j] = static_cast<ScriptCode>(i);
                script_buf[j + 1] = static_cast<ScriptCode>(i >> 8);
                script_buf[j + 2] = static_cast<ScriptCode>(i >> 16);
                j = next;
            }
        }
    }

#ifdef DEBUG_DISP
    for (i = 0; i < script_pos; i++)
    {
        if ((i & 15) == 0)
            PRINTF("%04x : ", i);
        PRINTF("%02x ", script_buf[i]);
        if ((i & 15) == 15)
            PRINTF("\n");
    }
    PRINTF("\n");
#endif

    return script_buf;
}

//
// 実行系
//
enum
{ STOP = 1, END, RERUNLINE, GOTO, RETFUNC };

/*==========================================
 * ridからsdへの解決
 *------------------------------------------
 */
static
struct map_session_data *script_rid2sd(ScriptState *st)
{
    struct map_session_data *sd = map_id2sd(st->rid);
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
    struct map_session_data *sd = NULL;
    if (data->type == ScriptCode::NAME)
    {
        char *name = str_buf + str_data[data->u.num & 0x00ffffff].str;
        char prefix = *name;
        char postfix = name[strlen(name) - 1];

        if (prefix != '$')
        {
            if ((sd = script_rid2sd(st)) == NULL)
                PRINTF("get_val error name?:%s\n", name);
        }
        if (postfix == '$')
        {

            data->type = ScriptCode::CONSTSTR;
            if (prefix == '@' || prefix == 'l')
            {
                if (sd)
                    data->u.str = pc_readregstr(sd, data->u.num);
            }
            else if (prefix == '$')
            {
                data->u.str =
                    (char *) numdb_search(mapregstr_db, data->u.num);
            }
            else
            {
                PRINTF("script: get_val: illegal scope string variable.\n");
                data->u.str = "!!ERROR!!";
            }
            if (data->u.str == NULL)
                data->u.str = "";

        }
        else
        {

            data->type = ScriptCode::INT;
            if (str_data[data->u.num & 0x00ffffff].type == ScriptCode::INT)
            {
                // unreachable
                data->u.num = str_data[data->u.num & 0x00ffffff].val;
            }
            else if (str_data[data->u.num & 0x00ffffff].type == ScriptCode::PARAM)
            {
                if (sd)
                    data->u.num =
                        pc_readparam(sd,
                                      SP(str_data[data->u.num & 0x00ffffff].val));
            }
            else if (prefix == '@' || prefix == 'l')
            {
                if (sd)
                    data->u.num = pc_readreg(sd, data->u.num);
            }
            else if (prefix == '$')
            {
                data->u.num = (int) numdb_search(mapreg_db, data->u.num);
            }
            else if (prefix == '#')
            {
                if (name[1] == '#')
                {
                    if (sd)
                        data->u.num = pc_readaccountreg2(sd, name);
                }
                else
                {
                    if (sd)
                        data->u.num = pc_readaccountreg(sd, name);
                }
            }
            else
            {
                if (sd)
                    data->u.num = pc_readglobalreg(sd, name);
            }
        }
    }
}

/*==========================================
 * 変数の読み取り2
 *------------------------------------------
 */
static
struct script_data get_val2(ScriptState *st, int num)
{
    struct script_data dat;
    dat.type = ScriptCode::NAME;
    dat.u.num = num;
    get_val(st, &dat);
    return dat;
}

/*==========================================
 * 変数設定用
 *------------------------------------------
 */
static
void set_reg(struct map_session_data *sd, int num, const char *name, struct script_data vd)
{
    char prefix = *name;
    char postfix = name[strlen(name) - 1];

    if (postfix == '$')
    {
        const char *str = vd.u.str;
        if (prefix == '@' || prefix == 'l')
        {
            pc_setregstr(sd, num, str);
        }
        else if (prefix == '$')
        {
            mapreg_setregstr(num, str);
        }
        else
        {
            PRINTF("script: set_reg: illegal scope string variable !");
        }
    }
    else
    {
        // 数値
        int val = vd.u.num;
        if (str_data[num & 0x00ffffff].type == ScriptCode::PARAM)
        {
            pc_setparam(sd, SP(str_data[num & 0x00ffffff].val), val);
        }
        else if (prefix == '@' || prefix == 'l')
        {
            pc_setreg(sd, num, val);
        }
        else if (prefix == '$')
        {
            mapreg_setreg(num, val);
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
void set_reg(struct map_session_data *sd, int num, const char *name, int id)
{
    struct script_data vd;
    vd.u.num = id;
    set_reg(sd, num, name, vd);
}

static
void set_reg(struct map_session_data *sd, int num, const char *name, const char *zd)
{
    struct script_data vd;
    vd.u.str = zd;
    set_reg(sd, num, name, vd);
}

/*==========================================
 * 文字列への変換
 *------------------------------------------
 */
static
const char *conv_str(ScriptState *st, struct script_data *data)
{
    get_val(st, data);
    if (data->type == ScriptCode::INT)
    {
        char *buf;
        buf = (char *) calloc(16, 1);
        sprintf(buf, "%d", data->u.num);
        data->type = ScriptCode::STR;
        data->u.str = buf;
    }
#if 1
    else if (data->type == ScriptCode::NAME)
    {
        // テンポラリ。本来無いはず
        data->type = ScriptCode::CONSTSTR;
        data->u.str = str_buf + str_data[data->u.num].str;
    }
#endif
    return data->u.str;
}

/*==========================================
 * 数値へ変換
 *------------------------------------------
 */
static
int conv_num(ScriptState *st, struct script_data *data)
{
    get_val(st, data);
    if (data->type == ScriptCode::STR || data->type == ScriptCode::CONSTSTR)
    {
        const char *p = data->u.str;
        data->u.num = atoi(p);
        if (data->type == ScriptCode::STR)
            free(const_cast<char *>(p));
        data->type = ScriptCode::INT;
    }
    return data->u.num;
}

/*==========================================
 * スタックへ数値をプッシュ
 *------------------------------------------
 */
static
void push_val(struct script_stack *stack, ScriptCode type, int val)
{
    if (stack->sp >= stack->sp_max)
    {
        stack->sp_max += 64;
        stack->stack_data = (struct script_data *)
            realloc(stack->stack_data, sizeof(stack->stack_data[0]) *
                                        stack->sp_max);
        memset(stack->stack_data + (stack->sp_max - 64), 0,
                64 * sizeof(*(stack->stack_data)));
    }
//  if(battle_config.etc_log)
//      PRINTF("push (%d,%d)-> %d\n",type,val,stack->sp);
    stack->stack_data[stack->sp].type = type;
    stack->stack_data[stack->sp].u.num = val;
    stack->sp++;
}

/*==========================================
 * スタックへ文字列をプッシュ
 *------------------------------------------
 */
static
void push_str(struct script_stack *stack, ScriptCode type, const char *str)
{
    if (stack->sp >= stack->sp_max)
    {
        stack->sp_max += 64;
        stack->stack_data = (struct script_data *)
            realloc(stack->stack_data, sizeof(stack->stack_data[0]) *
                                        stack->sp_max);
        memset(stack->stack_data + (stack->sp_max - 64), '\0',
                64 * sizeof(*(stack->stack_data)));
    }
//  if(battle_config.etc_log)
//      PRINTF("push (%d,%x)-> %d\n",type,str,stack->sp);
    stack->stack_data[stack->sp].type = type;
    stack->stack_data[stack->sp].u.str = str;
    stack->sp++;
}

/*==========================================
 * スタックへ複製をプッシュ
 *------------------------------------------
 */
static
void push_copy(struct script_stack *stack, int pos_)
{
    switch (stack->stack_data[pos_].type)
    {
        case ScriptCode::CONSTSTR:
            push_str(stack, ScriptCode::CONSTSTR, stack->stack_data[pos_].u.str);
            break;
        case ScriptCode::STR:
            push_str(stack, ScriptCode::STR, strdup(stack->stack_data[pos_].u.str));
            break;
        default:
            push_val(stack, stack->stack_data[pos_].type,
                      stack->stack_data[pos_].u.num);
            break;
    }
}

/*==========================================
 * スタックからポップ
 *------------------------------------------
 */
static
void pop_stack(struct script_stack *stack, int start, int end)
{
    int i;
    for (i = start; i < end; i++)
    {
        if (stack->stack_data[i].type == ScriptCode::STR)
        {
            free(const_cast<char *>(stack->stack_data[i].u.str));
        }
    }
    if (stack->sp > end)
    {
        memmove(&stack->stack_data[start], &stack->stack_data[end],
                 sizeof(stack->stack_data[0]) * (stack->sp - end));
    }
    stack->sp -= end - start;
}

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
    conv_str(st, &(st->stack->stack_data[st->start + 2]));
    clif_scriptmes(script_rid2sd(st), st->oid,
                    st->stack->stack_data[st->start + 2].u.str);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_goto(ScriptState *st)
{
    if (st->stack->stack_data[st->start + 2].type != ScriptCode::POS)
    {
        PRINTF("script: goto: not label !\n");
        st->state = END;
        return;
    }

    st->pos = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    st->state = GOTO;
}

/*==========================================
 * ユーザー定義関数の呼び出し
 *------------------------------------------
 */
static
void builtin_callfunc(ScriptState *st)
{
    const ScriptCode *scr;
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));

    // note: strdb_search returns a void *; but ScriptCode is really const
    if ((scr = static_cast<const ScriptCode *>(strdb_search(script_get_userfunc_db(), str))))
    {
        int j = 0;
#if 0
        for (int i = st->start + 3; i < st->end; i++, j++)
            push_copy(st->stack, i);
#endif

        push_val(st->stack, ScriptCode::INT, j); // 引数の数をプッシュ
        push_val(st->stack, ScriptCode::INT, st->defsp); // 現在の基準スタックポインタをプッシュ
        push_val(st->stack, ScriptCode::INT, (int) st->script);  // 現在のスクリプトをプッシュ
        push_val(st->stack, ScriptCode::RETINFO, st->pos);   // 現在のスクリプト位置をプッシュ

        st->pos = 0;
        st->script = scr;
        st->defsp = st->start + 4 + j;
        st->state = GOTO;
    }
    else
    {
        PRINTF("script:callfunc: function not found! [%s]\n", str);
        st->state = END;
    }
}

/*==========================================
 * サブルーティンの呼び出し
 *------------------------------------------
 */
static
void builtin_callsub(ScriptState *st)
{
    int pos_ = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    int j = 0;
#if 0
    for (int i = st->start + 3; i < st->end; i++, j++)
        push_copy(st->stack, i);
#endif

    push_val(st->stack, ScriptCode::INT, j); // 引数の数をプッシュ
    push_val(st->stack, ScriptCode::INT, st->defsp); // 現在の基準スタックポインタをプッシュ
    push_val(st->stack, ScriptCode::INT, (int) st->script);  // 現在のスクリプトをプッシュ
    push_val(st->stack, ScriptCode::RETINFO, st->pos);   // 現在のスクリプト位置をプッシュ

    st->pos = pos_;
    st->defsp = st->start + 4 + j;
    st->state = GOTO;
}

/*==========================================
 * サブルーチン/ユーザー定義関数の終了
 *------------------------------------------
 */
static
void builtin_return(ScriptState *st)
{
#if 0
    if (st->end > st->start + 2)
    {                           // 戻り値有り
        push_copy(st->stack, st->start + 2);
    }
#endif
    st->state = RETFUNC;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_next(ScriptState *st)
{
    st->state = STOP;
    clif_scriptnext(script_rid2sd(st), st->oid);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_close(ScriptState *st)
{
    st->state = END;
    clif_scriptclose(script_rid2sd(st), st->oid);
}

static
void builtin_close2(ScriptState *st)
{
    st->state = STOP;
    clif_scriptclose(script_rid2sd(st), st->oid);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_menu(ScriptState *st)
{
    char *buf;
    int i, len = 0;            // [fate] len is the total # of bytes we need to transmit the string choices
    int menu_choices = 0;
    int finished_menu_items = 0;   // [fate] set to 1 after we hit the first empty string

    struct map_session_data *sd;

    sd = script_rid2sd(st);

    // We don't need to do this iteration if the player cancels, strictly speaking.
    for (i = st->start + 2; i < st->end; i += 2)
    {
        int choice_len;
        conv_str(st, &(st->stack->stack_data[i]));
        choice_len = strlen(st->stack->stack_data[i].u.str);
        len += choice_len + 1;  // count # of bytes we'll need for packet.  Only used if menu_or_input = 0.

        if (choice_len && !finished_menu_items)
            ++menu_choices;
        else
            finished_menu_items = 1;
    }

    if (sd->state.menu_or_input == 0)
    {
        st->state = RERUNLINE;
        sd->state.menu_or_input = 1;

        buf = (char *) calloc(len + 1, 1);
        buf[0] = 0;
        for (i = st->start + 2; menu_choices > 0; i += 2, --menu_choices)
        {
            strcat(buf, st->stack->stack_data[i].u.str);
            strcat(buf, ":");
        }
        clif_scriptmenu(script_rid2sd(st), st->oid, buf);
        free(buf);
    }
    else if (sd->npc_menu == 0xff)
    {                           // cansel
        sd->state.menu_or_input = 0;
        st->state = END;
    }
    else
    {                           // goto動作
        // ragemu互換のため
        pc_setreg(sd, add_str("l15"), sd->npc_menu);
        pc_setreg(sd, add_str("@menu"), sd->npc_menu);
        sd->state.menu_or_input = 0;
        if (sd->npc_menu > 0 && sd->npc_menu <= menu_choices)
        {
            if (st->stack->
                stack_data[st->start + sd->npc_menu * 2 + 1].type != ScriptCode::POS)
            {
                st->state = END;
                return;
            }
            st->pos =
                conv_num(st,
                          &(st->
                            stack->stack_data[st->start + sd->npc_menu * 2 +
                                              1]));
            st->state = GOTO;
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
    int range, min, max;

    if (st->end > st->start + 3)
    {
        min = conv_num(st, &(st->stack->stack_data[st->start + 2]));
        max = conv_num(st, &(st->stack->stack_data[st->start + 3]));
        if (max < min)
        {
            int tmp;
            tmp = min;
            min = max;
            max = tmp;
        }
        range = max - min + 1;
        push_val(st->stack, ScriptCode::INT, (range <= 0 ? 0 : MRAND(range)) + min);
    }
    else
    {
        range = conv_num(st, &(st->stack->stack_data[st->start + 2]));
        push_val(st->stack, ScriptCode::INT, range <= 0 ? 0 : MRAND(range));
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

    a = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    b = conv_num(st, &(st->stack->stack_data[st->start + 3]));

    push_val(st->stack, ScriptCode::INT, (int) pow(a * 0.001, b));

}

/*==========================================
 * Check whether the PC is at the specified location
 *------------------------------------------
 */
static
void builtin_isat(ScriptState *st)
{
    int x, y;
    struct map_session_data *sd = script_rid2sd(st);

    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    x = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    y = conv_num(st, &(st->stack->stack_data[st->start + 4]));

    if (!sd)
        return;

    push_val(st->stack, ScriptCode::INT,
              (x == sd->bl.x)
              && (y == sd->bl.y) && (!strcmp(str, map[sd->bl.m].name)));

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_warp(ScriptState *st)
{
    int x, y;
    struct map_session_data *sd = script_rid2sd(st);

    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    x = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    y = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    if (strcmp(str, "Random") == 0)
        pc_randomwarp(sd, 3);
    else if (strcmp(str, "SavePoint") == 0)
    {
        if (map[sd->bl.m].flag.noreturn)    // 蝶禁止
            return;

        pc_setpos(sd, sd->status.save_point.map,
                   sd->status.save_point.x, sd->status.save_point.y, 3);
    }
    else if (strcmp(str, "Save") == 0)
    {
        if (map[sd->bl.m].flag.noreturn)    // 蝶禁止
            return;

        pc_setpos(sd, sd->status.save_point.map,
                   sd->status.save_point.x, sd->status.save_point.y, 3);
    }
    else
        pc_setpos(sd, str, x, y, 0);
}

/*==========================================
 * エリア指定ワープ
 *------------------------------------------
 */
static
void builtin_areawarp_sub(struct block_list *bl, const char *mapname, int x, int y)
{
    if (strcmp(mapname, "Random") == 0)
        pc_randomwarp((struct map_session_data *) bl, 3);
    else
        pc_setpos((struct map_session_data *) bl, mapname, x, y, 0);
}

static
void builtin_areawarp(ScriptState *st)
{
    int x, y, m;
    int x0, y0, x1, y1;

    const char *mapname = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    x0 = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    y0 = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    x1 = conv_num(st, &(st->stack->stack_data[st->start + 5]));
    y1 = conv_num(st, &(st->stack->stack_data[st->start + 6]));
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 7]));
    x = conv_num(st, &(st->stack->stack_data[st->start + 8]));
    y = conv_num(st, &(st->stack->stack_data[st->start + 9]));

    if ((m = map_mapname2mapid(mapname)) < 0)
        return;

    map_foreachinarea(std::bind(builtin_areawarp_sub, ph::_1, str, x, y),
                       m, x0, y0, x1, y1, BL_PC);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_heal(ScriptState *st)
{
    int hp, sp;

    hp = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    sp = conv_num(st, &(st->stack->stack_data[st->start + 3]));
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

    hp = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    sp = conv_num(st, &(st->stack->stack_data[st->start + 3]));
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

    hp = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    sp = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    pc_percentheal(script_rid2sd(st), hp, sp);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_input(ScriptState *st)
{
    struct map_session_data *sd = NULL;
    int num =
        (st->end >
         st->start + 2) ? st->stack->stack_data[st->start + 2].u.num : 0;
    const char *name =
        (st->end >
         st->start + 2) ? str_buf + str_data[num & 0x00ffffff].str : "";
//  char prefix=*name;
    char postfix = name[strlen(name) - 1];

    sd = script_rid2sd(st);
    if (sd->state.menu_or_input)
    {
        sd->state.menu_or_input = 0;
        if (postfix == '$')
        {
            // 文字列
            if (st->end > st->start + 2)
            {                   // 引数1個
                set_reg(sd, num, name, sd->npc_str);
            }
            else
            {
                PRINTF("builtin_input: string discarded !!\n");
            }
        }
        else
        {

            //commented by Lupus (check Value Number Input fix in clif.c)
            //** Fix by fritz :X keeps people from abusing old input bugs
            if (sd->npc_amount < 0) //** If input amount is less then 0
            {
                clif_tradecancelled(sd);   // added "Deal has been cancelled" message by Valaris
                builtin_close(st); //** close
            }

            // 数値
            if (st->end > st->start + 2)
            {                   // 引数1個
                set_reg(sd, num, name, sd->npc_amount);
            }
            else
            {
                // ragemu互換のため
                pc_setreg(sd, add_str("l14"), sd->npc_amount);
            }
        }
    }
    else
    {
        st->state = RERUNLINE;
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

    sel = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    if (!sel)
        return;

    // 関数名をコピー
    push_copy(st->stack, st->start + 3);
    // 間に引数マーカを入れて
    push_val(st->stack, ScriptCode::ARG, 0);
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
    struct map_session_data *sd = NULL;
    int num = st->stack->stack_data[st->start + 2].u.num;
    char *name = str_buf + str_data[num & 0x00ffffff].str;
    char prefix = *name;
    char postfix = name[strlen(name) - 1];

    if (st->stack->stack_data[st->start + 2].type != ScriptCode::NAME)
    {
        PRINTF("script: builtin_set: not name\n");
        return;
    }

    if (prefix != '$')
        sd = script_rid2sd(st);

    if (postfix == '$')
    {
        // 文字列
        const char *str = conv_str(st, &(st->stack->stack_data[st->start + 3]));
        set_reg(sd, num, name, str);
    }
    else
    {
        // 数値
        int val = conv_num(st, &(st->stack->stack_data[st->start + 3]));
        set_reg(sd, num, name, val);
    }

}

/*==========================================
 * 配列変数設定
 *------------------------------------------
 */
static
void builtin_setarray(ScriptState *st)
{
    struct map_session_data *sd = NULL;
    int num = st->stack->stack_data[st->start + 2].u.num;
    char *name = str_buf + str_data[num & 0x00ffffff].str;
    char prefix = *name;
    char postfix = name[strlen(name) - 1];
    int i, j;

    if (prefix != '$' && prefix != '@')
    {
        PRINTF("builtin_setarray: illegal scope !\n");
        return;
    }
    if (prefix != '$')
        sd = script_rid2sd(st);

    for (j = 0, i = st->start + 3; i < st->end && j < 128; i++, j++)
    {
        if (postfix == '$')
            set_reg(sd, num + (j << 24), name, conv_str(st, &(st->stack->stack_data[i])));
        else
            set_reg(sd, num + (j << 24), name, conv_num(st, &(st->stack->stack_data[i])));
    }
}

/*==========================================
 * 配列変数クリア
 *------------------------------------------
 */
static
void builtin_cleararray(ScriptState *st)
{
    struct map_session_data *sd = NULL;
    int num = st->stack->stack_data[st->start + 2].u.num;
    char *name = str_buf + str_data[num & 0x00ffffff].str;
    char prefix = *name;
    char postfix = name[strlen(name) - 1];
    int sz = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    int i;

    if (prefix != '$' && prefix != '@')
    {
        PRINTF("builtin_cleararray: illegal scope !\n");
        return;
    }
    if (prefix != '$')
        sd = script_rid2sd(st);

    if (postfix == '$')
        for (i = 0; i < sz; i++)
            set_reg(sd, num + (i << 24), name, conv_str(st, &(st->stack->stack_data[st->start + 3])));
    else
        for (i = 0; i < sz; i++)
            set_reg(sd, num + (i << 24), name, conv_num(st, &(st->stack->stack_data[st->start + 3])));

}

/*==========================================
 * 配列変数のサイズ所得
 *------------------------------------------
 */
static
int getarraysize(ScriptState *st, int num, int postfix)
{
    int i = (num >> 24), c = i;
    for (; i < 128; i++)
    {
        struct script_data vd = get_val2(st, num + (i << 24));
        if (postfix == '$' ? bool(*vd.u.str) : bool(vd.u.num))
            c = i;
    }
    return c + 1;
}

static
void builtin_getarraysize(ScriptState *st)
{
    int num = st->stack->stack_data[st->start + 2].u.num;
    char *name = str_buf + str_data[num & 0x00ffffff].str;
    char prefix = *name;
    char postfix = name[strlen(name) - 1];

    if (prefix != '$' && prefix != '@')
    {
        PRINTF("builtin_copyarray: illegal scope !\n");
        return;
    }

    push_val(st->stack, ScriptCode::INT, getarraysize(st, num, postfix));
}

/*==========================================
 * 指定要素を表す値(キー)を所得する
 *------------------------------------------
 */
static
void builtin_getelementofarray(ScriptState *st)
{
    if (st->stack->stack_data[st->start + 2].type == ScriptCode::NAME)
    {
        int i = conv_num(st, &(st->stack->stack_data[st->start + 3]));
        if (i > 127 || i < 0)
        {
            PRINTF("script: getelementofarray (operator[]): param2 illegal number %d\n",
                 i);
            push_val(st->stack, ScriptCode::INT, 0);
        }
        else
        {
            push_val(st->stack, ScriptCode::NAME,
                      (i << 24) | st->stack->stack_data[st->start + 2].u.num);
        }
    }
    else
    {
        PRINTF("script: getelementofarray (operator[]): param1 not name !\n");
        push_val(st->stack, ScriptCode::INT, 0);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_setlook(ScriptState *st)
{
    LOOK type = LOOK(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    int val = conv_num(st, &(st->stack->stack_data[st->start + 3]));

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
    struct map_session_data *sd;

    struct script_data *data;

    sd = script_rid2sd(st);

    data = &(st->stack->stack_data[st->start + 2]);
    get_val(st, data);
    if (data->type == ScriptCode::STR || data->type == ScriptCode::CONSTSTR)
    {
        const char *name = conv_str(st, data);
        struct item_data *item_data;
        if ((item_data = itemdb_searchname(name)) != NULL)
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
    push_val(st->stack, ScriptCode::INT, count);

}

/*==========================================
 * 重量チェック
 *------------------------------------------
 */
static
void builtin_checkweight(ScriptState *st)
{
    int nameid = 0, amount;
    struct map_session_data *sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &(st->stack->stack_data[st->start + 2]);
    get_val(st, data);
    if (data->type == ScriptCode::STR || data->type == ScriptCode::CONSTSTR)
    {
        const char *name = conv_str(st, data);
        struct item_data *item_data = itemdb_searchname(name);
        if (item_data)
            nameid = item_data->nameid;
    }
    else
        nameid = conv_num(st, data);

    amount = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    if (amount <= 0 || nameid < 500)
    {                           //if get wrong item ID or amount<=0, don't count weight of non existing items
        push_val(st->stack, ScriptCode::INT, 0);
    }

    sd = script_rid2sd(st);
    if (itemdb_weight(nameid) * amount + sd->weight > sd->max_weight)
    {
        push_val(st->stack, ScriptCode::INT, 0);
    }
    else
    {
        push_val(st->stack, ScriptCode::INT, 1);
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
    struct item item_tmp;
    struct map_session_data *sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &(st->stack->stack_data[st->start + 2]);
    get_val(st, data);
    if (data->type == ScriptCode::STR || data->type == ScriptCode::CONSTSTR)
    {
        const char *name = conv_str(st, data);
        struct item_data *item_data = itemdb_searchname(name);
        nameid = 727;           //Default to iten
        if (item_data != NULL)
            nameid = item_data->nameid;
    }
    else
        nameid = conv_num(st, data);

    if ((amount =
         conv_num(st, &(st->stack->stack_data[st->start + 3]))) <= 0)
    {
        return;               //return if amount <=0, skip the useles iteration
    }
    bool flag1 = false;
    //Violet Box, Blue Box, etc - random item pick
    if (nameid < 0)
    {                           // ランダム
        nameid = itemdb_searchrandomid(-nameid);
        flag1 = 1;
    }

    if (nameid > 0)
    {
        memset(&item_tmp, 0, sizeof(item_tmp));
        item_tmp.nameid = nameid;
        if (!flag1)
            item_tmp.identify = 1;
        else
            item_tmp.identify = !itemdb_isequip3(nameid);
        if (st->end > st->start + 5)    //アイテムを指定したIDに渡す
            sd = map_id2sd(conv_num(st, &(st->stack->stack_data[st->start + 5])));
        if (sd == NULL)         //アイテムを渡す相手がいなかったらお帰り
            return;
        PickupFail flag;
        if ((flag = pc_additem(sd, &item_tmp, amount)) != PickupFail::OKAY)
        {
            clif_additem(sd, 0, 0, flag);
            map_addflooritem(&item_tmp, amount, sd->bl.m, sd->bl.x, sd->bl.y,
                              NULL, NULL, NULL, 0);
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
    int x, y, m;
    struct item item_tmp;
    struct map_session_data *sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &(st->stack->stack_data[st->start + 2]);
    get_val(st, data);
    if (data->type == ScriptCode::STR || data->type == ScriptCode::CONSTSTR)
    {
        const char *name = conv_str(st, data);
        struct item_data *item_data = itemdb_searchname(name);
        nameid = 512;           //Apple Item ID
        if (item_data)
            nameid = item_data->nameid;
    }
    else
        nameid = conv_num(st, data);

    amount = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    const char *mapname = conv_str(st, &(st->stack->stack_data[st->start + 4]));
    x = conv_num(st, &(st->stack->stack_data[st->start + 5]));
    y = conv_num(st, &(st->stack->stack_data[st->start + 6]));

    if (sd && strcmp(mapname, "this") == 0)
        m = sd->bl.m;
    else
        m = map_mapname2mapid(mapname);

    if (nameid < 0)
    {                           // ランダム
        nameid = itemdb_searchrandomid(-nameid);
        flag = 1;
    }

    if (nameid > 0)
    {
        memset(&item_tmp, 0, sizeof(item_tmp));
        item_tmp.nameid = nameid;
        if (!flag)
            item_tmp.identify = 1;
        else
            item_tmp.identify = !itemdb_isequip3(nameid);

//      clif_additem(sd,0,0,flag);
        map_addflooritem(&item_tmp, amount, m, x, y, NULL, NULL, NULL, 0);
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
    struct map_session_data *sd;
    struct script_data *data;

    sd = script_rid2sd(st);

    data = &(st->stack->stack_data[st->start + 2]);
    get_val(st, data);
    if (data->type == ScriptCode::STR || data->type == ScriptCode::CONSTSTR)
    {
        const char *name = conv_str(st, data);
        struct item_data *item_data = itemdb_searchname(name);
        //nameid=512;
        if (item_data)
            nameid = item_data->nameid;
    }
    else
        nameid = conv_num(st, data);

    amount = conv_num(st, &(st->stack->stack_data[st->start + 3]));

    if (nameid < 500 || amount <= 0)
    {                           //by Lupus. Don't run FOR if u got wrong item ID or amount<=0
        //PRINTF("wrong item ID or amount<=0 : delitem %i,\n",nameid,amount);
        return;
    }
    sd = script_rid2sd(st);

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
    struct map_session_data *sd;

    SP type = SP(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    if (st->end > st->start + 3)
        sd = map_nick2sd(conv_str(st, &(st->stack->stack_data[st->start + 3])));
    else
        sd = script_rid2sd(st);

    if (sd == NULL)
    {
        push_val(st->stack, ScriptCode::INT, -1);
        return;
    }

    push_val(st->stack, ScriptCode::INT, pc_readparam(sd, type));

}

/*==========================================
 *キャラ関係のID取得
 *------------------------------------------
 */
static
void builtin_getcharid(ScriptState *st)
{
    int num;
    struct map_session_data *sd;

    num = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    if (st->end > st->start + 3)
        sd = map_nick2sd(conv_str(st, &(st->stack->stack_data[st->start + 3])));
    else
        sd = script_rid2sd(st);
    if (sd == NULL)
    {
        push_val(st->stack, ScriptCode::INT, -1);
        return;
    }
    if (num == 0)
        push_val(st->stack, ScriptCode::INT, sd->status.char_id);
    if (num == 1)
        push_val(st->stack, ScriptCode::INT, sd->status.party_id);
    if (num == 2)
        push_val(st->stack, ScriptCode::INT, 0/*guild_id*/);
    if (num == 3)
        push_val(st->stack, ScriptCode::INT, sd->status.account_id);
}

/*==========================================
 *指定IDのPT名取得
 *------------------------------------------
 */
static
char *builtin_getpartyname_sub(int party_id)
{
    struct party *p;

    p = NULL;
    p = party_search(party_id);

    if (p != NULL)
    {
        char *buf;
        buf = (char *) calloc(24, 1);
        strcpy(buf, p->name);
        return buf;
    }

    return 0;
}

/*==========================================
 * キャラクタの名前
 *------------------------------------------
 */
static
void builtin_strcharinfo(ScriptState *st)
{
    struct map_session_data *sd;
    int num;

    sd = script_rid2sd(st);
    num = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    if (num == 0)
    {
        char *buf;
        buf = (char *) calloc(24, 1);
        strncpy(buf, sd->status.name, 23);
        push_str(st->stack, ScriptCode::STR, buf);
    }
    if (num == 1)
    {
        char *buf;
        buf = builtin_getpartyname_sub(sd->status.party_id);
        if (buf != 0)
            push_str(st->stack, ScriptCode::STR, buf);
        else
            push_str(st->stack, ScriptCode::CONSTSTR, "");
    }
    if (num == 2)
    {
        // was: guild name
        push_str(st->stack, ScriptCode::CONSTSTR, "");
    }

}

// indexed by the equip_* in db/const.txt
// TODO change to use EQUIP
static
EPOS equip[10] =
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
};

/*==========================================
 * GetEquipID(Pos);     Pos: 1-10
 *------------------------------------------
 */
static
void builtin_getequipid(ScriptState *st)
{
    int i, num;
    struct map_session_data *sd;
    struct item_data *item;

    sd = script_rid2sd(st);
    if (sd == NULL)
    {
        PRINTF("getequipid: sd == NULL\n");
        return;
    }
    num = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    i = pc_checkequip(sd, equip[num - 1]);
    if (i >= 0)
    {
        item = sd->inventory_data[i];
        if (item)
            push_val(st->stack, ScriptCode::INT, item->nameid);
        else
            push_val(st->stack, ScriptCode::INT, 0);
    }
    else
    {
        push_val(st->stack, ScriptCode::INT, -1);
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
    struct map_session_data *sd;
    struct item_data *item;
    char *buf;

    buf = (char *) calloc(64, 1);
    sd = script_rid2sd(st);
    num = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    i = pc_checkequip(sd, equip[num - 1]);
    if (i >= 0)
    {
        item = sd->inventory_data[i];
        if (item)
            sprintf(buf, "%s-[%s]", pos[num - 1], item->jname);
        else
            sprintf(buf, "%s-[%s]", pos[num - 1], pos[10]);
    }
    else
    {
        sprintf(buf, "%s-[%s]", pos[num - 1], pos[10]);
    }
    push_str(st->stack, ScriptCode::STR, buf);

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_statusup2(ScriptState *st)
{
    SP type = SP(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    int val = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    struct map_session_data *sd = script_rid2sd(st);
    pc_statusup2(sd, type, val);

}

/*==========================================
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
static
void builtin_bonus(ScriptState *st)
{
    SP type = SP(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    int val = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    struct map_session_data *sd = script_rid2sd(st);
    pc_bonus(sd, type, val);

}

/*==========================================
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
static
void builtin_bonus2(ScriptState *st)
{
    SP type = SP(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    int type2 = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    int val = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    struct map_session_data *sd = script_rid2sd(st);
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
    struct map_session_data *sd;

    SkillID id = SkillID(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    level = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    if (st->end > st->start + 4)
        flag = conv_num(st, &(st->stack->stack_data[st->start + 4]));
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
    struct map_session_data *sd;

    SkillID id = SkillID(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    level = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    sd = script_rid2sd(st);

    sd->status.skill[id].id = level ? id : SkillID();
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
    SkillID id = SkillID(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    push_val(st->stack, ScriptCode::INT, pc_checkskill(script_rid2sd(st), id));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_getgmlevel(ScriptState *st)
{
    push_val(st->stack, ScriptCode::INT, pc_isGM(script_rid2sd(st)));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_end(ScriptState *st)
{
    st->state = END;
}

/*==========================================
 * [Freeyorp] Return the current opt2
 *------------------------------------------
 */

static
void builtin_getopt2(ScriptState *st)
{
    struct map_session_data *sd;

    sd = script_rid2sd(st);

    push_val(st->stack, ScriptCode::INT, uint16_t(sd->opt2));

}

/*==========================================
 * [Freeyorp] Sets opt2
 *------------------------------------------
 */

static
void builtin_setopt2(ScriptState *st)
{
    struct map_session_data *sd;

    Opt2 new_opt2 = Opt2(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    sd = script_rid2sd(st);
    if (new_opt2 == sd->opt2)
        return;
    sd->opt2 = new_opt2;
    clif_changeoption(&sd->bl);
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

    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    x = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    y = conv_num(st, &(st->stack->stack_data[st->start + 4]));
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
    type = conv_num(st, &(st->stack->stack_data[st->start + 2]));

    switch (type)
    {
        /* Number of seconds elapsed today(0-86399, 00:00:00-23:59:59). */
        case 1:
        {
            time_t timer;
            struct tm *t;

            time(&timer);
            t = gmtime(&timer);
            push_val(st->stack, ScriptCode::INT,
                      ((t->tm_hour) * 3600 + (t->tm_min) * 60 + t->tm_sec));
            break;
        }
        /* Seconds since Unix epoch. */
        case 2:
            push_val(st->stack, ScriptCode::INT, (int) time(NULL));
            break;
        /* System tick(unsigned int, and yes, it will wrap). */
        case 0:
        default:
            push_val(st->stack, ScriptCode::INT, gettick());
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
    int type;
    time_t timer;
    struct tm *t;

    type = conv_num(st, &(st->stack->stack_data[st->start + 2]));

    time(&timer);
    t = gmtime(&timer);

    switch (type)
    {
        case 1:                //Sec(0~59)
            push_val(st->stack, ScriptCode::INT, t->tm_sec);
            break;
        case 2:                //Min(0~59)
            push_val(st->stack, ScriptCode::INT, t->tm_min);
            break;
        case 3:                //Hour(0~23)
            push_val(st->stack, ScriptCode::INT, t->tm_hour);
            break;
        case 4:                //WeekDay(0~6)
            push_val(st->stack, ScriptCode::INT, t->tm_wday);
            break;
        case 5:                //MonthDay(01~31)
            push_val(st->stack, ScriptCode::INT, t->tm_mday);
            break;
        case 6:                //Month(01~12)
            push_val(st->stack, ScriptCode::INT, t->tm_mon + 1);
            break;
        case 7:                //Year(20xx)
            push_val(st->stack, ScriptCode::INT, t->tm_year + 1900);
            break;
        default:               //(format error)
            push_val(st->stack, ScriptCode::INT, -1);
            break;
    }
}

/*==========================================
 * GetTimeStr("TimeFMT", Length);
 *------------------------------------------
 */
static
void builtin_gettimestr(ScriptState *st)
{
    char *tmpstr;
    int maxlen;
    time_t now = time(NULL);

    const char *fmtstr = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    maxlen = conv_num(st, &(st->stack->stack_data[st->start + 3]));

    tmpstr = (char *) calloc(maxlen + 1, 1);
    strftime(tmpstr, maxlen, fmtstr, gmtime(&now));
    tmpstr[maxlen] = '\0';

    push_str(st->stack, ScriptCode::STR, tmpstr);
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
    struct map_session_data *sd = script_rid2sd(st);

//  if (sync) {
    st->state = STOP;
    sd->npc_flags.storage = 1;
//  } else st->state = END;

    storage_storageopen(sd);
}

/*==========================================
 * NPCで経験値上げる
 *------------------------------------------
 */
static
void builtin_getexp(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);
    int base = 0, job = 0;

    base = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    job = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    if (base < 0 || job < 0)
        return;
    if (sd)
        pc_gainexp_reason(sd, base, job, PC_GAINEXP_REASON_SCRIPT);

}

/*==========================================
 * モンスター発生
 *------------------------------------------
 */
static
void builtin_monster(ScriptState *st)
{
    int mob_class, amount, x, y;
    const char *event = "";

    const char *mapname = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    x = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    y = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 5]));
    mob_class = conv_num(st, &(st->stack->stack_data[st->start + 6]));
    amount = conv_num(st, &(st->stack->stack_data[st->start + 7]));
    if (st->end > st->start + 8)
        event = conv_str(st, &(st->stack->stack_data[st->start + 8]));

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
    const char *event = "";

    const char *mapname = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    x0 = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    y0 = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    x1 = conv_num(st, &(st->stack->stack_data[st->start + 5]));
    y1 = conv_num(st, &(st->stack->stack_data[st->start + 6]));
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 7]));
    mob_class = conv_num(st, &(st->stack->stack_data[st->start + 8]));
    amount = conv_num(st, &(st->stack->stack_data[st->start + 9]));
    if (st->end > st->start + 10)
        event = conv_str(st, &(st->stack->stack_data[st->start + 10]));

    mob_once_spawn_area(map_id2sd(st->rid), mapname, x0, y0, x1, y1, str, mob_class,
                         amount, event);
}

/*==========================================
 * モンスター削除
 *------------------------------------------
 */
static
void builtin_killmonster_sub(struct block_list *bl, const char *event, int allflag)
{
    if (!allflag)
    {
        if (strcmp(event, ((struct mob_data *) bl)->npc_event) == 0)
            mob_delete((struct mob_data *) bl);
        return;
    }
    else if (allflag)
    {
        if (((struct mob_data *) bl)->spawndelay1 == -1
            && ((struct mob_data *) bl)->spawndelay2 == -1)
            mob_delete((struct mob_data *) bl);
        return;
    }
}

static
void builtin_killmonster(ScriptState *st)
{
    int m, allflag = 0;
    const char *mapname = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    const char *event = conv_str(st, &(st->stack->stack_data[st->start + 3]));
    if (strcmp(event, "All") == 0)
        allflag = 1;

    if ((m = map_mapname2mapid(mapname)) < 0)
        return;
    map_foreachinarea(std::bind(builtin_killmonster_sub, ph::_1, event, allflag),
                       m, 0, 0, map[m].xs, map[m].ys, BL_MOB);
}

static
void builtin_killmonsterall_sub(struct block_list *bl)
{
    mob_delete((struct mob_data *) bl);
}

static
void builtin_killmonsterall(ScriptState *st)
{
    int m;
    const char *mapname = conv_str(st, &(st->stack->stack_data[st->start + 2]));

    if ((m = map_mapname2mapid(mapname)) < 0)
        return;
    map_foreachinarea(builtin_killmonsterall_sub,
                       m, 0, 0, map[m].xs, map[m].ys, BL_MOB);
}

/*==========================================
 * NPC主体イベント実行
 *------------------------------------------
 */
static
void builtin_donpcevent(ScriptState *st)
{
    const char *event = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    npc_event_do(event);
}

/*==========================================
 * イベントタイマー追加
 *------------------------------------------
 */
static
void builtin_addtimer(ScriptState *st)
{
    int tick;
    tick = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    const char *event = conv_str(st, &(st->stack->stack_data[st->start + 3]));
    pc_addeventtimer(script_rid2sd(st), tick, event);
}

/*==========================================
 * NPCタイマー初期化
 *------------------------------------------
 */
static
void builtin_initnpctimer(ScriptState *st)
{
    struct npc_data *nd;
    if (st->end > st->start + 2)
        nd = npc_name2id(conv_str(st, &(st->stack->stack_data[st->start + 2])));
    else
        nd = (struct npc_data *) map_id2bl(st->oid);

    npc_settimerevent_tick(nd, 0);
    npc_timerevent_start(nd);
}

/*==========================================
 * NPCタイマー開始
 *------------------------------------------
 */
static
void builtin_startnpctimer(ScriptState *st)
{
    struct npc_data *nd;
    if (st->end > st->start + 2)
        nd = npc_name2id(conv_str(st, &(st->stack->stack_data[st->start + 2])));
    else
        nd = (struct npc_data *) map_id2bl(st->oid);

    npc_timerevent_start(nd);
}

/*==========================================
 * NPCタイマー停止
 *------------------------------------------
 */
static
void builtin_stopnpctimer(ScriptState *st)
{
    struct npc_data *nd;
    if (st->end > st->start + 2)
        nd = npc_name2id(conv_str(st, &(st->stack->stack_data[st->start + 2])));
    else
        nd = (struct npc_data *) map_id2bl(st->oid);

    npc_timerevent_stop(nd);
}

/*==========================================
 * NPCタイマー情報所得
 *------------------------------------------
 */
static
void builtin_getnpctimer(ScriptState *st)
{
    struct npc_data *nd;
    int type = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    int val = 0;
    if (st->end > st->start + 3)
        nd = npc_name2id(conv_str(st, &(st->stack->stack_data[st->start + 3])));
    else
        nd = (struct npc_data *) map_id2bl(st->oid);

    switch (type)
    {
        case 0:
            val = npc_gettimerevent_tick(nd);
            break;
        case 1:
            val = (nd->u.scr.nexttimer >= 0);
            break;
        case 2:
            val = nd->u.scr.timeramount;
            break;
    }
    push_val(st->stack, ScriptCode::INT, val);
}

/*==========================================
 * NPCタイマー値設定
 *------------------------------------------
 */
static
void builtin_setnpctimer(ScriptState *st)
{
    int tick;
    struct npc_data *nd;
    tick = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    if (st->end > st->start + 3)
        nd = npc_name2id(conv_str(st, &(st->stack->stack_data[st->start + 3])));
    else
        nd = (struct npc_data *) map_id2bl(st->oid);

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
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    flag = conv_num(st, &(st->stack->stack_data[st->start + 3]));

    if (flag & 0x0f)
    {
        struct block_list *bl = (flag & 0x08) ? map_id2bl(st->oid) :
            (struct block_list *) script_rid2sd(st);
        clif_GMmessage(bl, str, flag);
    }
    else
        intif_GMmessage(str, flag);
}

/*==========================================
 * 天の声アナウンス（特定マップ）
 *------------------------------------------
 */
static
void builtin_mapannounce_sub(struct block_list *bl, const char *str, int flag)
{
    clif_GMmessage(bl, str, flag | 3);
}

static
void builtin_mapannounce(ScriptState *st)
{
    int flag, m;

    const char *mapname = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 3]));
    flag = conv_num(st, &(st->stack->stack_data[st->start + 4]));

    if ((m = map_mapname2mapid(mapname)) < 0)
        return;
    map_foreachinarea(std::bind(builtin_mapannounce_sub, ph::_1, str, flag & 0x10),
            m, 0, 0, map[m].xs, map[m].ys, BL_PC);
}

/*==========================================
 * ユーザー数所得
 *------------------------------------------
 */
static
void builtin_getusers(ScriptState *st)
{
    int flag = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    struct block_list *bl = map_id2bl((flag & 0x08) ? st->oid : st->rid);
    int val = 0;
    switch (flag & 0x07)
    {
        case 0:
            val = map[bl->m].users;
            break;
        case 1:
            val = map_getusers();
            break;
    }
    push_val(st->stack, ScriptCode::INT, val);
}

/*==========================================
 * マップ指定ユーザー数所得
 *------------------------------------------
 */
static
void builtin_getmapusers(ScriptState *st)
{
    int m;
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    if ((m = map_mapname2mapid(str)) < 0)
    {
        push_val(st->stack, ScriptCode::INT, -1);
        return;
    }
    push_val(st->stack, ScriptCode::INT, map[m].users);
}

/*==========================================
 * エリア指定ユーザー数所得
 *------------------------------------------
 */
static
void builtin_getareausers_sub(struct block_list *, int *users)
{
    (*users)++;
}

static
void builtin_getareausers_living_sub(struct block_list *bl, int *users)
{
    if (!pc_isdead((struct map_session_data *)bl))
        (*users)++;
}

static
void builtin_getareausers(ScriptState *st)
{
    int m, x0, y0, x1, y1, users = 0;
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    x0 = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    y0 = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    x1 = conv_num(st, &(st->stack->stack_data[st->start + 5]));
    y1 = conv_num(st, &(st->stack->stack_data[st->start + 6]));

    int living = 0;
    if (st->end > st->start + 7)
    {
        living = conv_num(st, &(st->stack->stack_data[st->start + 7]));
    }
    if ((m = map_mapname2mapid(str)) < 0)
    {
        push_val(st->stack, ScriptCode::INT, -1);
        return;
    }
    map_foreachinarea(std::bind(living ? builtin_getareausers_living_sub: builtin_getareausers_sub, ph::_1, &users),
                       m, x0, y0, x1, y1, BL_PC);
    push_val(st->stack, ScriptCode::INT, users);
}

/*==========================================
 * エリア指定ドロップアイテム数所得
 *------------------------------------------
 */
static
void builtin_getareadropitem_sub(struct block_list *bl, int item, int *amount)
{
    struct flooritem_data *drop = (struct flooritem_data *) bl;

    if (drop->item_data.nameid == item)
        (*amount) += drop->item_data.amount;

}

static
void builtin_getareadropitem_sub_anddelete(struct block_list *bl, int item, int *amount)
{
    struct flooritem_data *drop = (struct flooritem_data *) bl;

    if (drop->item_data.nameid == item) {
        (*amount) += drop->item_data.amount;
        clif_clearflooritem(drop, 0);
        map_delobject(drop->bl.id, drop->bl.type);
    }
}

static
void builtin_getareadropitem(ScriptState *st)
{
    int m, x0, y0, x1, y1, item, amount = 0, delitems = 0;
    struct script_data *data;

    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    x0 = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    y0 = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    x1 = conv_num(st, &(st->stack->stack_data[st->start + 5]));
    y1 = conv_num(st, &(st->stack->stack_data[st->start + 6]));

    data = &(st->stack->stack_data[st->start + 7]);
    get_val(st, data);
    if (data->type == ScriptCode::STR || data->type == ScriptCode::CONSTSTR)
    {
        const char *name = conv_str(st, data);
        struct item_data *item_data = itemdb_searchname(name);
        item = 512;
        if (item_data)
            item = item_data->nameid;
    }
    else
        item = conv_num(st, data);

    if (st->end > st->start + 8)
        delitems = conv_num(st, &(st->stack->stack_data[st->start + 8]));

    if ((m = map_mapname2mapid(str)) < 0)
    {
        push_val(st->stack, ScriptCode::INT, -1);
        return;
    }
    if (delitems)
        map_foreachinarea(std::bind(builtin_getareadropitem_sub_anddelete, ph::_1, item, &amount),
                m, x0, y0, x1, y1, BL_ITEM);
    else
        map_foreachinarea(std::bind(builtin_getareadropitem_sub, ph::_1, item, &amount),
                m, x0, y0, x1, y1, BL_ITEM);

    push_val(st->stack, ScriptCode::INT, amount);
}

/*==========================================
 * NPCの有効化
 *------------------------------------------
 */
static
void builtin_enablenpc(ScriptState *st)
{
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    npc_enable(str, 1);
}

/*==========================================
 * NPCの無効化
 *------------------------------------------
 */
static
void builtin_disablenpc(ScriptState *st)
{
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    npc_enable(str, 0);
}

/*==========================================
 * 状態異常にかかる
 *------------------------------------------
 */
static
void builtin_sc_start(ScriptState *st)
{
    struct block_list *bl;
    int tick, val1;
    StatusChange type = StatusChange(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    tick = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    val1 = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    if (st->end > st->start + 5)    //指定したキャラを状態異常にする
        bl = map_id2bl(conv_num(st, &(st->stack->stack_data[st->start + 5])));
    else
        bl = map_id2bl(st->rid);
    if (bl->type == BL_PC
        && ((struct map_session_data *) bl)->state.potionpitcher_flag)
        bl = map_id2bl(((struct map_session_data *) bl)->skilltarget);
    skill_status_change_start(bl, type, val1, 0, 0, 0, tick, 0);
}

/*==========================================
 * 状態異常が直る
 *------------------------------------------
 */
static
void builtin_sc_end(ScriptState *st)
{
    struct block_list *bl;
    StatusChange type = StatusChange(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    bl = map_id2bl(st->rid);
    if (bl->type == BL_PC
        && ((struct map_session_data *) bl)->state.potionpitcher_flag)
        bl = map_id2bl(((struct map_session_data *) bl)->skilltarget);
    skill_status_change_end(bl, type, -1);
//  if(battle_config.etc_log)
//      PRINTF("sc_end : %d %d\n",st->rid,type);
}

static
void builtin_sc_check(ScriptState *st)
{
    struct block_list *bl;
    StatusChange type = StatusChange(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    bl = map_id2bl(st->rid);
    if (bl->type == BL_PC
        && ((struct map_session_data *) bl)->state.potionpitcher_flag)
        bl = map_id2bl(((struct map_session_data *) bl)->skilltarget);

    push_val(st->stack, ScriptCode::INT, skill_status_change_active(bl, type));

}

/*==========================================
 *
 *------------------------------------------
 */
static
void builtin_debugmes(ScriptState *st)
{
    conv_str(st, &(st->stack->stack_data[st->start + 2]));
    PRINTF("script debug : %d %d : %s\n", st->rid, st->oid,
            st->stack->stack_data[st->start + 2].u.str);
}

/*==========================================
 * ステータスリセット
 *------------------------------------------
 */
static
void builtin_resetstatus(ScriptState *st)
{
    struct map_session_data *sd;
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
    struct map_session_data *sd = NULL;
    sd = script_rid2sd(st);

    if (sd->status.sex == 0)
    {
        sd->status.sex = 1;
        sd->sex = 1;
    }
    else if (sd->status.sex == 1)
    {
        sd->status.sex = 0;
        sd->sex = 0;
    }
    chrif_char_ask_name(-1, sd->status.name, 5, 0, 0, 0, 0, 0, 0); // type: 5 - changesex
    chrif_save(sd);
}

/*==========================================
 * RIDのアタッチ
 *------------------------------------------
 */
static
void builtin_attachrid(ScriptState *st)
{
    st->rid = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    push_val(st->stack, ScriptCode::INT, (map_id2sd(st->rid) != NULL));
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
    push_val(st->stack, ScriptCode::INT,
              map_id2sd(conv_num(st,
                          &(st->stack->stack_data[st->start + 2]))) != NULL);
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
};

static
void builtin_setmapflag(ScriptState *st)
{
    int m, i;

    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    i = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    m = map_mapname2mapid(str);
    if (m >= 0)
    {
        switch (i)
        {
            case MF_NOMEMO:
                map[m].flag.nomemo = 1;
                break;
            case MF_NOTELEPORT:
                map[m].flag.noteleport = 1;
                break;
            case MF_NOBRANCH:
                map[m].flag.nobranch = 1;
                break;
            case MF_NOPENALTY:
                map[m].flag.nopenalty = 1;
                break;
            case MF_PVP_NOPARTY:
                map[m].flag.pvp_noparty = 1;
                break;
            case MF_NOZENYPENALTY:
                map[m].flag.nozenypenalty = 1;
                break;
            case MF_NOTRADE:
                map[m].flag.notrade = 1;
                break;
            case MF_NOWARP:
                map[m].flag.nowarp = 1;
                break;
            case MF_NOPVP:
                map[m].flag.nopvp = 1;
                break;
            case MF_NOICEWALL: // [Valaris]
                map[m].flag.noicewall = 1;
                break;
            case MF_SNOW:      // [Valaris]
                map[m].flag.snow = 1;
                break;
            case MF_FOG:       // [Valaris]
                map[m].flag.fog = 1;
                break;
            case MF_SAKURA:    // [Valaris]
                map[m].flag.sakura = 1;
                break;
            case MF_LEAVES:    // [Valaris]
                map[m].flag.leaves = 1;
                break;
            case MF_RAIN:      // [Valaris]
                map[m].flag.rain = 1;
                break;
        }
    }

}

static
void builtin_removemapflag(ScriptState *st)
{
    int m, i;

    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    i = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    m = map_mapname2mapid(str);
    if (m >= 0)
    {
        switch (i)
        {
            case MF_NOMEMO:
                map[m].flag.nomemo = 0;
                break;
            case MF_NOTELEPORT:
                map[m].flag.noteleport = 0;
                break;
            case MF_NOSAVE:
                map[m].flag.nosave = 0;
                break;
            case MF_NOBRANCH:
                map[m].flag.nobranch = 0;
                break;
            case MF_NOPENALTY:
                map[m].flag.nopenalty = 0;
                break;
            case MF_PVP_NOPARTY:
                map[m].flag.pvp_noparty = 0;
                break;
            case MF_NOZENYPENALTY:
                map[m].flag.nozenypenalty = 0;
                break;
            case MF_NOWARP:
                map[m].flag.nowarp = 0;
                break;
            case MF_NOPVP:
                map[m].flag.nopvp = 0;
                break;
            case MF_NOICEWALL: // [Valaris]
                map[m].flag.noicewall = 0;
                break;
            case MF_SNOW:      // [Valaris]
                map[m].flag.snow = 0;
                break;
            case MF_FOG:       // [Valaris]
                map[m].flag.fog = 0;
                break;
            case MF_SAKURA:    // [Valaris]
                map[m].flag.sakura = 0;
                break;
            case MF_LEAVES:    // [Valaris]
                map[m].flag.leaves = 0;
                break;
            case MF_RAIN:      // [Valaris]
                map[m].flag.rain = 0;
                break;

        }
    }

}

static
void builtin_getmapflag(ScriptState *st)
{
    int m, i, r = -1;

    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    i = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    m = map_mapname2mapid(str);
    if (m >= 0)
    {
        switch (i)
        {
            case MF_NOMEMO:
                r = map[m].flag.nomemo;
                break;
            case MF_NOTELEPORT:
                r = map[m].flag.noteleport;
                break;
            case MF_NOSAVE:
                r = map[m].flag.nosave;
                break;
            case MF_NOBRANCH:
                r = map[m].flag.nobranch;
                break;
            case MF_NOPENALTY:
                r = map[m].flag.nopenalty;
                break;
            case MF_PVP_NOPARTY:
                r = map[m].flag.pvp_noparty;
                break;
            case MF_NOZENYPENALTY:
                r = map[m].flag.nozenypenalty;
                break;
            case MF_NOWARP:
                r = map[m].flag.nowarp;
                break;
            case MF_NOPVP:
                r = map[m].flag.nopvp;
                break;
            case MF_NOICEWALL: // [Valaris]
                r = map[m].flag.noicewall;
                break;
            case MF_SNOW:      // [Valaris]
                r = map[m].flag.snow;
                break;
            case MF_FOG:       // [Valaris]
                r = map[m].flag.fog;
                break;
            case MF_SAKURA:    // [Valaris]
                r = map[m].flag.sakura;
                break;
            case MF_LEAVES:    // [Valaris]
                r = map[m].flag.leaves;
                break;
            case MF_RAIN:      // [Valaris]
                r = map[m].flag.rain;
                break;
        }
    }

    push_val(st->stack, ScriptCode::INT, r);
}

static
void builtin_pvpon(ScriptState *st)
{
    int m, i;
    struct map_session_data *pl_sd = NULL;

    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    m = map_mapname2mapid(str);
    if (m >= 0 && !map[m].flag.pvp && !map[m].flag.nopvp)
    {
        map[m].flag.pvp = 1;

        if (battle_config.pk_mode)  // disable ranking functions if pk_mode is on [Valaris]
            return;

        for (i = 0; i < fd_max; i++)
        {                       //人数分ループ
            if (session[i] && (pl_sd = (struct map_session_data *)session[i]->session_data)
                && pl_sd->state.auth)
            {
                if (m == pl_sd->bl.m && pl_sd->pvp_timer == -1)
                {
                    pl_sd->pvp_timer =
                        add_timer(gettick() + 200, pc_calc_pvprank_timer,
                                   pl_sd->bl.id, 0);
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
    int m, i;
    struct map_session_data *pl_sd = NULL;

    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    m = map_mapname2mapid(str);
    if (m >= 0 && map[m].flag.pvp && map[m].flag.nopvp)
    {
        map[m].flag.pvp = 0;

        if (battle_config.pk_mode)  // disable ranking options if pk_mode is on [Valaris]
            return;

        for (i = 0; i < fd_max; i++)
        {                       //人数分ループ
            if (session[i] && (pl_sd = (struct map_session_data *)session[i]->session_data)
                && pl_sd->state.auth)
            {
                if (m == pl_sd->bl.m)
                {
                    if (pl_sd->pvp_timer != -1)
                    {
                        delete_timer(pl_sd->pvp_timer,
                                      pc_calc_pvprank_timer);
                        pl_sd->pvp_timer = -1;
                    }
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
    type = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    if (type < 0 || type > 100)
        return;
    clif_emotion(map_id2bl(st->oid), type);
}

static
void builtin_mapwarp(ScriptState *st)   // Added by RoVeRT
{
    int x, y, m;
    int x0, y0, x1, y1;

    const char *mapname = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    x0 = 0;
    y0 = 0;
    x1 = map[map_mapname2mapid(mapname)].xs;
    y1 = map[map_mapname2mapid(mapname)].ys;
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 3]));
    x = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    y = conv_num(st, &(st->stack->stack_data[st->start + 5]));

    if ((m = map_mapname2mapid(mapname)) < 0)
        return;

    map_foreachinarea(std::bind(builtin_areawarp_sub, ph::_1, str, x, y),
            m, x0, y0, x1, y1, BL_PC);
}

static
void builtin_cmdothernpc(ScriptState *st)   // Added by RoVeRT
{
    const char *npc = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    const char *command = conv_str(st, &(st->stack->stack_data[st->start + 3]));

    npc_command(map_id2sd(st->rid), npc, command);
}

static
void builtin_mobcount_sub(struct block_list *bl, const char *event, int *c)
{
    if (strcmp(event, ((struct mob_data *) bl)->npc_event) == 0)
        (*c)++;
}

static
void builtin_mobcount(ScriptState *st)  // Added by RoVeRT
{
    int m, c = 0;
    const char *mapname = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    const char *event = conv_str(st, &(st->stack->stack_data[st->start + 3]));

    if ((m = map_mapname2mapid(mapname)) < 0)
    {
        push_val(st->stack, ScriptCode::INT, -1);
        return;
    }
    map_foreachinarea(std::bind(builtin_mobcount_sub, ph::_1, event, &c),
                       m, 0, 0, map[m].xs, map[m].ys, BL_MOB);

    push_val(st->stack, ScriptCode::INT, (c - 1));

}

static
void builtin_marriage(ScriptState *st)
{
    const char *partner = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    struct map_session_data *sd = script_rid2sd(st);
    struct map_session_data *p_sd = map_nick2sd(partner);

    if (sd == NULL || p_sd == NULL || pc_marriage(sd, p_sd) < 0)
    {
        push_val(st->stack, ScriptCode::INT, 0);
        return;
    }
    push_val(st->stack, ScriptCode::INT, 1);
}

static
void builtin_divorce(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);

    st->state = STOP;           // rely on pc_divorce to restart

    sd->npc_flags.divorce = 1;

    if (sd == NULL || pc_divorce(sd) < 0)
    {
        push_val(st->stack, ScriptCode::INT, 0);
        return;
    }

    push_val(st->stack, ScriptCode::INT, 1);
}

/*==========================================
 * IDからItem名
 *------------------------------------------
 */
static
void builtin_getitemname(ScriptState *st)
{
    struct item_data *i_data;
    char *item_name;
    struct script_data *data;

    data = &(st->stack->stack_data[st->start + 2]);
    get_val(st, data);
    if (data->type == ScriptCode::STR || data->type == ScriptCode::CONSTSTR)
    {
        const char *name = conv_str(st, data);
        i_data = itemdb_searchname(name);
    }
    else
    {
        int item_id = conv_num(st, data);
        i_data = itemdb_search(item_id);
    }

    item_name = (char *) calloc(24, 1);
    if (i_data)
        strncpy(item_name, i_data->jname, 23);
    else
        strncpy(item_name, "Unknown Item", 23);

    push_str(st->stack, ScriptCode::STR, item_name);

}

static
void builtin_getspellinvocation(ScriptState *st)
{
    const char *name = conv_str(st, &(st->stack->stack_data[st->start + 2]));

    const char *invocation = magic_find_invocation(name);
    if (!invocation)
        invocation = "...";

    push_str(st->stack, ScriptCode::STR, strdup(invocation));
}

static
void builtin_getpartnerid2(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);

    push_val(st->stack, ScriptCode::INT, sd->status.partner_id);
}

/*==========================================
 * PCの所持品情報読み取り
 *------------------------------------------
 */
static
void builtin_getinventorylist(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);
    int i, j = 0;
    if (!sd)
        return;
    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid > 0
            && sd->status.inventory[i].amount > 0)
        {
            pc_setreg(sd, add_str("@inventorylist_id") + (j << 24),
                       sd->status.inventory[i].nameid);
            pc_setreg(sd, add_str("@inventorylist_amount") + (j << 24),
                       sd->status.inventory[i].amount);
            pc_setreg(sd, add_str("@inventorylist_equip") + (j << 24),
                       uint16_t(sd->status.inventory[i].equip));
            pc_setreg(sd, add_str("@inventorylist_refine") + (j << 24),
                       sd->status.inventory[i].refine);
            pc_setreg(sd, add_str("@inventorylist_identify") + (j << 24),
                       sd->status.inventory[i].identify);
            pc_setreg(sd, add_str("@inventorylist_attribute") + (j << 24),
                       sd->status.inventory[i].attribute);
            pc_setreg(sd, add_str("@inventorylist_card1") + (j << 24),
                       sd->status.inventory[i].card[0]);
            pc_setreg(sd, add_str("@inventorylist_card2") + (j << 24),
                       sd->status.inventory[i].card[1]);
            pc_setreg(sd, add_str("@inventorylist_card3") + (j << 24),
                       sd->status.inventory[i].card[2]);
            pc_setreg(sd, add_str("@inventorylist_card4") + (j << 24),
                       sd->status.inventory[i].card[3]);
            j++;
        }
    }
    pc_setreg(sd, add_str("@inventorylist_count"), j);
}

static
void builtin_getactivatedpoolskilllist(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);
    SkillID pool_skills[MAX_SKILL_POOL];
    int skill_pool_size = skill_pool(sd, pool_skills);
    int i, count = 0;

    if (!sd)
        return;

    for (i = 0; i < skill_pool_size; i++)
    {
        SkillID skill_id = pool_skills[i];

        if (sd->status.skill[skill_id].id == skill_id)
        {
            pc_setreg(sd, add_str("@skilllist_id") + (count << 24),
                       uint16_t(sd->status.skill[skill_id].id));
            pc_setreg(sd, add_str("@skilllist_lv") + (count << 24),
                       sd->status.skill[skill_id].lv);
            pc_setreg(sd, add_str("@skilllist_flag") + (count << 24),
                       uint16_t(sd->status.skill[skill_id].flags));
            pc_setregstr(sd, add_str("@skilllist_name$") + (count << 24),
                          skill_name(skill_id));
            ++count;
        }
    }
    pc_setreg(sd, add_str("@skilllist_count"), count);

}

static
void builtin_getunactivatedpoolskilllist(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);
    int i, count = 0;

    if (!sd)
        return;

    for (i = 0; i < skill_pool_skills_size; i++)
    {
        SkillID skill_id = skill_pool_skills[i];

        if (sd->status.skill[skill_id].id == skill_id
            && !bool(sd->status.skill[skill_id].flags & SKILL_POOL_ACTIVATED))
        {
            pc_setreg(sd, add_str("@skilllist_id") + (count << 24),
                       uint16_t(sd->status.skill[skill_id].id));
            pc_setreg(sd, add_str("@skilllist_lv") + (count << 24),
                       sd->status.skill[skill_id].lv);
            pc_setreg(sd, add_str("@skilllist_flag") + (count << 24),
                       uint16_t(sd->status.skill[skill_id].flags));
            pc_setregstr(sd, add_str("@skilllist_name$") + (count << 24),
                          skill_name(skill_id));
            ++count;
        }
    }
    pc_setreg(sd, add_str("@skilllist_count"), count);

}

static
void builtin_poolskill(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);
    SkillID skill_id = SkillID(conv_num(st, &(st->stack->stack_data[st->start + 2])));

    skill_pool_activate(sd, skill_id);
    clif_skillinfoblock(sd);

}

static
void builtin_unpoolskill(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);
    SkillID skill_id = SkillID(conv_num(st, &(st->stack->stack_data[st->start + 2])));

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
    const char *name = NULL;
    struct block_list *bl = NULL;

    type = conv_num(st, &(st->stack->stack_data[st->start + 2]));

    if (st->end > st->start + 3)
    {
        struct script_data *sdata = &(st->stack->stack_data[st->start + 3]);

        get_val(st, sdata);

        if (sdata->type == ScriptCode::STR || sdata->type == ScriptCode::CONSTSTR)
            name = conv_str(st, sdata);
        else
            id = conv_num(st, sdata);
    }

    if (name)
    {
        struct map_session_data *sd = map_nick2sd(name);
        if (sd)
            bl = &sd->bl;
    }
    else if (id)
        bl = map_id2bl(id);
    else if (st->oid)
        bl = map_id2bl(st->oid);
    else
    {
        struct map_session_data *sd = script_rid2sd(st);
        if (sd)
            bl = &sd->bl;
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
    struct block_list *bl = map_id2bl(st->oid);

    if (bl == NULL)
        return;

    clif_specialeffect(bl,
                        conv_num(st,
                                  &(st->stack->stack_data[st->start + 2])),
                        0);

}

static
void builtin_specialeffect2(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);

    if (sd == NULL)
        return;

    clif_specialeffect(&sd->bl,
                        conv_num(st,
                                  &(st->stack->stack_data[st->start + 2])),
                        0);

}

/*==========================================
 * Nude [Valaris]
 *------------------------------------------
 */

static
void builtin_nude(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);

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
    struct map_session_data *sd = script_rid2sd(st);
    if (sd == NULL)
        return;

    EQUIP slot_id = EQUIP(conv_num(st, &(st->stack->stack_data[st->start + 2])));

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
    struct map_session_data *sd;

    sd = script_rid2sd(st);
    const char *cmd = conv_str(st, &(st->stack->stack_data[st->start + 2]));

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
    struct npc_data *nd = NULL;

    x = conv_num(st, &(st->stack->stack_data[st->start + 2]));
    y = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    const char *npc = conv_str(st, &(st->stack->stack_data[st->start + 4]));
    nd = npc_name2id(npc);

    if (!nd)
        return;

    short m = nd->bl.m;

    /* Crude sanity checks. */
    if (m < 0 || !nd->bl.prev
            || x < 0 || x > map[m].xs -1
            || y < 0 || y > map[m].ys - 1)
        return;

    npc_enable(npc, 0);
    map_delblock(&nd->bl); /* [Freeyorp] */
    nd->bl.x = x;
    nd->bl.y = y;
    map_addblock(&nd->bl);
    npc_enable(npc, 1);

}

/*==========================================
 * message [MouseJstr]
 *------------------------------------------
 */

static
void builtin_message(ScriptState *st)
{
    struct map_session_data *pl_sd = NULL;

    const char *player = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    const char *msg = conv_str(st, &(st->stack->stack_data[st->start + 3]));

    if ((pl_sd = map_nick2sd(player)) == NULL)
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
    char message[255];

    struct npc_data *nd = (struct npc_data *) map_id2bl(st->oid);
    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));

    if (nd)
    {
        memcpy(message, nd->name, 24);
        strcat(message, " : ");
        strcat(message, str);
        clif_message(&nd->bl, message);
    }

}

/*==========================================
  * getlook char info. getlook(arg)
  *------------------------------------------
  */
static
void builtin_getlook(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);

    LOOK type = LOOK(conv_num(st, &(st->stack->stack_data[st->start + 2])));
    int val = -1;
    switch (type)
    {
        case LOOK_HAIR:        //1
            val = sd->status.hair;
            break;
        case LOOK_WEAPON:      //2
            val = sd->status.weapon;
            break;
        case LOOK_HEAD_BOTTOM: //3
            val = sd->status.head_bottom;
            break;
        case LOOK_HEAD_TOP:    //4
            val = sd->status.head_top;
            break;
        case LOOK_HEAD_MID:    //5
            val = sd->status.head_mid;
            break;
        case LOOK_HAIR_COLOR:  //6
            val = sd->status.hair_color;
            break;
        case LOOK_CLOTHES_COLOR:   //7
            val = sd->status.clothes_color;
            break;
        case LOOK_SHIELD:      //8
            val = sd->status.shield;
            break;
        case LOOK_SHOES:       //9
            break;
    }

    push_val(st->stack, ScriptCode::INT, val);
}

/*==========================================
  *     get char save point. argument: 0- map name, 1- x, 2- y
  *------------------------------------------
*/
static
void builtin_getsavepoint(ScriptState *st)
{
    int x, y, type;
    char *mapname;
    struct map_session_data *sd;

    sd = script_rid2sd(st);

    type = conv_num(st, &(st->stack->stack_data[st->start + 2]));

    x = sd->status.save_point.x;
    y = sd->status.save_point.y;
    switch (type)
    {
        case 0:
            mapname = (char*)calloc(24, 1);
            strncpy(mapname, sd->status.save_point.map, 23);
            push_str(st->stack, ScriptCode::STR, mapname);
            break;
        case 1:
            push_val(st->stack, ScriptCode::INT, x);
            break;
        case 2:
            push_val(st->stack, ScriptCode::INT, y);
            break;
    }
}

/*==========================================
 *     areatimer
 *------------------------------------------
 */
static
void builtin_areatimer_sub(struct block_list *bl, int tick, const char *event)
{
    pc_addeventtimer((struct map_session_data *) bl, tick, event);
}

static
void builtin_areatimer(ScriptState *st)
{
    int tick, m;
    int x0, y0, x1, y1;

    const char *mapname = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    x0 = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    y0 = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    x1 = conv_num(st, &(st->stack->stack_data[st->start + 5]));
    y1 = conv_num(st, &(st->stack->stack_data[st->start + 6]));
    tick = conv_num(st, &(st->stack->stack_data[st->start + 7]));
    const char *event = conv_str(st, &(st->stack->stack_data[st->start + 8]));

    if ((m = map_mapname2mapid(mapname)) < 0)
        return;

    map_foreachinarea(std::bind(builtin_areatimer_sub, ph::_1, tick, event),
                       m, x0, y0, x1, y1, BL_PC);
}

/*==========================================
 * Check whether the PC is in the specified rectangle
 *------------------------------------------
 */
static
void builtin_isin(ScriptState *st)
{
    int x1, y1, x2, y2;
    struct map_session_data *sd = script_rid2sd(st);

    const char *str = conv_str(st, &(st->stack->stack_data[st->start + 2]));
    x1 = conv_num(st, &(st->stack->stack_data[st->start + 3]));
    y1 = conv_num(st, &(st->stack->stack_data[st->start + 4]));
    x2 = conv_num(st, &(st->stack->stack_data[st->start + 5]));
    y2 = conv_num(st, &(st->stack->stack_data[st->start + 6]));

    if (!sd)
        return;

    push_val(st->stack, ScriptCode::INT,
              (sd->bl.x >= x1 && sd->bl.x <= x2)
              && (sd->bl.y >= y1 && sd->bl.y <= y2)
              && (!strcmp(str, map[sd->bl.m].name)));

}

/*==========================================
 * Check whether the PC is dead
 *------------------------------------------
 */
static
void builtin_isdead(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);

    push_val(st->stack, ScriptCode::INT, pc_isdead(sd));
}

/*============================
 * Gets the PC's x pos
 *----------------------------
 */

static
void builtin_getx(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);

    push_val(st->stack, ScriptCode::INT, sd->bl.x);
}

/*============================
 * Gets the PC's y pos
 *----------------------------
 */
static
void builtin_gety(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);

    push_val(st->stack, ScriptCode::INT, sd->bl.y);
}

/*
 * Get the PC's current map's name
 */
static
void builtin_getmap(ScriptState *st)
{
    struct map_session_data *sd = script_rid2sd(st);

    // A map_data lives essentially forever.
    push_str(st->stack, ScriptCode::CONSTSTR, map[sd->bl.m].name);
}

//
// 実行部main
//
/*==========================================
 * コマンドの読み取り
 *------------------------------------------
 */
static
ScriptCode get_com(const ScriptCode *script, int *pos_)
{
    if (static_cast<uint8_t>(script[*pos_]) >= 0x80)
    {
        return ScriptCode::INT;
    }
    return script[(*pos_)++];
}

/*==========================================
 * 数値の所得
 *------------------------------------------
 */
static
int get_num(const ScriptCode *scr, int *pos_)
{
    const uint8_t *script = reinterpret_cast<const uint8_t *>(scr);
    int i, j;
    i = 0;
    j = 0;
    while (script[*pos_] >= 0xc0)
    {
        i += (script[(*pos_)++] & 0x7f) << j;
        j += 6;
    }
    return i + ((script[(*pos_)++] & 0x7f) << j);
}

/*==========================================
 * スタックから値を取り出す
 *------------------------------------------
 */
static
int pop_val(ScriptState *st)
{
    if (st->stack->sp <= 0)
        return 0;
    st->stack->sp--;
    get_val(st, &(st->stack->stack_data[st->stack->sp]));
    if (st->stack->stack_data[st->stack->sp].type == ScriptCode::INT)
        return st->stack->stack_data[st->stack->sp].u.num;
    return 0;
}

#define isstr(c) ((c).type==ScriptCode::STR || (c).type==ScriptCode::CONSTSTR)

/*==========================================
 * 加算演算子
 *------------------------------------------
 */
static
void op_add(ScriptState *st)
{
    st->stack->sp--;
    get_val(st, &(st->stack->stack_data[st->stack->sp]));
    get_val(st, &(st->stack->stack_data[st->stack->sp - 1]));

    if (isstr(st->stack->stack_data[st->stack->sp])
        || isstr(st->stack->stack_data[st->stack->sp - 1]))
    {
        conv_str(st, &(st->stack->stack_data[st->stack->sp]));
        conv_str(st, &(st->stack->stack_data[st->stack->sp - 1]));
    }
    if (st->stack->stack_data[st->stack->sp].type == ScriptCode::INT)
    {                           // ii
        st->stack->stack_data[st->stack->sp - 1].u.num +=
            st->stack->stack_data[st->stack->sp].u.num;
    }
    else
    {                           // ssの予定
        char *buf;
        buf = (char *)
            calloc(strlen(st->stack->stack_data[st->stack->sp - 1].u.str) +
                    strlen(st->stack->stack_data[st->stack->sp].u.str) + 1,
                    1);
        strcpy(buf, st->stack->stack_data[st->stack->sp - 1].u.str);
        strcat(buf, st->stack->stack_data[st->stack->sp].u.str);
        if (st->stack->stack_data[st->stack->sp - 1].type == ScriptCode::STR)
            free(const_cast<char *>(st->stack->stack_data[st->stack->sp - 1].u.str));
        if (st->stack->stack_data[st->stack->sp].type == ScriptCode::STR)
            free(const_cast<char *>(st->stack->stack_data[st->stack->sp].u.str));
        st->stack->stack_data[st->stack->sp - 1].type = ScriptCode::STR;
        st->stack->stack_data[st->stack->sp - 1].u.str = buf;
    }
}

/*==========================================
 * 二項演算子(文字列)
 *------------------------------------------
 */
static
void op_2str(ScriptState *st, ScriptCode op, int sp1, int sp2)
{
    const char *s1 = st->stack->stack_data[sp1].u.str;
    const char *s2 = st->stack->stack_data[sp2].u.str;
    int a = 0;

    switch (op)
    {
        case ScriptCode::EQ:
            a = (strcmp(s1, s2) == 0);
            break;
        case ScriptCode::NE:
            a = (strcmp(s1, s2) != 0);
            break;
        case ScriptCode::GT:
            a = (strcmp(s1, s2) > 0);
            break;
        case ScriptCode::GE:
            a = (strcmp(s1, s2) >= 0);
            break;
        case ScriptCode::LT:
            a = (strcmp(s1, s2) < 0);
            break;
        case ScriptCode::LE:
            a = (strcmp(s1, s2) <= 0);
            break;
        default:
            PRINTF("illegal string operater\n");
            break;
    }

    push_val(st->stack, ScriptCode::INT, a);

    if (st->stack->stack_data[sp1].type == ScriptCode::STR)
        free(const_cast<char *>(s1));
    if (st->stack->stack_data[sp2].type == ScriptCode::STR)
        free(const_cast<char *>(s2));
}

/*==========================================
 * 二項演算子(数値)
 *------------------------------------------
 */
static
void op_2num(ScriptState *st, ScriptCode op, int i1, int i2)
{
    switch (op)
    {
        case ScriptCode::SUB:
            i1 -= i2;
            break;
        case ScriptCode::MUL:
            i1 *= i2;
            break;
        case ScriptCode::DIV:
            i1 /= i2;
            break;
        case ScriptCode::MOD:
            i1 %= i2;
            break;
        case ScriptCode::AND:
            i1 &= i2;
            break;
        case ScriptCode::OR:
            i1 |= i2;
            break;
        case ScriptCode::XOR:
            i1 ^= i2;
            break;
        case ScriptCode::LAND:
            i1 = i1 && i2;
            break;
        case ScriptCode::LOR:
            i1 = i1 || i2;
            break;
        case ScriptCode::EQ:
            i1 = i1 == i2;
            break;
        case ScriptCode::NE:
            i1 = i1 != i2;
            break;
        case ScriptCode::GT:
            i1 = i1 > i2;
            break;
        case ScriptCode::GE:
            i1 = i1 >= i2;
            break;
        case ScriptCode::LT:
            i1 = i1 < i2;
            break;
        case ScriptCode::LE:
            i1 = i1 <= i2;
            break;
        case ScriptCode::R_SHIFT:
            i1 = i1 >> i2;
            break;
        case ScriptCode::L_SHIFT:
            i1 = i1 << i2;
            break;
    }
    push_val(st->stack, ScriptCode::INT, i1);
}

/*==========================================
 * 二項演算子
 *------------------------------------------
 */
static
void op_2(ScriptState *st, ScriptCode op)
{
    int i1, i2;
    const char *s1 = NULL, *s2 = NULL;

    i2 = pop_val(st);
    if (isstr(st->stack->stack_data[st->stack->sp]))
        s2 = st->stack->stack_data[st->stack->sp].u.str;

    i1 = pop_val(st);
    if (isstr(st->stack->stack_data[st->stack->sp]))
        s1 = st->stack->stack_data[st->stack->sp].u.str;

    if (s1 != NULL && s2 != NULL)
    {
        // ss => op_2str
        op_2str(st, op, st->stack->sp, st->stack->sp + 1);
    }
    else if (s1 == NULL && s2 == NULL)
    {
        // ii => op_2num
        op_2num(st, op, i1, i2);
    }
    else
    {
        // si,is => error
        PRINTF("script: op_2: int&str, str&int not allow.");
        push_val(st->stack, ScriptCode::INT, 0);
    }
}

/*==========================================
 * 単項演算子
 *------------------------------------------
 */
static
void op_1num(ScriptState *st, ScriptCode op)
{
    int i1;
    i1 = pop_val(st);
    switch (op)
    {
        case ScriptCode::NEG:
            i1 = -i1;
            break;
        case ScriptCode::NOT:
            i1 = ~i1;
            break;
        case ScriptCode::LNOT:
            i1 = !i1;
            break;
    }
    push_val(st->stack, ScriptCode::INT, i1);
}

/*==========================================
 * 関数の実行
 *------------------------------------------
 */
void run_func(ScriptState *st)
{
    int i, start_sp, end_sp, func;

    end_sp = st->stack->sp;
    for (i = end_sp - 1; i >= 0 && st->stack->stack_data[i].type != ScriptCode::ARG;
         i--);
    if (i == 0)
    {
        if (battle_config.error_log)
            PRINTF("function not found\n");
//      st->stack->sp=0;
        st->state = END;
        return;
    }
    start_sp = i - 1;
    st->start = i - 1;
    st->end = end_sp;

    func = st->stack->stack_data[st->start].u.num;
    if (st->stack->stack_data[st->start].type != ScriptCode::NAME
        || str_data[func].type != ScriptCode::FUNC)
    {
        PRINTF("run_func: not function and command! \n");
//      st->stack->sp=0;
        st->state = END;
        return;
    }
#ifdef DEBUG_RUN
    if (battle_config.etc_log)
    {
        PRINTF("run_func : %s? (%d(%d))\n", str_buf + str_data[func].str,
                func, str_data[func].type);
        PRINTF("stack dump :");
        for (i = 0; i < end_sp; i++)
        {
            switch (st->stack->stack_data[i].type)
            {
                case ScriptCode::INT:
                    PRINTF(" int(%d)", st->stack->stack_data[i].u.num);
                    break;
                case ScriptCode::NAME:
                    PRINTF(" name(%s)",
                            str_buf +
                            str_data[st->stack->stack_data[i].u.num].str);
                    break;
                case ScriptCode::ARG:
                    PRINTF(" arg");
                    break;
                case ScriptCode::POS:
                    PRINTF(" pos(%d)", st->stack->stack_data[i].u.num);
                    break;
                default:
                    PRINTF(" %d,%d", st->stack->stack_data[i].type,
                            st->stack->stack_data[i].u.num);
            }
        }
        PRINTF("\n");
    }
#endif
    if (str_data[func].func)
    {
        str_data[func].func(st);
    }
    else
    {
        if (battle_config.error_log)
            PRINTF("run_func : %s? (%d(%d))\n", str_buf + str_data[func].str,
                    func, str_data[func].type);
        push_val(st->stack, ScriptCode::INT, 0);
    }

    pop_stack(st->stack, start_sp, end_sp);

    if (st->state == RETFUNC)
    {
        // ユーザー定義関数からの復帰
        int olddefsp = st->defsp;

        pop_stack(st->stack, st->defsp, start_sp); // 復帰に邪魔なスタック削除
        if (st->defsp < 4
            || st->stack->stack_data[st->defsp - 1].type != ScriptCode::RETINFO)
        {
            PRINTF("script:run_func (return) return without callfunc or callsub!\n");
            st->state = END;
            return;
        }
        i = conv_num(st, &(st->stack->stack_data[st->defsp - 4])); // 引数の数所得
        st->pos = conv_num(st, &(st->stack->stack_data[st->defsp - 1]));   // スクリプト位置の復元
        st->script = (ScriptCode *) conv_num(st, &(st->stack->stack_data[st->defsp - 2]));   // スクリプトを復元
        st->defsp = conv_num(st, &(st->stack->stack_data[st->defsp - 3])); // 基準スタックポインタを復元

        pop_stack(st->stack, olddefsp - 4 - i, olddefsp);  // 要らなくなったスタック(引数と復帰用データ)削除

        st->state = GOTO;
    }
}

/*==========================================
 * スクリプトの実行メイン部分
 *------------------------------------------
 */
static
void run_script_main(const ScriptCode *script, int pos_, int, int,
                      ScriptState *st, const ScriptCode *rootscript)
{
    int rerun_pos;
    int cmdcount = script_config.check_cmdcount;
    int gotocount = script_config.check_gotocount;
    struct script_stack *stack = st->stack;

    st->defsp = stack->sp;
    st->script = script;

    rerun_pos = st->pos;
    for (st->state = 0; st->state == 0;)
    {
        switch (ScriptCode c = get_com(script, &st->pos))
        {
            case ScriptCode::EOL:
                if (stack->sp != st->defsp)
                {
                    if (battle_config.error_log)
                        PRINTF("stack.sp (%d) != default (%d)\n", stack->sp,
                                st->defsp);
                    stack->sp = st->defsp;
                }
                rerun_pos = st->pos;
                break;
            case ScriptCode::INT:
                push_val(stack, ScriptCode::INT, get_num(script, &st->pos));
                break;
            case ScriptCode::POS:
            case ScriptCode::NAME:
                push_val(stack, c, (*(const int *)(script + st->pos)) & 0xffffff);
                st->pos += 3;
                break;
            case ScriptCode::ARG:
                push_val(stack, c, 0);
                break;
            case ScriptCode::STR:
                push_str(stack, ScriptCode::CONSTSTR, reinterpret_cast<const char *>(script + st->pos));
                while (script[st->pos++] != ScriptCode::NOP);
                break;
            case ScriptCode::FUNC:
                run_func(st);
                if (st->state == GOTO)
                {
                    rerun_pos = st->pos;
                    script = st->script;
                    st->state = 0;
                    if (gotocount > 0 && (--gotocount) <= 0)
                    {
                        PRINTF("run_script: infinity loop !\n");
                        st->state = END;
                    }
                }
                break;

            case ScriptCode::ADD:
                op_add(st);
                break;

            case ScriptCode::SUB:
            case ScriptCode::MUL:
            case ScriptCode::DIV:
            case ScriptCode::MOD:
            case ScriptCode::EQ:
            case ScriptCode::NE:
            case ScriptCode::GT:
            case ScriptCode::GE:
            case ScriptCode::LT:
            case ScriptCode::LE:
            case ScriptCode::AND:
            case ScriptCode::OR:
            case ScriptCode::XOR:
            case ScriptCode::LAND:
            case ScriptCode::LOR:
            case ScriptCode::R_SHIFT:
            case ScriptCode::L_SHIFT:
                op_2(st, c);
                break;

            case ScriptCode::NEG:
            case ScriptCode::NOT:
            case ScriptCode::LNOT:
                op_1num(st, c);
                break;

            case ScriptCode::NOP:
                st->state = END;
                break;

            default:
                if (battle_config.error_log)
                    PRINTF("unknown command : %d @ %d\n", c, pos_);
                st->state = END;
                break;
        }
        if (cmdcount > 0 && (--cmdcount) <= 0)
        {
            PRINTF("run_script: infinity loop !\n");
            st->state = END;
        }
    }
    switch (st->state)
    {
        case STOP:
            break;
        case END:
        {
            struct map_session_data *sd = map_id2sd(st->rid);
            st->pos = -1;
            if (sd && sd->npc_id == st->oid)
                npc_event_dequeue(sd);
        }
            break;
        case RERUNLINE:
        {
            st->pos = rerun_pos;
        }
            break;
    }

    if (st->state != END)
    {
        // 再開するためにスタック情報を保存
        struct map_session_data *sd = map_id2sd(st->rid);
        if (sd /* && sd->npc_stackbuf==NULL */ )
        {
            if (sd->npc_stackbuf)
                free(sd->npc_stackbuf);
            sd->npc_stackbuf = (char *)
                calloc(sizeof(stack->stack_data[0]) * stack->sp_max, 1);
            memcpy(sd->npc_stackbuf, stack->stack_data,
                    sizeof(stack->stack_data[0]) * stack->sp_max);
            sd->npc_stack = stack->sp;
            sd->npc_stackmax = stack->sp_max;
            sd->npc_script = script;
            sd->npc_scriptroot = rootscript;
        }
    }

}

/*==========================================
 * スクリプトの実行
 *------------------------------------------
 */
int run_script(const ScriptCode *script, int pos_, int rid, int oid)
{
    return run_script_l(script, pos_, rid, oid, 0, NULL);
}

int run_script_l(const ScriptCode *script, int pos_, int rid, int oid,
                  int args_nr, argrec_t *args)
{
    struct script_stack stack;
    ScriptState st;
    struct map_session_data *sd = map_id2sd(rid);
    const ScriptCode *rootscript = script;
    int i;
    if (script == NULL || pos_ < 0)
        return -1;

    if (sd && sd->npc_stackbuf && sd->npc_scriptroot == rootscript)
    {
        // 前回のスタックを復帰
        script = sd->npc_script;
        stack.sp = sd->npc_stack;
        stack.sp_max = sd->npc_stackmax;
        stack.stack_data = (struct script_data *)
            calloc(stack.sp_max, sizeof(stack.stack_data[0]));
        memcpy(stack.stack_data, sd->npc_stackbuf,
                sizeof(stack.stack_data[0]) * stack.sp_max);
        free(sd->npc_stackbuf);
        sd->npc_stackbuf = NULL;
    }
    else
    {
        // スタック初期化
        stack.sp = 0;
        stack.sp_max = 64;
        stack.stack_data = (struct script_data *)
            calloc(stack.sp_max, sizeof(stack.stack_data[0]));
    }
    st.stack = &stack;
    st.pos = pos_;
    st.rid = rid;
    st.oid = oid;
    for (i = 0; i < args_nr; i++)
    {
        if (args[i].name[strlen(args[i].name) - 1] == '$')
            pc_setregstr(sd, add_str(args[i].name), args[i].v.s);
        else
            pc_setreg(sd, add_str(args[i].name), args[i].v.i);
    }
    run_script_main(script, pos_, rid, oid, &st, rootscript);

    free(stack.stack_data);
    stack.stack_data = NULL;
    return st.pos;
}

/*==========================================
 * マップ変数の変更
 *------------------------------------------
 */
void mapreg_setreg(int num, int val)
{
    if (val != 0)
        numdb_insert(mapreg_db, num, val);
    else
        numdb_erase(mapreg_db, num);

    mapreg_dirty = 1;
}

/*==========================================
 * 文字列型マップ変数の変更
 *------------------------------------------
 */
void mapreg_setregstr(int num, const char *str)
{
    char *p;

    if ((p = (char *)numdb_search(mapregstr_db, num)) != NULL)
        free(p);

    if (str == NULL || *str == 0)
    {
        numdb_erase(mapregstr_db, num);
        mapreg_dirty = 1;
        return;
    }
    p = (char *) calloc(strlen(str) + 1, 1);
    strcpy(p, str);
    numdb_insert(mapregstr_db, num, p);
    mapreg_dirty = 1;
}

/*==========================================
 * 永続的マップ変数の読み込み
 *------------------------------------------
 */
static
void script_load_mapreg(void)
{
    std::ifstream in(mapreg_txt);

    if (!in.is_open())
        return;

    std::string line;
    while (std::getline(in, line))
    {
        std::string buf1, buf2;
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
            int s = add_str(buf1.c_str());
            int key = (index << 24) | s;
            if (buf1.back() == '$')
            {
                char *p = strdup(buf2.c_str());
                numdb_insert(mapregstr_db, key, p);
            }
            else
            {
                int v;
                if (!extract(buf2, &v))
                    goto borken;
                numdb_insert(mapreg_db, key, v);
            }
        }
        else
        {
        borken:
            PRINTF("%s: %s broken data !\n", mapreg_txt, buf1);
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
void script_save_mapreg_intsub(db_key_t key, db_val_t data, FILE *fp)
{
    int num = key.i & 0x00ffffff, i = key.i >> 24;
    char *name = str_buf + str_data[num].str;
    if (name[1] != '@')
    {
        if (i == 0)
            FPRINTF(fp, "%s\t%d\n", name, (int) data);
        else
            FPRINTF(fp, "%s,%d\t%d\n", name, i, (int) data);
    }
}

static
void script_save_mapreg_strsub(db_key_t key, db_val_t data, FILE *fp)
{
    int num = key.i & 0x00ffffff, i = key.i >> 24;
    char *name = str_buf + str_data[num].str;
    if (name[1] != '@')
    {
        if (i == 0)
            FPRINTF(fp, "%s\t%s\n", name, (char *) data);
        else
            FPRINTF(fp, "%s,%d\t%s\n", name, i, (char *) data);
    }
}

static
void script_save_mapreg(void)
{
    FILE *fp;
    int lock;

    if ((fp = lock_fopen(mapreg_txt, &lock)) == NULL)
        return;
    numdb_foreach(mapreg_db, std::bind(script_save_mapreg_intsub, ph::_1, ph::_2, fp));
    numdb_foreach(mapregstr_db, std::bind(script_save_mapreg_strsub, ph::_1, ph::_2, fp));
    lock_fclose(fp, mapreg_txt, &lock);
    mapreg_dirty = 0;
}

static
void script_autosave_mapreg(timer_id, tick_t, custom_id_t, custom_data_t)
{
    if (mapreg_dirty)
        script_save_mapreg();
}

void script_config_read()
{
    script_config.warn_func_no_comma = 1;
    script_config.warn_cmd_no_comma = 1;
    script_config.warn_func_mismatch_paramnum = 1;
    script_config.warn_cmd_mismatch_paramnum = 1;
    script_config.check_cmdcount = 8192;
    script_config.check_gotocount = 512;
}

/*==========================================
 * 終了
 *------------------------------------------
 */

static
void mapregstr_db_final(db_key_t, db_val_t data)
{
    free(data);
}

static
void userfunc_db_final(db_key_t key, db_val_t data)
{
    free(key.ms);
    free(data);
}

void do_final_script(void)
{
    if (mapreg_dirty >= 0)
        script_save_mapreg();
#if 0
    // labels are allocated just out of this
    // (so it's a leak ...)
    // this is disabled because it leads to a crash
    // due to double-free
    if (script_buf)
        free(script_buf);
#endif

    if (mapreg_db)
        numdb_final(mapreg_db, NULL);
    if (mapregstr_db)
        strdb_final(mapregstr_db, mapregstr_db_final);
    if (scriptlabel_db)
        strdb_final(scriptlabel_db, NULL);
    if (userfunc_db)
        strdb_final(userfunc_db, userfunc_db_final);

    if (str_data)
        free(str_data);
    if (str_buf)
        free(str_buf);
}

/*==========================================
 * 初期化
 *------------------------------------------
 */
void do_init_script(void)
{
    mapreg_db = numdb_init();
    mapregstr_db = numdb_init();
    script_load_mapreg();

    add_timer_interval(gettick() + MAPREG_AUTOSAVE_INTERVAL,
                        script_autosave_mapreg, 0, 0,
                        MAPREG_AUTOSAVE_INTERVAL);

    scriptlabel_db = strdb_init(50);
}

#define BUILTIN(func, args) \
{builtin_##func, #func, args}

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
    BUILTIN(gettimestr, "si"),
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
    BUILTIN(isdead, ""),
    BUILTIN(unequipbyid, "i"),
    BUILTIN(getx, ""),
    BUILTIN(gety, ""),
    BUILTIN(getmap, ""),
    {NULL, NULL, NULL},
};

