include ../../make.defs

all: ladmin

COMMON_OBJ = ../common/core.o ../common/socket.o ../common/timer.o ../common/db.o ../common/malloc.o ../common/mt_rand.o
COMMON_H = ../common/core.h ../common/socket.h ../common/timer.h ../common/mmo.h ../common/version.h ../common/db.h ../common/malloc.h ../common/mt_rand.h

ladmin: ladmin.o ../common/md5calc.o $(COMMON_OBJ)
ladmin.o: ladmin.c ladmin.h ../common/md5calc.h $(COMMON_H)

clean:
	rm -f *.o ladmin
