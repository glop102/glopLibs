#include "jpeg.h"

namespace GLOP_IMAGE_JPEG{
	
bool validJPEG(FILE* imageFP){
	bool valid = false;
	if(getc(imageFP) == 0xFF && getc(imageFP) == 0xD8) valid = true;
	fseek(imageFP,-2,SEEK_CUR); //reset to the beginning

	return valid;
}

void unpackImage(FILE* imageFP, ImageData* data){
	// General Outline
	//
	// Allocate and initialize a JPEG decompression object
	// Specify the source of the compressed data (eg, a file)
	// Call jpeg_read_header() to obtain image info
	// Set parameters for decompression
	// jpeg_start_decompress(...);
	// while (scan lines remain to be read)
	// 	jpeg_read_scanlines(...);
	// jpeg_finish_decompress(...);
	// Release the JPEG decompression object

	#ifdef JPEG_DEBUG
		printf("Found Valid JPEG\n");
	#endif
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	// tell it to use the file pointer as the source
	jpeg_stdio_src(&cinfo, imageFP);

	// get the basic file info
	jpeg_read_header(&cinfo, TRUE);
	data->width = cinfo.image_width;
	data->height = cinfo.image_height;
	data->pixels = (Pixel*)malloc(data->width * data->height * sizeof(Pixel));
	data->bitDepth = 8;
	if(cinfo.out_color_space == JCS_RGB){
		data->pixelType = RGB;
	} else if(cinfo.out_color_space == JCS_GRAYSCALE){
		data->pixelType = GRAY;
	} else{
		data->pixelType = RGB; // probably a sane default
		printf("UNKNOWN COLORSPACE\n");
	}
	// cinfo.out_color_space = JCS_RGB; // make it always output RGB

	// time to move on to the next stage
	jpeg_start_decompress(&cinfo);
	unsigned char lineBuffer[data->width * data->pixelType];
	unsigned char* ptr[1];
	ptr[0] = lineBuffer;
	for(int y=0; y<data->height ;y++){
		jpeg_read_scanlines(&cinfo,ptr,1);
		for(int x=0; x<data->width; x++){
			if(data->pixelType == RGB){
				Pixel& pix = data->pixels[(data->width * y) + x];
				pix.R = lineBuffer[3*x + 0];
				pix.G = lineBuffer[3*x + 1];
				pix.B = lineBuffer[3*x + 2];
				pix.A = 255;
			}else if(data->pixelType == GRAY){
				Pixel& pix = data->pixels[(data->width * y) + x];
				pix.R = lineBuffer[x];
				pix.G = lineBuffer[x];
				pix.B = lineBuffer[x];
				pix.A = 255;
			}

		}
	}

	//and now is time to clean up
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
}
void packImage(FILE* imageFP,ImageData* data, SAVE_OPTIONS_JPG saveOptions){
	// General Outline
	//
	// Allocate and initialize a JPEG compression object
	// Specify the destination for the compressed data (eg, a file)
	// Set parameters for compression, including image size & colorspace
	// jpeg_start_compress(...);
	// while (scan lines remain to be written)
	// 	jpeg_write_scanlines(...);
	// jpeg_finish_compress(...);
	// Release the JPEG compression object

	int QUALITY = 10000; // default that should NEVER be used - detects if we want somthing other than default
	if(saveOptions & QUALITY_BEST) QUALITY = 100;
	else if(saveOptions & QUALITY_FINE) QUALITY = 90;
	else if(saveOptions & QUALITY_OKAY) QUALITY = 80;
	else if(saveOptions & QUALITY_BAD) QUALITY = 70;
	else if(saveOptions & QUALITY_AWEFUL) QUALITY = 50;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	// what file are we gonna write out to
	jpeg_stdio_dest(&cinfo, imageFP);

	//some basic info
	cinfo.image_width = data->width;
	cinfo.image_height = data->height;
	if(data->pixelType == RGB || data->pixelType == RGB_ALPHA){
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;
	} else if(data->pixelType == GRAY || data->pixelType == GRAY_ALPHA){
		cinfo.input_components = 1;
		cinfo.in_color_space = JCS_GRAYSCALE;
	}
	jpeg_set_defaults(&cinfo);
	if(QUALITY != 10000) jpeg_set_quality(&cinfo, QUALITY, false); // do this after the defaults

	// time to get cooking
	jpeg_start_compress(&cinfo, TRUE);

	unsigned char lineBuffer[data->width * 3];
	unsigned char* ptr[1];
	ptr[0] = lineBuffer;

	for(int y=0; y<data->height; y++){
		if(data->pixelType == RGB || data->pixelType == RGB_ALPHA){
			for(int x=0; x<data->width; x++){
				Pixel &pix = data->pixels[y * data->width + x];
				lineBuffer[3*x + 0] = pix.R;
				lineBuffer[3*x + 1] = pix.G;
				lineBuffer[3*x + 2] = pix.B;
			}
		}else{
			for(int x=0; x<data->width; x++){
				Pixel &pix = data->pixels[y * data->width + x];
				lineBuffer[x] = pix.G;
			}
		}
		jpeg_write_scanlines(&cinfo,ptr,1);
	}

	// time to do some cleanup
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
}
	
} // GLOP_IMAGE_JPEG