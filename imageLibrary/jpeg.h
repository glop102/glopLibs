#ifndef GLOP_IMAGE_LIBRARY_JPEG
#define GLOP_IMAGE_LIBRARY_JPEG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "image_globals.h"
#include "../binaryTree/binaryTree.h"

namespace GLOP_IMAGE_JPEG{

//============================================================================
//   JPEG IMAGE DECODING
//============================================================================
// https://www.w3.org/Graphics/JPEG/itu-t81.pdf

bool validJPEG(unsigned char *filebuffer);
void unpackImage(unsigned char* filebuffer,int bufferLength, ImageData* data);
void unpackDHT(unsigned char* filebuffer, int x, int size, BinaryTree<int> *tree);

} // GLOP_IMAGE_JPEG

#endif