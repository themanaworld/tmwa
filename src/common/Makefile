include ../../make.defs
all: core.o socket.o timer.o grfio.o db.o lock.o nullpo.o mt_rand.o md5calc.o

core.o: core.c core.h
socket.o: socket.c socket.h mmo.h
timer.o: timer.c timer.h
grfio.o: grfio.c grfio.h
db.o: db.c db.h
lock.o: lock.c lock.h
nullpo.o: nullpo.c nullpo.h
mt_rand.o: mt_rand.c mt_rand.h
md5calc.o: md5calc.c md5calc.h

clean:
	rm -f *.o
