#include "png.h"

namespace GLOP_IMAGE_PNG{

//=========================================================================
// GENERAL FUNCTIONS
//=========================================================================
bool validPNG(FILE* imageFP){
	//PNG validation segment
	//the first 8 bytes are very specific and required to specify a valid PNG
	//we check here that it is valid
	unsigned char temp[]={0,0,0,0,0,0,0,0};
	unsigned char header[]={0x89,'P','N','G','\r','\n',0x1A,'\n'};
	long read = fread(temp,1,8,imageFP);
	fseek(imageFP,-8,SEEK_CUR); //reset to the beginning
	if(read!=8)
		return false;

	bool valid=true;
	for(int x=0;x<8;x++){
		if(header[x]!=temp[x])valid=false;
	}
	#ifdef PNG_DEBUG
	if(!valid)printf("\ninvalid PNG file\n");
	#endif
	return valid;
}

void unpackImage(FILE* imageFP,struct ImageData *data){
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,NULL, NULL);

	if (png_ptr == NULL){
		data->width=0;
		data->height=0;
		return; // error!!
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);

	if (info_ptr == NULL){
		png_destroy_read_struct(&png_ptr,(png_infopp)NULL, (png_infopp)NULL);
		data->width=0;
		data->height=0;
		return;
	}

	png_init_io(png_ptr,imageFP);

	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_SCALE_16 | PNG_TRANSFORM_GRAY_TO_RGB | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND, NULL); // read in the file and decode ALL the info!! scale everything to 8-bit RGB or 16-bit RGB

	data->width = png_get_image_width(png_ptr,info_ptr);
	data->height = png_get_image_height(png_ptr,info_ptr);
	data->bitDepth = png_get_bit_depth(png_ptr,info_ptr);
	data->pixelType = png_get_color_type(png_ptr,info_ptr)==PNG_COLOR_TYPE_RGB ? RGB : RGB_ALPHA;
	png_color_16p image_background;
	if (png_get_bKGD(png_ptr, info_ptr, &image_background)){
		//there was a valid background chunk
		// printf("Background %u %u %u\n",image_background->red,image_background->green,image_background->blue);
		data->background.R = image_background->red;
		data->background.G = image_background->green;
		data->background.B = image_background->blue;
	}
	//grab the text data
	png_text* text_ptr;
	int num_text;
	png_get_text(png_ptr,info_ptr,&text_ptr,&num_text);
	for(int x=0; x<num_text; x++){
		data->textPairs[text_ptr[x].key].push_back(text_ptr[x].text);
	}
	

	data->pixels = (Pixel*)malloc(sizeof(Pixel) * data->width * data->height);
	unsigned char** row_pointers; // array or pointers to the rows of pixel data
	row_pointers = png_get_rows(png_ptr, info_ptr);

	//if(data->bitDepth == 8){
		for(unsigned long y=0; y<data->height; y++){
			for(unsigned long x=0; x<data->width; x++){
				Pixel &pix = data->pixels[y*data->width +x];
				if(data->pixelType == RGB){
					pix.R = row_pointers[y][3*x + 0];
					pix.G = row_pointers[y][3*x + 1];
					pix.B = row_pointers[y][3*x + 2];
					pix.A = 255;
				}else{
					pix.R = row_pointers[y][4*x + 0];
					pix.G = row_pointers[y][4*x + 1];
					pix.B = row_pointers[y][4*x + 2];
					pix.A = row_pointers[y][4*x + 3];
				}
			}
		}
	//} else if(data->bitDepth == 16){
	//	for(unsigned long y=0; y<data->height; y++){
	//		for(unsigned long x=0; x<data->width; x++){
	//			Pixel &pix = data->pixels[y*data->width +x];
	//			if(data->pixelType == RGB){
	//				pix.R = row_pointers[y][6*x + 0]<<8 | row_pointers[y][6*x + 1];
	//				pix.G = row_pointers[y][6*x + 2]<<8 | row_pointers[y][6*x + 3];
	//				pix.B = row_pointers[y][6*x + 4]<<8 | row_pointers[y][6*x + 5];
	//				pix.A = 0xFFFF;
	//			}else{
	//				pix.R = row_pointers[y][8*x + 0]<<8 | row_pointers[y][8*x + 1];
	//				pix.G = row_pointers[y][8*x + 2]<<8 | row_pointers[y][8*x + 3];
	//				pix.B = row_pointers[y][8*x + 4]<<8 | row_pointers[y][8*x + 5];
	//				pix.A = row_pointers[y][8*x + 6]<<8 | row_pointers[y][8*x + 7];
	//			}
	//		}
	//	}
	//}else{
	//	printf("Error: bitdepth not set to 8 or 16 - is %d\n",data->bitDepth);
	//	png_destroy_read_struct(&png_ptr,&info_ptr, (png_infopp)NULL);
	//	data->width=0;
	//	data->height=0;
	//	return;
	//}

	png_destroy_read_struct(&png_ptr,&info_ptr, (png_infopp)NULL);
	return;
}

