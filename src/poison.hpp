// impossible(*) to use safely
// removed in C11
#pragma GCC poison gets

// TODO fill in as they are removed from source code:
// double (use a fixed class)
#pragma GCC poison float

#pragma GCC poison dynamic_cast

// Local time is forbidden.
#pragma GCC poison timelocal // timegm
#pragma GCC poison mktime // timegm
#pragma GCC poison localtime // gmtime
#pragma GCC poison localtime_r // gmtime_r

#pragma GCC poison time // TimeT::now() or gettick()

// Avoid manual allocations.
// Use some sort of managed container, or at least dumb_ptr

// new is needed when resetting unique_ptr.
// No it isn't. It doesn't matter if it's a little more verbose; it's cleaner.
//#define really_new1 new
// delete is needed for unique_ptr's deleter.
#define really_delete1 delete

#pragma GCC poison new
#pragma GCC poison delete

#pragma GCC poison malloc
#pragma GCC poison calloc
#pragma GCC poison realloc
#pragma GCC poison free

#pragma GCC poison strdup
#pragma GCC poison strndup

// complete list of glibc whose results may need to be free()d
// not believed to be used
#pragma GCC poison posix_memalign
#pragma GCC poison aligned_alloc
#pragma GCC poison memalign
#pragma GCC poison valloc
#pragma GCC poison pvalloc

#pragma GCC poison asprintf
#pragma GCC poison vasprintf

#pragma GCC poison canonicalize_file_name

#pragma GCC poison cfree

#pragma GCC poison open_memstream
#pragma GCC poison open_wmemstream

// *scanf %ms is done very carefully.
//#pragma GCC poison scanf
//#pragma GCC poison fscanf
//#pragma GCC poison sscanf
//#pragma GCC poison vscanf
//#pragma GCC poison vsscanf
//#pragma GCC poison vfscanf

#pragma GCC poison getcwd
#pragma GCC poison get_current_dir_name

#pragma GCC poison malloc_get_state

#pragma GCC poison realpath

#pragma GCC poison tempnam

#pragma GCC poison wcsdup

#pragma GCC poison memcpy
#pragma GCC poison memmove
#pragma GCC poison memset
#pragma GCC poison memcmp
#pragma GCC poison strncpy // in favor of strzcpy

#pragma GCC poison string // in favor of FString, MString, etc.
#pragma GCC poison strcasecmp
#pragma GCC poison toupper
#pragma GCC poison tolower
#pragma GCC poison isupper
#pragma GCC poison islower
