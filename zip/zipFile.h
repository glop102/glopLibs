#ifndef GLOP_ZIP_LIB
#define GLOP_ZIP_LIB

#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

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

ZipFile::ZipFile(){
	//vector of files starts empty
}
ZipFile::~ZipFile(){
	for(int x=0;x<files.size();x++){
		free(files[x].data);
		files[x].data=NULL;
	}
}

void ZipFile::addFile(std::string filename, char* data, unsigned int size){
	ZipEntry entry;
	entry.filename=filename;
	entry.data=(unsigned char*)malloc(size);
	entry.dataSize=size;
	memcpy(entry.data,data,size);
	entry.loaded=true;

	files.push_back(entry);
}
void ZipFile::addFile(std::string filename){
	FILE* fff=fopen(filename.c_str(),"rb");
	if(fff==NULL)return; // error

	ZipEntry entry;
	entry.data=(unsigned char*)malloc(10000);
	while(!feof(fff)){
		int written = fread(entry.data,1,9999,fff); //read until the end of the file - ussually only 1 or two of these operations
	}
	free(entry.data);

	entry.dataSize=ftell(fff);
	entry.data=(unsigned char*)malloc(entry.dataSize);
	fseek(fff,0,SEEK_SET); //reset to the beginning
	int written = fread(entry.data,1,entry.dataSize,fff);

	entry.filename=filename;
	entry.loaded=true;
	//entry data
	//entry data size
	entry.crc=0;

	files.push_back(entry);
}

//================================================================================================================
//  Saving File Functions
//================================================================================================================

unsigned int offsetFromBeginning;

void writeHeader(FILE* fff, ZipEntry &entry){
	entry.headerOffset=offsetFromBeginning; // keep track of where the
	offsetFromBeginning+=30;
	offsetFromBeginning+=entry.filename.length();
	offsetFromBeginning+=entry.dataSize;

	// magic number for the block
	uint8_t temp[4] = {0x50,0x4b,0x03,0x04};
	fwrite(&temp,4,1,fff);

	//minimum version needed to extract
	temp[0]=0;
	temp[1]=0;
	temp[2]=0;
	temp[3]=0;
	fwrite(temp,2,1,fff);

	//General purpose flags
	temp[0]=0;
	temp[1]=0;
	temp[2]=0;
	temp[3]=0;
	fwrite(temp,2,1,fff);

	//compression method
    // no compression
	temp[0]=0;
	temp[1]=0;
	temp[2]=0;
	temp[3]=0;
	fwrite(temp,2,1,fff);

	//modification time
	temp[0]=0;
	temp[1]=0;
	temp[2]=0;
	temp[3]=0;
	fwrite(temp,2,1,fff);

	//modification date
	temp[0]=0;
	temp[1]=0;
	temp[2]=0;
	temp[3]=0;
	fwrite(temp,2,1,fff);

	//CRC32 code
	temp[0]=0;
	temp[1]=0;
	temp[2]=0;
	temp[3]=0;
	fwrite(temp,4,1,fff);

	//compressed size
	//same size since we did not compress at all
	temp[0]= (entry.dataSize>>(8*0)) & 255;
	temp[1]= (entry.dataSize>>(8*1)) & 255;
	temp[2]= (entry.dataSize>>(8*2)) & 255;
	temp[3]= (entry.dataSize>>(8*3)) & 255;
	fwrite(temp,4,1,fff);

	//uncompressed size
	temp[0]= (entry.dataSize>>(8*0)) & 255;
	temp[1]= (entry.dataSize>>(8*1)) & 255;
	temp[2]= (entry.dataSize>>(8*2)) & 255;
	temp[3]= (entry.dataSize>>(8*3)) & 255;
	fwrite(temp,4,1,fff);

	//filename size
	temp[0]= (entry.filename.length()>>(8*0)) & 255;
	temp[1]= (entry.filename.length()>>(8*1)) & 255;
	temp[2]=0;
	temp[3]=0;
	fwrite(temp,2,1,fff);

	//extra field size
	//we dont use it so 0
	temp[0]=0;
	temp[1]=0;
	temp[2]=0;
	temp[3]=0;
	fwrite(temp,2,1,fff);

	//filename itself
	fwrite(entry.filename.c_str(),entry.filename.length(),1,fff);
}

void writeFile(FILE* fff, ZipEntry &entry){
	//honestly this is overkill for this single line, but in the future i may want to add support for compression and such
	fwrite(entry.data,entry.dataSize,1,fff);
}

