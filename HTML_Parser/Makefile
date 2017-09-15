HEADERS=glopHTML.h
SOURCES=glopHTML.cpp
OBJS=glopHTML.o

glopHTML.o: glopHTML.h glopHTML.cpp
	g++ -o glopHTML.o -c glopHTML.cpp -std=c++11

test: test.cpp $(OBJS)
	g++ -o test.o -c test.cpp -std=c++11
	g++ -o test test.o $(OBJS) -l gtest -l pthread -l gtest_main
