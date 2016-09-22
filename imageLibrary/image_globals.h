#ifndef GLOP_IMAGE_GLOBALS
#define GLOP_IMAGE_GLOBALS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <map>
#include <vector>

typedef enum {
	GRAY=1,       //Originally a basic grayscale
	GRAY_ALPHA=2, //originally grascale with an alpha channel
	RGB=3,        //Good Old-fashioned RGB
	RGB_ALPHA=4   //Best - so this is what we always convert to  :P
} PixelType;

struct Pixel{
	uint16_t R; //RED
	uint16_t G; //GREEN
	uint16_t B; //BLUE
	uint16_t A; //ALPHA
	bool operator==(const Pixel& other)const{
		bool temp=true; // assume true first
		temp &= other.R==this->R;
		temp &= other.G==this->G;
		temp &= other.B==this->B;
		temp &= other.A==this->A;
		return temp;
	}
	bool operator<(const Pixel& other)const{
		return this->hash() < other.hash();
	}
	bool operator>(const Pixel& other)const{
		return this->hash() > other.hash();
	}
	uint64_t hash()const{
		uint64_t f;
		f  = (uint64_t)this->R << (8*6);
		f |= (uint64_t)this->G << (8*4);
		f |= (uint64_t)this->B << (8*2);
		f |= (uint64_t)this->A << (8*0);
		return f;
	}
};
struct PixelHash{
	uint64_t operator()(const Pixel& other)const{
		return other.hash();
	}
};

struct ImageData{ //contains the basic info of the 
	PixelType pixelType; // what are the channels being used in this - coincidently is also equal to the interger for the number of channels
	uint32_t width;
	uint32_t height;
	struct Pixel *pixels;
	std::map<std::string,std::vector<std::string> > textPairs; //key value pairs - we will allow mutiple values per key
	uint16_t bitDepth;
	Pixel background;
};

typedef enum {
	PNG   = 0,
	JPEG  = 1,
	JPG   = 1,
	GIF   = 2
}SAVE_TYPE;

typedef enum{
	NOTHING         = 0b00000000,
	INTERLACING     = 0b00000001,
	USE_PALLETE     = 0b00000010,
	NO_COMPRESSION  = 0b00000100,
	MIN_COMPRESSION = 0b00001000,
	MAX_COMPRESSION = 0b00010000
}SAVE_OPTIONS_PNG;

#endif