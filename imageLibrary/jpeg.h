#ifndef GLOP_IMAGE_LIBRARY_JPEG
#define GLOP_IMAGE_LIBRARY_JPEG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <jpeglib.h>
#include "image_globals.h"

namespace GLOP_IMAGE_JPEG{

//============================================================================
//   JPEG IMAGE DECODING
//============================================================================
// https://www.w3.org/Graphics/JPEG/itu-t81.pdf
//
// how to use turbojpeg api
// http://stackoverflow.com/questions/9094691/examples-or-tutorials-of-using-libjpeg-turbos-turbojpeg
//
// I have decided to go with the traditional libjpeg api due to some issues
// You can mostly rely on libjpeg being installed on most any system
// https://github.com/Windower/libjpeg/blob/master/libjpeg.txt

bool validJPEG(FILE *imageFP);
void unpackImage(FILE* imageFP, ImageData* data);
void packImage(FILE* imageFP,ImageData* data, SAVE_OPTIONS_JPG saveOptions);

} // GLOP_IMAGE_JPEG

#endif