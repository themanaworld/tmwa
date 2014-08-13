SHELL=bash

MKDIR_FIRST = @mkdir -p ${@D}
SRC_DIR := $(patsubst %/,%,$(dir ${MAKEFILE_LIST}))
PIES := $(shell cd ${SRC_DIR}; find src/ -name '*.py')
PIES := $(filter-out src/main-gdb-%.py,${PIES})

export PATH:=$(realpath ${SRC_DIR}/tools):${PATH}

vpath %.cpp ${SRC_DIR}
vpath %.hpp ${SRC_DIR}
vpath %.tcc ${SRC_DIR}
vpath tools/% ${SRC_DIR}
vpath %.py ${SRC_DIR}

obj/generated.stamp:
	$(MKDIR_FIRST)
	touch $@
obj/generated.stamp: obj/generate-proto2.stamp
obj/generate-proto2.stamp: tools/protocol.py
	$(MKDIR_FIRST)
	rm -f obj/generated.stamp
	mkdir -p ${SRC_DIR}/src/proto2
	cd ${SRC_DIR} && protocol.py
	touch $@
obj/generated.stamp: obj/generate-debug-debug.stamp
obj/generate-debug-debug.stamp: tools/debug-debug-scripts ${PIES}
	$(MKDIR_FIRST)
	rm -f obj/generated.stamp
	mkdir -p ${SRC_DIR}/src/debug-debug
	debug-debug-scripts $(wordlist 2,$(words $^),$^) > ${SRC_DIR}/src/debug-debug/test.cpp
	touch $@

