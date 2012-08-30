#ifndef MAGIC_INTERPRETER_AUX_HPP
#define MAGIC_INTERPRETER_AUX_HPP

#define CHECK_TYPE(v, t) ((v)->ty == t)

#define VAR(i) ((!env->vars || env->vars[i].ty == TY_UNDEF)? env->base_env->vars[i] : env->vars[i])

#endif /* !defined(MAGIC_INTERPRETER_AUX_H_) */
