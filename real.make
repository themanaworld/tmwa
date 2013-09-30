##    real.make - The One Makefile that builds them all.
##
##    Copyright © 2012-2013 Ben Longbons <b.r.longbons@gmail.com>
##
##    This file is part of The Mana World (Athena server)
##
##    This program is free software: you can redistribute it and/or modify
##    it under the terms of the GNU General Public License as published by
##    the Free Software Foundation, either version 3 of the License, or
##    (at your option) any later version.
##
##    This program is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##    GNU General Public License for more details.
##
##    You should have received a copy of the GNU General Public License
##    along with this program.  If not, see <http://www.gnu.org/licenses/>.

# With the One Makefile, you never have to remember to update the list of
# objects you need to link your programs. It is designed to behave (almost)
# exactly the way you expect a Makefile to act - which cannot be said of
# automake, cmake, or qmake.
#
# The One Makefile lives under the name 'real.make', because it is
# reponsible for doing all the actual building, but it is not the file
# that the user actually calls (using make) directly. The reason for this
# is that the One Makefile requires a certain environment that it cannot
# arrange for on its own.
#
# Specifically:
#   The -r and -R flags must be passed.
#   A list of variables must be included, but regenerating Makefile from
#   Makefile.in is time-consuming, so real.make actually *includes* the
#   file that called it.
#
# For an example of how to do this safely, look at the Makefile.in shipped
# by TMWA. Of course, you could use any other mechanism to generate a
# Makefile, as long as it supplies the appropriate list of variables.


# TODO:
# 1. Implement support for static libraries
#   This should be trivial.
# 2. Implement support for shared libraries
#   This requires building two different .o files and such.
# 3. Implement support for mixed C and C++ source trees
#   This just requires writing more patsubst in various places
#   At that point, they should *probably* be refactored out into functions.
#   However, it would be hard to allow linking some binaries as pure C.
#   Unless maybe we should use .c.o and .cpp.o ?
# 4. See if something can be done about all the mkdirs.
# 5. Remove the few (obvious) bits that are hard-coded for TMWA.
# 6. Handle testing better. I'm guessing I should actually compile just
#   one foo_test.cpp file into each executable test ...
#
# IWBNMI:
# 1. Add 'make check' and 'make installcheck'.
# 2. 'make distclean' should remove "anything that ./configure created".
# 3. 'make dist' should be implemented. Git only or not?
# 4. 'make install' should install documentation.
# 5. Split 'make install-exec' and 'make install-data'. Beware etc and var!
# 6. '--program-prefix' and '--program-suffix' (easy). What about sed?
# 7. Support DESTDIR during 'make install' (URGENT).
# 8. 'make distcheck' using the 'make dist' tarball?
# 9. Add rules for gettext.
# 10. Support per-target build flags? (requires renaming)

ifeq ($(findstring s,$(firstword ${MAKEFLAGS})),)
ifeq (${MAKE_RESTARTS},)
# TODO: should I write this in tengwar?
# The major problem is that it's usually encoded in the PUA
# and thus requires a font specification.
# There *is* a formal request and tentative allocation in the SMP,
# but it has been languishing for 15 years.
# TODO: regardless of the preceding, look up the words for 'build' and 'link'.
# (Does there exist a word that could mean "makefile"?
# Maybe something like 'instructional scroll')

# Note: the space is necessary
$(info )
$(info Welcome to the One Makefile)
$(info Copyright 2012-2013 Ben Longbons)
$(info )
$(info One Makefile to build them all,)
$(info One Makefile to find them,)
$(info One Makefile to bring them all)
$(info and in the darkness link them.)
$(info )
else
$(info The Road goes ever on and on ...)
endif
endif

ifeq ($(findstring r, $(firstword ${MAKEFLAGS})),)
$(error Missing -r - please do not invoke this makefile directly.)
endif

ifeq ($(findstring R, $(firstword ${MAKEFLAGS})),)
$(error Missing -R - please do not invoke this makefile directly.)
endif

ifneq "$(notdir ${MAKEFILE_LIST})" "real.make"
$(error not used as sole toplevel makefile!)
endif

include Makefile # for variables - this is handled VERY carefully

# bash is needed for 'set -o pipefail' below - I have had real bugs there!
# It's just not worth the bother to see if another shell works when it
# needs to *and* fails when it needs to. Just use bash.
SHELL=bash

# path lists
LEXERS := $(shell cd ${SRC_DIR}; find src/ -name '*.lpp')
PARSERS := $(shell cd ${SRC_DIR}; find src/ -name '*.ypp')
GEN_SOURCES := \
    $(patsubst %.lpp,%.cpp,${LEXERS}) \
    $(patsubst %.ypp,%.cpp,${PARSERS})
GEN_HEADERS := \
    $(patsubst %.ypp,%.hpp,${PARSERS})
