.PHONY: all
all: test_prog
.PHONY: shared
shared:glopParser.so

glopParser.so: glopParser.h glopParser.cpp
	g++ -c -fpic glopParser.h glopParser.cpp -o glopParser.o
	g++ -shared glopParser.o -o libglopParser.so

glopParser.o: glopParser.h glopParser.cpp
	g++ -c glopParser.cpp

test_prog: glopParser.o test_prog.cpp
	g++ test_prog.cpp glopParser.o -o test_prog --std=c++11

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf test_prog