#ifndef GLOP_ZIP_LIB
#define GLOP_ZIP_LIB

#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <stdio.h>
#include <unistd.h>

struct ZipEntry{
	std::string filename;
	bool loaded; //has it been loaded in from disk yet?
	unsigned char *data; //the file data itself
	unsigned int dataSize;
	uint32_t crc;
	unsigned int headerOffset;
};

class ZipFile{
	std::vector<ZipEntry> files;
public:
	ZipFile(); //make an empty zip file
	ZipFile(std::string); // open a file to read or modify
	~ZipFile();

	void addFile(std::string filename, char* data, unsigned int size);
	void addFile(std::string filename);
	ZipFile readFile(std::string filename);

	void saveFile(std::string filename);
	void saveFile(FILE* fff);
	void saveFile(unsigned int fd);
};

#endif