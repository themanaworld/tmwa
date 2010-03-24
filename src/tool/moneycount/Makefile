all: moneycount

OBJECTS = main.o athena_text.o
CPP = g++

moneycount: $(OBJECTS)
	$(CPP) -o $@ $(OBJECTS) $(COMMON_OBJS) $(LIBS)
main.o: main.cpp
athena_text.o: athena_text.cpp athena_text.h


clean:
	rm -f *.o moneycount