REAL_SOURCES := $(shell cd ${SRC_DIR}; find src/ -name '*.cpp')
REAL_HEADERS := $(shell cd ${SRC_DIR}; find src/ -name '*.hpp')
SOURCES := ${GEN_SOURCES} ${REAL_SOURCES}
HEADERS := ${GEN_HEADERS} ${REAL_HEADERS}
DEPENDS := $(patsubst src/%.cpp,obj/%.d,${SOURCES})
PREPROCESSED := $(patsubst %.d,%.ii,${DEPENDS})
IRS := $(patsubst %.d,%.ll,${DEPENDS})
BITCODES := $(patsubst %.d,%.bc,${DEPENDS})
ASSEMBLED := $(patsubst %.d,%.s,${DEPENDS})
OBJECTS := $(patsubst %.d,%.o,${DEPENDS})
GEN_DEPENDS := $(patsubst src/%.cpp,obj/%.d,${GEN_SOURCES})
GEN_PREPROCESSED := $(patsubst %.d,%.ii,${GEN_DEPENDS})
GEN_IRS := $(patsubst %.d,%.ll,${GEN_DEPENDS})
GEN_BITCODES := $(patsubst %.d,%.bc,${GEN_DEPENDS})
GEN_ASSEMBLED := $(patsubst %.d,%.s,${GEN_DEPENDS})
GEN_OBJECTS := $(patsubst %.d,%.o,${GEN_DEPENDS})
MAIN_SOURCES := $(filter %/main.cpp,${SOURCES})
BINARIES := $(patsubst src/%/main.cpp,bin/tmwa-%,${MAIN_SOURCES})

TEST_DEPENDS := $(patsubst src/%/test.cpp,obj/%/autolist.d,$(filter %/test.cpp,${SOURCES}))
DEPENDS += ${TEST_DEPENDS}
TEST_BINARIES := $(patsubst obj/%/autolist.d,bin/test-%,${TEST_DEPENDS})

# tricky part

# We can't put comments in a macro so here goes:
# 1: Include the contents of the current %.d file ($1).
# 2: For each header, substitute the corresponding %.o's dependency file.
# 3: Blank, reserved for header->libtmwa-common.a if that gets implemented.
# 4: Remove all non-deps - clutter and lonely headers.
# 5: Prevent infinite loops later by filtering out deps we've already seen.
# 6: Merge new deps into the existing dep list.
# 7: Recurse over all new deps (see 5).
define RECURSIVE_DEPS_IMPL
$(eval more_deps := $(shell cat $(patsubst %/test.d,%/autolist.d,${1})))
$(eval more_deps := $(patsubst src/%.hpp,obj/%.d,${more_deps}))

$(eval more_deps := $(filter ${DEPENDS},${more_deps}))
$(eval more_deps := $(filter-out ${cur_deps},${more_deps}))
$(eval cur_deps += ${more_deps})
$(foreach dep,${more_deps},$(call RECURSIVE_DEPS_IMPL,${dep}))
endef

# 1: Initialize the dep list ($1 is a %.d).
# 2: Call the real function on it.
# 3: Blank for clarity.
# 4: Expand to text. Note that *nothing* else actually produces anything!
define RECURSIVE_DEPS
$(eval cur_deps := ${1})
$(call RECURSIVE_DEPS_IMPL,${1})

${cur_deps}
endef

# Apply the rules to all the main.cpp files
$(foreach exe,${BINARIES},$(eval ${exe}: $(strip $(patsubst %.d,%.o,$(call RECURSIVE_DEPS,$(patsubst bin/tmwa-%,obj/%/main.d,${exe}))))))
$(foreach exe,${TEST_BINARIES},$(eval ${exe}: $(strip $(patsubst %.d,%.o,$(call RECURSIVE_DEPS,$(patsubst bin/test-%,obj/%/test.d,${exe}))))))

# utility functions for the rules
MKDIR_FIRST = @mkdir -p ${@D}

# Stuff sensitive to attoconf
CXXFLAGS += ${WARNINGS}
ifeq (${ENABLE_WARNINGS},yes)
WARNINGS := -include ${SRC_DIR}/src/warnings.hpp
endif
${GEN_DEPENDS} ${GEN_PREPROCESSED} ${GEN_IRS} ${GEN_BITCODES} ${GEN_ASSEMBLED} ${GEN_OBJECTS}: override WARNINGS :=
${GEN_DEPENDS} ${GEN_PREPROCESSED} ${GEN_IRS} ${GEN_BITCODES} ${GEN_ASSEMBLED} ${GEN_OBJECTS}: override CPPFLAGS += -I ${SRC_DIR}/$(patsubst obj/%,src/%,${@D})

# related to gdb bug 15801
ifeq (${ENABLE_ABI6},yes)
CXXFLAGS += -fabi-version=6
endif

# This needs to edit CXX instead of CXXFLAGS in order to make
# the %.ii rule work.
ifeq (${ENABLE_CYGWIN_HACKS},yes)
override CXX += -std=gnu++0x
else
override CXX += -std=c++0x
endif

CXXFLAGS += -fstack-protector
override CXXFLAGS += -fno-strict-aliasing
override CXXFLAGS += -fvisibility=hidden

# actual rules
vpath %.ypp ${SRC_DIR}
vpath %.lpp ${SRC_DIR}
vpath %.cpp ${SRC_DIR}
vpath %.hpp ${SRC_DIR}

