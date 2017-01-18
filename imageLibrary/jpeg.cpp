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
	if(_jpegDecompressor == NULL){
		free(filebuffer);
		data->width=0;
		data->height=0;
		return;
	}
	//retrieve the basic info about the file
	tjDecompressHeader2(_jpegDecompressor, filebuffer, bufferLength, (int*)&(data->width), (int*)&(data->height), &jpegSubsamp);
	buffer = (unsigned char*)malloc(data->width*data->height*3); // will contain the decompressed image
	//do the acctuall decompression
	tjDecompress2(_jpegDecompressor, filebuffer, bufferLength, buffer, data->width, 0/*pitch*/, data->height, TJPF_RGB, TJFLAG_FASTDCT);

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
	free(buffer);
}
void packImage(FILE* imageFP,ImageData* data, SAVE_OPTIONS_JPG saveOptions){
	int QUALITY = 92; // a sane default - range is 0 (terrible) to 100 (great)
	if(saveOptions & QUALITY_BEST) QUALITY = 100;
	else if(saveOptions & QUALITY_DEFAULT) QUALITY = 92;
	else if(saveOptions & QUALITY_FINE) QUALITY = 90;
	else if(saveOptions & QUALITY_OKAY) QUALITY = 80;
	else if(saveOptions & QUALITY_BAD) QUALITY = 70;
	else if(saveOptions & QUALITY_AWEFUL) QUALITY = 50;

	//copy the data into the buffer
	unsigned char* buffer = (unsigned char*)malloc(data->width * data->height * 3);
	for(int y=0; y<data->height; y++){
		for(int x=0; x<data->width; x++){
			Pixel &pix = data->pixels[y * data->width + x];
			buffer[y * data->width * 3 + x*3 + 0] = pix.R;
			buffer[y * data->width * 3 + x*3 + 1] = pix.G;
			buffer[y * data->width * 3 + x*3 + 2] = pix.B;
		}
	}

	tjhandle _jpegCompressor = tjInitCompress();
	if(_jpegCompressor == NULL){
		free(buffer);
		return;
	}

	unsigned char* jpegBuffer = NULL; // gets given a pointer since we say our size is zero
	unsigned long jpegBufferSize = 0;
	tjCompress2(_jpegCompressor,buffer,data->width,0,data->height,TJPF_RGB,&jpegBuffer,&jpegBufferSize,TJSAMP_444,QUALITY,TJFLAG_FASTDCT);

	fwrite(jpegBuffer,1,jpegBufferSize,imageFP);

	tjDestroy(_jpegCompressor);
}
	
} // GLOP_IMAGE_JPEG