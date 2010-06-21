all: moneycount

OBJECTS = main.o athena_text.o inf.o portability_fixes.o
CPP = g++

moneycount: $(OBJECTS)
	$(CPP) -o $@ $(OBJECTS) $(COMMON_OBJS) $(LIBS)
main.o: main.cpp
athena_text.o: athena_text.cpp athena_text.h
inf.o: inf.cpp inf.hpp
portability_fixes.o: portability_fixes.cpp portability_fixes.hpp portability_exceptions.hpp

clean:
	rm -f *.o moneycount
