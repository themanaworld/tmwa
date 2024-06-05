cmake_minimum_required(VERSION 3.10)

# Function for conveniently capturing git output, used to set version related variables
find_package(Git REQUIRED)
function(git_capture_output var)
    execute_process(COMMAND ${GIT_EXECUTABLE} ${ARGN}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE ${var}
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    message(STATUS "${var} = ${${var}}")
    set(${var} ${${var}} PARENT_SCOPE)
endfunction()

git_capture_output(VERSION_FULL describe --tags)
git_capture_output(VERSION_HASH rev-parse HEAD)

# Capture the major.minor.patch part of the version based on the last tag
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" VERSION ${VERSION_FULL})

# Capture the tweak part of the version
if(${VERSION_FULL} MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+-([0-9]+)-g[0-9a-f]+$")
    set(VERSION "${VERSION}.${CMAKE_MATCH_1}")
else()
    set(VERSION "${VERSION}.0")
endif()

project(tmwAthena VERSION ${VERSION} LANGUAGES CXX)

# Prefer to use G++ as the compiler
set(CMAKE_CXX_COMPILER g++)
# Set C++ standard to C++11
# Note we want -std=c++11, not -std=gnu++11, as we want to avoid GNU extensions
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Some sources and includes are generated, such as the protocol headers.
# We defer to generate.make for these rules.
# Note that these are raw Makefile rules, not CMake rules, so a simple
# add_custom_command() won't work.
execute_process(COMMAND make -f ${CMAKE_SOURCE_DIR}/generate.mk
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# The generate target must be re-run when the scripts change
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/generate.mk"
  "${PROJECT_SOURCE_DIR}/tools/config.py"
  "${PROJECT_SOURCE_DIR}/tools/protocol.py"
)

# Search through the tree for sources
# For each subfolder in src, add all .cpp files to a subfolder's SOURCES
# variable.
set(ALL_SOURCES "")
foreach(dir admin ast char compat conf generic high ints io login map mmo net proto-base range sexpr shared strings tests wire strtest)
    file(GLOB_RECURSE ${dir}_SOURCES CONFIGURE_DEPENDS src/${dir}/*.cpp)
    # Exclude any _test.cpp files from the build
    set(ALL_SOURCES ${ALL_SOURCES} ${${dir}_SOURCES})
    list(FILTER ${dir}_SOURCES EXCLUDE REGEX ".*_test.cpp")
    message("Adding sources in ${dir}: ${${dir}_SOURCES}")
endforeach()

# All targets include the include/ directory
include_directories(include)

# We want -fvisibility=hidden for regular objects, but not shared libraries.
# CMake provides a helpful preset for this.
# FIXME this is currently broken
#set(CMAKE_CXX_VISIBILITY_PRESET hidden)
# General purpose build flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fstack-protector -fno-strict-aliasing -flto")
# Enable link time optimization, and track function and data sections. We let
# the linker remove unused code.
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -ffunction-sections -fdata-sections -Wl,--gc-sections")
# Next, add warnings
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
# Add debug information
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

# Vendor Name: String (no newlines, no parentheses)
# This is usually one word, and does not (usually) change over time.
# (Examples: Gentoo, Debian, Fedora, Ubuntu)
set(VENDOR_NAME Vanilla)
# Vendor Point: Integer (max value 65535)
# This is intended to be the "packaging revision number", assuming that's
# an integer. At a minimum, please try to make it nonzero if you have
# any non-upstream patches (unconditionally nonzero is also okay).
# (If your revision 0 package has patches ... please be nicer to upstream)
set(VENDOR_POINT 0)
# URL where the source may be found (after searching for version number).
# See AGPLv3 section 13
set(VENDOR_SOURCE https://git.themanaworld.org/legacy/tmwa)

# Convenience
set(VERSION_STRING "TMWA ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} dev${PROJECT_VERSION_TWEAK} +${VENDOR_POINT} (${VENDOR_NAME})")
set(VERSION_DOTS "${PROJECT_VERSION}.${VENDOR_POINT}")

include(GNUInstallDirs)
set(PACKAGEDATADIR "${CMAKE_INSTALL_FULL_DATAROOTDIR}/tmwa")
set(LOCALSTATEDIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}")
set(SYSCONFDIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}")

# Generate the install.hpp and version.hpp files in the source directory.
# TODO: Is there a nicer way to handle this so we're still able to make fully
# out-of-tree builds, generating the files in the build directory, but still
# having them found by includes?
configure_file(src/conf/install.hpp.in ${CMAKE_SOURCE_DIR}/src/conf/install.hpp @ONLY)
configure_file(src/conf/version.hpp.in ${CMAKE_SOURCE_DIR}/src/conf/version.hpp @ONLY)
set(conf_SOURCES ${conf_SOURCES} src/conf/install.hpp src/conf/version.hpp)

# Add a shared library: libtmwa-shared.so.0
# When we add_executable later, we need to link against this library.
add_library(tmwa-shared SHARED ${shared_SOURCES}
  src/io/dir.cpp
  src/io/fd.cpp
  src/io/read.cpp
  src/io/write.cpp
  ${strings_SOURCES}
)
# SO versioning
set(ABI_VERSION 0)
set_target_properties(tmwa-shared PROPERTIES
  VERSION ${ABI_VERSION}.${VERSION_DOTS}
  SOVERSION ${ABI_VERSION})

# We have four binaries we want to build: tmwa-{login,char,map,admin}
add_executable(tmwa-login ${login_SOURCES}
  ${generic_SOURCES}
  ${high_SOURCES}
  ${io_SOURCES}
  ${mmo_SOURCES}
  ${net_SOURCES}
  ${wire_SOURCES}
)
target_link_libraries(tmwa-login tmwa-shared)

add_executable(tmwa-char ${char_SOURCES}
  ${generic_SOURCES}
  ${high_SOURCES}
  ${io_SOURCES}
  ${mmo_SOURCES}
  ${net_SOURCES}
  ${wire_SOURCES}
)
target_link_libraries(tmwa-char tmwa-shared)

add_executable(tmwa-map ${map_SOURCES}
  ${ast_SOURCES}
  ${compat_SOURCES}
  ${generic_SOURCES}
  ${high_SOURCES}
  ${io_SOURCES}
  ${mmo_SOURCES}
  ${net_SOURCES}
  ${wire_SOURCES}
  )
target_link_libraries(tmwa-map tmwa-shared)

add_executable(tmwa-admin ${admin_SOURCES}
  ${generic_SOURCES}
  ${high_SOURCES}
  ${io_SOURCES}
  ${mmo_SOURCES}
  ${net_SOURCES}
  ${wire_SOURCES}
)
target_link_libraries(tmwa-admin tmwa-shared)

# TODO: Call make -f ${CMAKE_SOURCE_DIR}/generate.mk clean to clean up the
# generated files. We want this to be run every time we call make clean.

