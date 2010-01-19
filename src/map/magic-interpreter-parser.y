%{
#include "magic-interpreter.h"
#include "magic-expr.h"
#include <stdarg.h>

magic_conf_t magic_conf;

static int
intern_id(char *id_name);


static expr_t *
fun_expr(char *name, int args_nr, expr_t **args, int line, int column);

static expr_t *
dot_expr(expr_t *lhs, int id);

#define BIN_EXPR(x, name, arg1, arg2, line, column) { expr_t *e[2]; e[0] = arg1; e[1] = arg2; x = fun_expr(name, 2, e, line, column); }

static int failed_flag = 0;

static void
magic_frontend_error(const char *msg);

static void
fail(int line, int column, char *fmt, ...);

static spell_t *
new_spell(spellguard_t *guard);

static spellguard_t *
spellguard_implication(spellguard_t *a, spellguard_t *b);

static spellguard_t *
new_spellguard(int ty);

static effect_t *
new_effect(int ty);

static effect_t *
set_effect_continuation(effect_t *src, effect_t *continuation);

static void
add_spell(spell_t *spell, int line_nr);

static void
add_teleport_anchor(teleport_anchor_t *anchor, int line_nr);

static effect_t *
op_effect(char *name, int args_nr, expr_t **args, int line, int column);

int
magic_frontend_lex();

static void
install_proc(proc_t *proc);

static effect_t *
call_proc(char *name, int args_nr, expr_t **args, int line_nr, int column);

static void
bind_constant(char *name, val_t *val, int line_nr);

static val_t *
find_constant(char *name);


%}

%name-prefix="magic_frontend_"

%locations

%union {
    int i;
    char *s;
    int op;
    magic_conf_t *magic_conf;
    val_t value;
    expr_t *expr;
    e_location_t location;
    e_area_t area;
    args_rec_t arg_list;
    struct { int letdefs_nr; letdef_t *letdefs; } letdefs;
    spell_t *spell;
    struct { int id, ty; } spellarg_def;
    letdef_t vardef;
    spellguard_t *spellguard;
    component_t *components;
    struct {int id, count; } component;
    effect_t *effect;
    proc_t *proc;
};

%expect 7

%token <i> INT
%token <s> STRING
%token <s> ID
%token <i> DIR

%token '='
%token '<'
%token '>'
%token '+'
%token '-'
%token '*'
%token '/'
%token '%'
%token '@'
%token ','
%token '.'
%token ':'
%token ';'
%token '|'
%token '['
%token ']'
%token '&'
%token '^'

%token CONST
%token PROCEDURE
%token CALL
%token SILENT
%token LOCAL
%token NONMAGIC
%token SHL
%token SHR
%token EQ
%token NEQ
%token GTE
%token LTE
%token ANDAND
%token OROR
%token <s> SCRIPT_DATA
%token TO
%token TOWARDS
%token TELEPORT_ANCHOR
%token SPELL
%token LET
%token IN
%token END
%token DARROW
%token STRING_TY
%token REQUIRE
%token CATALYSTS
%token COMPONENTS
%token MANA
%token CASTTIME
%token SKIP
%token ABORT
%token BREAK
%token EFFECT
%token ATEND
%token ATTRIGGER
%token PC_F
%token NPC_F
%token MOB_F
%token ENTITY_F
%token TARGET_F
%token IF
%token THEN
%token ELSE
%token FOREACH
%token FOR
%token DO
%token SLEEP

%type <value> value
%type <location> location
%type <area> area
%type <arg_list> arg_list
%type <arg_list> arg_list_ne
%type <letdefs> defs
%type <spell> spelldef
%type <spellarg_def> argopt
%type <vardef> def
%type <spellguard> spellbody_list
%type <spellguard> spellbody
%type <spellguard> spellguard
%type <spellguard> spellguard_list
%type <spellguard> prereq
%type <component> item
%type <components> items
%type <components> item_list
%type <i> item_name
%type <i> selection;
%type <effect> effect
%type <effect> effect_list
%type <effect> maybe_trigger
%type <effect> maybe_end
%type <i> spell_flags;

%type <expr> expr
%type <i> arg_ty
%type <proc> proc_formals_list
%type <proc> proc_formals_list_ne

