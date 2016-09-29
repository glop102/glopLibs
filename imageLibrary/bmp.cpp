#include "bmp.h"

#define BMP_DEBUG

namespace GLOP_IMAGE_BMP{
	unsigned long distToPixels = 0;
	unsigned int headerSize = 0;
	unsigned int bitsPerPixel=0;
	unsigned int compressionMethod=0;
	unsigned int colorTableLength=0;
	bool upsideDownRows = true; // it is ussually true
	std::vector<Pixel> colorTable;

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
		upsideDownRows = true; // it is ussually true

		//start by reading the header and get the basic info about the image
		int x=14;
		switch(headerSize){
			case  12: // windows 2
			case  64: // OS/2 v2
			case  16: // OS/2 v2 special case
			case  52: // undocumented
			case  56: // unofficial but seen on adobe forum
			case 108: // Windows NT 4.0 , Win95
				printf("BitMap Header Version too old\nNot Decoding Image\n");
				return;
			case  40: // Windows NT 3.1x
			case 124: // Windows NT 5.0 , Win98
				x+=4; // get past the bytes that say how large the header is
				headerSize-=4;

				data->width = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
				x+=4; headerSize-=4;
				{ // bmp files are allowed to have negative heights
					int32_t heightTemp = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
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
				switch(bitsPerPixel){
					case 1:
					case 2:
					case 4:
					case 8:
					case 16:
					case 24:
					case 32:
						break;
					default:
						printf("Incorrect bitsPerPixel value: %d\n",bitsPerPixel);
						data->height=0;
						data->width=0;
						return;
				}
				if(bitsPerPixel == 32)
					data->pixelType = PixelType::RGB_ALPHA;
				else
					data->pixelType = PixelType::RGB; // assume is RGB until otherwise

				compressionMethod = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
				x+=4; headerSize-=4;
				switch(compressionMethod){
					case 1: // RunLengthEncoding 8BIT
					case 2: // RunLengthEncoding 4BIT
					case 3: // BitMasks - a bit mask for each channel is defined for the pixel size
					case 4: // Jpeg Image
					case 5: // PNG Image
					case 6: // Win Only
					case 11: // Win Only
					case 12: // Win Only
					case 13: // Win Only
						printf("Unsuported Compression Method\n");
						data->height=0;
						data->width=0;
						return;
					case 0: // no compression
						break;
					default:
						printf("Unknown Compression Method\n");
						data->height=0;
						data->width=0;
						return;
				}

				//image size - raw bytes size of the whole pixel array
				x+=4; headerSize-=4;
				//x meters per pixel
				//y meters per pixel
				x+=8; headerSize-=8;

				colorTableLength = (filebuffer[x+0]<<(8*0)) | (filebuffer[x+1]<<(8*1)) | (filebuffer[x+2]<<(8*2)) | (filebuffer[x+3]<<(8*3));
				x+=4; headerSize-=4;

				//important color count - we can ignore this one
				x+=4;
				headerSize-=4;

				//now lets jump to the color table
				x+=headerSize;
				headerSize=0;

				colorTable.clear(); // lets reset this baby and start fresh for the new image
				colorTable.reserve(colorTableLength);
				while(x<distToPixels){
					Pixel temp;
					temp.B=filebuffer[x + 0]; // colors are in the table as BGR
					temp.G=filebuffer[x + 1];
					temp.R=filebuffer[x + 2];
					temp.A=filebuffer[x + 3];
					if(temp.A != 0) data->pixelType=RGB_ALPHA;
					x+=4;
				}
				
				#ifdef BMP_DEBUG
					printf("Header Data\n");
					printf("Width\t\t: %d\n",data->width);
					printf("Height\t\t: %d\n",data->height);
					printf("PixelBits\t: %d\n",bitsPerPixel);
					printf("Compression\t: %d\n",compressionMethod);
					printf("Color Table\t: %d\n",colorTableLength);
					for(int y=0;y<colorTable.size();y++){
						Pixel& t = colorTable[y];
						printf("\tR%d  G%d  B%d  A%d\n",t.R,t.G,t.B,t.A);
					}
				#endif

				break;
			default:
				printf("Cant make sense of bitmap Information Header\n");
				return;
		}

		x = distToPixels; // Now, lets skip ahead to the pixels (header size + file descriptor size)
		decodePixelArray(filebuffer+x,bufferLength,data);

	}

