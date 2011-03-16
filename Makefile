include make.defs

all: login-server char-server map-server ladmin
common:
	${MAKE} -C src/common
login-server: common
	${MAKE} -C src/login
	${CP} src/login/login login-server
char-server: common
	${MAKE} -C src/char
	${CP} src/char/char char-server
map-server: common
	${MAKE} -C src/map
	${CP} src/map/map map-server
ladmin: common
	${MAKE} -C src/ladmin

clean: clean-common clean-login clean-char clean-map clean-ladmin clean-tools

clean-common:
	${MAKE} -C src/common clean
clean-login:
	${MAKE} -C src/login clean
clean-char:
	${MAKE} -C src/char clean
clean-map:
	${MAKE} -C src/map clean
clean-ladmin:
	${MAKE} -C src/ladmin clean

# This target is separate for historical reasons, and because it is optional
tools: common eathena-monitor
eathena-monitor:
	${MAKE} -C src/tool
	${CP} src/tool/eathena-monitor .
clean-tools:
	${MAKE} -C src/tool clean
