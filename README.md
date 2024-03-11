# The Mana World Athena

![The Mana World logo](share/tmwa/TheManaWorldLogo.png)

This is TMWA, an MMORPG server used by The Mana World that uses a protocol
based on one of many projects that foolishly chose the name "Athena".
Specifically, it was forked from eAthena, a Ragnarok Online clone, in 2004.

Take a look at the [wiki](http://wiki.themanaworld.org/index.php/How_to_Develop) for user instructions.

**Important note:** building from a GitHub-generated tarball does not work!
You must either build from a git checkout or from a 'make dist' tarball.


The rest of this file contains information relevant only to:

1. Distributors.
2. Contributors.

TMWA has a [bugtracker](https://git.themanaworld.org/legacy/tmwa/-/issues).

But it's probably worth getting on IRC first:
* Use an IRC client: irc://irc.libera.chat/themanaworld-dev
* Or just use the [webchat](https://web.libera.chat/?channel=#themanaworld-dev).

Note that this channel is not specific to technical discussion of TMWA (and
attoconf), but for any discussions related to TMW development.

## 1. Distributors.
### Important notes:

- Go read [version.make](version.make)
- TMWA requires git to build by default, use 'make dist' to get a tarball.

### Platform dependencies:
#### Architecture:

    tested: x86, amd64, x32
    likely to work: all architectures (patches welcome if they don't)

#### Operating system:
    known bad: Linux 2.6.26 and earlier
    maintained: Linux 3.2 and later
    likely to break: Cygwin, BSD
#### Filesystem:
    must support symlinks

### Build dependencies:
#### Python:
    required: 2.7.x only, installed in $PATH as 'python'
#### C library:
    recommended: glibc 2.17 or higher
    supported: glibc 2.13
    known bad: glibc 2.8 or below
    unsupported: anything that's not glibc
#### C++ compiler:
    required: g++ 4.7.2 or higher
    recommended: g++ 4.8.1 or higher
    not recommended: clang++ 3.3 or higher (all versions have unfixed bugs)
#### C++ library:
    recommended: libstdc++ to match g++; may need patch for clang++
    may work: libc++
#### attoconf:
    special: see below
### Runtime dependencies:
#### glibc:
    depends on what it was built against
#### libstdc++:
    depends on what it was built against
### Instructions:
#### Configuration:
    ./configure
_Takes most of the options GNU Autoconf's configure does - I won't
    repeat the output of `./configure --help` here._

_`--prefix=/usr`, not `--prefix usr`, in order to prevent an ambiguity.
    "In the face of ambiguity, refuse the temptation to guess."_

_Out-of-tree builds work._

_Note that there is no option to disable dependency tracking, as it
    is also used to generate link information. There is also no option
    to ignore unknown options - I refuse to lie._
#### Build:
    make -jN
#### Build test:
    make test

_Nowhere near complete useful yet. Requires source of Google Test._

    make format; git diff --exit-code
#### Install:
    make install DESTDIR=/whatever

_See [what is installed](#what-is-installed) below_
#### Install test:
_not implemented_
#### Distribution tarballs:
    make dist
    make bindist

### Note about attoconf:
TMWA's `./configure` script is implemented using a python package
'attoconf', which I wrote over a weekend after reading GNU autoconf's
documentation and realizing that it was 1. insane, and 2. still trying
to solve the wrong sort of problem.

Currently, attoconf's API is still in the "experimental" stage, so the
real rule is "does ./configure work?".
When it gets to 1.0, it will start guaranteeing compatibility.

Attoconf is available at [Github](https://github.com/o11c/attoconf/) and is a
well-behaving python package.

Attoconf requires Python 2.7; a port to Python 2.6 is doable with a bit
of work, but it is not known if this would benefit anybody.

If you're Arch - you broke Python for us all, you clean up your own mess.
Patches to call a nonexistent `/usr/bin/python2` will NOT be accepted.

### What is installed:
#### Overview:
Currently, `make install` installs 5 binaries, along with a handful
of libraries, headers, data files, config files, and debug files, each
of which has a `make install-something` target.

The 4 main programs below are typically running on the same machine,
though in theory they may be configured to run on different machines
in a fast LAN. Also, the internal protocol between the programs is
subject to change without notice, so they *must* be upgraded
simultaneously.

These programs currently read most of their files relative to the
current working directory; this was the only thing that makes sense
since the files are dependent on the server-data. However, a migration
to installed files has begun.

#### tmwa-admin:
Formerly known as `ladmin` ("local").

This is an essential tool to maintain the server's flatfile "databases".
It doesn't actually touch the files directly, just connects to
tmwa-login.

Even when everything is rewritten to use SQL, it will be kept, if just
to keep a consistent interface. In fact, if we use SQLite we *can't*
edit the databases independently. This wouldn't be a problem with
Postgres, but people seem to think it's hard to install (that's not my
experience on Debian though. Did they ever try themselves, or are they
just repeating what they've heard?)

#### tmwa-login:
Formerly known as `login-server`.

User-facing server to deal with account checks.

Also accepts internal connections from `tmwa-admin` and `tmwa-char`,
subject to a plaintext password (and for `tmwa-admin`, also an IP check).

#### tmwa-char:
Formerly known as `char-server`.

User-facing server to deal with character persistence.

Connects to `tmwa-login`; also takes internal connections from `tmwa-map`.

Note that it is fully supported for more than one `tmwa-char` to connect
to the same `tmwa-login`; the client will be presented with a list of
"worlds" before leaving the login server.

#### tmwa-map:
Formerly known as `map-server`.

Connects to `tmwa-char`.

It is technically possible for more than one `tmwa-map` to connect to
a single tmwa-char, but this is poorly supported by our current config
and moderation tools, and there are likely undiscovered bugs.

#### About server data:
Just having the binaries is not enough: you also need a complete set of
content: config files, game scripts, savefiles, and client updates.

A web server to serve the updates is also strongly recommended, as even
developers get annoyed when wushin makes us work straight from his
client-data repo.

Currently, there is only *one* set of server data that is known to be
compatible with TMWA: https://git.themanaworld.org/legacy/serverdata

The only recommended way of using this is by following the instructions
in the [How to Develop](https://wiki.themanaworld.org/index.php/Dev:How_to_Develop) article. These instructions are only designed
for people contributing to TMW itself, not to people trying to start
a fork - we know all forks are doomed to be unsuccessful anyway, so
please don't split the development effort, and you can't split the
player community.

In particular, the instructions do NOT provide information on how to
secure a server by changing all the default passwords.

There are 3 other known sets of complete server data: regional ones
for Germany and Brasil, and Evol. Evol requires their own fork of
the tmwa server (for some reason they don't like me to call it evola),
and nobody seems to know of the foreign servers are keeping active.

Note also that The Mana World has not investigated the copyright status
of other sets of server data.

## 2. Contributors.

You're welcome to help maintain this server, but please make sure to
get in touch with the currently active maintainers first.

Remember that this server is what currently keeps The Mana World alive,
so any changes should be made with extreme care. It is important that
each change be tested and reviewed before it goes live.

Finally, if you got yourself somewhat familiar with this code, please
consider sticking around either to fix further issues as they come up
or to help reviewing changes by others. Thanks!
