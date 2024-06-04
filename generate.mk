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

stamp/generated.stamp:
	$(MKDIR_FIRST)
	touch $@

stamp/generated.stamp: stamp/generate-proto2.stamp
stamp/generate-proto2.stamp: tools/protocol.py
	$(MKDIR_FIRST)
	rm -f stamp/generated.stamp
	mkdir -p ${SRC_DIR}/src/proto2
	cd ${SRC_DIR} && protocol.py
	touch $@

stamp/generated.stamp: stamp/generate-debug-debug.stamp
stamp/generate-debug-debug.stamp: tools/debug-debug-scripts ${PIES}
	$(MKDIR_FIRST)
	rm -f stamp/generated.stamp
	mkdir -p ${SRC_DIR}/src/debug-debug
	rm -f ${SRC_DIR}/src/debug-debug/test.cpp
	debug-debug-scripts ${SRC_DIR}/src/debug-debug/ $(wordlist 2,$(words $^),$^)
	touch $@

stamp/generated.stamp: stamp/generate-config.stamp
stamp/generate-config.stamp: tools/config.py
	$(MKDIR_FIRST)
	rm -f stamp/generated.stamp
	cd ${SRC_DIR} && config.py
	touch $@
