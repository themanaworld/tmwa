include ../../make.defs

OBJ = main.o parse.o generate.o htmlstyle.o logs.o pages/about.o pages/sample.o pages/notdone.o
all: webserver
webserver: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ}
	



clean:
	rm -f *.o
	rm -f pages/*.o
	rm -f webserver
	
