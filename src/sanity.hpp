/// Keep spatulas out of the build environment
#ifndef TMWA_SANITY_HPP
#define TMWA_SANITY_HPP

# ifndef __cplusplus
#  error "Please compile in C++ mode"
# endif // __cplusplus

# if __GNUC__ < 4
#  error "Your compiler is absolutely ancient. You have no chance ..."
# endif // __GNUC__ < 4

/// Convert type assumptions to use the standard types here
# include <cstdint>
/// size_t, NULL
# include <cstddef>

# if __GNUC__ == 4
// clang identifies as GCC 4.2, but is mostly okay.
// Until a bug-free release of it happens, though, I won't recommend it.
// (patched) clang 3.1 would be the requirement
#  if __GNUC_MINOR__ < 6 && !defined(__clang__)
#   error "Please upgrade to at least GCC 4.6"
#  endif // __GNUC_MINOR__ < 6 && !defined(__clang__)
# endif // __GNUC__ == 4

# if not defined(__i386__) and not defined(__x86_64__)
// Known platform dependencies:
// endianness for the [RW]FIFO.* macros
// possibly, some signal-handling
#  error "Unsupported platform, we use x86 / amd64 only"
# endif // not __i386__

#endif // TMWA_SANITY_HPP
