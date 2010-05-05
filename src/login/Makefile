include ../../make.defs

all: login-server
txt: login-server

COMMON_OBJ = ../common/core.o ../common/socket.o ../common/timer.o ../common/db.o ../common/lock.o ../common/malloc.o ../common/mt_rand.o
COMMON_H = ../common/core.h ../common/socket.h ../common/timer.h ../common/mmo.h ../common/version.h ../common/db.h ../common/lock.h ../common/malloc.h ../common/mt_rand.h

login-server: login.o md5calc.o $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o ../../$@ login.o md5calc.o $(COMMON_OBJ)
login.o: login.c login.h md5calc.h $(COMMON_H)
md5calc.o: md5calc.c md5calc.h

clean:
	rm -f *.o ../../login-server