%left OROR
%left ANDAND
%left '<' '>' GTE LTE NEQ EQ
%left '+' '-'
%left '*' '/' '%'
%left SHL SHR '&' '^' '|'
%right '='
%left OR
%left DARROW
%left '.'

%%

spellconf		: /* empty */
                          {}
                        | spellconf_option semicolons spellconf
                          {}
			;


semicolons		: /* empty */
			  {}
                        | semicolons ';'
                          {}
			;


proc_formals_list	: /* empty  */
                          { $$ = aCalloc(sizeof(proc_t), 1); }
			| proc_formals_list_ne
                          { $$ = $1; }
			;

proc_formals_list_ne	: ID
                          { $$ = aCalloc(sizeof(proc_t), 1);
                            $$->args_nr = 1;
                            $$->args = malloc(sizeof(int));
                            $$->args[0] = intern_id($1);
                          }
			| proc_formals_list_ne ',' ID
                          { $$ = $1;
                            $$->args = realloc($$->args, sizeof(int) * (1 + $$->args_nr));
                            $$->args[$$->args_nr++] = intern_id($3);
                          }
			;

spellconf_option	: ID '=' expr
			  {
                            int var_id;
                            if (find_constant($1)) {
                                    fail(@1.first_line, 0, "Attempt to redefine constant `%s' as global\n", $1);
                                    free($1);
                            } else {
                                    var_id = intern_id($1);
                                    magic_eval(&magic_default_env, &magic_conf.vars[var_id], $3);
                            }
                          }
			| CONST ID '=' expr
			  {
                            val_t var;
                            magic_eval(&magic_default_env, &var, $4);
                            bind_constant($2, &var, @1.first_line);
                          }
                        | TELEPORT_ANCHOR ID ':' expr '=' expr
                          {
                              teleport_anchor_t *anchor = calloc(sizeof(teleport_anchor_t), 1);
                              anchor->name = $2;
                              anchor->invocation = magic_eval_str(&magic_default_env, $4);
                              anchor->location = $6;

                              if (!failed_flag)
                                  add_teleport_anchor(anchor, @1.first_line);
                              else
                                  free(anchor);
                              failed_flag = 0;
                          }
			| PROCEDURE ID '(' proc_formals_list ')' '=' effect_list
                          {
                              proc_t *proc = $4;
                              proc->name = $2;
                              proc->body = $7;
                              if (!failed_flag)
                                  install_proc(proc);
                              failed_flag = 0;
                          }
                        | spell_flags SPELL ID argopt ':' expr '=' spelldef
                          { spell_t *spell = $8;
                            spell->name = $3;
                            spell->invocation = magic_eval_str(&magic_default_env, $6);
                            spell->arg = $4.id;
                            spell->spellarg_ty = $4.ty;
                            spell->flags = $1;
                            if (!failed_flag)
                                add_spell(spell, @1.first_line);
                            failed_flag = 0;
                          }

spell_flags		: /* empty */
				{ $$ = 0; }
			| LOCAL spell_flags
                        	{ if ($2 & SPELL_FLAG_LOCAL)
                                        fail(@1.first_line, @1.first_column, "`LOCAL' specified more than once");
                                   $$ = $2 | SPELL_FLAG_LOCAL;
                                }
			| NONMAGIC spell_flags
                        	{ if ($2 & SPELL_FLAG_NONMAGIC)
                                        fail(@1.first_line, @1.first_column, "`NONMAGIC' specified more than once");
                                   $$ = $2 | SPELL_FLAG_NONMAGIC;
                                }
			| SILENT spell_flags
                        	{ if ($2 & SPELL_FLAG_SILENT)
                                        fail(@1.first_line, @1.first_column, "`SILENT' specified more than once");
                                   $$ = $2 | SPELL_FLAG_SILENT;
                                }


argopt			: /* empty */
                          { $$.ty = SPELLARG_NONE; }
                        | '(' ID ':' arg_ty ')'
                          { $$.id = intern_id($2);
                            $$.ty = $4; }
                        ;


arg_ty                  : PC_F
                          { $$ = SPELLARG_PC; }
                        | STRING_TY
                          { $$ = SPELLARG_STRING; }
                        ;


