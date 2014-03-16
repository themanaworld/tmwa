#ifndef TMWA_COMPAT_RAWMEM_HPP
#define TMWA_COMPAT_RAWMEM_HPP

# include <cstddef>
# include <cstdint>
# include <cstring>

inline
void really_memcpy(uint8_t *dest, const uint8_t *src, size_t n)
{
    memcpy(dest, src, n);
}

inline
void really_memmove(uint8_t *dest, const uint8_t *src, size_t n)
{
    memmove(dest, src, n);
}
inline
bool really_memequal(const uint8_t *a, const uint8_t *b, size_t n)
{
    return memcmp(a, b, n) == 0;
}

inline
void really_memset0(uint8_t *dest, size_t n)
{
    memset(dest, '\0', n);
}
#endif // TMWA_COMPAT_RAWMEM_HPP
