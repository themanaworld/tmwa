# $Id$

ifeq ($(shell uname -m), x86_64)
M32=-m32
endif

CC = gcc ${M32} -pipe

PLATFORM = $(shell uname)

ifeq ($(findstring FreeBSD,$(PLATFORM)), FreeBSD)
MAKE = gmake
else
MAKE = make
endif

OPT = -g -O2 ${M32}

ifeq ($(findstring CYGWIN,$(PLATFORM)), CYGWIN)
OS_TYPE = -DCYGWIN
CFLAGS = $(OPT) -Wall -DFD_SETSIZE=4096 -I../common $(PACKETDEF) $(OS_TYPE)
else
OS_TYPE =
CFLAGS = $(OPT) -Wall -Wno-pointer-sign -I../common $(PACKETDEF) $(OS_TYPE)
endif

MKDEF = CC="$(CC)" CFLAGS="$(CFLAGS)"


all clean: src/common/GNUmakefile src/login/GNUmakefile src/char/GNUmakefile src/map/GNUmakefile src/ladmin/GNUmakefile
	cd src ; cd common ; $(MAKE) $(MKDEF) $@ ; cd ..
	cd src ; cd login ; $(MAKE) $(MKDEF) $@ ; cd ..
	cd src ; cd char ; $(MAKE) $(MKDEF) $@ ; cd ..
	cd src ; cd map ; $(MAKE) $(MKDEF) $@ ; cd ..
	cd src ; cd ladmin ; $(MAKE) $(MKDEF) $@ ; cd ..

tools:
	cd tool && $(MAKE) $(MKDEF) && cd ..
	$(CC) -o setupwizard setupwizard.c	

src/common/GNUmakefile: src/common/Makefile
	sed -e 's/$$>/$$^/' src/common/Makefile > src/common/GNUmakefile
src/login/GNUmakefile: src/login/Makefile
	sed -e 's/$$>/$$^/' src/login/Makefile > src/login/GNUmakefile
src/char/GNUmakefile: src/char/Makefile
	sed -e 's/$$>/$$^/' src/char/Makefile > src/char/GNUmakefile
src/map/GNUmakefile: src/map/Makefile
	sed -e 's/$$>/$$^/' src/map/Makefile > src/map/GNUmakefile
src/ladmin/GNUmakefile: src/ladmin/Makefile
	sed -e 's/$$>/$$^/' src/ladmin/Makefile > src/ladmin/GNUmakefile
src/txt-converter/login/GNUmakefile: src/txt-converter/login/Makefile
	sed -e 's/$$>/$$^/' src/txt-converter/login/Makefile > src/txt-converter/login/GNUmakefile
src/txt-converter/char/GNUmakefile: src/txt-converter/char/Makefile
	sed -e 's/$$>/$$^/' src/txt-converter/char/Makefile > src/txt-converter/char/GNUmakefile