value			: DIR
				{ $$.ty = TY_DIR;
                                  $$.v.v_int = $1; }
			| INT
				{ $$.ty = TY_INT;
                                  $$.v.v_int = $1; }
			| STRING
				{ $$.ty = TY_STRING;
                                  $$.v.v_string = $1; }
			;


expr			: value
				{ $$ = magic_new_expr(EXPR_VAL);
                                  $$->e.e_val = $1; }
			| ID
				{
                                        val_t *val;
                                        if ((val = find_constant($1))) {
                                                $$ = magic_new_expr(EXPR_VAL);
                                                $$->e.e_val = *val;
                                        } else {
                                                $$ = magic_new_expr(EXPR_ID);
                                                $$->e.e_id = intern_id($1);
                                        }
                                }
			| area
				{ $$ = magic_new_expr(EXPR_AREA);
                                  $$->e.e_area = $1; }
			| expr '+' expr
	                        { BIN_EXPR($$, "+", $1, $3, @1.first_line, @1.first_column); }
			| expr '-' expr
	                        { BIN_EXPR($$, "-", $1, $3, @1.first_line, @1.first_column); }
			| expr '*' expr
	                        { BIN_EXPR($$, "*", $1, $3, @1.first_line, @1.first_column); }
			| expr '%' expr
	                        { BIN_EXPR($$, "%", $1, $3, @1.first_line, @1.first_column); }
			| expr '/' expr
	                        { BIN_EXPR($$, "/", $1, $3, @1.first_line, @1.first_column); }
			| expr '<' expr
	                        { BIN_EXPR($$, ">", $3, $1, @1.first_line, @1.first_column); }
			| expr '>' expr
	                        { BIN_EXPR($$, ">", $1, $3, @1.first_line, @1.first_column); }
			| expr '&' expr
	                        { BIN_EXPR($$, "&", $1, $3, @1.first_line, @1.first_column); }
			| expr '^' expr
	                        { BIN_EXPR($$, "^", $1, $3, @1.first_line, @1.first_column); }
			| expr '|' expr
	                        { BIN_EXPR($$, "|", $1, $3, @1.first_line, @1.first_column); }
			| expr SHL expr
	                        { BIN_EXPR($$, "<<", $1, $3, @1.first_line, @1.first_column); }
			| expr SHR expr
	                        { BIN_EXPR($$, ">>", $1, $3, @1.first_line, @1.first_column); }
			| expr LTE expr
	                        { BIN_EXPR($$, ">=", $3, $1, @1.first_line, @1.first_column); }
			| expr GTE expr
	                        { BIN_EXPR($$, ">=", $1, $3, @1.first_line, @1.first_column); }
			| expr ANDAND expr
	                        { BIN_EXPR($$, "&&", $1, $3, @1.first_line, @1.first_column); }
			| expr OROR expr
	                        { BIN_EXPR($$, "||", $1, $3, @1.first_line, @1.first_column); }
			| expr EQ expr
	                        { BIN_EXPR($$, "=", $1, $3, @1.first_line, @1.first_column); }
			| expr '=' expr
	                        { BIN_EXPR($$, "=", $1, $3, @1.first_line, @1.first_column); }
			| expr NEQ expr
	                        { BIN_EXPR($$, "=", $1, $3, @1.first_line, @1.first_column);
                                  $$ = fun_expr("not", 1, &$$, @1.first_line, @1.first_column); }
			| ID '(' arg_list ')'
                        	{ $$ = fun_expr($1, $3.args_nr, $3.args, @1.first_line, @1.first_column);
                                  if ($3.args)
                                          free($3.args);
                                  free($1); }
			| '(' expr ')'
                        	{ $$ = $2; }
			| expr '.' ID
                        	{ $$ = dot_expr($1, intern_id($3)); }
			;

arg_list		: /* empty */
				{ $$.args_nr = 0; }
			| arg_list_ne
				{ $$ = $1 }
			;


arg_list_ne		: expr
				{ $$.args = aCalloc(sizeof(expr_t *), 1);
                                  $$.args_nr = 1;
                                  $$.args[0] = $1;
                                }
			| arg_list_ne ',' expr
                        	{ $$.args = realloc($$.args, (1 + $$.args_nr) * sizeof(expr_t *));
                                  $$.args[$$.args_nr++] = $3;
                                }
			;


