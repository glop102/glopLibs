TARGETS = imageLibrary/image.a

all : $(TARGETS)
	g++ -shared $(TARGETS) -o libGlopLibs.so

imageLibrary/build/image.a:
	cd imageLibrary && make image.a && cd ..

clean :
	cd imageLibrary && make clean && cd ..
	- rm glopLibs.so