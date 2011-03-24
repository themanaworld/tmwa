#ifndef UTILS_H
#define UTILS_H
/*
Notes about memory allocation in tmwAthena:
There used to be 3 sources of allocation: these macros,
a{C,M,Re}alloc in common/malloc.{h,c}, and direct calls.
I deleted malloc.{h,c} because it was redundant;
future calls should either use this or depend on the coming segfault.
*/
# define CREATE(result, type, number) \
   if (!((result) = (type *) calloc ((number), sizeof(type))))   \
      { perror("SYSERR: malloc failure"); abort(); } else (void)0

# define RECREATE(result,type,number) \
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
      { perror("SYSERR: realloc failure"); abort(); } else (void)0

#endif //UTILS_H