location		: '@' '(' expr ',' expr ',' expr ')'
				{ $$.m = $3; $$.x = $5; $$.y = $7; }
			;

area			: location
				{ $$.ty = AREA_LOCATION;
                                  $$.a.a_loc = $1;
				}
			| location '@' '+' '(' expr ',' expr ')'
				{ $$.ty = AREA_RECT;
                                  $$.a.a_rect.loc = $1;
                                  $$.a.a_rect.width = $5;
                                  $$.a.a_rect.height = $7;
                                }
			| location TOWARDS expr ':' '(' expr ',' expr ')'
				{ $$.ty = AREA_BAR;
                                  $$.a.a_bar.loc = $1;
                                  $$.a.a_bar.width = $6;
                                  $$.a.a_bar.depth = $8;
                                  $$.a.a_bar.dir = $3;
                                }
			;


spelldef		: spellbody_list
                                {  $$ = new_spell($1); }
			| LET defs IN spellbody_list
                                {  $$ = new_spell($4); 
                                   $$->letdefs_nr = $2.letdefs_nr;
                                   $$->letdefs = $2.letdefs;
                                   $$->spellguard = $4;
                                }
			;


defs			: semicolons
				{ $$.letdefs_nr = 0;
                                  $$.letdefs = (letdef_t *) malloc(1);
                                }
			| defs def semicolons
	                        { $$ = $1;
                                  $$.letdefs_nr++;
                                  $$.letdefs = realloc($$.letdefs, sizeof(letdef_t) * $$.letdefs_nr);
                                  $$.letdefs[$1.letdefs_nr] = $2;
                                }
                        ;


def			: ID '=' expr
				{
                                        if (find_constant($1)) {
                                                fail(@1.first_line, @1.first_column, "Attempt to re-define constant `%s' as LET-bound variable.\n", $1);
                                                free($1);
                                        } else {
                                                $$.id = intern_id($1);
                                                $$.expr = $3;
                                        }
                                }
			;


spellbody_list		: spellbody
				{ $$ = $1; }
			| spellbody '|' spellbody_list
                        	{ spellguard_t *sg = new_spellguard(SPELLGUARD_CHOICE);
                                  sg->next = $1;
                                  sg->s.s_alt = $3;
                                  $$ = sg;
                                }
			;


spellbody		: spellguard DARROW spellbody
				{ $$ = spellguard_implication($1, $3); }
			| '(' spellbody_list ')'
                        	{ $$ = $2; }
			| EFFECT effect_list maybe_trigger maybe_end
                        	{ spellguard_t *sg = new_spellguard(SPELLGUARD_EFFECT);
                                  sg->s.s_effect.effect = $2;
                                  sg->s.s_effect.at_trigger = $3;
                                  sg->s.s_effect.at_end = $4;
                                  $$ = sg;
                                }
			;


maybe_trigger		: /* empty */
				{ $$ = NULL; }
			| ATTRIGGER effect_list
				{ $$ = $2; }
			;


maybe_end		: /* empty */
				{ $$ = NULL; }
			| ATEND effect_list
				{ $$ = $2; }
			;


spellguard		: prereq
				{ $$ = $1; }
			| spellguard OR spellguard
                        	{ spellguard_t *sg = new_spellguard(SPELLGUARD_CHOICE);
                                  sg->next = $1;
                                  sg->s.s_alt = $3;
                                  $$ = sg;
                                }
			| '(' spellguard_list ')'
                        	{ $$ = $2; }
			;


spellguard_list		: spellguard
				{ $$ = $1; }
			| spellguard ',' spellguard_list
                        	{ $$ = spellguard_implication ($1, $3); }
			;


prereq			: REQUIRE expr
				{ $$ = new_spellguard(SPELLGUARD_CONDITION);
                                  $$->s.s_condition = $2;
                                }
			| CATALYSTS items
				{ $$ = new_spellguard(SPELLGUARD_CATALYSTS);
                                  $$->s.s_catalysts = $2;
                                }
			| COMPONENTS items
				{ $$ = new_spellguard(SPELLGUARD_COMPONENTS);
                                  $$->s.s_components = $2;
                                }
			| MANA expr
				{ $$ = new_spellguard(SPELLGUARD_MANA);
                                  $$->s.s_mana = $2;
                                }
			| CASTTIME expr
				{ $$ = new_spellguard(SPELLGUARD_CASTTIME);
                                  $$->s.s_casttime = $2;
                                }
			;


