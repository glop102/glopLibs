#ifndef GLOP_IMAGE_LIBRARY_BMP
#define GLOP_IMAGE_LIBRARY_BMP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "image_globals.h"

namespace GLOP_IMAGE_BMP{
	bool validBMP(FILE *imageFP);
	void unpackImage(FILE *imageFP, ImageData* data);
	void decodePixelArray(FILE *imageFP, ImageData* data);
	void packImage(FILE* imageFP,struct ImageData *data, SAVE_OPTIONS_BMP saveOptions);
}

#endif