#include "jpeg.h"

namespace GLOP_IMAGE_JPEG{
	
bool validJPEG(FILE* imageFP){
	fseek(imageFP,0,SEEK_SET); //reset to the beginning
	if(getc(imageFP) == 0xFF && getc(imageFP) == 0xD8) return true;
	return false;
}

void unpackImage(FILE* imageFP, ImageData* data){
	#ifdef JPEG_DEBUG
		printf("Found Valid JPEG\n");
	#endif
	fseek(imageFP, 0L, SEEK_END);
	unsigned int bufferLength = ftell(imageFP);
	fseek(imageFP,0,SEEK_SET); //reset to the beginning

	unsigned char* filebuffer=(unsigned char*)malloc(bufferLength);
	int written = fread(filebuffer,1,bufferLength,imageFP);
	if(written != bufferLength){
		printf("Error - could not read all the file bytes\n");
	}

	int jpegSubsamp; // will be given the subsampling that was used to encode the jpeg
	unsigned char *buffer;

	//get setup
	tjhandle _jpegDecompressor = tjInitDecompress();
	//retrieve the basic info about the file
	tjDecompressHeader2(_jpegDecompressor, filebuffer, bufferLength, (int*)&(data->width), (int*)&(data->height), &jpegSubsamp);
	printf("retrieved the file info\n");
	printf("\t%u x %u\n",data->width,data->height);
	buffer = (unsigned char*)malloc(data->width*data->height*3); // will contain the decompressed image
	printf("about to decompress\n");
	//do the acctuall decompression
	tjDecompress2(_jpegDecompressor, filebuffer, bufferLength, buffer, data->width, 0/*pitch*/, data->height, TJPF_RGB, TJFLAG_FASTDCT);
	printf("decoded the pixels\n");

	tjDestroy(_jpegDecompressor);
	free(filebuffer);

	data->pixels = (Pixel*)malloc(data->height * data->width * sizeof(Pixel));
	data->bitDepth = 8;
	data->pixelType = RGB;
	for(int y=0; y<data->height; y++){
		for(int x=0; x<data->width; x++){
			Pixel &pix = data->pixels[y * data->width + x];
			pix.R = buffer[y * data->width * 3 + x*3 + 0];
			pix.G = buffer[y * data->width * 3 + x*3 + 1];
			pix.B = buffer[y * data->width * 3 + x*3 + 2];
			pix.A = 255;
		}
	}
}
	
} // GLOP_IMAGE_JPEG