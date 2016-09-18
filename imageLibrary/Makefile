ZLIB = zlib-1.2.8/libz.a #-Lzlib-1.2.8
CFLAGS = -std=c++11 -O3

all : build main.cpp build/image.o
	cd zlib-1.2.8 && make all && cd ..
	g++ main.cpp build/*.o $(ZLIB) $(CFLAGS) -o main

build :
	- mkdir build #make a place to put the .o files

build/image.o : build/png.o build/jpeg.o
	g++ $(CFLAGS) -c image.cpp -o build/image.o
build/png.o : png.cpp png.h
	g++ $(CFLAGS) -c png.cpp -o build/png.o
build/jpeg.o : jpeg.cpp jpeg.h
	g++ $(CFLAGS) -c jpeg.cpp -o build/jpeg.o

clean :
	- rm *.o
	- rm build/*.o
	- rm main
	cd zlib-1.2.8 && make clean