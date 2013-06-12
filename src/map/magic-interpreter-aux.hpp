#ifndef MAGIC_INTERPRETER_AUX_HPP
#define MAGIC_INTERPRETER_AUX_HPP

#include "magic-interpreter.t.hpp"

template<class T>
bool CHECK_TYPE(T *v, TYPE t)
{
    return v->ty == t;
}

#endif // MAGIC_INTERPRETER_AUX_HPP