items			: '[' item_list ']'
				{ $$ = $2; }
			;


item_list		: item
				{ $$ = NULL;
                                  magic_add_component(&$$, $1.id, $1.count);
                                }
			| item_list ',' item
				{ $$ = $1;
                                  magic_add_component(&$$, $3.id, $3.count);
                                }
			;


item			: INT '*' item_name
				{ $$.id = $3; $$.count = $1; }
			| item_name
				{ $$.id = $1; $$.count = 1; }
			;


item_name		: STRING
				{ struct item_data *item = itemdb_searchname($1);
                                  if (!item) {
                                          fail (@1.first_line, @1.first_column, "Unknown item `%s'\n", $1);
                                          $$ = 0;
                                  } else
                                          $$ = item->nameid;
                                  free ($1);
                                }
			| INT
				{ $$ = $1; }
			;


selection		: PC_F
				{ $$ = FOREACH_FILTER_PC; }
			| MOB_F
				{ $$ = FOREACH_FILTER_MOB; }
			| ENTITY_F
				{ $$ = FOREACH_FILTER_ENTITY; }
			| SPELL
				{ $$ = FOREACH_FILTER_SPELL; }
			| TARGET_F
				{ $$ = FOREACH_FILTER_TARGET; }
			| NPC_F
				{ $$ = FOREACH_FILTER_NPC; }
			;


effect			: '(' effect_list ')'
				{ $$ = $2; }
			| SKIP ';'
                        	{ $$ = new_effect(EFFECT_SKIP); }
			| ABORT ';'
                        	{ $$ = new_effect(EFFECT_ABORT); }
			| END ';'
                        	{ $$ = new_effect(EFFECT_END); }
			| BREAK ';'
                        	{ $$ = new_effect(EFFECT_BREAK); }
			| ID '=' expr ';'
                        	{
                                        if (find_constant($1)) {
                                                fail(@1.first_line, @1.first_column, "Attempt to re-define constant `%s' in assignment.", $1);
                                                free($1);
                                        } else {
                                                $$ = new_effect(EFFECT_ASSIGN);
                                                $$->e.e_assign.id = intern_id($1);
                                                $$->e.e_assign.expr = $3;
                                        }
                                }
			| FOREACH selection ID IN expr DO effect
                        	{ $$ = new_effect(EFFECT_FOREACH);
                                  $$->e.e_foreach.id = intern_id($3);
                                  $$->e.e_foreach.area = $5;
                                  $$->e.e_foreach.body = $7;
                                  $$->e.e_foreach.filter = $2;
                                }
			| FOR ID '=' expr TO expr DO effect
                        	{ $$ = new_effect(EFFECT_FOR);
                                  $$->e.e_for.id = intern_id($2);
                                  $$->e.e_for.start = $4;
                                  $$->e.e_for.stop = $6;
                                  $$->e.e_for.body = $8;
                                }
			| IF expr THEN effect ELSE effect
                        	{ $$ = new_effect(EFFECT_IF);
                                  $$->e.e_if.cond = $2;
                                  $$->e.e_if.true_branch = $4;
                                  $$->e.e_if.false_branch = $6;
                                }
			| IF expr THEN effect
                        	{ $$ = new_effect(EFFECT_IF);
                                  $$->e.e_if.cond = $2;
                                  $$->e.e_if.true_branch = $4;
                                  $$->e.e_if.false_branch = new_effect(EFFECT_SKIP);
                                }
			| SLEEP expr ';'
                        	{ $$ = new_effect(EFFECT_SLEEP);
                                  $$->e.e_sleep = $2;
                                }
			| ID '(' arg_list ')' ';'
                                { $$ = op_effect($1, $3.args_nr, $3.args, @1.first_line, @1.first_column);
                                  free($1);
                                }
			| SCRIPT_DATA
                        	{ $$ = new_effect(EFFECT_SCRIPT);
                                  $$->e.e_script = parse_script((unsigned char *) $1, @1.first_line);
                                  free($1);
                                  if ($$->e.e_script == NULL)
                                      fail(@1.first_line, @1.first_column, "Failed to compile script\n");
                                }
			| CALL ID '(' arg_list ')' ';'
                        	{ $$ = call_proc($2, $4.args_nr, $4.args, @1.first_line, @1.first_column);
                                  free($2);
                                }
			;

