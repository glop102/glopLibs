ZLIB = zlib-1.2.8/libz.a #-Lzlib-1.2.8
CFLAGS = -std=c++11 -O2

all : main.cpp image.a
	g++ main.cpp image.a $(CFLAGS) -o main

build :
	- mkdir build #make a place to put the .o files

image.a : build build/image.o build/png.o build/jpeg.o build/bmp.o zlib-1.2.8/libz.a
	ar rvs image.a zlib-1.2.8/*.o build/*.o

build/image.o :  build build/png.o build/jpeg.o build/bmp.o
	g++ $(CFLAGS) -c image.cpp -o build/image.o
build/png.o : png.cpp png.h
	g++ $(CFLAGS) -c png.cpp -o build/png.o
build/jpeg.o : jpeg.cpp jpeg.h
	g++ $(CFLAGS) -c jpeg.cpp -o build/jpeg.o
build/bmp.o : bmp.cpp bmp.h
	g++ $(CFLAGS) -c bmp.cpp -o build/bmp.o
zlib-1.2.8/libz.a :
	cd zlib-1.2.8 && make all && cd ..

clean :
	- rm *.o
	- rm build/*.o
	- rm main
	- rm image.a
	cd zlib-1.2.8 && make clean