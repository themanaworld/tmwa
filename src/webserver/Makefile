include ../../make.defs

all:
	#Generate framework...
	$(CC) $(CFLAGS) -c parse.c
	$(CC) $(CFLAGS) -c generate.c
	$(CC) $(CFLAGS) -c htmlstyle.c
	$(CC) $(CFLAGS) -c logs.c
	
	#Generate "pages"...
	cd pages && $(CC) $(CFLAGS) -c about.c && cd ..
	cd pages && $(CC) $(CFLAGS) -c sample.c && cd ..
	cd pages && $(CC) $(CFLAGS) -c notdone.c && cd ..

	#Building the server...
	$(CC) $(CFLAGS) -o webserver main.c parse.o generate.o htmlstyle.o \
	logs.o pages/about.o pages/sample.o pages/notdone.o
	



clean:
	rm -f *.o
	rm -f pages/*.o
	rm -f webserver
	
