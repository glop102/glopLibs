#include "jpeg.h"

namespace GLOP_IMAGE_JPEG{

BinaryTree<int> luminanceDC;
BinaryTree<int> luminanceAC;
BinaryTree<int> chrominanceDC;
BinaryTree<int> chrominanceAC;
	
bool validJPEG(unsigned char *filebuffer){
	if(filebuffer[0] == 0xFF && filebuffer[1] == 0xD8) return true;
	return false;
}

void unpackImage(unsigned char* filebuffer,int bufferLength, ImageData* data){
	#ifdef JPEG_DEBUG
		printf("Found Valid JPEG\n");
	#endif

	//Temperary stuffs
	data->pixels=NULL;
	data->width=0;
	data->height=0;

	for(int x=0; x<bufferLength; /**/){
		if(filebuffer[x] != 0xFF){
			printf("ERROR: not a chunk at byte %d\n\n",x);
			return;
		}

		x++;
		//lets first see if it is on of the fixed-size or non-payload chunks
		if(0xD8 == filebuffer[x]){
			printf("Start Of Image\n");
			x++;
		}else if(0xDD == filebuffer[x]){
			printf("Restart Interval ");
			x++;
			x+=4;
		}else if(0xD0 == filebuffer[x] || 0xD1 == filebuffer[x] || 0xD2 == filebuffer[x] || 0xD3 == filebuffer[x] || 0xD4 == filebuffer[x] || 0xD5 == filebuffer[x] || 0xD6 == filebuffer[x] || 0xD7 == filebuffer[x]){
			printf("Restart Block %d\n",filebuffer[x] & 0b1111);
		}else if(0xD9 == filebuffer[x]){
			printf("End Of Image\n\n");
			return;
		}else{
			//nope, we will now assume it is a variable size payload chunk
			//get the chunk size - this includes the two bytes telling us the size
			int size=filebuffer[x+1]<<8;
			size |= filebuffer[x+2];

			if(0xC0 == filebuffer[x]){
				printf("Start Of Frame (Baseline DCT)\n");
				x++;
				x+=2; // get past the size bytes

				printf("\tbitdepth : %d\n",filebuffer[x]);
				printf("\tHeight: %d\n", (filebuffer[x+1]<<8) | filebuffer[x+2]);
				printf("\tWidth : %d\n", (filebuffer[x+3]<<8) | filebuffer[x+4]);
				//printf("\tNumber Of channels : %d\n", filebuffer[5]);

				x+=size-2; // size of the payload and length bytes
			}else if(0xC1 == filebuffer[x]){
				printf("Start Of Frame (Extended sequential DCT)\n");
				x++;

				x+=size; // size of the payload and length bytes
			}else if(0xC2 == filebuffer[x]){
				printf("Start Of Frame (Progressive DCT)\n");
				x++;

				x+=size; // size of the payload and length bytes
			}else if(0xC3 == filebuffer[x]){
				printf("Start Of Frame (Lossless sequential DCT)\n");
				x++;

				x+=size; // size of the payload and length bytes
			}else if(0xC4 == filebuffer[x]){
				printf("Defining Huffman Table\n");
				x++; // get past the chunk marker
				x+=2; // get past the bytes that tell us the size
				size-=2; // size left

				int type = filebuffer[x]; // type of huffman table
				x++;
				size--;
				if(type==0x00){
					printf("Luminance DC\n");
					unpackDHT(filebuffer,x,size,&luminanceAC);
				}else if(type==0x10){
					printf("Luminance AC\n");
					unpackDHT(filebuffer,x,size,&luminanceDC);
				}else if(type==0x01){
					printf("Chrominance DC\n");
					unpackDHT(filebuffer,x,size,&chrominanceAC);
				}else if(type==0x11){
					printf("Chrominance AC\n");
					unpackDHT(filebuffer,x,size,&chrominanceDC);
				}

				x+=size; // size of the payload excluding the bytes telling us the length
			}else if(0xDB == filebuffer[x]){
				printf("Defining Quantization Table\n");
				x++;

				x+=size; // size of the payload and length bytes
			}else if(0xDA == filebuffer[x]){
				printf("Start Of Scan\n");
				x++;

				x+=size; // size of the payload and length bytes
			}else if(0xFE == filebuffer[x]){
				printf("Comment\n");
				x++;
				for(int y=2;y<size;y++)
					printf("%c",filebuffer[x+y]);

				x+=size; // size of the payload and length bytes
			}else if(0xE0 == (filebuffer[x] & 0xF0)){
				printf("Application Specific Data\n");
				x++; // get past the chunk ID
				x+=2; //get past the chunk size

				if(filebuffer[x]=='J' && filebuffer[x+1]=='F' && filebuffer[x+2]=='I' && filebuffer[x+3]=='F' && filebuffer[x+4]==0){
					printf("\tJFIF application data\n");
					printf("\tVersion %d.%d\n",filebuffer[x+5],filebuffer[x+6]);
					switch(filebuffer[x+7]){
						case 0:printf("\tThumbnail Style : Pixel\n"); break;
						case 1:printf("\tThumbnail Style : Inches\n"); break;
						case 2:printf("\tThumbnail Style : Centimeters\n"); break;
					}
					printf("\tHorizontalDensity     : %d\n",(filebuffer[x+ 8]<<8) | filebuffer[x+ 9]);
					printf("\tVerticalDensity       : %d\n",(filebuffer[x+10]<<8) | filebuffer[x+11]);
					printf("\tHorizontal Pixel count: %d\n",filebuffer[x+12]); // size here is only for the thumbnail
					printf("\tVertical Pixel count  : %d\n",filebuffer[x+13]);
					if(size-2 != filebuffer[x+12]*filebuffer[x+13]*3+14)
						printf("\twrong size chunk, %d %d\n",size-2,filebuffer[x+12]*filebuffer[x+13]*3+14);
				}else{
					printf("\t%x %x %x %x\n", filebuffer[x],filebuffer[x+1],filebuffer[x+2],filebuffer[x+3]);
				}

				x+=size-2; // size of the payload and length bytes
			}else {
				printf("Unknown Chunk Flag 0x%02x\n",filebuffer[x]);
				x++;
				x+=size;
			}
		}
	}
}

void unpackDHT(unsigned char* filebuffer, int x, int size, BinaryTree<int> *tree){
	int rows[16]; // how many entries are in each row?
	for(int b=0;b<16;b++){
		//printf("\trow %d - %d entries\n",b,filebuffer[x+b]);
		rows[b] = filebuffer[x+b];
	}
	x+=16;
	size-=16;

	int code=0;
	for(int b=0; b<16; b++){ // go through each row
		for(int c=0;c<rows[b];c++){ // for each row go through every entry
			//printf("\t%x\t%x\n",filebuffer[x+c],code);
			tree->addNode(code,b+1,filebuffer[x+c]);
			code++;
		}
		x+=rows[b];
		size-=rows[b];
		code = code << 1;
	}

	tree->reset(); // get it ready to be used
	//tree->sequence(0x1fe,9);
}
	
} // GLOP_IMAGE_JPEG