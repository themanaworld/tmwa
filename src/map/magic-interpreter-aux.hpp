#ifndef TMWA_MAP_MAGIC_INTERPRETER_AUX_HPP
#define TMWA_MAP_MAGIC_INTERPRETER_AUX_HPP

# include "magic-interpreter.t.hpp"

template<class T>
bool CHECK_TYPE(T *v, TYPE t)
{
    return v->ty == t;
}

#endif // TMWA_MAP_MAGIC_INTERPRETER_AUX_HPP
