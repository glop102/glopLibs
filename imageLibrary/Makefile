CFLAGS = -std=c++11 -O2 -lpng -ljpeg

all: image.a libimage.so main
.PHONY:all

main : main.cpp image.a
	g++ main.cpp $(CFLAGS) -L. -limage -Wl,-rpath=. -o main

build :
	- mkdir build #make a place to put the .o files

libimage.so: image.a
	g++ -shared -Wl,-soname,libimage.so -o libimage.so build/*.o -lpng -ljpeg

image.a : build build/image.o build/png.o build/jpeg.o build/bmp.o
	ar rvs image.a build/*.o

build/image.o :  build image.cpp image.h
	g++ $(CFLAGS) -fPIC -c image.cpp -o build/image.o
build/png.o : png.cpp png.h
	g++ $(CFLAGS) -fPIC -c png.cpp -o build/png.o
build/jpeg.o : jpeg.cpp jpeg.h
	g++ $(CFLAGS) -fPIC -c jpeg.cpp -o build/jpeg.o
build/bmp.o : bmp.cpp bmp.h
	g++ $(CFLAGS) -fPIC -c bmp.cpp -o build/bmp.o


#============================================================================
# base libraries that i was using
#============================================================================
libjpeg_turbo: libjpeg-turbo-1.5.1/.libs/libturbojpeg.a
	-mkdir build/jpeg
	cd build/jpeg; ar -x ../../libjpeg-turbo-1.5.1/.libs/libturbojpeg.a
libjpeg-turbo-1.5.1/.libs/libturbojpeg.a:
	cd libjpeg-turbo-1.5.1; ./configure; make

libpng: libpng-1.6.28/.libs/libpng16.a
	-mkdir build/png
	cd build/png; ar -x ../../libpng-1.6.28/.libs/libpng16.a
libpng-1.6.28/.libs/libpng16.a:
	cd libpng-1.6.28; ./configure; make

zlib: zlib-1.2.11/libz.a
	-mkdir build/zlib
	cd build/zlib; ar -x ../../zlib-1.2.11/libz.a
zlib-1.2.11/libz.a:
	cd zlib-1.2.11; ./configure; make

clean :
	rm -f *.o
	rm -f *.so
	rm -rf build/*
	rm -f main # test executable
	rm -f image.a libimage.so # the library files
	rm -f temp* # known temp files

distclean: full-clean
full-clean: clean
	- cd libjpeg-turbo-1.5.1 && make distclean
	- cd libpng-1.6.28 && make distclean
	- cd zlib-1.2.11 && make distclean