void writeCentralDirectory(FILE* fff, std::vector<ZipEntry> &files){
	for(int x=0; x<files.size(); x++){
		ZipEntry entry = files[x];

		//magic number
		uint8_t temp[4] = {0x50,0x4b,0x01,0x02};
		fwrite(temp,4,1,fff);

		//version made by
		temp[0]=0;
		temp[1]=0;
		fwrite(temp,2,1,fff);
		//minimum version
		temp[0]=0;
		temp[1]=0;
		fwrite(temp,2,1,fff);
		//general purpose flags
		temp[0]=0;
		temp[1]=0;
		fwrite(temp,2,1,fff);
		//compression method
		temp[0]=0;
		temp[1]=0;
		fwrite(temp,2,1,fff);
		//last modified time
		temp[0]=0;
		temp[1]=0;
		fwrite(temp,2,1,fff);
		//last modified date
		temp[0]=0;
		temp[1]=0;
		fwrite(temp,2,1,fff);
		//CRC32
		temp[0]=0;
		temp[1]=0;
		temp[2]=0;
		temp[3]=0;
		fwrite(temp,4,1,fff);

		//compressed size
		//same size since we did not compress at all
		temp[0]= (entry.dataSize>>(8*0)) & 255;
		temp[1]= (entry.dataSize>>(8*1)) & 255;
		temp[2]= (entry.dataSize>>(8*2)) & 255;
		temp[3]= (entry.dataSize>>(8*3)) & 255;
		fwrite(temp,4,1,fff);
		//uncompressed size
		temp[0]= (entry.dataSize>>(8*0)) & 255;
		temp[1]= (entry.dataSize>>(8*1)) & 255;
		temp[2]= (entry.dataSize>>(8*2)) & 255;
		temp[3]= (entry.dataSize>>(8*3)) & 255;
		fwrite(temp,4,1,fff);

		//filename size
		temp[0]= (entry.filename.length()>>(8*0)) & 255;
		temp[1]= (entry.filename.length()>>(8*1)) & 255;
		temp[2]=0;
		temp[3]=0;
		fwrite(temp,2,1,fff);
		//extra field length
		temp[0]=0;
		temp[1]=0;
		fwrite(temp,2,1,fff);
		//file comment length
		temp[0]=0;
		temp[1]=0;
		fwrite(temp,2,1,fff);
		//disk number where the file starts
		temp[0]=0;
		temp[1]=0;
		fwrite(temp,2,1,fff);
		//internal file attributes
		temp[0]=0;
		temp[1]=0;
		fwrite(temp,2,1,fff);
		//external file attributes
		temp[0]=0;
		temp[1]=0;
		temp[2]=0;
		temp[3]=0;
		fwrite(temp,4,1,fff);
		//offset of file header from beginning of archive
		temp[0]= (entry.headerOffset>>(8*0)) & 255;
		temp[1]= (entry.headerOffset>>(8*1)) & 255;
		temp[2]= (entry.headerOffset>>(8*2)) & 255;
		temp[3]= (entry.headerOffset>>(8*3)) & 255;
		fwrite(temp,4,1,fff);

		//filename itself
		fwrite(entry.filename.c_str(),entry.filename.length(),1,fff);
	}
}

void writeEndOfcentralDirectory(FILE* fff,std::vector<ZipEntry> &files){

	int numberOfBytes_directory=0; // number of bytes the central directory is offset from the beginning of the archive
	for(int x=0;x<files.size();x++){
		numberOfBytes_directory+=46; // fixed width fields
		numberOfBytes_directory+=files[x].filename.length();
	}

	//============================================================================

	//magic number
	uint8_t temp[4]={0x50,0x4b,0x05,0x06};
	fwrite(temp,4,1,fff);

	//what disk is this on
	temp[0]=0;
	temp[1]=0;
	fwrite(temp,2,1,fff);
	//disk that the central directory starts on
	temp[0]=0;
	temp[1]=0;
	fwrite(temp,2,1,fff);
	//number of central directory records on this disk
	temp[0]= files.size()     & 255;
	temp[1]=(files.size()>>8) & 255;
	fwrite(temp,2,1,fff);
	//number of central directory records total
	temp[0]= files.size()     & 255;
	temp[1]=(files.size()>>8) & 255;
	fwrite(temp,2,1,fff);

	//size of central directory
	temp[0]= (numberOfBytes_directory>>(8*0)) & 255;
	temp[1]= (numberOfBytes_directory>>(8*1)) & 255;
	temp[2]= (numberOfBytes_directory>>(8*2)) & 255;
	temp[3]= (numberOfBytes_directory>>(8*3)) & 255;
	fwrite(temp,4,1,fff);
	//offset of the central directory from the beginning of the archive
	temp[0]= (offsetFromBeginning>>(8*0)) & 255;
	temp[1]= (offsetFromBeginning>>(8*1)) & 255;
	temp[2]= (offsetFromBeginning>>(8*2)) & 255;
	temp[3]= (offsetFromBeginning>>(8*3)) & 255;
	fwrite(temp,4,1,fff);

	//comment length
	temp[0]=0;
	temp[1]=0;
	fwrite(temp,2,1,fff);
}

void ZipFile::saveFile(std::string filename){
	FILE* fff=fopen(filename.c_str(),"wb");
	if(fff==NULL) return; // error

	saveFile(fff);

	fclose(fff);
}
void ZipFile::saveFile(FILE* fff){
	offsetFromBeginning=0; //reset to start counting again

	for(int x=0; x<files.size(); x++){
		writeHeader(fff,files[x]);
		writeFile(fff,files[x]);
	}

	writeCentralDirectory(fff,files);
	writeEndOfcentralDirectory(fff,files);
}
void ZipFile::saveFile(unsigned int fd){
	FILE* fff=fdopen(fd,"w");
	if(fff==NULL) return; // error

	saveFile(fff);

	//WARNING: DO NOT fclose()
	//the pointer will be closed when a call to close() is done
}

#endif