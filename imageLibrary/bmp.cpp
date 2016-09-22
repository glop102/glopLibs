#include "bmp.h"

namespace GLOP_IMAGE_BMP{
	unsigned long distToPixels = 0;
	unsigned int headerSize = 0;
	unsigned int bitsPerPixel=0;
	unsigned int compressionMethod=0;
	unsigned int colorTableLength=0;
	unsigned int RED_MASK=0; // mask of the bytes that the pixels are to get out this channel
	unsigned int GREEN_MASK=0;
	unsigned int BLUE_MASK=0;
	unsigned int ALPHA_MASK=0;
	bool upsideDownRows = true; // it is ussually true

	bool validBMP(unsigned char *filebuffer){
		if(filebuffer[0] != 'B' || filebuffer[1]!='M')
			return false;

		//now we check if at byte 0x14 if it is one of the known good values
		//this byte is the size of the header and is different for different versions
		//most ofter it is 128 as that size is for the newest format
		switch(filebuffer[14]){
			case  12: // windows 2
			case  64: // OS/2 v2
			case  16: // OS/2 v2 special case
			case  40: // Windows NT 3.1x
			case  52: // undocumented
			case  56: // unofficial but seen on adobe forum
			case 108: // Windows NT 4.0 , Win95
			case 124: // Windows NT 5.0 , Win98
			break;
			default:
				return false;
		}
		return true;
	}

	void unpackImage(unsigned char* filebuffer,int bufferLength, ImageData* data){
		distToPixels = (filebuffer[10]<<(8*0)) | (filebuffer[11]<<(8*1)) | (filebuffer[12]<<(8*2)) | ((filebuffer[13])<<(8*3));
		headerSize = filebuffer[14];
		bitsPerPixel=0;
		compressionMethod=0;
		colorTableLength=0;
		RED_MASK=0; // mask of the bytes that the pixels are to get out this channel
		GREEN_MASK=0;
		BLUE_MASK=0;
		ALPHA_MASK=0;
		upsideDownRows = true; // it is ussually true

		//start by reading the header and get the basic info about the image
		int x=14;
		switch(headerSize){
			case  12: // windows 2
			case  64: // OS/2 v2
			case  16: // OS/2 v2 special case
			case  40: // Windows NT 3.1x
			case  52: // undocumented
			case  56: // unofficial but seen on adobe forum
			case 108: // Windows NT 4.0 , Win95
				printf("BitMap Header Version too old\nNot Decoding Image\n");
				return;
			case 124: // Windows NT 5.0 , Win98
				x+=4; // get past the bytes that say how large the header is
				headerSize-=4;

				data->width = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
				x+=4; headerSize-=4;
				{ // bmp files are allowed to have negative heights
					int heightTemp = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
					if(heightTemp>=0)
						data->height = heightTemp;
					else{
						data->height = -1*heightTemp;
						upsideDownRows = false;
					}
				}
				x+=4; headerSize-=4;
				data->pixels = (struct Pixel*)malloc(data->width * data->height * sizeof(Pixel) );

				// planes = filebuffer[] .....
				x+=2; headerSize-=2;
				bitsPerPixel = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) ; 
				x+=2; headerSize-=2;

				compressionMethod = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
				x+=4; headerSize-=4;

				//image size
				x+=4; headerSize-=4;
				//x meters per pixel
				//y meters per pixel
				x+=8; headerSize-=8;

				colorTableLength = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
				x+=4; headerSize-=4;

				//important color count
				x+=4;
				headerSize-=4;

				RED_MASK = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
				x+=4; headerSize-=4;
				GREEN_MASK = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
				x+=4; headerSize-=4;
				BLUE_MASK = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
				x+=4; headerSize-=4;
				ALPHA_MASK = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
				x+=4; headerSize-=4;
				
				#ifdef BMP_DEBUG
					printf("Header Data\n");
					printf("Width\t\t: %d\n",data->width);
					printf("Height\t\t: %d\n",data->height);
					printf("PixelBits\t: %d\n",bitsPerPixel);
					printf("Compression\t: %d\n",compressionMethod);
					printf("Color Table\t: %d\n",colorTableLength);
					printf("RED_MASK\t: %x\n",RED_MASK);
					printf("GREEN_MASK\t: %x\n",GREEN_MASK);
					printf("BLUE_MASK\t: %x\n",BLUE_MASK);
					printf("ALPHA_MASK\t: %x\n",ALPHA_MASK);
				#endif

				//blahblahblah
				//color spaces finish out the rest of the header

				break;
		}

