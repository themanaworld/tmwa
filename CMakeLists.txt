cmake_minimum_required(VERSION 3.10)

project(tmwAthena)

# Prefer to use G++ as the compiler
set(CMAKE_CXX_COMPILER g++)
# Set C++ standard to C++11
# Note we want -std=c++11, not -std=gnu++11, as we want to avoid GNU extensions
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

set_source_files_properties($(ALL_SOURCES) PROPERTIES IMPLICIT_DEPENDS CXX)


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
set_target_properties(tmwa-shared PROPERTIES VERSION 23.10.22.24.0 SOVERSION 0)

# We have four binaries we want to build: tmwa-{login,char,map,admin}
#add_executable(tmwa-login src/login/main.cpp)
#target_link_libraries(tmwa-login tmwa-shared)
#add_executable(tmwa-char src/char/main.cpp)
#target_link_libraries(tmwa-char tmwa-shared)
add_executable(tmwa-map ${map_SOURCES}
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

add_executable(tmwa-test ${strtest_SOURCES})
target_link_libraries(tmwa-test tmwa-shared)

# Some sources and includes are generated, such as the protocol headers.
# We defer to generate.make for these rules.
# Note that these are raw Makefile rules, not CMake rules, so a simple
# add_custom_command() won't work.
add_custom_target(generate
    COMMAND make -f ${CMAKE_SOURCE_DIR}/generate.mk
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating sources and includes"
)

# Call make -f ${CMAKE_SOURCE_DIR}/generate.mk clean to clean up the generated
# files. We want this to be run every time we call make clean.


# The generate target must be run before building the binaries
#add_dependencies(tmwa-login generate)
#add_dependencies(tmwa-char generate)
add_dependencies(tmwa-map generate)
add_dependencies(tmwa-admin generate)
