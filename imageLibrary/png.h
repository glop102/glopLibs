#ifndef GLOP_IMAGE_LIBRARY_PNG
#define GLOP_IMAGE_LIBRARY_PNG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include "image_globals.h"
#include "zlib-1.2.8/zlib.h"

namespace GLOP_IMAGE_PNG{

//=========================================================================
// PNG utillities
//=========================================================================
// This set of functions takes in a normal FILE pointer of the image
//
// The FILe pointer is expected to only be incremented by the functions
// in this file. Please be aware of this as you will likely break things
// if you screw around with the pointer yourself.
//
// Reference - https://en.wikipedia.org/wiki/Portable_Network_Graphics
// Full Data - https://www.w3.org/TR/PNG/
//=========================================================================
// GENERAL FUNCTIONS
//
// validPNG - check if the file is a valid PNG
// unpackImage - puts all the image data into a struct
//             - the struct is made by the calling function
// packImage - makes a file with the image data
//           - this is responsible for doing automatic optimizations
//=========================================================================
// SPECIFIC FUNCTIONS
//
// = = = DECODING = = =
// getNextChunk - get the array of data in the next chunk
//              - returns a struct with the data in it
//__getDataFromStream - get a particular int from the stream at a posistion
//					  - it takes into acocunt the bitdepth
// unpcakPixelStream - using zlib, return a pointer to the stream of pixels
//					 - this is a purly decompressing function
// unfilterPixelStream - given the pixel stream, undo the filter that was applied to it
//					   - this means the pixel values are ready to go
// decodePixelStream - get the pixel values and put them into the proper buffer for the library
//
//
// = = = ENCODING = = =
// encodePixelStream - given the pixels, make a pixel stream to be filtered/compressed/written
// filterPixelStream - find the best filter for each scanline and apply it to the stream
// packPixelStream - compress the pixel stream and write it to the file
//=========================================================================

//---- Data Types
struct ChunkData{
	uint32_t length;
	char type[5];
	unsigned char* data;
	uint32_t CRC;
};

//=========================================================================
//---- GeneralFunctions
bool validPNG(unsigned char* fileBuffer);
void unpackImage(unsigned char* fileBuffer,unsigned int bufferSize,struct ImageData *data); // decode the file that is buffered into usable data
void packImage(FILE* imageFP,struct ImageData *data, SAVE_OPTIONS_PNG saveOptions); // encode the data into the file buffer pointer and tell them how long the buffer is

//---- Specific Functions
// get the compressed pixel stream and unpack it into an array
unsigned char* unpackPixelStream(struct ImageData *data,struct ChunkData chunk,int &curtPos,int bufferSize,bool interlaced); 
// given the pixel stream, put the pixels into the image
void decodePixelStream(struct ImageData *data,unsigned char *pixelStream,std::vector<Pixel> &palleteList,std::vector<Pixel> &transparencyList); 
void decodePixelStream_interlaced(struct ImageData *data,unsigned char *pixelStream,std::vector<Pixel> &palleteList,std::vector<Pixel> &transparencyList);
// given the pixel stream, unfilter the stream so it can be passed to the decode functions
void unfilterPixelStream(struct ImageData *data,unsigned char* pixelStream,std::vector<Pixel> &palleteList);
void unfilterPixelStream_interlaced(struct ImageData *data,unsigned char* pixelStream,std::vector<Pixel> &palleteList);

// compress the pixel stream and write it out to the file
void packPixelStream(unsigned char *pixelStream,int bufferSize,FILE* imageFP,int compressionLevel);
// given the pixel stream, put the pixels into the image
void encodePixelStream(struct ImageData *data,unsigned char *pixelStream,std::vector<Pixel> &palleteList);
void encodePixelStream_interlaced(struct ImageData *data,unsigned char *pixelStream,std::vector<Pixel> &palleteList);
// given the pixel stream, unfilter the stream so it can be passed to the decode functions
void filterPixelStream(struct ImageData *data,unsigned char* pixelStream,std::vector<Pixel> &palleteList);
void filterPixelStream_interlaced(struct ImageData *data,unsigned char* pixelStream,std::vector<Pixel> &palleteList);
// given the image data, we will make a pallete to use for encoding the image - will quantize an image with more than 256 colors
std::vector<Pixel> createPallete(struct ImageData *data);

//---- Utility Functions
// utility to get the next chunk of data from the file
struct ChunkData getNextChunk(unsigned char* fileBuffer, unsigned int startingPos,unsigned int maxLength);
// utility to reverse the bytes in a memory location
void __REVERSE_BYTES(unsigned char*,int);
// given a memory location, get data of bits long from the stream - allowed bits 1,2,4,8,16
uint16_t __getDataFromStream(unsigned char* stream,int bitDepth,int dataNum);
void __setDataIntoStream(uint16_t data,unsigned char* stream,int bitDepth,int dataNum);
//check to see if we have 256 or fewer colors
bool __256orLessColors(ImageData* data);
//check to see if we only have a single color that is transparent in the image
bool __hasSingleTransparentColor(ImageData* data);
//get the first transparent color from the image that it finds - used in conjunction with the above function
Pixel __singleTransparentColor(ImageData* data);
//CRC stuffs
void make_crc_table(void); // pre-computes things to put into a table
unsigned long update_crc(unsigned long crc, unsigned char *buf,int len); // take a currently used crc and add more bytes to what it represents
unsigned long crc(unsigned char *buf, int len); // make a crc of a buffer of some length
//gives us the total length of the pixel stream - useful for determining buffer size
unsigned long getPixelStreamSize(struct ImageData *data);
unsigned long getPixelStreamSize_interlaced(struct ImageData *data);

} //GLOP_IMAGE_PNG

#endif //GLOP_IMAGE_LIBRARY_PNG