.DELETE_ON_ERROR:
.DEFAULT_GOAL := all
# main goals
all: ${BINARIES}
cpp: ${GEN_SOURCES} ${GEN_HEADERS}
ii: ${PREPROCESSED}
ll: ${IRS}
bc: ${BITCODES}
s: ${ASSEMBLED}
o: ${OBJECTS}

mostlyclean:
	rm -rf obj conf-raw
clean: mostlyclean
	rm -rf bin
distclean: clean
	rm -f ${GEN_SOURCES} ${GEN_HEADERS}

%.cpp: %.lpp
	$(MKDIR_FIRST)
	${FLEX} -o $@ $<
%.cpp %.hpp: %.ypp
	$(MKDIR_FIRST)
	${BISON} -d -o $*.cpp $<
obj/%.d: src/%.cpp
	$(MKDIR_FIRST)
	set -o pipefail; \
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -MG -MP -MM $< \
	    -MT '$@ $(patsubst %.d,%.o,$@)' \
	    | sed -e ':again; s:/[^/ ]*/../:/:; t again' \
	    -e 's: ${SRC_DIR}/: :g' \
	    > $@
# the above SRC_DIR replacement is not really safe, but it works okayish.
obj/%.ii: src/%.cpp
	$(MKDIR_FIRST)
	${CXX} ${CPPFLAGS} -E -o $@ $<
obj/%.ll: src/%.cpp
	$(MKDIR_FIRST)
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -S -emit-llvm -o $@ $<
obj/%.bc: src/%.cpp
	$(MKDIR_FIRST)
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c -emit-llvm -o $@ $<
obj/%.s: src/%.cpp
	$(MKDIR_FIRST)
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -S -o $@ $<
obj/%.o: src/%.cpp
	$(MKDIR_FIRST)
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c -o $@ $<

obj/%/autolist.d: $(filter-out %/autolist.d,${DEPENDS})
	echo $@: $(filter %_test.d,$^) > $@
include ${DEPENDS}

# I'm not convinced keeping the bin/ is a good idea
bin/%:
	$(MKDIR_FIRST)
	${CXX} ${LDFLAGS} $^ ${LDLIBS} -o $@

${TEST_BINARIES}: obj/gtest-all.o

# This isn't perfect.
$(filter %_test.o,${OBJECTS}) obj/gtest-all.o: override CPPFLAGS += -DGTEST_HAS_PTHREAD=0 -I${GTEST_DIR}
obj/gtest-all.o: override WARNINGS :=
obj/gtest-all.o: ${GTEST_DIR}/src/gtest-all.cc
	$(MKDIR_FIRST)
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c -o $@ $<

test: $(patsubst bin/%,.run-%,${TEST_BINARIES})
.run-%: bin/%
	$<

install:
	install -d ${DESTDIR}${BINDIR}
	install --backup=${ENABLE_BACKUPS_DURING_INSTALL} -t ${DESTDIR}${BINDIR} \
	    $(wildcard ${BINARIES})
# Is wildcard really the right thing to do? ^
# cases to consider:
#   all binaries built
#   all binaries present, but some outdated. This is hard unless I dep.
#   some binaries built
#   no binaries built
ifeq (${ENABLE_COMPAT_SYMLINKS},yes)
	@echo Installing compatibility symlinks
	ln --backup=${ENABLE_BACKUPS_DURING_INSTALL} -sf tmwa-login ${DESTDIR}${BINDIR}/login-server
	ln --backup=${ENABLE_BACKUPS_DURING_INSTALL} -sf tmwa-char ${DESTDIR}${BINDIR}/char-server
	ln --backup=${ENABLE_BACKUPS_DURING_INSTALL} -sf tmwa-map ${DESTDIR}${BINDIR}/map-server
	ln --backup=${ENABLE_BACKUPS_DURING_INSTALL} -sf tmwa-admin ${DESTDIR}${BINDIR}/ladmin
	ln --backup=${ENABLE_BACKUPS_DURING_INSTALL} -sf tmwa-monitor ${DESTDIR}${BINDIR}/eathena-monitor
else
	@echo Not installing compatibility symlinks
endif
tags: ${SOURCES} ${HEADERS}
	ctags --totals --c-kinds=+px -f $@ $^

Makefile: ${SRC_DIR}/Makefile.in
	@echo Makefile.in updated, you must rerun configure
	@false

include ${SRC_DIR}/version.make

# TODO - fix pattern priority bug so I can make these .hpp
#
# This is complicated and still isn't optimal.
conf-raw/int-%.h: FORCE
	$(MKDIR_FIRST)
	@grep -s -q '^$(value $*)$$' $@ \
	|| { \
	    echo "#define $* \\"; \
	    echo '$(value $*)'; \
	} > $@
conf-raw/str-%.h: FORCE
	$(MKDIR_FIRST)
	@grep -s -q '^"$(value $*)"$$' $@ \
	|| { \
	    echo "#define $* \\"; \
	    echo '"$(value $*)"'; \
	} > $@
FORCE: ;
override CPPFLAGS += -I .
