all: cli
	#Compiled cli

cli: library
	g++ cliTest.cpp zipFile.o -o cli.out

library:
	g++ -c zipFile.cpp -o zipFile.o
