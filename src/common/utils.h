#include "mt_rand.h"

#ifndef NULL
#define NULL (void *)0
#endif

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

/* strcasecmp -> stricmp -> str_cmp */

#ifdef LCCWIN32
int  strcasecmp (const char *arg1, const char *arg2);
int  strncasecmp (const char *arg1, const char *arg2, int n);
void str_upper (char *name);
void str_lower (char *name);
char *rindex (char *str, char c);
#endif

void dump (unsigned char *buffer, int num);

#define CREATE(result, type, number)  do {\
   if ((number) * sizeof(type) <= 0)   \
      printf("SYSERR: Zero bytes or less requested at %s:%d.\n", __FILE__, __LINE__);   \
   if (!((result) = (type *) calloc ((number), sizeof(type))))   \
      { perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
      { printf("SYSERR: realloc failure"); abort(); } } while(0)

/*
 * ModuloRand and ModuloPlusRand. These macros are used to replace the
 * vast number of calls to rand()%mod ..
 * MRAND(10), returns 0-9.
 * MPRAND(5,10) returns 5-14.
 */
#define MRAND(mod) (int) (mt_random() % (mod))
#define MPRAND(add, mod) add + MRAND(mod)
