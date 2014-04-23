#ifndef GENERATING_DEPENDENCIES
//    poison.hpp - List of dangerous functions and objects.
//
//    Copyright © 2013 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

// just mention "fwd.hpp" to make formatter happy

// impossible(*) to use safely
// removed in C11
# pragma GCC poison gets

// TODO fill in as they are removed from source code:
// double (use a fixed class)
# pragma GCC poison float

# pragma GCC poison dynamic_cast

// Local time is forbidden.
# pragma GCC poison timelocal // timegm
# pragma GCC poison mktime // timegm
# pragma GCC poison localtime // gmtime
# pragma GCC poison localtime_r // gmtime_r

# pragma GCC poison time // TimeT::now() or gettick()

// Avoid manual allocations.
// Use some sort of managed container, or at least dumb_ptr

// new is needed when resetting unique_ptr.
// No it isn't. It doesn't matter if it's a little more verbose; it's cleaner.
//#define really_new1 new
// delete is needed for unique_ptr's deleter.
# define really_delete1 delete

# pragma GCC poison new
# pragma GCC poison delete

# pragma GCC poison malloc
# pragma GCC poison calloc
# pragma GCC poison realloc
# pragma GCC poison free

# pragma GCC poison strdup
# pragma GCC poison strndup

// complete list of glibc whose results may need to be free()d
// not believed to be used
# pragma GCC poison posix_memalign
# pragma GCC poison aligned_alloc
# pragma GCC poison memalign
# pragma GCC poison valloc
# pragma GCC poison pvalloc

# pragma GCC poison asprintf
# pragma GCC poison vasprintf

# pragma GCC poison canonicalize_file_name

# pragma GCC poison cfree

# pragma GCC poison open_memstream
# pragma GCC poison open_wmemstream

// *scanf %ms is done very carefully.
//#pragma GCC poison scanf
//#pragma GCC poison fscanf
//#pragma GCC poison sscanf
//#pragma GCC poison vscanf
//#pragma GCC poison vsscanf
//#pragma GCC poison vfscanf

# pragma GCC poison getcwd
# pragma GCC poison get_current_dir_name

# pragma GCC poison malloc_get_state

# pragma GCC poison realpath

# pragma GCC poison tempnam

# pragma GCC poison wcsdup

# pragma GCC poison memcpy
# pragma GCC poison memmove
# pragma GCC poison memset
# pragma GCC poison memcmp
# pragma GCC poison strncpy // in favor of strzcpy

# pragma GCC poison string // in favor of FString, MString, etc.
# pragma GCC poison strcasecmp
# pragma GCC poison toupper
# pragma GCC poison tolower
# pragma GCC poison isupper
# pragma GCC poison islower

// in favor of io::ReadFile and io::WriteFile
// note that stdout and stderr are NOT poisoned (yet)
# pragma GCC poison FILE
# pragma GCC poison istream
# pragma GCC poison ostream
# pragma GCC poison iostream
# pragma GCC poison ifstream
# pragma GCC poison ofstream
# pragma GCC poison fstream

#endif // GENERATING_DEPENDENCIES
