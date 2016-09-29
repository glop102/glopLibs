#ifndef GLOP_IMAGE_LIBRARY_BMP
#define GLOP_IMAGE_LIBRARY_BMP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "image_globals.h"

namespace GLOP_IMAGE_BMP{
	bool validBMP(unsigned char *filebuffer);
	void unpackImage(unsigned char* filebuffer,int bufferLength, ImageData* data);
	void decodePixelArray(unsigned char* filebuffer,int bufferLength, ImageData* data);
	void packImage(FILE* imageFP,struct ImageData *data, SAVE_OPTIONS_BMP saveOptions);
}

#endif