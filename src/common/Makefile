include ../../make.defs
txt sql all: core.o socket.o timer.o grfio.o db.o lock.o nullpo.o malloc.o mt_rand.o

core.o: core.c core.h
socket.o: socket.c socket.h mmo.h
timer.o: timer.c timer.h
grfio.o: grfio.c grfio.h
db.o: db.c db.h
lock.o: lock.h
nullpo.o: nullpo.c nullpo.h
malloc.o: malloc.c malloc.h
mt_rand.o: mt_rand.c mt_rand.h

clean:
	rm -f *.o
