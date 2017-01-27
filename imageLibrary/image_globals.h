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
	uint8_t R; //RED
	uint8_t G; //GREEN
	uint8_t B; //BLUE
	uint8_t A; //ALPHA
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
	GIF   = 2,
	BMP   = 3
}SAVE_TYPE;

typedef enum{
	PNG_NOTHING     = 0b00000000,
	INTERLACING     = 0b00000001,
	//recomened to not use these options and let the libpng use the default settings
	NO_COMPRESSION  = 0b00000010, // does waht it says on the tin
	MIN_COMPRESSION = 0b00000100, // just the most simple compression
	MAX_COMPRESSION = 0b00001000  // do everything you can to cram it in
}SAVE_OPTIONS_PNG;

typedef enum{ // empty because i can't thing of any useful options
} SAVE_OPTIONS_BMP;

typedef enum{ // empty because i can't thing of any useful options
	JPG_NOTHING     = 0b00000000,
	QUALITY_BEST    = 0b00000001,
	QUALITY_FINE    = 0b00000010,
	QUALITY_OKAY    = 0b00000100,
	QUALITY_BAD     = 0b00001000,
	QUALITY_AWEFUL  = 0b00010000
} SAVE_OPTIONS_JPG;

#endif
