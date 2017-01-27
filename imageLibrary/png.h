#ifndef GLOP_IMAGE_LIBRARY_PNG
#define GLOP_IMAGE_LIBRARY_PNG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <png.h>
#include <zlib.h>
#include "image_globals.h"

namespace GLOP_IMAGE_PNG{
	bool validPNG(FILE* imageFP);
	void unpackImage(FILE* imageFP,struct ImageData *data);
	void packImage(FILE* imageFP,struct ImageData *data, SAVE_OPTIONS_PNG saveOptions);
} //GLOP_IMAGE_PNG

#endif //GLOP_IMAGE_LIBRARY_PNG
