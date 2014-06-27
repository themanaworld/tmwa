// no include guards
// This is the first file in every compilation, passed by the makefile.
// This file contains only preprocessor directions.

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

// This file is currently targeted at:
// GCC 4.6 (incomplete due to bugs)
// GCC 4.7 (for few minor workarounds)
// GCC 4.8 (zarro boogs found)
// GCC 4.9 (zarro boogs found)
// clang 3.1 (may ICE later)
// clang 3.2 (with a few major workarounds)

// List of warnings that require arguments,
// and thus cannot reliably be activated:
//  gcc 4.6:
//      -Wlarger-than=<1024>
//      -Wnormalized=<id|nfc|nfd>
//      -Wsuggest-attribute=<noreturn,const,pure,format>
//  gcc 4.7:
//      -Wstack-usage=<8192>
//  ???
//      -Wstrict-aliasing=<1>
//      -Wstrict-overflow=<1>

// options that enable other options
// only warnings so I can catch the errors
#pragma GCC diagnostic warning "-Wall"
#pragma GCC diagnostic warning "-Wextra"
#pragma GCC diagnostic warning "-Wunused"
#pragma GCC diagnostic warning "-Wformat"

#ifdef __clang__
# if __clang_major__ < 3
#  error "your clang is way too old"
# elif __clang_major__ == 3
#  if __clang_minor__ < 1
#   error "your clang is too old"
#  endif // __clang_minor__
# endif // __clang_major__
#else // __clang__
# if __GNUC__ < 4
#  error "your gcc is way too old"
#  if __GNUC_MINOR__ < 6
#   error "your gcc is too old"
#  elif __GNUC_MINOR__ == 6
#   if __GNUC_PATCHLEVEL__ < 3
#    error "TODO: test this patchlevel"
#   endif // __GNUC_PATCHLEVEL__
#  elif __GNUC_MINOR__ == 7
#   if __GNUC_PATCHLEVEL__ < 2
#    error "your gcc has a known bad patchlevel"
#   endif // __GNUC_PATCHLEVEL__
#  endif // __GNUC_MINOR__
# endif // __GNUC__
#endif // __clang__

// BEGIN Macros to make my life easier

// stringification requirement - #sw within #ar
// this is a lie ^
#define P(ar) _Pragma(#ar)

// Use "GCC diagnostic" for warnings applicable to all versions.
#define I(sw) P(GCC diagnostic ignored sw)
#define W(sw) P(GCC diagnostic warning sw)
#define E(sw) P(GCC diagnostic error sw)
// configurable thing (also change in clang below!)
#define X(sw) I(sw)


#ifdef __clang__

// Use "clang diagnostic" for warnings specific to clang
# define IC(sw) P(clang diagnostic ignored sw)
# define WC(sw) P(clang diagnostic warning sw)
# define EC(sw) P(clang diagnostic error sw)
# define XC(sw) IC(sw) // this is below

// warning specific to gcc
# define IG(sw) static_assert('I', sw "skipped for clang");
# define WG(sw) static_assert('W', sw "skipped for clang");
# define EG(sw) static_assert('E', sw "skipped for clang");
# define XG(sw) static_assert('X', sw "skipped for clang");

# define IG47(sw) static_assert('I', sw "only for gcc 4.7+");
# define WG47(sw) static_assert('W', sw "only for gcc 4.7+");
# define EG47(sw) static_assert('E', sw "only for gcc 4.7+");
# define XG47(sw) static_assert('X', sw "only for gcc 4.7+");

# define IG48(sw) static_assert('I', sw "only for gcc 4.8+");
# define WG48(sw) static_assert('W', sw "only for gcc 4.8+");
# define EG48(sw) static_assert('E', sw "only for gcc 4.8+");
# define XG48(sw) static_assert('X', sw "only for gcc 4.8+");

# define IG49(sw) static_assert('I', sw "only for gcc 4.9+");
# define WG49(sw) static_assert('W', sw "only for gcc 4.9+");
# define EG49(sw) static_assert('E', sw "only for gcc 4.9+");
# define XG49(sw) static_assert('X', sw "only for gcc 4.9+");

# define I47(sw) I(sw)
# define W47(sw) W(sw)
# define E47(sw) E(sw)
# define X47(sw) X(sw)

# define I48(sw) I(sw)
# define W48(sw) W(sw)
# define E48(sw) E(sw)
# define X48(sw) X(sw)

# define I49(sw) I(sw)
# define W49(sw) W(sw)
# define E49(sw) E(sw)
# define X49(sw) X(sw)

