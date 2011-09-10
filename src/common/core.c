#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "core.h"
#include "socket.h"
#include "timer.h"
#include "version.h"
#include "mt_rand.h"
#include "nullpo.h"

/// Defined by each server
extern int  do_init (int, char **);
extern void term_func (void);

static void chld_proc (int UNUSED)
{
    wait(NULL);
}
static void sig_proc (int UNUSED)
{
    term_func ();
    for (int i = 0; i < fd_max; i++)
        if (session[i])
            close (i);
    exit (0);
}

// Added by Gabuzomeu
//
// This is an implementation of signal() using sigaction() for portability.
// (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
// Programming in the UNIX Environment_.
//
typedef void (*sigfunc)(int);
sigfunc compat_signal (int signo, sigfunc func)
{
    struct sigaction sact, oact;

    sact.sa_handler = func;
    sigemptyset (&sact.sa_mask);
    sact.sa_flags = 0;

    if (sigaction (signo, &sact, &oact) < 0)
        return SIG_ERR;

    return oact.sa_handler;
}

bool runflag = true;

int main (int argc, char **argv)
{
    /// Note that getpid() and getppid() may be very close
    mt_seed (time (NULL) ^ (getpid () << 16) ^ (getppid () << 8));

    do_socket ();

    compat_signal (SIGPIPE, SIG_IGN);
    compat_signal (SIGTERM, sig_proc);
    compat_signal (SIGINT, sig_proc);
    compat_signal (SIGCHLD, chld_proc);

    // Signal to create coredumps by system when necessary (crash)
    compat_signal (SIGSEGV, SIG_DFL);
    compat_signal (SIGBUS, SIG_DFL);
    compat_signal (SIGTRAP, SIG_DFL);
    compat_signal (SIGILL, SIG_DFL);

    do_init (argc, argv);
    while (runflag)
    {
        do_sendrecv (do_timer (gettick_nocache ()));
        do_parsepacket ();
    }
}