effect_list		: /* empty */
                        	{ $$ = new_effect(EFFECT_SKIP); }
			| effect semicolons effect_list
                        	{ $$ = set_effect_continuation($1, $3); }
			;
			

%%

/* We do incremental realloc here to store our results.  Since this happens only once
 * during startup for a relatively manageable set of configs, it should be fine. */

static int
intern_id(char *id_name)
{
        int i;

        for (i = 0; i < magic_conf.vars_nr; i++)
                if (!strcmp(id_name, magic_conf.var_name[i])) {
                        free(id_name);
                        return i;
                }

        /* Must add new */
        i = magic_conf.vars_nr++;
        magic_conf.var_name = realloc(magic_conf.var_name, magic_conf.vars_nr * sizeof(char *));
        magic_conf.var_name[i] = id_name;
        magic_conf.vars = realloc(magic_conf.vars, magic_conf.vars_nr * sizeof(val_t));
        magic_conf.vars[i].ty = TY_UNDEF;

        return i;
}

static void
add_spell(spell_t *spell, int line_nr)
{
        int index = magic_conf.spells_nr;
        int i;

        for (i = 0; i < index; i++) {
                if (!strcmp(magic_conf.spells[i]->name, spell->name)) {
                        fail(line_nr, 0, "Attempt to redefine spell `%s'\n", spell->name);
                        return;
                }
                if (!strcmp(magic_conf.spells[i]->invocation, spell->invocation)) {
                        fail(line_nr, 0, "Attempt to redefine spell invocation `%s' between spells `%s' and `%s'\n",
                             spell->invocation, magic_conf.spells[i]->name, spell->name);
                        return;
                }
        }
        magic_conf.spells_nr++;

        magic_conf.spells = realloc(magic_conf.spells, magic_conf.spells_nr * sizeof (spell_t*));
        magic_conf.spells[index] = spell;

            
}

static void
add_teleport_anchor(teleport_anchor_t *anchor, int line_nr)
{
        int index = magic_conf.anchors_nr;
        int i;

        for (i = 0; i < index; i++) {
                if (!strcmp(magic_conf.anchors[i]->name, anchor->name)) {
                        fail(line_nr, 0, "Attempt to redefine teleport anchor `%s'\n", anchor->name);
                        return;
                }
                if (!strcmp(magic_conf.anchors[i]->invocation, anchor->invocation)) {
                        fail(line_nr, 0, "Attempt to redefine anchor invocation `%s' between anchors `%s' and `%s'\n",
                             anchor->invocation, magic_conf.anchors[i]->name, anchor->name);
                        return;
                }
        }
        magic_conf.anchors_nr++;

        magic_conf.anchors = realloc(magic_conf.anchors, magic_conf.anchors_nr * sizeof (teleport_anchor_t*));
        magic_conf.anchors[index] = anchor;
}


static void
fail(int line, int column, char *fmt, ...)
{
        va_list ap;
        fprintf(stderr, "[magic-init]  L%d:%d: ", line, column);
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        failed_flag = 1;
}

static expr_t *
dot_expr(expr_t *expr, int id)
{
        expr_t *retval = magic_new_expr(EXPR_SPELLFIELD);
        retval->e.e_field.id = id;
        retval->e.e_field.expr = expr;

        return retval;
}

static expr_t *
fun_expr(char *name, int args_nr, expr_t **args, int line, int column)
{
        int id;
        expr_t *expr;
        fun_t *fun = magic_get_fun(name, &id);

        if (!fun) {
                fail(line, column, "Unknown function `%s'\n", name);
        } else if (strlen(fun->signature) != args_nr) {
                fail(line, column, "Incorrect number of arguments to function `%s': Expected %d, found %d\n", name, strlen(fun->signature), args_nr);
                fun = NULL;
        }

        if (fun) {
                int i;

                expr = magic_new_expr(EXPR_FUNAPP);
                expr->e.e_funapp.line_nr = line;
                expr->e.e_funapp.column = column;
                expr->e.e_funapp.id = id;
                expr->e.e_funapp.args_nr = args_nr;

                for (i = 0; i < args_nr; i++)
                        expr->e.e_funapp.args[i] = args[i];
        } else { /* failure */
                expr = magic_new_expr(EXPR_VAL);
                expr->e.e_val.ty = TY_FAIL;
        }

        return expr;
}