#else

// warnings specific to clang
# define IC(sw) static_assert('I', sw "skipped for gcc");
# define WC(sw) static_assert('W', sw "skipped for gcc");
# define EC(sw) static_assert('E', sw "skipped for gcc");
# define XC(sw) static_assert('X', sw "skipped for gcc");

// warnings specific to gcc
# define IG(sw) I(sw)
# define WG(sw) W(sw)
# define EG(sw) E(sw)
# define XG(sw) X(sw)

// used both for warnings not implemented in a version
// and for warnings that falsely trigger
# if __GNUC__ == 4
#  if __GNUC_MINOR__ >= 7
#   define IG47(sw) IG(sw)
#   define WG47(sw) WG(sw)
#   define EG47(sw) EG(sw)
#   define XG47(sw) XG(sw)

#   define I47(sw) I(sw)
#   define W47(sw) W(sw)
#   define E47(sw) E(sw)
#   define X47(sw) X(sw)
#  else
#   define IG47(sw) static_assert('I', sw "only for gcc 4.7+");
#   define WG47(sw) static_assert('W', sw "only for gcc 4.7+");
#   define EG47(sw) static_assert('E', sw "only for gcc 4.7+");
#   define XG47(sw) static_assert('X', sw "only for gcc 4.7+");

#   define I47(sw) static_assert('I', sw "only for gcc 4.7+ or clang");
#   define W47(sw) static_assert('W', sw "only for gcc 4.7+ or clang");
#   define E47(sw) static_assert('E', sw "only for gcc 4.7+ or clang");
#   define X47(sw) static_assert('X', sw "only for gcc 4.7+ or clang");
#  endif // __GNUC_MINOR__
#  if __GNUC_MINOR__ >= 8
#   define IG48(sw) IG(sw)
#   define WG48(sw) WG(sw)
#   define EG48(sw) EG(sw)
#   define XG48(sw) XG(sw)

#   define I48(sw) IG(sw)
#   define W48(sw) WG(sw)
#   define E48(sw) EG(sw)
#   define X48(sw) XG(sw)
#  else
#   define IG48(sw) static_assert('I', sw "only for gcc 4.8+");
#   define WG48(sw) static_assert('W', sw "only for gcc 4.8+");
#   define EG48(sw) static_assert('E', sw "only for gcc 4.8+");
#   define XG48(sw) static_assert('X', sw "only for gcc 4.8+");

#   define I48(sw) static_assert('I', sw "only for gcc 4.8+ or clang");
#   define W48(sw) static_assert('W', sw "only for gcc 4.8+ or clang");
#   define E48(sw) static_assert('E', sw "only for gcc 4.8+ or clang");
#   define X48(sw) static_assert('X', sw "only for gcc 4.8+ or clang");
#  endif // __GNUC_MINOR__
#  if __GNUC_MINOR__ >= 9
#   define IG49(sw) IG(sw)
#   define WG49(sw) WG(sw)
#   define EG49(sw) EG(sw)
#   define XG49(sw) XG(sw)

#   define I49(sw) IG(sw)
#   define W49(sw) WG(sw)
#   define E49(sw) EG(sw)
#   define X49(sw) XG(sw)
#  else
#   define IG49(sw) static_assert('I', sw "only for gcc 4.9+");
#   define WG49(sw) static_assert('W', sw "only for gcc 4.9+");
#   define EG49(sw) static_assert('E', sw "only for gcc 4.9+");
#   define XG49(sw) static_assert('X', sw "only for gcc 4.9+");

#   define I49(sw) static_assert('I', sw "only for gcc 4.9+ or clang");
#   define W49(sw) static_assert('W', sw "only for gcc 4.9+ or clang");
#   define E49(sw) static_assert('E', sw "only for gcc 4.9+ or clang");
#   define X49(sw) static_assert('X', sw "only for gcc 4.9+ or clang");
#  endif // __GNUC_MINOR__
# endif // __GNUC__
#endif // __clang__

// END macros to make my life easier


/// Warn about things that will change when compiling
/// with an ABI-compliant compiler
// see note about -fabi-version=6 in the makefile
E("-Wabi")

/// Warn if a subobject has an abi_tag attribute that
/// the complete object type does not have
WG48("-Wabi-tag")

/// Warn about suspicious uses of memory addresses
E("-Waddress")

/// Warn about returning structures, unions or arrays
I("-Waggregate-return")

/// Warn if an array is accessed out of bounds
E("-Warray-bounds")

/// Warn about inappropriate attribute usage
E("-Wattributes")

