// impossible(*) to use safely
// removed in C11
#pragma GCC poison gets

// TODO fill in as they are removed from source code:
// double (use a fixed class)
#pragma GCC poison float
// mem* and str* from <string.h>, in favor of <algorithm>

// Local time is forbidden.
#pragma GCC poison timelocal // timegm
#pragma GCC poison mktime // timegm
#pragma GCC poison localtime // gmtime
#pragma GCC poison localtime_r // gmtime_r

#pragma GCC poison time // TimeT::now() or gettick()
