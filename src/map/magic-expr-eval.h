#ifndef MAGIC_EXPR_EVAL
#define MAGIC_EXPR_EVAL

/* Helper definitions for dealing with functions and operations */

static int heading_x[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
static int heading_y[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };

int
 magic_signature_check (char *opname, char *funname, char *signature,
                        int args_nr, val_t * args, int line, int column);

void
magic_area_rect (int *m, int *x, int *y, int *width, int *height,
                 area_t * area);

#define ARGINT(x) args[x].v.v_int
#define ARGDIR(x) args[x].v.v_int
#define ARGSTR(x) args[x].v.v_string
#define ARGENTITY(x) args[x].v.v_entity
#define ARGLOCATION(x) args[x].v.v_location
#define ARGAREA(x) args[x].v.v_area
#define ARGSPELL(x) args[x].v.v_spell
#define ARGINVOCATION(x) args[x].v.v_invocation

#define RESULTINT result->v.v_int
#define RESULTDIR result->v.v_int
#define RESULTSTR result->v.v_string
#define RESULTENTITY result->v.v_entity
#define RESULTLOCATION result->v.v_location
#define RESULTAREA result->v.v_area
#define RESULTSPELL result->v.v_spell
#define RESULTINVOCATION result->v.v_invocation

#define TY(x) args[x].ty
#define ETY(x) ARGENTITY(x)->type

#define ARGPC(x)  ((struct map_session_data *)ARGENTITY(x))
#define ARGNPC(x)  ((struct map_session_data *)ARGENTITY(x))
#define ARGMOB(x)  ((struct map_session_data *)ARGENTITY(x))

#define ARG_MAY_BE_AREA(x) (TY(x) == TY_AREA || TY(x) == TY_LOCATION)

#endif /* !defined(MAGIC_EXPR_EVAL) */
