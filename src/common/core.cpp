#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "core.hpp"
#include "socket.hpp"
#include "timer.hpp"
#include "version.hpp"
#include "mt_rand.hpp"
#include "nullpo.hpp"

// Added by Gabuzomeu
//
// This is an implementation of signal() using sigaction() for portability.
// (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
// Programming in the UNIX Environment_.
//
typedef void (*sigfunc)(int);
static sigfunc compat_signal (int signo, sigfunc func)
{
    struct sigaction sact, oact;

    sact.sa_handler = func;
    sigfillset (&sact.sa_mask);
    sigdelset(&sact.sa_mask, SIGSEGV);
    sigdelset(&sact.sa_mask, SIGBUS);
    sigdelset(&sact.sa_mask, SIGTRAP);
    sigdelset(&sact.sa_mask, SIGILL);
    sigdelset(&sact.sa_mask, SIGFPE);
    sact.sa_flags = 0;

    if (sigaction (signo, &sact, &oact) < 0)
        return SIG_ERR;

    return oact.sa_handler;
}

static void chld_proc (int UNUSED)
{
    wait(NULL);
}
static void sig_proc (int UNUSED)
{
    for (int i = 1; i < 31; ++i)
        compat_signal(i, SIG_IGN);
    term_func ();
    _exit (0);
}

bool runflag = true;

/*
    Note about fatal signals:

    Under certain circumstances,
    the following signals MUST not be ignored:
    SIGFPE, SIGSEGV, SIGILL
    Unless you use SA_SIGINFO and *carefully* check the origin,
    that means they must be SIG_DFL.
 */
int main (int argc, char **argv)
{
    /// Note that getpid() and getppid() may be very close
    mt_seed (time (NULL) ^ (getpid () << 16) ^ (getppid () << 8));

    do_socket ();

    do_init (argc, argv);
    // set up exit handlers *after* the initialization has happened.
    // This is because term_func is likely to depend on successful init.

    compat_signal (SIGPIPE, SIG_IGN);
    compat_signal (SIGTERM, sig_proc);
    compat_signal (SIGINT, sig_proc);
    compat_signal (SIGCHLD, chld_proc);

    // Signal to create coredumps by system when necessary (crash)
    compat_signal (SIGSEGV, SIG_DFL);
    compat_signal (SIGBUS, SIG_DFL);
    compat_signal (SIGTRAP, SIG_DFL);
    compat_signal (SIGILL, SIG_DFL);
    compat_signal (SIGFPE, SIG_DFL);

    atexit (term_func);

    while (runflag)
    {
        do_sendrecv (do_timer (gettick_nocache ()));
        do_parsepacket ();
    }
}