//fileBuffer will be set to the buffer of chars that need to be written directly out to a file - or a socket - or something
//bufferSize will be set to the size of the fileBuffer
//data is the image we are encoding - only one frame is allowed so that is why we are not taking in a vector of frames
//save options - what it says - can specify different save options for when encoding the file
void packImage(FILE* imageFP,struct ImageData *data, SAVE_OPTIONS_PNG saveOptions){
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,NULL, NULL);

	if (png_ptr == NULL){
		return; // error!!
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);

	if (info_ptr == NULL){
		png_destroy_write_struct(&png_ptr,NULL);
		return;
	}

	png_init_io(png_ptr,imageFP);

	if(saveOptions & SAVE_OPTIONS_PNG::MAX_COMPRESSION){
		png_set_compression_level(png_ptr,Z_BEST_COMPRESSION);
	}else if(saveOptions & SAVE_OPTIONS_PNG::MIN_COMPRESSION){
		png_set_compression_level(png_ptr,Z_BEST_SPEED);
	}else if(saveOptions & SAVE_OPTIONS_PNG::NO_COMPRESSION){
		png_set_compression_level(png_ptr,Z_NO_COMPRESSION);
	}

	png_set_IHDR(png_ptr, info_ptr,
		data->width, data->height,
		data->bitDepth,
		data->pixelType == RGB ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA,
		(saveOptions & SAVE_OPTIONS_PNG::INTERLACING) ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
		);
	png_color_16 background;
	background.red   = data->background.R;
	background.green = data->background.G;
	background.blue  = data->background.B;
	png_set_bKGD(png_ptr, info_ptr, &background);
	{ //put in the text stuff
		int num_text=0;
		for(auto mp = data->textPairs.begin(); mp!=data->textPairs.end();mp++){
			num_text+=mp->second.size();
		}

		png_text text_ptr[num_text];
		int tempNum=0;
		for(auto mp = data->textPairs.begin();mp!=data->textPairs.end();mp++){
			for(int x=0;x<mp->second.size();x++){
				text_ptr[tempNum].compression = PNG_TEXT_COMPRESSION_NONE;
				text_ptr[tempNum].key = (char*)mp->first.c_str();
				text_ptr[tempNum].text = (char*)mp->second[x].c_str();
				text_ptr[tempNum].itxt_length = 0;
				text_ptr[tempNum].text_length = mp->second[x].length();
				text_ptr[tempNum].lang = NULL;
				tempNum++;
			}
		}
		png_set_text(png_ptr, info_ptr, text_ptr, num_text);
	}

	int rowByteWidth = data->width;
	if(data->bitDepth==16) rowByteWidth*=2;
	if(data->pixelType==RGB) rowByteWidth*=3;
	else rowByteWidth*=4;
	png_byte* row_pointers[data->height];
	for(int x=0;x<data->height;x++){
		row_pointers[x] = (png_byte*)malloc(rowByteWidth);
	}

	if(data->bitDepth == 8){
		for(unsigned long y=0; y<data->height; y++){
			for(unsigned long x=0; x<data->width; x++){
				Pixel &pix = data->pixels[y*data->width +x];
				if(data->pixelType == RGB){
					row_pointers[y][3*x + 0] = pix.R;
					row_pointers[y][3*x + 1] = pix.G;
					row_pointers[y][3*x + 2] = pix.B;
				}else{
					row_pointers[y][4*x + 0] = pix.R;
					row_pointers[y][4*x + 1] = pix.G;
					row_pointers[y][4*x + 2] = pix.B;
					row_pointers[y][4*x + 3] = pix.A;
				}
			}
		}
	} else if(data->bitDepth == 16){
		for(unsigned long y=0; y<data->height; y++){
			for(unsigned long x=0; x<data->width; x++){
				Pixel &pix = data->pixels[y*data->width +x];
				if(data->pixelType == RGB){
					row_pointers[y][6*x + 0] = pix.R>>8;
					row_pointers[y][6*x + 1] = pix.R&0xFF;
					row_pointers[y][6*x + 2] = pix.G>>8;
					row_pointers[y][6*x + 3] = pix.G&0xFF;
					row_pointers[y][6*x + 4] = pix.B>>8;
					row_pointers[y][6*x + 5] = pix.B&0xFF;
				}else{
					row_pointers[y][8*x + 0] = pix.R>>8;
					row_pointers[y][8*x + 1] = pix.R&0xFF;
					row_pointers[y][8*x + 2] = pix.G>>8;
					row_pointers[y][8*x + 3] = pix.G&0xFF;
					row_pointers[y][8*x + 4] = pix.B>>8;
					row_pointers[y][8*x + 5] = pix.B&0xFF;
					row_pointers[y][8*x + 6] = pix.A>>8;
					row_pointers[y][8*x + 7] = pix.A&0xFF;
				}
			}
		}
	}

	png_set_rows(png_ptr,info_ptr,row_pointers);

	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_PACKING, NULL); // write all data to file , and tell it to handle 1,2,4 bit pixels

	#ifdef PNG_DEBUG
		printf("Done Saving Image\n\n");
	#endif
	png_destroy_write_struct(&png_ptr,&info_ptr);
	for(int x=0;x<data->height;x++){
		free(row_pointers[x]);
	}
}

} // GLOP_IMAGE_PNG
