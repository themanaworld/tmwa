// $Id: script.h,v 1.2 2004/09/25 05:32:19 MouseJstr Exp $
#ifndef SCRIPT_HPP
#define SCRIPT_HPP

struct script_data
{
    int  type;
    union
    {
        int  num;
        char *str;
    } u;
};

struct script_stack
{
    int  sp, sp_max;
    struct script_data *stack_data;
};
struct script_state
{
    struct script_stack *stack;
    int  start, end;
    int  pos, state;
    int  rid, oid;
    char *script, *new_script;
    int  defsp, new_pos, new_defsp;
};

unsigned char *parse_script (unsigned char *, int);
typedef struct argrec
{
    char *name;
    union
    {
        int  i;
        char *s;
    } v;
} argrec_t;
int  run_script_l (unsigned char *, int, int, int, int, argrec_t * args);
int  run_script (unsigned char *, int, int, int);

struct dbt *script_get_label_db (void);
struct dbt *script_get_userfunc_db (void);

int  script_config_read (char *cfgName);
int  do_init_script (void);
int  do_final_script (void);

extern char mapreg_txt[];

#endif
