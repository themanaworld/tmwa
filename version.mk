ABI_VERSION := 0

# These lines are automatically replaced in tarballs generated by 'make dist'
# Note so you stop forgetting: export-subst attribute can't do tag-relative
VERSION_FULL := $(shell git --git-dir=${SRC_DIR}/.git describe --tags HEAD)
VERSION_HASH := $(shell git --git-dir=${SRC_DIR}/.git rev-parse HEAD)

version_bits := $(subst v, ,$(subst -, ,$(subst ., ,${VERSION_FULL})))
# Note: these four numbers are limited to 255.
# Currently, my tags are yy.mm.dd instead of semantic versioning.
# The dev number is how many commits since the last tag.
VERSION_MAJOR := $(word 1,${version_bits})
VERSION_MINOR := $(word 2,${version_bits})
VERSION_PATCH := $(word 3,${version_bits})
VERSION_DEVEL := $(word 4,${version_bits})
ifeq "${VERSION_DEVEL}" ""
    VERSION_DEVEL := 0
endif



# Settings for those distributing modified versions.
#
# It is strongly recommended that all distributors set these,
# but the only *requirement* is that modified versions that are run
# (semi)publicly give VENDOR_SOURCE to satisfy section 13 of the AGPLv3.

# TODO should these be passed to configure instead?

# Vendor Name: String (no newlines, no parentheses)
# This is usually one word, and does not (usually) change over time.
# (Examples: Gentoo, Debian, Fedora, Ubuntu)
VENDOR_NAME := Vanilla
# Vendor Point: Integer (max value 65535)
# This is intended to be the "packaging revision number", assuming that's
# an integer. At a minimum, please try to make it nonzero if you have
# any non-upstream patches (unconditionally nonzero is also okay).
# (If your revision 0 package has patches ... please be nicer to upstream)
VENDOR_POINT := 0
# URL where the source may be found (after searching for version number).
# See AGPLv3 section 13
VENDOR_SOURCE := https://git.themanaworld.org/legacy/tmwa

# Convenience
VERSION_STRING := TMWA ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} dev${VERSION_DEVEL} +${VENDOR_POINT} (${VENDOR_NAME})
VERSION_DOTS := ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_DEVEL}.${VENDOR_POINT}

# Shared libraries
SO_SHORT := so.${ABI_VERSION}
SO_LONG := ${SO_SHORT}.${VERSION_DOTS}
# and thanks for all the fish

# This is a phony target, so that it always runs.
# Targets which depend on this will always have their recipes run.
FORCE:: ;
.PHONY: FORCE

# Fully generate version.hpp here, where we have all the relevant information.
# version.mk is included by the top level Makefile, so simply explaning how to
# make it here will let it be built later, when needed.
# Note that some variable substitutions are slightly different here to use the
# name used by standard CMake macros, such as PROJECT_VERSION_TWEAK instead of
# VERSION_DEVEL.
src/conf/version.hpp: src/conf/version.hpp.in FORCE
	sed -e 's/@VERSION_FULL@/${VERSION_FULL}/g' \
	    -e 's/@VERSION_HASH@/${VERSION_HASH}/g' \
	    -e 's/@VERSION_STRING@/${VERSION_STRING}/g' \
	    -e 's/@PROJECT_VERSION_MAJOR@/${VERSION_MAJOR}/g' \
	    -e 's/@PROJECT_VERSION_MINOR@/${VERSION_MINOR}/g' \
	    -e 's/@PROJECT_VERSION_PATCH@/${VERSION_PATCH}/g' \
	    -e 's/@PROJECT_VERSION_TWEAK@/${VERSION_DEVEL}/g' \
	    -e 's/@VENDOR_NAME@/${VENDOR_NAME}/g' \
	    -e 's/@VENDOR_POINT@/${VENDOR_POINT}/g' \
	    -e 's|@VENDOR_SOURCE@|${VENDOR_SOURCE}|g' \
	    -e 's/@ABI_VERSION@/${ABI_VERSION}/g' \
	    -e 's/@SO_SHORT@/${SO_SHORT}/g' \
	    -e 's/@SO_LONG@/${SO_LONG}/g' \
	    $< > $@


version:
	@echo version '${VERSION_STRING}'
	@echo based on commit ${VERSION_FULL} aka ${VERSION_HASH}
	@echo source ${VENDOR_SOURCE}
	@echo abi version ${ABI_VERSION}
	@echo 'lib so -> ${SO_SHORT} -> ${SO_LONG}'
.PHONY: version