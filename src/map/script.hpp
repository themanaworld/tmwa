#ifndef TMWA_MAP_SCRIPT_HPP
#define TMWA_MAP_SCRIPT_HPP

# include <cstdint>
# include <cstring> // for inlined get_str - TODO remove

# include <vector>

# include "../strings/rstring.hpp"
# include "../strings/astring.hpp"
# include "../strings/zstring.hpp"

# include "../generic/db.hpp"

# include "../mmo/dumb_ptr.hpp"
# include "../mmo/utils.hpp"

# include "map.t.hpp"

enum class ByteCode : uint8_t;
struct str_data_t;

class ScriptBuffer
{
    typedef ZString::iterator ZSit;

    std::vector<ByteCode> script_buf;
public:
    // construction methods used only by script.cpp
    void add_scriptc(ByteCode a);
    void add_scriptb(uint8_t a);
    void add_scripti(uint32_t a);
    void add_scriptl(str_data_t *a);
    void set_label(str_data_t *ld, int pos_);
    ZSit parse_simpleexpr(ZSit p);
    ZSit parse_subexpr(ZSit p, int limit);
    ZSit parse_expr(ZSit p);
    ZSit parse_line(ZSit p);
    void parse_script(ZString src, int line);

    // consumption methods used only by script.cpp
    ByteCode operator[](size_t i) const { return script_buf[i]; }
    ZString get_str(size_t i) const
    {
        return ZString(strings::really_construct_from_a_pointer, reinterpret_cast<const char *>(&script_buf[i]), nullptr);
    }

    // method used elsewhere
};

struct ScriptPointer
{
    const ScriptBuffer *code;
    size_t pos;

    ScriptPointer()
    : code()
    , pos()
    {}

    ScriptPointer(const ScriptBuffer *c, size_t p)
    : code(c)
    , pos(p)
    {}

    ByteCode peek() const { return (*code)[pos]; }
    ByteCode pop() { return (*code)[pos++]; }
    ZString pops()
    {
        ZString rv = code->get_str(pos);
        pos += rv.size();
        ++pos;
        return rv;
    }
};

// internal
class SIR
{
    uint32_t impl;
    SIR(SP v)
    : impl(static_cast<uint32_t>(v))
    {}
    SIR(unsigned v, uint8_t i)
    : impl((i << 24) | v)
    {}
public:
    SIR() : impl() {}

    unsigned base() const { return impl & 0x00ffffff; }
    uint8_t index() const { return impl >> 24; }
    SIR iplus(uint8_t i) const { return SIR(base(), index() + i); }
    static SIR from(unsigned v, uint8_t i=0) { return SIR(v, i); }

    SP sp() const { return static_cast<SP>(impl); }
    static SIR from(SP v) { return SIR(v); }

    friend bool operator == (SIR l, SIR r) { return l.impl == r.impl; }
    friend bool operator < (SIR l, SIR r) { return l.impl < r.impl; }
};

struct script_data
{
    ByteCode type;
    union uu
    {
        SIR reg;
        int numi;
        dumb_string str;
        // Not a ScriptPointer - pos is stored in a separate slot,
        // to avoid exploding the struct for everyone.
        const ScriptBuffer *script;

        uu() { memset(this, '\0', sizeof(*this)); }
        ~uu() = default;
        uu(const uu&) = default;
        uu& operator = (const uu&) = default;
    } u;
};

struct script_stack
{
    std::vector<struct script_data> stack_datav;
};

enum class ScriptEndState;
// future improvements coming!
class ScriptState
{
public:
    struct script_stack *stack;
    int start, end;
    ScriptEndState state;
    int rid, oid;
    ScriptPointer scriptp, new_scriptp;
    int defsp, new_defsp;
};

std::unique_ptr<const ScriptBuffer> parse_script(ZString, int);
struct argrec_t
{
    ZString name;
    union _aru
    {
        int i;
        ZString s;

        _aru(int n) : i(n) {}
        _aru(ZString z) : s(z) {}
    } v;

    argrec_t(ZString n, int i) : name(n), v(i) {}
    argrec_t(ZString n, ZString z) : name(n), v(z) {}
};
int run_script_l(ScriptPointer, int, int, int, argrec_t *args);
int run_script(ScriptPointer, int, int);

struct ScriptLabel;
extern
Map<ScriptLabel, int> scriptlabel_db;
extern
UPMap<RString, const ScriptBuffer> userfunc_db;

void do_init_script(void);
void do_final_script(void);

extern AString mapreg_txt;

extern int script_errors;

bool read_constdb(ZString filename);

void set_script_var_i(dumb_ptr<map_session_data> sd, VarName var, int e, int val);
void set_script_var_s(dumb_ptr<map_session_data> sd, VarName var, int e, XString val);

int get_script_var_i(dumb_ptr<map_session_data> sd, VarName var, int e);
ZString get_script_var_s(dumb_ptr<map_session_data> sd, VarName var, int e);

#endif // TMWA_MAP_SCRIPT_HPP
