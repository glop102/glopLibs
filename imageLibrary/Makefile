CFLAGS = -std=c++11 -O2 -lpng -lturbojpeg

main : main.cpp image.a
	g++ main.cpp image.a $(CFLAGS) -o main

build :
	- mkdir build #make a place to put the .o files

image.a : build build/image.o build/png.o build/jpeg.o build/bmp.o
	ar rvs image.a build/*.o

build/image.o :  build image.cpp build/png.o build/jpeg.o build/bmp.o
	g++ $(CFLAGS) -c image.cpp -o build/image.o
build/png.o : png.cpp png.h
	g++ $(CFLAGS) -c png.cpp -o build/png.o
build/jpeg.o : jpeg.cpp jpeg.h
	g++ $(CFLAGS) -c jpeg.cpp -o build/jpeg.o
build/bmp.o : bmp.cpp bmp.h
	g++ $(CFLAGS) -c bmp.cpp -o build/bmp.o

clean :
	rm -f *.o
	rm -f build/*.o
	rm -f main
	rm -f image.a