/// Warn when a built-in preprocessor macro is
// undefined or redefined
E("-Wbuiltin-macro-redefined")

/// Warn about C++ constructs whose meaning differs
/// between ISO C++ 1998 and ISO C++ 2011
// This has gone funky lately. It probably doesn't do anything useful anyway.
//E("-Wc++0x-compat")
//W("-Wc++11-compat")
I("-Wc++0x-compat")

// I care about whether my code compiles with the standard as implemented
// by certain compilers, not whether it matches with an *exact* standard.
#ifdef __clang__
# if __has_warning("-Wc++1y-extensions")
IC("-Wc++1y-extensions")
# else
static_assert('E', "-Wc++1y-extensions not in this clang version");
# endif
#else
static_assert('E', "-Wc++1y-extensions not in GCC");
#endif

/// Warn about pointer casts which increase alignment
E("-Wcast-align")

/// Warn about casts which discard qualifiers
E("-Wcast-qual")

/// Warn about subscripts whose type is "char"
E("-Wchar-subscripts")

/// Warn about variables that might be changed by
/// "longjmp" or "vfork"
EG("-Wclobbered")

/// Warn about possibly nested block comments, and
/// C++ comments spanning more than one physical line
E("-Wcomment")

/// Warn for conditionally-supported constructs
EG49("-Wconditionally-supported")

// A fixable difference between c++11 and c++14
#ifdef __clang__
# if __has_warning("-Wconstexpr-not-const")
EC("-Wconstexpr-not-const")
# else
static_assert('E', "-Wconstexpr-not-const not in this clang version");
# endif
#else
static_assert('E', "-Wconstexpr-not-const not in GCC");
#endif

/// Warn for implicit type conversions that may
/// change a value
X("-Wconversion")

/// Warn for converting NULL from/to a non-pointer
/// type
E("-Wconversion-null")

/// Warn in case profiles in -fprofile-use do not
/// match
WG("-Wcoverage-mismatch")

///
EC("-Wcovered-switch-default")

/// Warn when a #warning directive is encountered
WG("-Wcpp")

/// Warn when all constructors and destructors are
/// private
E("-Wctor-dtor-privacy")

/// Warn about __TIME__, __DATE__ and __TIMESTAMP__
/// usage
EG49("-Wdate-time")

/// Warn when deleting a pointer to incomplete type
EG49("-Wdelete-incomplete")

/// Warn about deleting polymorphic objects with non-
/// virtual destructors
E47("-Wdelete-non-virtual-dtor")

/// Warn if a deprecated compiler feature, class,
/// method, or field is used
W("-Wdeprecated")

/// Warn about uses of __attribute__((deprecated)")
/// declarations
W("-Wdeprecated-declarations")
#ifdef QUIET
I("-Wdeprecated-declarations")
#endif

/// Warn when an optimization pass is disabled
W("-Wdisabled-optimization")

/// Warn about compile-time integer division by zero
E("-Wdiv-by-zero")

///
WC("-Wdocumentation")

/// Warn about implicit conversions from "float" to
/// "double"
IG("-Wdouble-promotion")

/// Warn about violations of Effective C++ style rules
I("-Weffc++")

/// Warn about an empty body in an if or else
/// statement
E("-Wempty-body")

/// Warn about stray tokens after #elif and #endif
E("-Wendif-labels")

/// Warn about comparison of different enum types
E("-Wenum-compare")

///
EC("-Wextra-semi")

/// Warn if testing floating point numbers for
/// equality
E("-Wfloat-equal")

/// Warn about printf/scanf/strftime/strfmon format
/// string anomalies
// see below
EG("-Wformat")
// but gcc 4.8 warns on %ms, since we enabled -Wpedantic.
//WG48("-Wformat")

/// Warn about format strings that contain NUL bytes
EG("-Wformat-contains-nul")

/// Warn if passing too many arguments to a function
/// for its format string
E("-Wformat-extra-args")

/// Warn about format strings that are not literals
EG("-Wformat-nonliteral")
// Available in clang, but not smart enough to handle constexpr.
IC("-Wformat-nonliteral")

/// Warn about possible security problems with format
/// functions
EG("-Wformat-security")
// Same.
IC("-Wformat-security")

/// Warn about strftime formats yielding 2-digit years
E("-Wformat-y2k")

/// Warn about zero-length formats
I("-Wformat-zero-length")

/// Warn when attempting to free a non-heap object
EG47("-Wfree-nonheap-object")

// -Wgnu is a clang alias for -Wpedantic

