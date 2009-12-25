#ifndef MAGIC_EXPR_H_
#define MAGIC_EXPR_H_
#include "magic-interpreter.h"
#include "magic-interpreter-aux.h"

#ifndef MAX
#  define MAX(x,y) (((x)>(y)) ? (x) : (y))
#endif
#ifndef MIN
#  define MIN(x,y) (((x)<(y)) ? (x) : (y))
#endif

#ifndef INT_MAX
#  define INT_MAX (1<<30)       // It's more than that, but this is quite sufficient for our purposes.
#endif

/*
 * Argument types:
 *  i : int
 *  d : dir
 *  s : string
 *  e : entity
 *  l : location
 *  a : area
 *  S : spell
 *  I : invocation
 *  . : any, except for fail/undef
 *  _ : any, including fail, but not undef
 */
typedef struct fun
{
    char *name;
    char *signature;
    char ret_ty;
    int  (*fun) (env_t * env, int args_nr, val_t * result, val_t * args);
} fun_t;

typedef struct op
{
    char *name;
    char *signature;
    int  (*op) (env_t * env, int args_nr, val_t * args);
} op_t;

/**
 * Retrieves a function by name
 * @param name The name to look up
 * @return A function of that name, or NULL, and a function index
 */
fun_t *magic_get_fun (char *name, int *index);

/**
 * Retrieves an operation by name
 * @param name The name to look up
 * @return An operation of that name, or NULL, and a function index
 */
op_t *magic_get_op (char *name, int *index);

/**
 * Evaluates an expression and stores the result in `dest'
 */
void magic_eval (env_t * env, val_t * dest, expr_t * expr);

/**
 * Evaluates an expression and coerces the result into an integer
 */
int  magic_eval_int (env_t * env, expr_t * expr);

/**
 * Evaluates an expression and coerces the result into a string
 */
char *magic_eval_str (env_t * env, expr_t * expr);

int  map_is_solid (int m, int x, int y);

expr_t *magic_new_expr (int ty);

void magic_clear_var (val_t * v);

void magic_copy_var (val_t * dest, val_t * src);

void magic_random_location (location_t * dest, area_t * area);

int                             // ret -1: not a string, ret 1: no such item, ret 0: OK




 magic_find_item (val_t * args, int index, struct item *item, int *stackable);

#define GET_ARG_ITEM(index, dest, stackable) switch(magic_find_item(args, index, &dest, &stackable)) { case -1 : return 1; case 1 : return 0; }

int  magic_location_in_area (int m, int x, int y, area_t * area);

#endif /* !defined(MAGIC_EXPR_H_) */