static spell_t *
new_spell(spellguard_t *guard)
{
        static int spell_counter = 0;

        spell_t *retval = calloc(1, sizeof(spell_t));
        retval->index = ++spell_counter;
        retval->spellguard = guard;
        return retval;
}

static spellguard_t *
new_spellguard(int ty)
{
        spellguard_t *retval = calloc(1, sizeof(spellguard_t));
        retval->ty = ty;
        return retval;
}

static spellguard_t *
spellguard_implication(spellguard_t *a, spellguard_t *b)
{
        spellguard_t *retval = a;

        if (a == b) /* This can happen due to reference sharing:
                     * e.g.,
                     *  (R0 -> (R1 | R2)) => (R3)
                     * yields
                     *  (R0 -> (R1 -> R3 | R2 -> R3))
                     *
                     * So if we now add => R4 to that, we want
                     *  (R0 -> (R1 -> R3 -> R4 | R2 -> R3 -> R4))
                     *
                     * but we only need to add it once, because the R3 reference is shared.
                    */
                return retval;

                /* If the premise is a disjunction, b is the continuation of _all_ branches */
        if (a->ty == SPELLGUARD_CHOICE)
                spellguard_implication(a->s.s_alt, b);
        if (a->next)
                spellguard_implication(a->next, b);
        else
                a->next = b;
                

        return retval;
}

static effect_t *
new_effect(int ty)
{
        effect_t *effect = (effect_t *) calloc(1, sizeof(effect_t));
        effect->ty = ty;
        return effect;
}

static effect_t *
set_effect_continuation(effect_t *src, effect_t *continuation)
{
        effect_t *retval = src;
        /* This function is completely analogous to `spellguard_implication' above; read the control flow implications above first before pondering it. */

        if (src == continuation)
                return retval;

        /* For FOR and FOREACH, we use special stack handlers and thus don't have to set
         * the continuation.  It's only IF that we need to handle in this fashion. */
        if (src->ty == EFFECT_IF) {
                set_effect_continuation(src->e.e_if.true_branch, continuation);
                set_effect_continuation(src->e.e_if.false_branch, continuation);
        }
        if (src->next)
                set_effect_continuation(src->next, continuation);
        else
                src->next = continuation;

        return retval;
}

static effect_t *
op_effect(char *name, int args_nr, expr_t **args, int line, int column)
{
        int id;
        effect_t *effect;
        op_t *op = magic_get_op(name, &id);

        if (!op)
                fail(line, column, "Unknown operation `%s'\n", name);
        else if (strlen(op->signature) != args_nr) {
                fail(line, column, "Incorrect number of arguments to operation `%s': Expected %d, found %d\n", name, strlen(op->signature), args_nr);
                op = NULL;
        }

        if (op) {
                int i;

                effect = new_effect(EFFECT_OP);
                effect->e.e_op.line_nr = line;
                effect->e.e_op.column = column;
                effect->e.e_op.id = id;
                effect->e.e_op.args_nr = args_nr;

                for (i = 0; i < args_nr; i++)
                        effect->e.e_op.args[i] = args[i];
        } else /* failure */
                effect = new_effect(EFFECT_SKIP);

        return effect;
}


proc_t *procs = NULL;
int procs_nr = 0;


static void
install_proc(proc_t *proc)
{
        if (!procs) {
                procs = proc;
                procs_nr = 1;
        } else {
                procs = realloc(procs, sizeof(proc_t) * (1 + procs_nr));
                procs[procs_nr++] = *proc;
        }
}