// Foo{x: y}
EC("-Wgnu-designator")


/// Warn whenever type qualifiers are ignored.
E("-Wignored-qualifiers")

///
EC("-Wimplicit-fallthrough")

/// Warn about C++11 inheriting constructors when the
/// base has a variadic constructor
WG48("-Winherited-variadic-ctor")

/// Warn about variables which are initialized to
/// themselves
E("-Winit-self")

/// Warn when an inlined function cannot be inlined
X("-Winline")

/// Warn when there is a cast to a pointer from an
/// integer of a different size
E("-Wint-to-pointer-cast")

/// Warn when an atomic memory model parameter is
/// known to be outside the valid range.
WG47("-Winvalid-memory-model")

/// Warn about invalid uses of the "offsetof" macro
E("-Winvalid-offsetof")

/// Warn about PCH files that are found but not used
E("-Winvalid-pch")

/// Warn when a string or character literal is
/// followed by a ud-suffix which does not begin with
/// an underscore.
WG48("-Wliteral-suffix")

/// Warn when a logical operator is suspiciously
/// always evaluating to true or false
WG("-Wlogical-op")

/// Do not warn about using "long long" when -pedantic
I("-Wlong-long")

/// Warn about suspicious declarations of "main"
E("-Wmain")

/// Warn about maybe uninitialized automatic variables
EG47("-Wmaybe-uninitialized")

// bitch about 'struct Foo' vs 'class Foo'
IC("-Wmismatched-tags")

/// Warn about possibly missing braces around
/// initializers
// beware of things like std::array!
E("-Wmissing-braces")

/// Warn about global functions without previous
/// declarations
// This doesn't work for clang, it wants -Wmissing-prototypes instead.
E("-Wmissing-declarations")

/// Warn about missing fields in struct initializers
// Actually supported by GCC, but gives warnings when I don't want, e.g.:
// Foo foo = {};
EC("-Wmissing-field-initializers")
IG("-Wmissing-field-initializers")

/// Warn about functions which might be candidates
/// for format attributes
E("-Wmissing-format-attribute")

/// Warn about user-specified include directories
/// that do not exist
E("-Wmissing-include-dirs")

/// Warn about functions which might be candidates
/// for __attribute__((noreturn)")
W("-Wmissing-noreturn")

// clang uses this instead of -Wmissing-declarations
EC("-Wmissing-prototypes")

///
// like -Wmissing-declarations but for variables instead of functions
#ifndef GTEST_HAS_PTHREAD // this is a hack
EC("-Wmissing-variable-declarations")
#endif

/// Warn about constructs not instrumented by
/// -fmudflap
EG("-Wmudflap")

/// Warn about use of multi-character character
/// constants
E("-Wmultichar")

/// Warn about narrowing conversions within { } that
/// are ill-formed in C++11
EG47("-Wnarrowing")

/// Warn when a noexcept expression evaluates to
/// false even though the expression can't actually
/// throw
WG("-Wnoexcept")

/// Warn when non-templatized friend functions are
/// declared within a template
EG("-Wnon-template-friend")

/// Warn about non-virtual destructors
E("-Wnon-virtual-dtor")

/// Warn about NULL being passed to argument slots
/// marked as requiring non-NULL
E("-Wnonnull")

///
EC("-Wnull-conversion")

/// Warn if a C-style cast is used in a program
E("-Wold-style-cast")

/// Warn about overflow in arithmetic expressions
W("-Woverflow")

/// Warn if a simd directive is overridden by the
/// vectorizer cost model
EG49("-Wopenmp-simd")

/// Warn if a string is longer than the maximum
/// portable length specified by the standard
//X("-Woverlength-strings")

/// Warn about overloaded virtual function names
E("-Woverloaded-virtual")

/// Warn when the packed attribute has no effect on
/// struct layout
E("-Wpacked")

/// Warn about packed bit-fields whose offset changed
/// in GCC 4.4
WG("-Wpacked-bitfield-compat")

/// Warn when padding is required to align structure
/// members
I("-Wpadded")

/// Warn about possibly missing parentheses
E("-Wparentheses")

/// Issue warnings needed for strict compliance to
/// the standard
//EG48("-Wpedantic")
// lots of minor extensions are used
IG48("-Wpedantic")
// a bit too noisy
EC("-Wpedantic")

/// Warn when converting the type of pointers to
/// member functions
EG("-Wpmf-conversions")

/// Warn about function pointer arithmetic
E("-Wpointer-arith")

/// Warn about misuses of pragmas
EG("-Wpragmas")

