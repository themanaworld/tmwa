#ifndef CORE_HPP
#define CORE_HPP

#include "sanity.hpp"

#include "../strings/fwd.hpp"

/// core.c contains a server-independent main() function
/// and then runs a do_sendrecv loop

/// When this is cleared, the server exits gracefully
/// only used by map server's GM command: @mapexit
extern bool runflag;

/// This is an external function defined by each server
/// This function must register stuff for the parse loop
extern int do_init(int, ZString *);

/// Cleanup function called whenever a signal kills us
/// or when if we manage to exit() gracefully.
extern void term_func(void);

#endif // CORE_HPP
