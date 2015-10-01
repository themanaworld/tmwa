#pragma once
//    diagnostics.hpp - List of useful warnings and macros to control them.
//
//    Copyright © 2013-2015 Ben Longbons <b.r.longbons@gmail.com>
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

// Last updated for: GCC 5.2

namespace tmwa
{
// Notes about reading this file:
// * Currently only implemented for C++, but could work in C
// * the HAS_DIAG_ is subject to an additional check for clang
// * because token dispatching, it can't be #define HAS_XXX (GCC >= yyy)
// * gcc 4.6 is required for function scope pragmas
// * when upgrading compiler, diff 'gcc --help=warnings'
//   (Unfortunately this is not good about warnings specific to a single
//   language in some version, which may change, and often lies)
// * clang-specific warning support is incomplete

// List of warnings that require arguments,
// and thus cannot reliably be activated:
//  ???:
//      -Wstrict-aliasing=<1>
//      -Wstrict-overflow=<1>
//  gcc 4.6:
//      -Wlarger-than=<1024>
//      -Wnormalized=<none|id|nfc|nfd>
//      -Wsuggest-attribute=<noreturn,const,pure,format>
//  gcc 4.7:
//      -Wstack-usage=<8192>
//  gcc 5:
//      -Wabi=<8> (but no argument works)
//      -Warray-bounds=<???> (but no argument works)

#ifdef __GNUC__
# ifdef __clang__
#  define GCC (0)
#  define CLANG (1) // clang's versions are vendor-specific - stupid apple
# else
#  define GCC (__GNUC__ * 100 + __GNUC_MINOR__)
#  define CLANG (0)
# endif
#endif
#define GCC_PATCH (GCC * 100 + __GNUC_PATCHLEVEL__)

#if GCC >= 406 || CLANG
# define PRAGMA(x) _Pragma(#x) static_assert(1, "I like my semicolons")
#else
# define PRAGMA(x) static_assert(1, "pragmas not available, can't do: " #x)
#endif

#define DIAG_E(tag) DO_DIAG_IF(HAS_DIAG_##tag)(error, DIAG_##tag)
#define DIAG_W(tag) DO_DIAG_IF(HAS_DIAG_##tag)(error, DIAG_##tag)
#define DIAG_I(tag) DO_DIAG_IF(HAS_DIAG_##tag)(ignored, DIAG_##tag)
#define DIAG_X(tag) DO_DIAG_IF(HAS_DIAG_##tag)(ignored, DIAG_##tag)

#define DO_DIAG_IF(c) JOIN_PLEASE(PRAGMA_IF_,c)
#define JOIN_PLEASE(a, b) a##b
#define PRAGMA_IF_0(lvl, str) static_assert(1, "warning " #str " not enabled for this compiler version, sorry")
#if CLANG
# define PRAGMA_IF_1(lvl, str) DO_DIAG_IF2(__has_warning(str))(lvl, str)
# define DO_DIAG_IF2(s) JOIN_PLEASE(PRAGMA_IF2_, s)
# define PRAGMA_IF2_0(lvl, str) static_assert(1, "warning " #str " not available in this compiler version, sorry")
# define PRAGMA_IF2_1(lvl, str) PRAGMA(GCC diagnostic lvl str)
#else
# define PRAGMA_IF_1(lvl, str) PRAGMA(GCC diagnostic lvl str)
#endif

#define DIAG_PUSH() PRAGMA(GCC diagnostic push)
#define DIAG_POP() PRAGMA(GCC diagnostic pop)


/// (with no argument) Warn about things that will change when compiling
/// with an ABI-compliant compiler
/// (with an argmuent) Warn about things that change between the current
/// -fabi-version and the specified version
// see note about -fabi-version=6 in the makefile
#define DIAG_abi "-Wabi"
#if 1
# define HAS_DIAG_abi 1
#else
# define HAS_DIAG_abi 0
#endif

/// Warn if a subobject has an abi_tag attribute that
/// the complete object type does not have
#define DIAG_abi_tag "-Wabi-tag"
#if GCC >= 408
# define HAS_DIAG_abi_tag 1
#else
# define HAS_DIAG_abi_tag 0
#endif

/// Warn about suspicious uses of memory addresses
#define DIAG_address "-Waddress"
#if 1
# define HAS_DIAG_address 1
#else
# define HAS_DIAG_address 0
#endif

/// Warn about returning structures, unions or arrays
#define DIAG_aggregate_return "-Waggregate-return"
#if 1
# define HAS_DIAG_aggregate_return 1
#else
# define HAS_DIAG_aggregate_return 0
#endif

/// Warn if an array is accessed out of bounds
#define DIAG_array_bounds "-Warray-bounds"
#if 1
# define HAS_DIAG_array_bounds 1
#else
# define HAS_DIAG_array_bounds 0
#endif

/// Warn about inappropriate attribute usage
#define DIAG_attributes "-Wattributes"
#if 1
# define HAS_DIAG_attributes 1
#else
# define HAS_DIAG_attributes 0
#endif

/// Warn about boolean expression compared with an
/// integer value different from true/false
#define DIAG_bool_compare "-Wbool-compare"
#if GCC >= 500
# define HAS_DIAG_bool_compare 1
#else
# define HAS_DIAG_bool_compare 0
#endif

/// Warn when a built-in preprocessor macro is
/// undefined or redefined
#define DIAG_builtin_macro_redefined "-Wbuiltin-macro-redefined"
#if 1
# define HAS_DIAG_builtin_macro_redefined 1
#else
# define HAS_DIAG_builtin_macro_redefined 0
#endif

/// Warn about C++ constructs whose meaning differs
/// between ISO C++ 1998 and ISO C++ 2011
#if CLANG || GCC >= 407
# define DIAG_cxx0x_compat "-Wc++11-compat"
# define DIAG_cxx11_compat "-Wc++11-compat"
#else
# define DIAG_cxx0x_compat "-Wc++0x-compat"
# define DIAG_cxx11_compat "-Wc++0x-compat"
#endif
#if 1
# define HAS_DIAG_cxx0x_compat 1
# define HAS_DIAG_cxx11_compat 1
#else
# define HAS_DIAG_cxx0x_compat 0
# define HAS_DIAG_cxx11_compat 0
#endif

/// Warn about C++ constructs whose meaning differs
/// between ISO C++ 2011 and ISO C++ 2014
#define DIAG_cxx14_compat "-Wc++14-compat"
#if GCC >= 500
# define HAS_DIAG_cxx14_compat 1
#else
# define HAS_DIAG_cxx14_compat 0
#endif

// I care about whether my code compiles with the standard as implemented
// by certain compilers, not whether it matches with an *exact* standard.
#define DIAG_cxx1y_extensions "-Wc++1y-extensions"
#if CLANG
# define HAS_DIAG_cxx1y_extensions 1
#else
# define HAS_DIAG_cxx1y_extensions 0
#endif

/// Warn about pointer casts which increase alignment
#define DIAG_cast_align "-Wcast-align"
#if 1
# define HAS_DIAG_cast_align 1
#else
# define HAS_DIAG_cast_align 0
#endif

/// Warn about casts which discard qualifiers
#define DIAG_cast_qual "-Wcast-qual"
#if 1
# define HAS_DIAG_cast_qual 1
#else
# define HAS_DIAG_cast_qual 0
#endif

/// Warn about subscripts whose type is "char"
#define DIAG_char_subscripts "-Wchar-subscripts"
#if 1
# define HAS_DIAG_char_subscripts 1
#else
# define HAS_DIAG_char_subscripts 0
#endif

/// Warn about memory access errors found by Pointer
/// Bounds Checker
#define DIAG_chkp "-Wchkp"
#if GCC >= 500
# define HAS_DIAG_chkp 1
#else
# define HAS_DIAG_chkp 0
#endif

/// Warn about variables that might be changed by
/// "longjmp" or "vfork"
#define DIAG_clobbered "-Wclobbered"
#if GCC
# define HAS_DIAG_clobbered 1
#else
# define HAS_DIAG_clobbered 0
#endif

/// Warn about possibly nested block comments, and
/// C++ comments spanning more than one physical line
#define DIAG_comment "-Wcomment"
#if 1
# define HAS_DIAG_comment 1
#else
# define HAS_DIAG_comment 0
#endif

/// Warn for conditionally-supported constructs
#define DIAG_conditionally_supported "-Wconditionally-supported"
#if GCC >= 409
# define HAS_DIAG_conditionally_supported 1
#else
# define HAS_DIAG_conditionally_supported 0
#endif

// A fixable difference between c++11 and c++14
#define DIAG_constexpr_not_const "-Wconstexpr-not-const"
#if CLANG
# define HAS_DIAG_constexpr_not_const 1
#else
# define HAS_DIAG_constexpr_not_const 0
#endif

/// Warn for implicit type conversions that may
/// change a value
#define DIAG_conversion "-Wconversion"
#if 1
# define HAS_DIAG_conversion 1
#else
# define HAS_DIAG_conversion 0
#endif

/// Warn for converting NULL from/to a non-pointer
/// type
#define DIAG_conversion_null "-Wconversion-null"
#if 1
# define HAS_DIAG_conversion_null 1
#else
# define HAS_DIAG_conversion_null 0
#endif

/// Warn in case profiles in -fprofile-use do not
/// match
#define DIAG_coverage_mismatch "-Wcoverage-mismatch"
#if GCC
# define HAS_DIAG_coverage_mismatch 1
#else
# define HAS_DIAG_coverage_mismatch 0
#endif

///
#define DIAG_covered_switch_default "-Wcovered-switch-default"
#if CLANG
# define HAS_DIAG_covered_switch_default 1
#else
# define HAS_DIAG_covered_switch_default 0
#endif

/// Warn when a #warning directive is encountered
#define DIAG_cpp "-Wcpp"
#if GCC
# define HAS_DIAG_cpp 1
#else
# define HAS_DIAG_cpp 0
#endif

/// Warn when all constructors and destructors are
/// private
#define DIAG_ctor_dtor_privacy "-Wctor-dtor-privacy"
#if 1
# define HAS_DIAG_ctor_dtor_privacy 1
#else
# define HAS_DIAG_ctor_dtor_privacy 0
#endif

/// Warn about __TIME__, __DATE__ and __TIMESTAMP__
/// usage
#define DIAG_date_time "-Wdate-time"
#if GCC >= 409
# define HAS_DIAG_date_time 1
#else
# define HAS_DIAG_date_time 0
#endif

/// Warn when deleting a pointer to incomplete type
#define DIAG_delete_incomplete "-Wdelete-incomplete"
#if GCC >= 409
# define HAS_DIAG_delete_incomplete 1
#else
# define HAS_DIAG_delete_incomplete 0
#endif

/// Warn about deleting polymorphic objects with non-
/// virtual destructors
#define DIAG_delete_non_virtual_dtor "-Wdelete-non-virtual-dtor"
#if GCC >= 407
# define HAS_DIAG_delete_non_virtual_dtor 1
#else
# define HAS_DIAG_delete_non_virtual_dtor 0
#endif

/// Warn if a deprecated compiler feature, class,
/// method, or field is used
#define DIAG_deprecated "-Wdeprecated"
#if 1
# define HAS_DIAG_deprecated 1
#else
# define HAS_DIAG_deprecated 0
#endif

/// Warn about uses of __attribute__((deprecated)")
/// declarations
#define DIAG_deprecated_declarations "-Wdeprecated-declarations"
#if 1
# define HAS_DIAG_deprecated_declarations 1
#else
# define HAS_DIAG_deprecated_declarations 0
#endif

/// Warn when an optimization pass is disabled
#define DIAG_disabled_optimization "-Wdisabled-optimization"
#if 1
# define HAS_DIAG_disabled_optimization 1
#else
# define HAS_DIAG_disabled_optimization 0
#endif

/// Warn about compile-time integer division by zero
#define DIAG_div_by_zero "-Wdiv-by-zero"
#if 1
# define HAS_DIAG_div_by_zero 1
#else
# define HAS_DIAG_div_by_zero 0
#endif

///
#define DIAG_documentation "-Wdocumentation"
#if CLANG
# define HAS_DIAG_documentation 1
#else
# define HAS_DIAG_documentation 0
#endif

/// Warn about implicit conversions from "float" to
/// "double"
#define DIAG_double_promotion "-Wdouble-promotion"
#if GCC
# define HAS_DIAG_double_promotion 1
#else
# define HAS_DIAG_double_promotion 0
#endif

/// Warn about violations of Effective C++ style rules
#define DIAG_effcxx "-Weffc++"
#if 1
# define HAS_DIAG_effcxx 1
#else
# define HAS_DIAG_effcxx 0
#endif

/// Warn about an empty body in an if or else
/// statement
#define DIAG_empty_body "-Wempty-body"
#if 1
# define HAS_DIAG_empty_body 1
#else
# define HAS_DIAG_empty_body 0
#endif

/// Warn about stray tokens after #elif and #endif
#define DIAG_endif_labels "-Wendif-labels"
#if 1
# define HAS_DIAG_endif_labels 1
#else
# define HAS_DIAG_endif_labels 0
#endif

/// Warn about comparison of different enum types
#define DIAG_enum_compare "-Wenum-compare"
#if 1
# define HAS_DIAG_enum_compare 1
#else
# define HAS_DIAG_enum_compare 0
#endif

///
#define DIAG_extra_semi "-Wextra-semi"
#if CLANG
# define HAS_DIAG_extra_semi 1
#else
# define HAS_DIAG_extra_semi 0
#endif

/// Warn if testing floating point numbers for
/// equality
#define DIAG_float_equal "-Wfloat-equal"
#if 1
# define HAS_DIAG_float_equal 1
#else
# define HAS_DIAG_float_equal 0
#endif

/// Warn about printf/scanf/strftime/strfmon format
/// string anomalies
// see below
#define DIAG_format "-Wformat"
#if GCC
# define HAS_DIAG_format 1
#else
# define HAS_DIAG_format 0
#endif
// but gcc 4.8 warns on %ms, since we enabled -Wpedantic.
//WG48("-Wformat")

/// Warn about format strings that contain NUL bytes
#define DIAG_format_contains_nul "-Wformat-contains-nul"
#if GCC
# define HAS_DIAG_format_contains_nul 1
#else
# define HAS_DIAG_format_contains_nul 0
#endif

/// Warn if passing too many arguments to a function
/// for its format string
#define DIAG_format_extra_args "-Wformat-extra-args"
#if 1
# define HAS_DIAG_format_extra_args 1
#else
# define HAS_DIAG_format_extra_args 0
#endif

/// Warn about format strings that are not literals
// Available in clang, but not smart enough to handle constexpr.
#define DIAG_format_nonliteral "-Wformat-nonliteral"
#if GCC || (CLANG && 1)
# define HAS_DIAG_format_nonliteral 1
#else
# define HAS_DIAG_format_nonliteral 0
#endif

/// Warn about possible security problems with format
/// functions
// Same.
#define DIAG_format_security "-Wformat-security"
#if GCC || (CLANG && 1)
# define HAS_DIAG_format_security 1
#else
# define HAS_DIAG_format_security 0
#endif

/// Warn about sign differences with format functions
#define DIAG_format_signedness "-Wformat-signedness"
#if GCC >= 500
# define HAS_DIAG_format_signedness 1
#else
# define HAS_DIAG_format_signedness 0
#endif

/// Warn about strftime formats yielding 2-digit years
#define DIAG_format_y2k "-Wformat-y2k"
#if 1
# define HAS_DIAG_format_y2k 1
#else
# define HAS_DIAG_format_y2k 0
#endif

/// Warn about zero-length formats
#define DIAG_format_zero_length "-Wformat-zero-length"
#if 1
# define HAS_DIAG_format_zero_length 1
#else
# define HAS_DIAG_format_zero_length 0
#endif

/// Warn when attempting to free a non-heap object
#define DIAG_free_nonheap_object "-Wfree-nonheap-object"
#if GCC >= 407
# define HAS_DIAG_free_nonheap_object 1
#else
# define HAS_DIAG_free_nonheap_object 0
#endif

// -Wgnu is a clang alias for -Wpedantic

// Foo{x: y}
#define DIAG_gnu_designator "-Wgnu-designator"
#if CLANG
# define HAS_DIAG_gnu_designator 1
#else
# define HAS_DIAG_gnu_designator 0
#endif


/// Warn whenever type qualifiers are ignored.
#define DIAG_ignored_qualifiers "-Wignored-qualifiers"
#if 1
# define HAS_DIAG_ignored_qualifiers 1
#else
# define HAS_DIAG_ignored_qualifiers 0
#endif

///
#define DIAG_implicit_fallthrough "-Wimplicit-fallthrough"
#if CLANG
# define HAS_DIAG_implicit_fallthrough 1
#else
# define HAS_DIAG_implicit_fallthrough 0
#endif

/// Warn about C++11 inheriting constructors when the
/// base has a variadic constructor
#define DIAG_inherited_variadic_ctor "-Winherited-variadic-ctor"
#if GCC >= 408
# define HAS_DIAG_inherited_variadic_ctor 1
#else
# define HAS_DIAG_inherited_variadic_ctor 0
#endif

/// Warn about variables which are initialized to
/// themselves
#define DIAG_init_self "-Winit-self"
#if 1
# define HAS_DIAG_init_self 1
#else
# define HAS_DIAG_init_self 0
#endif

/// Warn when an inlined function cannot be inlined
#define DIAG_inline "-Winline"
#if 1
# define HAS_DIAG_inline 1
#else
# define HAS_DIAG_inline 0
#endif

/// Warn when there is a cast to a pointer from an
/// integer of a different size
#define DIAG_int_to_pointer_cast "-Wint-to-pointer-cast"
#if 1
# define HAS_DIAG_int_to_pointer_cast 1
#else
# define HAS_DIAG_int_to_pointer_cast 0
#endif

/// Warn when an atomic memory model parameter is
/// known to be outside the valid range.
#define DIAG_invalid_memory_model "-Winvalid-memory-model"
#if GCC >= 407
# define HAS_DIAG_invalid_memory_model 1
#else
# define HAS_DIAG_invalid_memory_model 0
#endif

/// Warn about invalid uses of the "offsetof" macro
#define DIAG_invalid_offsetof "-Winvalid-offsetof"
#if 1
# define HAS_DIAG_invalid_offsetof 1
#else
# define HAS_DIAG_invalid_offsetof 0
#endif

/// Warn about PCH files that are found but not used
#define DIAG_invalid_pch "-Winvalid-pch"
#if 1
# define HAS_DIAG_invalid_pch 1
#else
# define HAS_DIAG_invalid_pch 0
#endif

/// Warn when a string or character literal is
/// followed by a ud-suffix which does not begin with
/// an underscore.
#define DIAG_literal_suffix "-Wliteral-suffix"
#if GCC >= 408
# define HAS_DIAG_literal_suffix 1
#else
# define HAS_DIAG_literal_suffix 0
#endif

/// Warn when logical not is used on the left hand
/// side operand of a comparison
#define DIAG_logical_not_parentheses "-Wlogical-not-parentheses"
#if GCC >= 500
# define HAS_DIAG_logical_not_parentheses 1
#else
# define HAS_DIAG_logical_not_parentheses 0
#endif

/// Warn when a logical operator is suspiciously
/// always evaluating to true or false
#define DIAG_logical_op "-Wlogical-op"
#if GCC
# define HAS_DIAG_logical_op 1
#else
# define HAS_DIAG_logical_op 0
#endif

/// Do not warn about using "long long" when -pedantic
#define DIAG_long_long "-Wlong-long"
#if 1
# define HAS_DIAG_long_long 1
#else
# define HAS_DIAG_long_long 0
#endif

/// Warn about suspicious declarations of "main"
#define DIAG_main "-Wmain"
#if 1
# define HAS_DIAG_main 1
#else
# define HAS_DIAG_main 0
#endif

/// Warn about maybe uninitialized automatic variables
#define DIAG_maybe_uninitialized "-Wmaybe-uninitialized"
// buggy in 4.7 with tmwa::Option<>
#if GCC >= 408
# define HAS_DIAG_maybe_uninitialized 1
#else
# define HAS_DIAG_maybe_uninitialized 0
#endif

// bitch about 'struct Foo' vs 'class Foo'
#define DIAG_mismatched_tags "-Wmismatched-tags"
#if CLANG
# define HAS_DIAG_mismatched_tags 1
#else
# define HAS_DIAG_mismatched_tags 0
#endif

/// Warn about possibly missing braces around
/// initializers
// beware of things like std::array!
#define DIAG_missing_braces "-Wmissing-braces"
#if 1
# define HAS_DIAG_missing_braces 1
#else
# define HAS_DIAG_missing_braces 0
#endif

/// Warn about global functions without previous
/// declarations
// This doesn't work for clang, it wants -Wmissing-prototypes instead.
#define DIAG_missing_declarations "-Wmissing-declarations"
#if 1
# define HAS_DIAG_missing_declarations 1
#else
# define HAS_DIAG_missing_declarations 0
#endif

/// Warn about missing fields in struct initializers
// Actually supported by GCC, but gives warnings when I don't want, e.g.:
// Foo foo = {};
#define DIAG_missing_field_initializers "-Wmissing-field-initializers"
#if CLANG || (GCC && 1)
# define HAS_DIAG_missing_field_initializers 1
#else
# define HAS_DIAG_missing_field_initializers 0
#endif

/// Warn about functions which might be candidates
/// for format attributes
#define DIAG_missing_format_attribute "-Wmissing-format-attribute"
#if 1
# define HAS_DIAG_missing_format_attribute 1
#else
# define HAS_DIAG_missing_format_attribute 0
#endif

/// Warn about user-specified include directories
/// that do not exist
#define DIAG_missing_include_dirs "-Wmissing-include-dirs"
#if 1
# define HAS_DIAG_missing_include_dirs 1
#else
# define HAS_DIAG_missing_include_dirs 0
#endif

/// Warn about functions which might be candidates
/// for __attribute__((noreturn)")
#define DIAG_missing_noreturn "-Wmissing-noreturn"
#if 1
# define HAS_DIAG_missing_noreturn 1
#else
# define HAS_DIAG_missing_noreturn 0
#endif

// clang uses this instead of -Wmissing-declarations
#define DIAG_missing_prototypes "-Wmissing-prototypes"
#if CLANG
# define HAS_DIAG_missing_prototypes 1
#else
# define HAS_DIAG_missing_prototypes 0
#endif

///
// like -Wmissing-declarations but for variables instead of functions
#define DIAG_missing_variable_declarations "-Wmissing-variable-declarations"
#if CLANG
# define HAS_DIAG_missing_variable_declarations 1
#else
# define HAS_DIAG_missing_variable_declarations 0
#endif

/// Warn about constructs not instrumented by
/// -fmudflap
#define DIAG_mudflap "-Wmudflap"
#if GCC
# define HAS_DIAG_mudflap 1
#else
# define HAS_DIAG_mudflap 0
#endif

/// Warn about use of multi-character character
/// constants
#define DIAG_multichar "-Wmultichar"
#if 1
# define HAS_DIAG_multichar 1
#else
# define HAS_DIAG_multichar 0
#endif

/// Warn about narrowing conversions within { } that
/// are ill-formed in C++11
#define DIAG_narrowing "-Wnarrowing"
#if GCC >= 407
# define HAS_DIAG_narrowing 1
#else
# define HAS_DIAG_narrowing 0
#endif

/// Warn when a noexcept expression evaluates to
/// false even though the expression can't actually
/// throw
#define DIAG_noexcept "-Wnoexcept"
#if GCC
# define HAS_DIAG_noexcept 1
#else
# define HAS_DIAG_noexcept 0
#endif

/// Warn when non-templatized friend functions are
/// declared within a template
#define DIAG_non_template_friend "-Wnon-template-friend"
#if GCC
# define HAS_DIAG_non_template_friend 1
#else
# define HAS_DIAG_non_template_friend 0
#endif

/// Warn about non-virtual destructors
#define DIAG_non_virtual_dtor "-Wnon-virtual-dtor"
#if 1
# define HAS_DIAG_non_virtual_dtor 1
#else
# define HAS_DIAG_non_virtual_dtor 0
#endif

/// Warn about NULL being passed to argument slots
/// marked as requiring non-NULL
#define DIAG_nonnull "-Wnonnull"
#if 1
# define HAS_DIAG_nonnull 1
#else
# define HAS_DIAG_nonnull 0
#endif

///
#define DIAG_null_conversion "-Wnull-conversion"
#if CLANG
# define HAS_DIAG_null_conversion 1
#else
# define HAS_DIAG_null_conversion 0
#endif

/// Warn about some C++ One Definition Rule
/// violations during link time optimization
#define DIAG_odr "-Wodr"
#if GCC >= 500
# define HAS_DIAG_odr 1
#else
# define HAS_DIAG_odr 0
#endif

/// Warn if a C-style cast is used in a program
#define DIAG_old_style_cast "-Wold-style-cast"
#if 1
# define HAS_DIAG_old_style_cast 1
#else
# define HAS_DIAG_old_style_cast 0
#endif

/// Warn about overflow in arithmetic expressions
#define DIAG_overflow "-Woverflow"
#if 1
# define HAS_DIAG_overflow 1
#else
# define HAS_DIAG_overflow 0
#endif

/// Warn if a simd directive is overridden by the
/// vectorizer cost model
#define DIAG_openmp_simd "-Wopenmp-simd"
#if GCC >= 409
# define HAS_DIAG_openmp_simd 1
#else
# define HAS_DIAG_openmp_simd 0
#endif

/// Warn if a string is longer than the maximum
/// portable length specified by the standard
//X("-Woverlength-strings")

/// Warn about overloaded virtual function names
#define DIAG_overloaded_virtual "-Woverloaded-virtual"
#if 1
# define HAS_DIAG_overloaded_virtual 1
#else
# define HAS_DIAG_overloaded_virtual 0
#endif

/// Warn when the packed attribute has no effect on
/// struct layout
#define DIAG_packed "-Wpacked"
#if 1
# define HAS_DIAG_packed 1
#else
# define HAS_DIAG_packed 0
#endif

/// Warn about packed bit-fields whose offset changed
/// in GCC 4.4
#define DIAG_packed_bitfield_compat "-Wpacked-bitfield-compat"
#if GCC
# define HAS_DIAG_packed_bitfield_compat 1
#else
# define HAS_DIAG_packed_bitfield_compat 0
#endif

/// Warn when padding is required to align structure
/// members
#define DIAG_padded "-Wpadded"
#if 1
# define HAS_DIAG_padded 1
#else
# define HAS_DIAG_padded 0
#endif

/// Warn about possibly missing parentheses
#define DIAG_parentheses "-Wparentheses"
#if 1
# define HAS_DIAG_parentheses 1
#else
# define HAS_DIAG_parentheses 0
#endif

/// Issue warnings needed for strict compliance to
/// the standard
// a bit too noisy
//EG48("-Wpedantic")
// lots of minor extensions are used
#define DIAG_pedantic "-Wpedantic"
#if GCC >= 408 || CLANG
# define HAS_DIAG_pedantic 1
#else
# define HAS_DIAG_pedantic 0
#endif

/// Warn when converting the type of pointers to
/// member functions
#define DIAG_pmf_conversions "-Wpmf-conversions"
#if GCC
# define HAS_DIAG_pmf_conversions 1
#else
# define HAS_DIAG_pmf_conversions 0
#endif

/// Warn about function pointer arithmetic
#define DIAG_pointer_arith "-Wpointer-arith"
#if 1
# define HAS_DIAG_pointer_arith 1
#else
# define HAS_DIAG_pointer_arith 0
#endif

/// Warn about misuses of pragmas
#define DIAG_pragmas "-Wpragmas"
#if GCC
# define HAS_DIAG_pragmas 1
#else
# define HAS_DIAG_pragmas 0
#endif

/// Warn about multiple declarations of the same
/// object
#define DIAG_redundant_decls "-Wredundant-decls"
#if 1
# define HAS_DIAG_redundant_decls 1
#else
# define HAS_DIAG_redundant_decls 0
#endif

/// Warn when the compiler reorders code
#define DIAG_reorder "-Wreorder"
#if 1
# define HAS_DIAG_reorder 1
#else
# define HAS_DIAG_reorder 0
#endif

/// Warn about returning a pointer/reference to a
/// local or temporary variable.
#define DIAG_return_local_addr "-Wreturn-local-addr"
#if GCC >= 408
# define HAS_DIAG_return_local_addr 1
#else
# define HAS_DIAG_return_local_addr 0
#endif

/// Warn whenever a function's return type defaults
/// to "int" (C), or about inconsistent return types
/// (C++")
#define DIAG_return_type "-Wreturn-type"
#if 1
# define HAS_DIAG_return_type 1
#else
# define HAS_DIAG_return_type 0
#endif

/// Warn about possible violations of sequence point
/// rules
#define DIAG_sequence_point "-Wsequence-point"
#if 1
# define HAS_DIAG_sequence_point 1
#else
# define HAS_DIAG_sequence_point 0
#endif

/// Warn when one local variable shadows another
#define DIAG_shadow "-Wshadow"
#if 1
# define HAS_DIAG_shadow 1
#else
# define HAS_DIAG_shadow 0
#endif

/// Warn if a local declaration hides an instance
/// variable
#define DIAG_shadow_ivar "-Wshadow-ivar"
#if GCC >= 500
# define HAS_DIAG_shadow_ivar 1
#else
# define HAS_DIAG_shadow_ivar 0
#endif

/// Warn if shift count is negative
#define DIAG_shift_count_negative "-Wshift-count-negative"
#if GCC >= 500
# define HAS_DIAG_shift_count_negative 1
#else
# define HAS_DIAG_shift_count_negative 0
#endif

/// Warn if shift count >= width of type
#define DIAG_shift_count_overflow "-Wshift-count-overflow"
#if GCC >= 500
# define HAS_DIAG_shift_count_overflow 1
#else
# define HAS_DIAG_shift_count_overflow 0
#endif

/// Warn about signed-unsigned comparisons
#define DIAG_sign_compare "-Wsign-compare"
#if 1
# define HAS_DIAG_sign_compare 1
#else
# define HAS_DIAG_sign_compare 0
#endif

/// Warn when overload promotes from unsigned to
/// signed
#define DIAG_sign_promo "-Wsign-promo"
#if 1
# define HAS_DIAG_sign_promo 1
#else
# define HAS_DIAG_sign_promo 0
#endif

/// Warn about missing sized deallocation functions
#define DIAG_sized_deallocation "-Wsized-deallocation"
#if GCC >= 500
# define HAS_DIAG_sized_deallocation 1
#else
# define HAS_DIAG_sized_deallocation 0
#endif

/// Warn when sizeof is applied on a parameter
/// declared as an array
#define DIAG_sizeof_array_argument "-Wsizeof-array-argument"
#if GCC >= 500
# define HAS_DIAG_sizeof_array_argument 1
#else
# define HAS_DIAG_sizeof_array_argument 0
#endif

/// Warn about suspicious length parameters to
/// certain string functions if the argument uses
/// sizeof
#define DIAG_sizeof_pointer_memaccess "-Wsizeof-pointer-memaccess"
#if GCC >= 408
# define HAS_DIAG_sizeof_pointer_memaccess 1
#else
# define HAS_DIAG_sizeof_pointer_memaccess 0
#endif

/// Warn when not issuing stack smashing protection
/// for some reason
#define DIAG_stack_protector "-Wstack-protector"
#if 1
# define HAS_DIAG_stack_protector 1
#else
# define HAS_DIAG_stack_protector 0
#endif

/// Warn about code which might break strict aliasing
/// rules
#define DIAG_strict_aliasing "-Wstrict-aliasing"
#if 1
# define HAS_DIAG_strict_aliasing 1
#else
# define HAS_DIAG_strict_aliasing 0
#endif

/// Warn about uncasted NULL used as sentinel
#define DIAG_strict_null_sentinel "-Wstrict-null-sentinel"
#if GCC
# define HAS_DIAG_strict_null_sentinel 1
#else
# define HAS_DIAG_strict_null_sentinel 0
#endif

/// Warn about optimizations that assume that signed
/// overflow is undefined
#define DIAG_strict_overflow "-Wstrict-overflow"
#if 1
# define HAS_DIAG_strict_overflow 1
#else
# define HAS_DIAG_strict_overflow 0
#endif

/// Warn about C++ virtual methods where adding final
/// keyword would improve code quality
#define DIAG_suggest_final_methods "-Wsuggest-final-methods"
#if GCC >= 500
# define HAS_DIAG_suggest_final_methods 1
#else
# define HAS_DIAG_suggest_final_methods 0
#endif

/// Warn about C++ polymorphic types where adding
/// final keyword would improve code quality
#define DIAG_suggest_final_types "-Wsuggest-final-types"
#if GCC >= 500
# define HAS_DIAG_suggest_final_types 1
#else
# define HAS_DIAG_suggest_final_types 0
#endif

/// Suggest that the override keyword be used when
/// the declaration of a virtual function overrides
/// another.
#define DIAG_suggest_override "-Wsuggest-override"
#if GCC >= 500
# define HAS_DIAG_suggest_override 1
#else
# define HAS_DIAG_suggest_override 0
#endif

/// Warn about enumerated switches, with no default,
/// missing a case
#define DIAG_switch "-Wswitch"
#if 1
# define HAS_DIAG_switch 1
#else
# define HAS_DIAG_switch 0
#endif

/// Warn about switches with boolean controlling
/// expression
#define DIAG_switch_bool "-Wswitch-bool"
#if GCC >= 500
# define HAS_DIAG_switch_bool 1
#else
# define HAS_DIAG_switch_bool 0
#endif

/// Warn about enumerated switches missing a
/// "default:" statement
#define DIAG_switch_default "-Wswitch-default"
#if 1
# define HAS_DIAG_switch_default 1
#else
# define HAS_DIAG_switch_default 0
#endif

/// Warn about all enumerated switches missing a
/// specific case
#define DIAG_switch_enum "-Wswitch-enum"
#if 1
# define HAS_DIAG_switch_enum 1
#else
# define HAS_DIAG_switch_enum 0
#endif

/// Warn when __sync_fetch_and_nand and
/// __sync_nand_and_fetch built-in functions are used
#define DIAG_sync_nand "-Wsync-nand"
#if GCC
# define HAS_DIAG_sync_nand 1
#else
# define HAS_DIAG_sync_nand 0
#endif

/// Warn whenever a trampoline is generated
#define DIAG_trampolines "-Wtrampolines"
#if GCC
# define HAS_DIAG_trampolines 1
#else
# define HAS_DIAG_trampolines 0
#endif

/// Warn if trigraphs are encountered that might
/// affect the meaning of the program
#define DIAG_trigraphs "-Wtrigraphs"
#if 1
# define HAS_DIAG_trigraphs 1
#else
# define HAS_DIAG_trigraphs 0
#endif

/// Warn if a comparison is always true or always
/// false due to the limited range of the data type
#define DIAG_type_limits "-Wtype-limits"
#if 1
# define HAS_DIAG_type_limits 1
#else
# define HAS_DIAG_type_limits 0
#endif

/// Warn if an undefined macro is used in an #if
/// directive
#define DIAG_undef "-Wundef"
#if 1
# define HAS_DIAG_undef 1
#else
# define HAS_DIAG_undef 0
#endif

/// Warn about uninitialized automatic variables
#define DIAG_uninitialized "-Wuninitialized"
#if 1
# define HAS_DIAG_uninitialized 1
#else
# define HAS_DIAG_uninitialized 0
#endif

/// Warn about unrecognized pragmas
#define DIAG_unknown_pragmas "-Wunknown-pragmas"
#if 1
# define HAS_DIAG_unknown_pragmas 1
#else
# define HAS_DIAG_unknown_pragmas 0
#endif

///
// Not an error because of some remaining enum+default
#define DIAG_unreachable_code "-Wunreachable-code"
#if CLANG
# define HAS_DIAG_unreachable_code 1
#else
# define HAS_DIAG_unreachable_code 0
#endif

/// Warn if the loop cannot be optimized due to
/// nontrivial assumptions.
#define DIAG_unsafe_loop_optimizations "-Wunsafe-loop-optimizations"
#if GCC
# define HAS_DIAG_unsafe_loop_optimizations 1
#else
# define HAS_DIAG_unsafe_loop_optimizations 0
#endif

/// Warn when a function parameter is only set,
/// otherwise unused
#define DIAG_unused_but_set_parameter "-Wunused-but-set-parameter"
#if GCC
# define HAS_DIAG_unused_but_set_parameter 1
#else
# define HAS_DIAG_unused_but_set_parameter 0
#endif

/// Warn when a variable is only set, otherwise unused
#define DIAG_unused_but_set_variable "-Wunused-but-set-variable"
#if GCC
# define HAS_DIAG_unused_but_set_variable 1
#else
# define HAS_DIAG_unused_but_set_variable 0
#endif

/// Warn when a function is unused
#define DIAG_unused_function "-Wunused-function"
#if 1
# define HAS_DIAG_unused_function 1
#else
# define HAS_DIAG_unused_function 0
#endif

/// Warn when a label is unused
#define DIAG_unused_label "-Wunused-label"
#if 1
# define HAS_DIAG_unused_label 1
#else
# define HAS_DIAG_unused_label 0
#endif

/// Warn when typedefs locally defined in a function
/// are not used
#define DIAG_unused_local_typedefs "-Wunused-local-typedefs"
#if GCC >= 407
# define HAS_DIAG_unused_local_typedefs 1
#else
# define HAS_DIAG_unused_local_typedefs 0
#endif

/// Warn about macros defined in the main file that
/// are not used
#define DIAG_unused_macros "-Wunused-macros"
#if 1
# define HAS_DIAG_unused_macros 1
#else
# define HAS_DIAG_unused_macros 0
#endif

/// Warn when a function parameter is unused
#define DIAG_unused_parameter "-Wunused-parameter"
#if 1
# define HAS_DIAG_unused_parameter 1
#else
# define HAS_DIAG_unused_parameter 0
#endif

/// Warn if a caller of a function, marked with
/// attribute warn_unused_result, does not use its
/// return value
#define DIAG_unused_result "-Wunused-result"
#if 1
# define HAS_DIAG_unused_result 1
#else
# define HAS_DIAG_unused_result 0
#endif

/// Warn when an expression value is unused
#define DIAG_unused_value "-Wunused-value"
#if 1
# define HAS_DIAG_unused_value 1
#else
# define HAS_DIAG_unused_value 0
#endif

/// Warn when a variable is unused
#define DIAG_unused_variable "-Wunused-variable"
#if 1
# define HAS_DIAG_unused_variable 1
#else
# define HAS_DIAG_unused_variable 0
#endif

/// Warn about useless casts
#define DIAG_useless_cast "-Wuseless-cast"
#if GCC >= 408
# define HAS_DIAG_useless_cast 1
#else
# define HAS_DIAG_useless_cast 0
#endif

/// Warn about questionable usage of the macros used
/// to retrieve variable arguments
#define DIAG_varargs "-Wvarargs"
#if GCC >= 408
# define HAS_DIAG_varargs 1
#else
# define HAS_DIAG_varargs 0
#endif

/// Warn about using variadic macros
#define DIAG_variadic_macros "-Wvariadic-macros"
#if 1
# define HAS_DIAG_variadic_macros 1
#else
# define HAS_DIAG_variadic_macros 0
#endif

/// Warn when a vector operation is compiled
/// outside the SIMD
#define DIAG_vector_operation_performance "-Wvector-operation-performance"
#if GCC >= 407
# define HAS_DIAG_vector_operation_performance 1
#else
# define HAS_DIAG_vector_operation_performance 0
#endif

/// Warn if a virtual base has a non-trivial move
/// assignment operator
#define DIAG_virtual_move_assign "-Wvirtual-move-assign"
#if GCC >= 408
# define HAS_DIAG_virtual_move_assign 1
#else
# define HAS_DIAG_virtual_move_assign 0
#endif

/// Warn if a variable length array is used
#define DIAG_vla "-Wvla"
#if 1
# define HAS_DIAG_vla 1
#else
# define HAS_DIAG_vla 0
#endif

/// Warn when a register variable is declared volatile
#define DIAG_volatile_register_var "-Wvolatile-register-var"
#if 1
# define HAS_DIAG_volatile_register_var 1
#else
# define HAS_DIAG_volatile_register_var 0
#endif

/// In C++, nonzero means warn about deprecated
/// conversion from string literals to 'char *'.  In
/// C, similar warning, except that the conversion is
/// of course not deprecated by the ISO C standard.
#define DIAG_write_strings "-Wwrite-strings"
#if 1
# define HAS_DIAG_write_strings 1
#else
# define HAS_DIAG_write_strings 0
#endif

/// Warn when a literal '0' is used as null
/// pointer
#define DIAG_zero_as_null_pointer_constant "-Wzero-as-null-pointer-constant"
#if GCC >= 407
# define HAS_DIAG_zero_as_null_pointer_constant 1
#else
# define HAS_DIAG_zero_as_null_pointer_constant 0
#endif
} // namespace tmwa
