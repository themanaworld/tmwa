VERSION_FULL := $(shell git --git-dir=${SRC_DIR}/.git describe --tags HEAD)
VERSION_HASH := $(shell git --git-dir=${SRC_DIR}/.git rev-parse HEAD)

version_bits := $(subst v, ,$(subst -, ,$(subst ., ,${VERSION_FULL})))
VERSION_MAJOR := $(word 1,${version_bits})
VERSION_MINOR := $(word 2,${version_bits})
VERSION_PATCH := $(word 3,${version_bits})
VERSION_DEVEL := $(word 4,${version_bits})
ifeq "${VERSION_DEVEL}" ""
    VERSION_DEVEL := 0
endif
# tracking dirty trees is more trouble than it's worth
#VERSION_FULL += $(shell cd ${SRC_DIR}; git diff --quiet HEAD || echo dirty)

VENDOR := Vanilla
VENDOR_VERSION := 0
