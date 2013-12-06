#ifndef MAGIC_EXPR_EVAL_HPP
#define MAGIC_EXPR_EVAL_HPP

# include "../strings/zstring.hpp"

# include "../common/utils2.hpp"

# include "magic-interpreter.hpp"

/* Helper definitions for dealing with functions and operations */

int magic_signature_check(ZString opname, ZString funname, ZString signature,
        int args_nr, val_t *args, int line, int column);

void magic_area_rect(map_local **m, int *x, int *y, int *width, int *height,
        area_t& area);

# define ARGINT(x) args[x].v.v_int
# define ARGDIR(x) args[x].v.v_dir
# define ARGSTR(x) ZString(args[x].v.v_string)
# define ARGENTITY(x) args[x].v.v_entity
# define ARGLOCATION(x) args[x].v.v_location
# define ARGAREA(x) args[x].v.v_area
# define ARGSPELL(x) args[x].v.v_spell
# define ARGINVOCATION(x) args[x].v.v_invocation

# define RESULTINT result->v.v_int
# define RESULTDIR result->v.v_dir
# define RESULTSTR result->v.v_string
# define RESULTENTITY result->v.v_entity
# define RESULTLOCATION result->v.v_location
# define RESULTAREA result->v.v_area
# define RESULTSPELL result->v.v_spell
# define RESULTINVOCATION result->v.v_invocation

# define ARG_TYPE(x) args[x].ty
# define ENTITY_TYPE(x) ARGENTITY(x)->bl_type

# define ARGPC(x)  (ARGENTITY(x)->is_player())
# define ARGNPC(x)  (ARGENTITY(x)->is_npc())
# define ARGMOB(x)  (ARGENTITY(x)->is_mob())

# define ARG_MAY_BE_AREA(x) (ARG_TYPE(x) == TYPE::AREA || ARG_TYPE(x) == TYPE::LOCATION)

#endif // MAGIC_EXPR_EVAL_HPP