static effect_t *
call_proc(char *name, int args_nr, expr_t **args, int line_nr, int column)
{
        proc_t *p = NULL;
        int i;
        effect_t *retval;

        for (i = 0; i < procs_nr; i++)
                if (!strcmp(procs[i].name, name)) {
                        p = &procs[i];
                        break;
                }

        if (!p) {
                fail(line_nr, column, "Unknown procedure `%s'\n", name);
                return new_effect(EFFECT_SKIP);
        }

        if (p->args_nr != args_nr) {
                fail(line_nr, column, "Procedure %s/%d invoked with %d parameters\n", name, p->args_nr, args_nr);
                return new_effect(EFFECT_SKIP);
        }

        retval = new_effect(EFFECT_CALL);
        retval->e.e_call.body = p->body;
        retval->e.e_call.args_nr = args_nr;
        retval->e.e_call.formals = p->args;
        retval->e.e_call.actuals = args;
        return retval;
}

struct const_def_rec {
        char *name;
        val_t val;
} *const_defs = NULL;

int const_defs_nr = 0;

static void
bind_constant(char *name, val_t *val, int line_nr)
{
        if (find_constant(name)) {
                fail(line_nr, 0, "Redefinition of constant `%s'\n", name);
                return;
        }

        if (!const_defs)
                const_defs = (struct const_def_rec *)malloc(sizeof(struct const_def_rec));
        else
                const_defs = (struct const_def_rec *)realloc(const_defs,
                                                             (const_defs_nr + 1) * sizeof(struct const_def_rec));

        const_defs[const_defs_nr].name = name;
        const_defs[const_defs_nr].val = *val;
        ++const_defs_nr;
}

static val_t *
find_constant(char *name)
{
        int i;
        for (i = 0; i < const_defs_nr; i++) {
                if (!strcmp(const_defs[i].name, name)) {
                        free(name);
                        return &const_defs[i].val;
                }
        }

        return NULL;
}




#define INTERN_ASSERT(name, id) { int zid = intern_id(name); if (zid != id) fprintf(stderr, "[magic-conf] INTERNAL ERROR: Builtin special var %s interned to %d, not %d as it should be!\n", name, zid, id); error_flag = 1; }

extern FILE *magic_frontend_in;

int
magic_init(char *conffile) // must be called after itemdb initialisation
{
        int error_flag = 0;

        magic_conf.vars_nr = 0;
        magic_conf.var_name = (char **)malloc(1);
        magic_conf.vars = (val_t *)malloc(1);

        magic_conf.obscure_chance = 95;
        magic_conf.min_casttime = 100;

        magic_conf.spells_nr = 0;
        magic_conf.spells = (spell_t **)malloc(1);

        magic_conf.anchors_nr = 0;
        magic_conf.anchors = (teleport_anchor_t **)malloc(1);

        INTERN_ASSERT("min_casttime", VAR_MIN_CASTTIME);
        INTERN_ASSERT("obscure_chance", VAR_OBSCURE_CHANCE);
        INTERN_ASSERT("caster", VAR_CASTER);
        INTERN_ASSERT("spellpower", VAR_SPELLPOWER);
        INTERN_ASSERT("self_spell", VAR_SPELL);
        INTERN_ASSERT("self_invocation", VAR_INVOCATION);
        INTERN_ASSERT("target", VAR_TARGET);
        INTERN_ASSERT("script_target", VAR_SCRIPTTARGET);
        INTERN_ASSERT("location", VAR_LOCATION);

        magic_frontend_in = fopen(conffile, "r");
        if (!magic_frontend_in) {
                fprintf(stderr, "[magic-conf] Magic configuration file `%s' not found -> no magic.\n", conffile);
                return 0;
        }
        magic_frontend_parse();

        if (magic_conf.vars[VAR_MIN_CASTTIME].ty == TY_INT)
                magic_conf.min_casttime = magic_conf.vars[VAR_MIN_CASTTIME].v.v_int;

        if (magic_conf.vars[VAR_OBSCURE_CHANCE].ty == TY_INT)
                magic_conf.obscure_chance = magic_conf.vars[VAR_OBSCURE_CHANCE].v.v_int;

        printf("[magic-conf] Magic initialised; obscure at %d%%.  %d spells, %d teleport anchors.\n",
               magic_conf.obscure_chance, magic_conf.spells_nr, magic_conf.anchors_nr);

        if (procs)
                free(procs);
        return error_flag;
}

extern int magic_frontend_lineno;

static void
magic_frontend_error(const char *msg)
{
    fprintf(stderr, "[magic-conf] Parse error: %s at line %d\n", msg, magic_frontend_lineno);
    failed_flag = 1;
}
