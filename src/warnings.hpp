#pragma once
// This is the first file in every compilation, passed by the makefile.
// This file contains only preprocessor directions.
// The preceding sentence is a lie.
//    warnings.hpp - Make the compiler do the hard work.
//
//    Copyright © 2013 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

// just mention "fwd.hpp" to make formatter happy

#include "diagnostics.hpp"


namespace tmwa
{
PRAGMA(GCC diagnostic warning "-Wall");
PRAGMA(GCC diagnostic warning "-Wextra");
PRAGMA(GCC diagnostic warning "-Wunused");
PRAGMA(GCC diagnostic warning "-Wformat");

DIAG_E(abi);
DIAG_W(abi_tag);
DIAG_E(address);
DIAG_I(aggregate_return);
DIAG_E(array_bounds);
DIAG_E(attributes);
DIAG_E(builtin_macro_redefined);
DIAG_I(cxx0x_compat);
DIAG_I(cxx1y_extensions);
DIAG_E(cast_align);
DIAG_E(cast_qual);
DIAG_E(char_subscripts);
DIAG_E(clobbered);
DIAG_E(comment);
DIAG_E(conditionally_supported);
DIAG_E(constexpr_not_const);
DIAG_X(conversion);
DIAG_E(conversion_null);
DIAG_W(coverage_mismatch);
DIAG_X(covered_switch_default);
DIAG_W(cpp);
DIAG_E(ctor_dtor_privacy);
DIAG_E(date_time);
DIAG_E(delete_incomplete);
DIAG_E(delete_non_virtual_dtor);
DIAG_W(deprecated);
#ifdef QUIET
DIAG_I(deprecated_declarations);
#else
DIAG_W(deprecated_declarations);
#endif
DIAG_W(disabled_optimization);
DIAG_E(div_by_zero);
DIAG_W(documentation);
DIAG_I(double_promotion);
DIAG_I(effcxx);
DIAG_E(empty_body);
DIAG_E(endif_labels);
DIAG_E(enum_compare);
DIAG_E(extra_semi);
DIAG_E(float_equal);
DIAG_E(format);
DIAG_E(format_contains_nul);
DIAG_E(format_extra_args);
#if CLANG
DIAG_I(format_nonliteral);
DIAG_I(format_security);
#else
DIAG_E(format_nonliteral);
DIAG_E(format_security);
#endif
DIAG_E(format_y2k);
DIAG_I(format_zero_length);
DIAG_E(free_nonheap_object);
DIAG_E(gnu_designator);
DIAG_E(ignored_qualifiers);
DIAG_E(implicit_fallthrough);
DIAG_W(inherited_variadic_ctor);
DIAG_E(init_self);
DIAG_X(inline);
DIAG_E(int_to_pointer_cast);
DIAG_W(invalid_memory_model);
DIAG_E(invalid_offsetof);
DIAG_E(invalid_pch);
DIAG_W(literal_suffix);
DIAG_W(logical_op);
DIAG_I(long_long);
DIAG_E(main);
DIAG_E(maybe_uninitialized);
DIAG_I(mismatched_tags);
DIAG_E(missing_braces);
DIAG_E(missing_declarations);
#if GCC
DIAG_I(missing_field_initializers);
#else
DIAG_E(missing_field_initializers);
#endif
DIAG_E(missing_format_attribute);
DIAG_E(missing_include_dirs);
DIAG_W(missing_noreturn);
DIAG_E(missing_prototypes);
#ifndef GTEST_HAS_PTHREAD // this is a hack
DIAG_E(missing_variable_declarations);
#else
DIAG_I(missing_variable_declarations);
#endif
DIAG_E(mudflap);
DIAG_E(multichar);
DIAG_E(narrowing);
DIAG_W(noexcept);
DIAG_E(non_template_friend);
DIAG_E(non_virtual_dtor);
DIAG_E(nonnull);
DIAG_E(null_conversion);
DIAG_E(old_style_cast);
DIAG_W(overflow);
DIAG_E(openmp_simd);
DIAG_E(overloaded_virtual);
DIAG_E(packed);
DIAG_W(packed_bitfield_compat);
DIAG_I(padded);
DIAG_E(parentheses);
DIAG_I(pedantic);
DIAG_E(pmf_conversions);
DIAG_E(pointer_arith);
DIAG_E(pragmas);
DIAG_W(redundant_decls);
DIAG_E(reorder);
DIAG_W(return_local_addr);
DIAG_E(return_type);
DIAG_E(sequence_point);
DIAG_E(shadow);
DIAG_X(sign_compare);
DIAG_E(sign_promo);
DIAG_W(sizeof_pointer_memaccess);
DIAG_X(stack_protector);
DIAG_E(strict_aliasing);
DIAG_W(strict_null_sentinel);
DIAG_X(strict_overflow);
DIAG_I(switch);
DIAG_I(switch_default);
DIAG_I(switch_enum);
DIAG_W(sync_nand);
DIAG_E(trampolines);
DIAG_E(trigraphs);
DIAG_E(type_limits);
DIAG_E(undef);
DIAG_E(uninitialized);
DIAG_E(unknown_pragmas);
DIAG_W(unreachable_code);
DIAG_X(unsafe_loop_optimizations);
DIAG_E(unused_but_set_parameter);
DIAG_E(unused_but_set_variable);
DIAG_E(unused_function);
DIAG_E(unused_label);
DIAG_E(unused_local_typedefs);
DIAG_W(unused_macros);
DIAG_E(unused_parameter);
DIAG_E(unused_result);
DIAG_E(unused_value);
DIAG_E(unused_variable);
DIAG_E(useless_cast);
DIAG_E(varargs);
DIAG_W(variadic_macros);
DIAG_W(vector_operation_performance);
DIAG_E(virtual_move_assign);
DIAG_I(vla);
DIAG_E(volatile_register_var);
DIAG_E(write_strings);
#ifndef GTEST_HAS_PTHREAD // this is a hack
DIAG_E(zero_as_null_pointer_constant);
#else
DIAG_I(zero_as_null_pointer_constant);
#endif
} // namespace tmwa
