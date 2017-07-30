.PHONY: all
all: test_prog
.PHONY: shared
shared:glopConfig.so

glopConfig.so: glopConfig.h glopConfig.cpp
	g++ -c -fpic glopConfig.h glopConfig.cpp -o glopConfig.o
	g++ -shared glopConfig.o -o libglopParser.so

glopConfig.o: glopConfig.h glopConfig.cpp
	g++ -c glopConfig.cpp

test_prog: glopConfig.o test_prog.cpp
	g++ test_prog.cpp glopConfig.o -o test_prog --std=c++11

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf test_prog