	void decodePixelArray(unsigned char* filebuffer,int bufferLength, ImageData* data){
		//==============================================
		// ACTUAL DECODING OF PIXELS NOW
		//==============================================

		//Scale all channels to be 8bit
		data->bitDepth=8;
		if(bitsPerPixel == 16){
			printf("ERROR: WTF is 16 bits per pixel even mean?\n");
			return;
		}

		// how long is each row of the pixel array in bytes
		unsigned int rowWidth = ((bitsPerPixel * (data->width) + 31) / 32)*4;

		#ifdef BMP_DEBUG
			printf("\n");
			printf("Row Width\t: %d\n",rowWidth);
			printf("");
		#endif

		for(unsigned int row=0;row<data->height;row++){
			for(unsigned int y=0;y<data->width;y++){
				Pixel *pix;
				if(upsideDownRows){
					pix = & (data->pixels[((data->height-1) - row) *data->width + y]);
				}else{
					pix = & (data->pixels[row*data->width + y]);
				}
				switch(bitsPerPixel){
					int x;
					case 1:
						x = filebuffer[row*rowWidth + (y/8)]; // get the byte that the curt pixel is in
						x = ( x>>(7-(y%8)) ) & 1; // get the single bit out that represents our pixel
						(*pix) = colorTable[x];
						break;
					case 2:
						x = filebuffer[row*rowWidth + (y/4)]; // get the byte that the curt pixel is in
						x = ( x>>(2*(3-(y%4))) ) & 3; // get the single bit out that represents our pixel
						(*pix) = colorTable[x];
						break;
					case 4:
						x = filebuffer[row*rowWidth + (y/2)]; // get the byte that the curt pixel is in
						x = ( x>>(4*(1-(y%2))) ) & 0b1111; // get the single bit out that represents our pixel
						(*pix) = colorTable[x];
						break;
					case 8:
						x = filebuffer[row*rowWidth + y]; // get the byte that the curt pixel is in
						(*pix) = colorTable[x];
						break;
					case 16: // I dont know what to do with this thing
						break;
					case 24:
						pix->B=filebuffer[row*rowWidth + (y*3) + 0];
						pix->G=filebuffer[row*rowWidth + (y*3) + 1];
						pix->R=filebuffer[row*rowWidth + (y*3) + 2];
						break;
					case 32:
						pix->A=filebuffer[row*rowWidth + (y*4) + 0];
						pix->B=filebuffer[row*rowWidth + (y*4) + 1];
						pix->G=filebuffer[row*rowWidth + (y*4) + 2];
						pix->R=filebuffer[row*rowWidth + (y*4) + 3];
						break;
				}
			}
		}
	}

	void packImage(FILE* imageFP,struct ImageData *data, SAVE_OPTIONS_BMP saveOptions){
		if(data->pixelType == GRAY || data->pixelType == RGB)
			bitsPerPixel = 24;
		else
			bitsPerPixel = 32;

		unsigned int rowWidth = ((bitsPerPixel * (data->width) + 31) / 32)*4; // width of each scanline in bytes
		unsigned int temp = rowWidth;
		if(rowWidth<40) temp=40;
		unsigned char buff[temp]; // good size buffer

		//===============================
		// File Header
		//===============================
		buff[0]='B';
		buff[1]='M';

		unsigned int filesize=0;
		filesize+=14; // File Header
		filesize+=40; // DIB Header
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

		buff[10] = 40+14; // offset to pixel array from the beginning of the file

		fwrite(buff,14,1,imageFP);

		//===============================
		// DIB Header
		//===============================
		for(int x=0 ; x<40 ; x++) // zero the buffer again
			buff[x]=0;

		buff[0] = 40;
		buff[4*1 + 0] = (data->width >> (8*0)) & 0xff;
		buff[4*1 + 1] = (data->width >> (8*1)) & 0xff;
		buff[4*1 + 2] = (data->width >> (8*2)) & 0xff;
		buff[4*1 + 3] = (data->width >> (8*3)) & 0xff;
		buff[4*2 + 0] = (data->height >> (8*0)) & 0xff;
		buff[4*2 + 1] = (data->height >> (8*1)) & 0xff;
		buff[4*2 + 2] = (data->height >> (8*2)) & 0xff;
		buff[4*2 + 3] = (data->height >> (8*3)) & 0xff;

		buff[4*3 + 0] = 1; // number of planes - whatever this means
		buff[4*3 + 2] = bitsPerPixel; // mark the number of bits per pixel

		buff[4*4 + 0] = 0; // no compression

		//image array size
		// buff[4*5 + 0] = ((rowWidth*data->height)>>(8*0)) & 0xff;
		// buff[4*5 + 1] = ((rowWidth*data->height)>>(8*1)) & 0xff;
		// buff[4*5 + 2] = ((rowWidth*data->height)>>(8*2)) & 0xff;
		// buff[4*5 + 3] = ((rowWidth*data->height)>>(8*3)) & 0xff;
		buff[4*5 + 0] = 0; //we are allowed to not put a value for this when it is just a listing of RGB values

		// buff[4*10 + 3] = 0xFF ; // RED_MASK
		// buff[4*11 + 2] = 0xFF ; // GREEN_MASK
		// buff[4*12 + 1] = 0xFF ; // BLUE_MASK
		// buff[4*13 + 0] = 0xFF ; // ALPHA_MASK

		fwrite(buff,40,1,imageFP);

		//===============================
		// Write the pixels to the array
		//===============================
		double scale = 8.0 / data->bitDepth; // scale to make everything 8bit channels

		for(unsigned int row=0;row<data->height;row++){
			for(unsigned int y=0;y<data->width;y++){

				Pixel* pix;
				pix = & (data->pixels[((data->height-1) - row) *data->width + y]);

				if(data->pixelType == RGB){
					buff[(y*3) + 2] = (pix->R) * scale;
					buff[(y*3) + 1] = (pix->G) * scale;
					buff[(y*3) + 0] = (pix->B) * scale;
				}else if(data->pixelType == RGB_ALPHA){
					buff[(y*4) + 3] = (pix->R) * scale;
					buff[(y*4) + 2] = (pix->G) * scale;
					buff[(y*4) + 1] = (pix->B) * scale;
					buff[(y*4) + 0] = (pix->A) * scale;
				}else if(data->pixelType == GRAY){
					buff[(y*3) + 2] = (pix->G) * scale;
					buff[(y*3) + 1] = (pix->G) * scale;
					buff[(y*3) + 0] = (pix->G) * scale;
				}else if(data->pixelType == GRAY_ALPHA){
					buff[(y*4) + 3] = (pix->G) * scale;
					buff[(y*4) + 2] = (pix->G) * scale;
					buff[(y*4) + 1] = (pix->G) * scale;
					buff[(y*4) + 0] = (pix->A) * scale;
				}
			}
			fwrite(buff,rowWidth,1,imageFP);
		}
	}
}
