/// return wrappers for unexpected NULL pointers
#ifndef SANITY_HPP
#define SANITY_HPP

# ifndef __cplusplus
#  error "Please compile in C++ mode"
# endif

# if __GNUC__ < 4
#  error "Your compiler is absolutely ancient. You have no chance ..."
# endif

# if __GNUC__ == 4
// clang identifies as GCC 4.2, but is mostly okay.
// Until a bug-free release of it happens, though, I won't recommend it.
// (patched) clang 3.1 would be the requirement
#  if __GNUC_MINOR__ < 6 && !defined(__clang__)
#   error "Please upgrade to at least GCC 4.6"
#  endif
#  if __GNUC_MINOR__ == 6
#   warning "Working around some annoying bugs in GCC 4.6 ..."
#   define ANNOYING_GCC46_WORKAROUNDS
#  endif
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

/// Convert type assumptions to use the standard types here
# include <cstdint>
/// size_t, NULL
# include <cstddef>

#endif // SANITY_HPP
