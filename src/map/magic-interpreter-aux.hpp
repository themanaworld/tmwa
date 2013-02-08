#ifndef MAGIC_INTERPRETER_AUX_HPP
#define MAGIC_INTERPRETER_AUX_HPP

#include "magic-interpreter.t.hpp"

template<class T>
bool CHECK_TYPE(T *v, TYPE t)
{
    return v->ty == t;
}

// FIXME: macro capture!
#define VAR(i)  \
    ((!env->vars || env->vars[i].ty == TYPE::UNDEF)   \
            ? env->base_env->vars[i]                \
            : env->vars[i])

#endif // MAGIC_INTERPRETER_AUX_HPP
