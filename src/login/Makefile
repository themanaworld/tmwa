include ../../make.defs

all: txt
txt: login

COMMON_OBJ = ../common/core.o ../common/socket.o ../common/timer.o ../common/db.o ../common/lock.o ../common/malloc.o ../common/mt_rand.o ../common/md5calc.o
COMMON_H = ../common/core.h ../common/socket.h ../common/timer.h ../common/mmo.h ../common/version.h ../common/db.h ../common/lock.h ../common/malloc.h ../common/mt_rand.h ../common/md5calc.h

login: $(COMMON_OBJ)
login.o: login.c login.h $(COMMON_H)

clean:
	rm -f *.o login
