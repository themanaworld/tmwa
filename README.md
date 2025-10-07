# The Mana World Athena
![The Mana World logo](share/tmwa/TheManaWorldLogo.png)

Welcome to The Mana World Athena! This is an MMORPG server used by The Mana World, based on a protocol that was inspired by the project named "Athena". In 2004, it was forked from eAthena, a Ragnarok Online clone.


If you are interested in contributing, please take a look at our [wiki](http://wiki.themanaworld.org/index.php/Development:How_to_Develop) for user instructions. It provides detailed information on how to get started.

**Important note:** Building from a GitHub-generated tarball does not work! You must either build from a git checkout or from a 'make dist' tarball.

The rest of this file contains information relevant to distributors and contributors.

## 1. Building

### CMake

Cmake is a standard build system. It is recommended for most users.

#### Configuration
- `mkdir build`
- `cd build`
- `cmake ..`

#### Build
- Run `make -j$(nproc)` to build the project.

#### Install
- Run `make install DESTDIR=/whatever` to install the project. See [what is installed](#what-is-installed) below.

### Attoconf (Legacy build system)

Attoconf is a deprecated bespoke build system. It is not recommended for typical use. It is currently the only way of running some tests. See [this PR](https://git.themanaworld.org/tmw/tmwa/-/merge_requests/280) and follow-ups for progress.

#### Configuration

- Run `./configure` to configure the build. Most of the options are similar to GNU Autoconf's configure. For example, you can create a `build` directory and run `../configure` from there to keep the source directory clean.

#### Build
- Run `make -j$(nproc)` to build the project.

#### Build test
- Run `make test` to run the build tests. This requires Google Test available - it's added here as a submodule. Note that test coverage is not yet complete.
- Run `make format` to format the code.

#### Install
- Run `make install DESTDIR=/whatever` to install the project. See [what is installed](#what-is-installed) below.

#### Install test
- Not implemented

#### Distribution tarballs
- Run `make dist` or `make bindist` to create distribution tarballs.

### What is installed
#### Overview
Currently, `make install` installs 5 binaries, along with libraries, headers, data files, config files, and debug files. Each component has a specific `make install-something` target.

The 4 main programs listed below are typically running on the same machine, but they can be configured to run on different machines in a fast LAN. Please note that the internal protocol between the programs may change without notice, so it's important to upgrade them simultaneously.

- `tmwa-admin`: Formerly known as `ladmin` ("local"). This tool is used to maintain the server's flatfile "databases". It connects to `tmwa-login` to make changes.
- `tmwa-login`: Formerly known as `login-server`. This server handles account checks and accepts internal connections from `tmwa-admin` and `tmwa-char`.
- `tmwa-char`: Formerly known as `char-server`. This server handles character persistence, connects to `tmwa-login`, and accepts connections from `tmwa-map`. Multiple instances of `tmwa-char` can connect to the same `tmwa-login`. If this is the case, the client will be presented with a choice of worlds before leaving the login server. The Mana World sometimes runs a test `tmwa-char` server connected to the main `tmwa-login` for ease of access.
- `tmwa-map`: Formerly known as `map-server`. This server connects to `tmwa-char`. Multiple instances of `tmwa-map` can connect to the same `tmwa-char`, with clients able to switch servers if they move to a map handled by a different map server. This has not been used by The Mana World, and may not work properly.

#### About server data
To run the server, you will need a complete set of content including config files, game scripts, savefiles, and client updates. We strongly recommend setting up a web server to serve the updates. You can find a compatible set of server data at https://git.themanaworld.org/tmw/serverdata. Please follow the instructions in the [How to Develop](https://wiki.themanaworld.org/index.php/Development:How_to_Develop) article for more information.

## 2. Contributors
We welcome contributions from developers like you! If you are interested in maintaining this server, please get in touch with the currently active maintainers first. It's important to make changes with extreme care and ensure that each change is thoroughly tested and reviewed before it goes live.

If you have become familiar with the codebase, we encourage you to stick around and help fix issues or review changes made by others. Your contributions are greatly appreciated!

## 3. Distributors
### Important notes
- Please read [version.mk](version.mk) for important version information.
- TMWA requires git to build by default. Use 'make dist' to get a tarball.

### Platform dependencies
#### Architecture
- Tested: x86, amd64, x32

Patches welcome for any architecture!

#### Operating system
- Supported: Linux 5.4.0 and later
- Has worked, but may break sometimes: Cygwin, BSD

#### Filesystem
- Must support symlinks

### Build dependencies
#### Python
- Required: 3.6+

#### C library
- Recommended: glibc 2.41 or higher
- Supported: glibc 2.27 or higher
- Unsupported: anything that's not glibc

#### C++ compiler
- Required: g++ 7.5.0 or higher
- Recommended: g++ 14.2.0 or higher
- Not recommended: clang++ (support in progress)

#### C++ library
- Recommended: libstdc++ to match g++; may need patch for clang++
- May work: libc++

#### attoconf
If you need to run tests via the legacy build system, see above.

### Runtime dependencies
`glibc` and `libstdc++` depend on what they were built against.
