#ifndef _MALLOC_H_
#define _MALLOC_H_

#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ ""
# endif
#endif

#define ALC_MARK __FILE__, __LINE__, __func__

void *aMalloc_ (size_t, const char *, int, const char *);
void *aCalloc_ (size_t, size_t, const char *, int, const char *);
void *aRealloc_ (void *, size_t, const char *, int, const char *);
char *aStrdup_ (const char *, const char *, int, const char *);

#define aMalloc(n) aMalloc_(n,ALC_MARK)
#define aMallocA(n) aMalloc_(n,ALC_MARK)
#define aCalloc(m,n) aCalloc_(m,n,ALC_MARK)
#define aCallocA(m,n) aCalloc_(m,n,ALC_MARK)
#define aRealloc(p,n) aRealloc_(p,n,ALC_MARK)
#define aStrdup(p) aStrdup_(p,ALC_MARK)
#define aFree(p) aFree_(p,ALC_MARK)

#endif
