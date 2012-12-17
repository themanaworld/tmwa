#ifndef LOCK_HPP
#define LOCK_HPP

#include <cstdio>

/// Locked FILE I/O
// Changes are made in a separate file until lock_fclose
FILE *lock_fopen(const char *filename, int *info);
void lock_fclose(FILE * fp, const char *filename, int *info);

#endif // LOCK_HPP
