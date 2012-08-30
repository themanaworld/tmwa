/// return wrappers for unexpected NULL pointers
#ifndef SANITY_HPP
#define SANITY_HPP
# ifndef __cplusplus
#  error "Please compile in C++ mode"
# endif
# if __GNUC__ < 3
// I don't specifically know what version this requires,
// but GCC 3 was the beginning of modern GCC
#  error "Please upgrade your compiler to at least GCC 3"
# endif
# ifndef __i386__
// Known platform dependencies:
// endianness for the [RW]FIFO.* macros
// possibly, some signal-handling
#  error "Unsupported platform"
# endif
# ifdef __x86_64__
// I'm working on it - I know there are some pointer-size assumptions.
#  error "Sorry, this code is believed not to be 64-bit safe"
#  error "please compile with -m32"
# endif

/// A name for unused function arguments - can be repeated
# define UNUSED /* empty works for C++ */
/// Convert type assumptions to use the standard types here
# include <cstdint>
/// size_t, NULL
# include <cstddef>

#endif // SANITY_HPP
