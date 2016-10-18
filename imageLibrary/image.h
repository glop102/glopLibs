

//==================================================================================
// GLOP IMAGE LIBRARY
//==================================================================================
// C++ based system for working with several different formats.
//
// The point of this library is that i could not find a simple interface for just 
// opening a picture and getting BOTH the pixel data and any associated text
// information. There are numerous libraries that i have found that will do this
// though i did not like any of the APIs. I like just getting my hands on the data
// to do things and not learn how 100 different function calls work because they
// have some extra baggage of pre-built function like resizing and such.
//==================================================================================
// HOW THIS WORKS - the client side
//==================================================================================
// - Make an 'Image' object
//   - pass in an open file
//   - pass in a filename
//   - pass in a char*array and associated pixel info
//   - pass in just width/height/PixelType for a blank one
// - Query for pixels
//   - imageObject.getPixel(x,y,RGBA);
//   - imageObject[x,y,RGBA];
//   - you must specify only R/G/B/A to get that one channel - no multichanel return
//     - enum {R,G,B,A}
//     - or just pass in 0,1,2,3
//==================================================================================
// HOW THIS WORKS - in the backend
//==================================================================================
// - The image filename or filepointer is passed into the constructor.
// - We proced down a list of functions (in other headers) that ask if it is valid
//   for that certain image type.
// - Once we find a valid image format to decode from, we unroll all the data we can
//   into a nice little struct. This struct is really just to make it simpler for 
//   the image specific functions to deal with.
// - on getting the struct, we copy out all the info, and are ready for user querys
//   or modifications
//==================================================================================

#ifndef GLOP_IMAGE_LIBRARY
#define GLOP_IMAGE_LIBRARY

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <math.h>
#include "image_globals.h"
#include "png.h"
#include "jpeg.h"
#include "bmp.h"

class Image{
private:
	struct ImageData imageData; // holds the current frame of the image
	std::vector<struct ImageData> frames; // holds multiple frames to allow motion images
	int currentFrame;
	bool constructImage_fromFile(FILE*); //does the heavy lifting of making the image struct from a file that we have buffered in
public:

	//CONSTRUCTORS - just reads in the buffered data and makes the base struct of data
	//These make a raw struct with much of it empty
	Image(unsigned long width,unsigned long height,PixelType pixelType);
	Image(unsigned char* pixelData,unsigned long width,unsigned long height,PixelType pixelType); //reading in buffered image data - 8bitdepth only
	//These read in a file and make the struct from an image
	Image(FILE* imageFP);
	Image(std::string filename);
	~Image();
	//maintaing the defaults for move operations
	Image() = default;
	Image(const Image &) = default;
	Image(Image &&) = default;
	Image& operator=(const Image&) = default;
	Image& operator=(Image&&) = default;

	uint16_t getPixel(unsigned long width,unsigned long height,int channel);
	std::string getTextValue(std::string key);
	std::vector<std::string> getTextKeys();
	std::vector<std::string> getTags();
	unsigned long width();
	unsigned long height();
	Pixel* operator[](unsigned int y);
	int getBitDepth();
	void setBitDepth(int);

	int numberOfFrames();
	void gotoFrame(int frame);
	int nextFrame(); // returns the frame number we just went to
	int getCurrentFrame();

	//save the image
	void saveImage(std::string filename,SAVE_TYPE saveType, uint16_t saveOptions); // to a file
	void saveImage(FILE* imageFP,SAVE_TYPE saveType, uint16_t saveOptions); //to a buffer for you to write to a file yourself
};

void printPixel(Pixel p);

#endif