/// Warn about multiple declarations of the same
/// object
W("-Wredundant-decls")

/// Warn when the compiler reorders code
E("-Wreorder")

/// Warn about returning a pointer/reference to a
/// local or temporary variable.
WG48("-Wreturn-local-addr")

/// Warn whenever a function's return type defaults
/// to "int" (C), or about inconsistent return types
/// (C++")
E("-Wreturn-type")

/// Warn about possible violations of sequence point
/// rules
E("-Wsequence-point")

/// Warn when one local variable shadows another
E("-Wshadow")

/// Warn about signed-unsigned comparisons
X("-Wsign-compare")

/// Warn when overload promotes from unsigned to
/// signed
E("-Wsign-promo")

/// This switch lacks documentation
WG48("-Wsizeof-pointer-memaccess")

/// Warn when not issuing stack smashing protection
/// for some reason
X("-Wstack-protector")

/// Warn about code which might break strict aliasing
/// rules
E("-Wstrict-aliasing")

/// Warn about uncasted NULL used as sentinel
WG("-Wstrict-null-sentinel")

/// Warn about optimizations that assume that signed
/// overflow is undefined
X("-Wstrict-overflow")

/// Warn about enumerated switches, with no default,
/// missing a case
I("-Wswitch")

/// Warn about enumerated switches missing a
/// "default:" statement
I("-Wswitch-default")

/// Warn about all enumerated switches missing a
/// specific case
I("-Wswitch-enum")

/// Warn when __sync_fetch_and_nand and
/// __sync_nand_and_fetch built-in functions are used
WG("-Wsync-nand")

/// Warn whenever a trampoline is generated
EG("-Wtrampolines")

/// Warn if trigraphs are encountered that might
/// affect the meaning of the program
E("-Wtrigraphs")

/// Warn if a comparison is always true or always
/// false due to the limited range of the data type
E("-Wtype-limits")

/// Warn if an undefined macro is used in an #if
/// directive
E("-Wundef")

/// Warn about uninitialized automatic variables
E("-Wuninitialized")

/// Warn about unrecognized pragmas
E("-Wunknown-pragmas")

///
// Not an error because of some remaining enum+default
WC("-Wunreachable-code")

/// Warn if the loop cannot be optimized due to
/// nontrivial assumptions.
XG("-Wunsafe-loop-optimizations")

/// Warn when a function parameter is only set,
/// otherwise unused
EG("-Wunused-but-set-parameter")

/// Warn when a variable is only set, otherwise unused
EG("-Wunused-but-set-variable")

/// Warn when a function is unused
E("-Wunused-function")

/// Warn when a label is unused
E("-Wunused-label")

/// Warn when typedefs locally defined in a function
/// are not used
EG47("-Wunused-local-typedefs")

/// Warn about macros defined in the main file that
/// are not used
W("-Wunused-macros")

/// Warn when a function parameter is unused
E("-Wunused-parameter")

/// Warn if a caller of a function, marked with
/// attribute warn_unused_result, does not use its
/// return value
E("-Wunused-result")

/// Warn when an expression value is unused
E("-Wunused-value")

/// Warn when a variable is unused
E("-Wunused-variable")

/// Warn about useless casts
EG48("-Wuseless-cast")

/// Warn about questionable usage of the macros used
/// to retrieve variable arguments
EG48("-Wvarargs")

/// Warn about using variadic macros
W("-Wvariadic-macros")

/// Warn when a vector operation is compiled
/// outside the SIMD
WG47("-Wvector-operation-performance")

/// Warn if a virtual base has a non-trivial move
/// assignment operator
EG48("-Wvirtual-move-assign")

/// Warn if a variable length array is used
I("-Wvla")

/// Warn when a register variable is declared volatile
E("-Wvolatile-register-var")

/// In C++, nonzero means warn about deprecated
/// conversion from string literals to 'char *'.  In
/// C, similar warning, except that the conversion is
/// of course not deprecated by the ISO C standard.
E("-Wwrite-strings")

/// Warn when a literal '0' is used as null
/// pointer
EG47("-Wzero-as-null-pointer-constant")


// clean up after myself
#undef P

#undef I
#undef W
#undef E
#undef X

#undef IC
#undef WC
#undef EC
#undef XC

#undef IG
#undef WG
#undef EG
#undef XG

#undef IG47
#undef WG47
#undef EG47
#undef XG47

#undef IG48
#undef WG48
#undef EG48
#undef XG48

#undef I47
#undef W47
#undef E47
#undef X47

#undef I48
#undef W48
#undef E48
#undef X48
