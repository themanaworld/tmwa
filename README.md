# The Mana World Athena

![The Mana World logo](share/tmwa/TheManaWorldLogo.png)

Welcome to The Mana World Athena! This is an MMORPG server used by The Mana World, based on a protocol that was inspired by the project named "Athena". In 2004, it was forked from eAthena, a Ragnarok Online clone.

If you are interested in contributing, please take a look at our [wiki](http://wiki.themanaworld.org/index.php/Development:How_to_Develop) for user instructions. It provides detailed information on how to get started.

**Important note:** Building from a GitHub-generated tarball does not work! You must either build from a git checkout or from a 'make dist' tarball.

The rest of this file contains information relevant to distributors and contributors.

## 1. Notes for Distributors

- The build system is currently in a transitory state, please use CMake going forward if possible. The attoconf build system is now deprecated.
- Please adjust `VENDOR_NAME`, `VENDOR_POINT` and `VENDOR_SOURCE` to found in [CMakeLists.txt](CMakeLists.txt) to indicate the binaries' source.
- Building TMWA requires a git checkout because it uses the latest tag to determine the version.

### 1.1 Platform Dependencies

- The server is actively tested and maintained on linux/amd64
- Other platforms are not actively supported, but most modern platforms will likely work

### 1.2 Build Dependencies

#### Python

- Required: 3.6+, installed in $PATH as 'python3'

#### Compiler

- g++ 7.5 or higher
- glibc 2.27 or higher
- clang++ is not supported, but will likely work

## 2. CMake Instructions

### 2.1 Configuration

- Run `cmake -S . -B build` to generate the useable files in a directory called `build`.
- By default, the linux install location is `$HOME/.local`, but can be altered by defining `CMAKE_INSTALL_PREFIX` on the command line like so: `cmake -S . -B build -DCMAKE_INSTALL_PREFIX=<dirname>`.

### 2.2 Build

- Run `cmake --build build` to compile the various binaries using the `build` directory.

### 2.3 Build Test

- To run the tests, ensure Google Test available - it's added here as a submodule.
- Then, run the following snippet to configure, build and run the tests, respectfully:

```
cmake -D TMWA_BUILD_TESTS=ON -S . -B build
cmake --build build
cmake --build build -t test
```

- Note that test coverage is not yet complete.

### 2.4 Install

- Run `cmake --install build` to install the compiled binaries using the `build` directory. If you specified an install directory outside of `$HOME`, you may need sudo permissions.

## 3. What is Installed

Currently, the above instructions installs 4 binaries, along with libraries, headers, config files, and debug files. The programs listed below are typically running on the same machine, but they can be configured to run on different machines in a fast LAN. Please note that the internal protocol between the programs may change without notice, so it's important to upgrade them simultaneously.

- `tmwa-admin`: Formerly known as `ladmin` ("local"). This tool is used to maintain the server's flatfile "databases". It connects to `tmwa-login` to make changes.
- `tmwa-login`: Formerly known as `login-server`. This server handles account checks and accepts internal connections from `tmwa-admin` and `tmwa-char`.
- `tmwa-char`: Formerly known as `char-server`. This server handles character persistence, connects to `tmwa-login`, and accepts connections from `tmwa-map`. Multiple instances of `tmwa-char` can connect to the same `tmwa-login`. If this is the case, the client will be presented with a choice of worlds before leaving the login server. The Mana World sometimes runs a test `tmwa-char` server connected to the main `tmwa-login` for ease of access.
- `tmwa-map`: Formerly known as `map-server`. This server connects to `tmwa-char`. Multiple instances of `tmwa-map` can connect to the same `tmwa-char`, with clients able to switch servers if they move to a map handled by a different map server. This has not been used by The Mana World, and may not work properly.

### 3.1 Running The Server

To run the server, you will need a complete set of content including config files, game scripts, savefiles, and client updates. We strongly recommend setting up a web server to serve the updates. You can find a compatible set of server data at https://git.themanaworld.org/tmw/serverdata. Please follow the instructions in the [How to Develop](https://wiki.themanaworld.org/index.php/Development:How_to_Develop) article for more information.

## 4. Contributors

We welcome contributions from developers like you! If you are interested in maintaining this server, please get in touch with the currently active maintainers first. It's important to make changes with extreme care and ensure that each change is thoroughly tested and reviewed before it goes live.

If you have become familiar with the codebase, we encourage you to stick around and help fix issues or review changes made by others. Your contributions are greatly appreciated!