		if(ALPHA_MASK != 0)
			data->pixelType = PixelType::RGB_ALPHA;
		else
			data->pixelType = PixelType::RGB;

		if(colorTableLength != 0){
			//make color table stuffs some time later
			x = 14+filebuffer[14]; // jump to after the header, where the color table is
		}else{
			x = distToPixels; // Now, lets skip ahead to the pixels
			decodePixelArray(x,filebuffer,bufferLength,data);
		}

	}

	void decodePixelArray(unsigned int offset, unsigned char* filebuffer,int bufferLength, ImageData* data){
		//==============================================
		// ACTUAL DECODING OF PIXELS NOW
		//==============================================
		// assuming no pallete and bitsize of pixels are 16,24,32

		// how far do we need to shift each of the channels after the mask
		uint8_t RED_SHIFT = 255;
		uint8_t GREEN_SHIFT = 255;
		uint8_t BLUE_SHIFT = 255;
		uint8_t ALPHA_SHIFT = 255;
		for(uint8_t x = 0 ; x<32 ; x++){
			if(RED_SHIFT == 255 && (RED_MASK >> x) & 1)
				RED_SHIFT = x;
			if(GREEN_SHIFT == 255 && (GREEN_MASK >> x) & 1)
				GREEN_SHIFT = x;
			if(BLUE_SHIFT == 255 && (BLUE_MASK >> x) & 1)
				BLUE_SHIFT = x;
			if(ALPHA_SHIFT == 255 && (ALPHA_MASK >> x) & 1)
				ALPHA_SHIFT = x;
		}

		//Scale all channels to be 8bit
		data->bitDepth=8;
		double RED_SCALE = 1.0;
		double GREEN_SCALE = 1.0;
		double BLUE_SCALE = 1.0;
		double ALPHA_SCALE = 1.0;

		if(RED_MASK   >> RED_SHIFT   != 0) RED_SCALE   = 0xff / (double)( RED_MASK   >> RED_SHIFT   );
		if(GREEN_MASK >> GREEN_SHIFT != 0) GREEN_SCALE = 0xff / (double)( GREEN_MASK >> GREEN_SHIFT );
		if(BLUE_MASK  >> BLUE_SHIFT  != 0) BLUE_SCALE  = 0xff / (double)( BLUE_MASK  >> BLUE_SHIFT  );
		if(ALPHA_MASK >> ALPHA_SHIFT != 0) ALPHA_SCALE = 0xff / (double)( ALPHA_MASK >> ALPHA_SHIFT );

		// how long is each row of the pixel array in bytes
		unsigned int rowWidth = ((bitsPerPixel * (data->width) + 31) / 32)*4;

		#ifdef BMP_DEBUG
			printf("RED_SHIFT\t: %d\n",RED_SHIFT);
			printf("GREEN_SHIFT\t: %d\n",GREEN_SHIFT);
			printf("BLUE_SHIFT\t: %d\n",BLUE_SHIFT);
			printf("ALPHA_SHIFT\t: %d\n",ALPHA_SHIFT);
			printf("Row Width\t: %d\n",rowWidth);
		#endif

		for(unsigned int row=0;row<data->height;row++){
			for(unsigned int y=0;y<data->width;y++){
				uint32_t pixelData = 0; // the bytes for this pixel
				for(int b=0 ; b<bitsPerPixel/8 ; b++){
					pixelData |= filebuffer[(row*rowWidth) + (y*bitsPerPixel/8) + b + offset] << (8*b);
				}
				Pixel* pix;
				if(upsideDownRows){
					pix = & (data->pixels[((data->height-1) - row) *data->width + y]);
				}else{
					pix = & (data->pixels[row*data->width + y]);
				}
				pix->R = ((pixelData & RED_MASK)   >> RED_SHIFT  );
				pix->G = ((pixelData & GREEN_MASK) >> GREEN_SHIFT);
				pix->B = ((pixelData & BLUE_MASK)  >> BLUE_SHIFT );
				pix->A = ((pixelData & ALPHA_MASK) >> ALPHA_SHIFT);

				pix->R *= RED_SCALE;
				pix->G *= GREEN_SCALE;
				pix->B *= BLUE_SCALE;
				pix->A *= ALPHA_SCALE;
			}
		}
	}

	void packImage(FILE* imageFP,struct ImageData *data, SAVE_OPTIONS_BMP saveOptions){
		if(data->pixelType == GRAY || data->pixelType == RGB)
			bitsPerPixel = 24;
		else
			bitsPerPixel = 32;
		unsigned int rowWidth = ((bitsPerPixel * (data->width) + 31) / 32)*4;
		unsigned int temp = rowWidth;
		if(rowWidth<124) temp=124;
		unsigned char buff[temp]; // good size buffer

		//===============================
		// File Header
		//===============================
		buff[0]='B';
		buff[1]='M';

		unsigned int filesize=0;
		filesize+=14; // File Header
		filesize+=124; // DIB Header
		filesize+=0;  // color table
		if(data->pixelType == GRAY || data->pixelType == RGB){
			filesize += (data->height * data->width) * 3;
		}else if(data->pixelType == GRAY_ALPHA || data->pixelType == RGB_ALPHA){
			filesize += (data->height * data->width) * 4;
		}
		buff[2]=filesize & 0xff;
		buff[3]=(filesize>>(8*1))&0xff;
		buff[4]=(filesize>>(8*2))&0xff;
		buff[5]=(filesize>>(8*3))&0xff;

		for(int x=0;x<8;x++)buff[6+x]=0; // make sure it is zeros

		buff[10] = 124; // offset to pixel array

		fwrite(buff,14,1,imageFP);

		//===============================
		// DIB Header
		//===============================
		for(int x=0 ; x<124 ; x++) // zero the buffer again
			buff[x]=0;

		buff[0] = 124;
		buff[4*1 + 0] = (data->width >> (8*0)) & 0xff;
		buff[4*1 + 1] = (data->width >> (8*1)) & 0xff;
		buff[4*1 + 2] = (data->width >> (8*2)) & 0xff;
		buff[4*1 + 3] = (data->width >> (8*3)) & 0xff;
		buff[4*2 + 0] = (data->height >> (8*0)) & 0xff;
		buff[4*2 + 1] = (data->height >> (8*1)) & 0xff;
		buff[4*2 + 2] = (data->height >> (8*2)) & 0xff;
		buff[4*2 + 3] = (data->height >> (8*3)) & 0xff;

		buff[4*3 + 0] = 1; // number of planes - whatever this means
		if(data->pixelType == GRAY || data->pixelType == RGB) // number of bits per pixel
			buff[4*3 + 2] = 24;
		else
			buff[4*3 + 2] = 32;

		buff[4*4 + 0] = 3;

		//image array size
		buff[4*5 + 0] = ((rowWidth*data->height)>>(8*0)) & 0xff;
		buff[4*5 + 1] = ((rowWidth*data->height)>>(8*1)) & 0xff;
		buff[4*5 + 2] = ((rowWidth*data->height)>>(8*2)) & 0xff;
		buff[4*5 + 3] = ((rowWidth*data->height)>>(8*3)) & 0xff;

		buff[4*10 + 3] = 0xFF ; // RED_MASK
		buff[4*11 + 2] = 0xFF ; // GREEN_MASK
		buff[4*12 + 1] = 0xFF ; // BLUE_MASK
		buff[4*13 + 0] = 0xFF ; // ALPHA_MASK

		fwrite(buff,124,1,imageFP);

		//===============================
		// Write the pixels to the array
		//===============================
		double scale = 8.0 / data->bitDepth; // scale to make everything 8bit channels

		for(unsigned int row=0;row<data->height;row++){
			for(unsigned int y=0;y<data->width;y++){

				Pixel* pix;
				pix = & (data->pixels[((data->height-1) - row) *data->width + y]);

				if(data->pixelType == GRAY || data->pixelType == RGB){
					buff[(y*3) + 2] = (pix->R) * scale;
					buff[(y*3) + 1] = (pix->G) * scale;
					buff[(y*3) + 0] = (pix->B) * scale;
				}else{
					buff[(y*4) + 3] = (pix->R) * scale;
					buff[(y*4) + 2] = (pix->G) * scale;
					buff[(y*4) + 1] = (pix->B) * scale;
					buff[(y*4) + 0] = (pix->A) * scale;
				}
			}
			fwrite(buff,rowWidth,1,imageFP);
		}
	}
}