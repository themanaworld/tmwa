#ifndef LOCK_HPP
#define LOCK_HPP

#include "sanity.hpp"

#include <cstdio>

#include "strings.hpp"

// TODO replace with a class

/// Locked FILE I/O
// Changes are made in a separate file until lock_fclose
FILE *lock_fopen(ZString filename, int *info);
void lock_fclose(FILE *fp, ZString filename, int *info);

#endif // LOCK_HPP
