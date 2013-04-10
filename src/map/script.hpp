#ifndef SCRIPT_HPP
#define SCRIPT_HPP

#include <cstdint>

#include <string>

#include "../common/db.hpp"

enum class ScriptCode : uint8_t;

struct script_data
{
    ScriptCode type;
    union
    {
        int num;
        const char *str;
        const ScriptCode *script;
    } u;
};

struct script_stack
{
    int sp, sp_max;
    struct script_data *stack_data;
};

// future improvements coming!
class ScriptState
{
public:
    struct script_stack *stack;
    int start, end;
    int pos, state;
    int rid, oid;
    const ScriptCode *script, *new_script;
    int defsp, new_pos, new_defsp;
};

const ScriptCode *parse_script(const char *, int);
typedef struct argrec
{
    const char *name;
    union _aru
    {
        int i;
        const char *s;

        _aru() = default;
        _aru(int n) : i(n) {}
        _aru(const char *z) : s(z) {}
    } v;
} argrec_t;
int run_script_l(const ScriptCode *, int, int, int, int, argrec_t *args);
int run_script(const ScriptCode *, int, int, int);

extern
Map<std::string, int> scriptlabel_db;
extern
DMap<std::string, const ScriptCode *> userfunc_db;

void script_config_read();
void do_init_script(void);
void do_final_script(void);

extern char mapreg_txt[256];

#endif // SCRIPT_HPP
