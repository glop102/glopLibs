#include "image.h"

//==================================================================================
// CONSTRUCTORS
//==================================================================================
Image::Image(FILE* imageFP){
	if(imageFP==NULL){
		imageData.width=0;
		imageData.height=0;
		imageData.pixels=NULL;
		return;
	}

	if(!constructImage_fromFile(imageFP)){
		imageData.width=0;
		imageData.height=0;
		imageData.pixels=NULL;
		return;
	}
}
Image::Image(std::string filename){
	FILE *imageFP=fopen(filename.c_str(),"rb");
	if(imageFP==NULL){
		imageData.width=0;
		imageData.height=0;
		imageData.pixels=NULL;
		return;
	}

	if(!constructImage_fromFile(imageFP)){
		imageData.width=0;
		imageData.height=0;
		imageData.pixels=NULL;
		fclose(imageFP);
		return;
	}
	fclose(imageFP);
}
Image::Image(unsigned long width,unsigned long height,PixelType pixelType){
	this->imageData.width=width;
	this->imageData.height=height;
	this->imageData.pixelType=pixelType;
	this->imageData.pixels=(Pixel*)malloc(sizeof(Pixel)*width*height);
	this->imageData.background.R=0;
	this->imageData.background.G=0;
	this->imageData.background.B=0;
	this->imageData.background.A=0;
	this->imageData.bitDepth = 8;
	this->frames.push_back(this->imageData);
	currentFrame=0;
}
Image::Image(unsigned char* pixelData,unsigned long width,unsigned long height,PixelType pixelType){
	this->imageData.width=width;
	this->imageData.height=height;
	this->imageData.pixelType=pixelType;
	this->imageData.pixels=(Pixel*)malloc(sizeof(Pixel)*width*height);
	this->imageData.background.R=0;
	this->imageData.background.G=0;
	this->imageData.background.B=0;
	this->imageData.background.A=0;
	this->imageData.bitDepth = 8;
	currentFrame=0;

	//constructing the pixel data
	for(int x=0;x<(width*height);x++){ //count what pixel we are on
		if(pixelType==GRAY){
			this->imageData.pixels[x].G=pixelData[x];
		}else if(pixelType==GRAY_ALPHA){
			this->imageData.pixels[x].G=pixelData[x*2];
			this->imageData.pixels[x].A=pixelData[(x*2)+1];
		}else if(pixelType==RGB){
			this->imageData.pixels[x].R=pixelData[x*3];
			this->imageData.pixels[x].G=pixelData[(x*3)+1];
			this->imageData.pixels[x].B=pixelData[(x*3)+2];
		}else if(pixelType==RGB_ALPHA){
			this->imageData.pixels[x].R=pixelData[x*4];
			this->imageData.pixels[x].G=pixelData[(x*4)+1];
			this->imageData.pixels[x].B=pixelData[(x*4)+2];
			this->imageData.pixels[x].A=pixelData[(x*4)+3];
		}
	}

	this->frames.push_back(this->imageData);
}

//==================================================================================
// DECONSTRUCTOR
//==================================================================================
Image::~Image(){
	// for every frame we need to free the memory
	for(int x=0;x<this->frames.size();x++)
		if(this->frames[x].pixels != NULL)
			free(this->frames[x].pixels);
}

//==================================================================================
// PRIVATE METHODS
//==================================================================================
bool Image::constructImage_fromFile(FILE *imageFP){
	/*
		Get this image class ready to go from a file passed to us
		This does the memory allocation and stuff to make the class ready to go

		It starts by reading in the whole file
			it sees how large the file is
			it reads all the bytes to a buffer of the right size
		It then runs throught the list of known and supported file formats
			It asks the sections if it is valid
			If it finds a valid one it then it asks it to decode the image
			The functions do the allocation and maths appropriate to the format
	*/
	fseek(imageFP, 0L, SEEK_END);
	int length=ftell(imageFP);
	#ifdef GLOP_IMAGE_DEBUG
		printf("FileSize:  %d\n\n",length);
	#endif

	unsigned char* fileBuffer=(unsigned char*)malloc(length);
	fseek(imageFP,0,SEEK_SET); //reset to the beginning
	int written = fread(fileBuffer,1,length,imageFP);

	//==========================
	//set data to known values - mostly all zeros
	imageData.width=0;
	imageData.height=0;
	imageData.bitDepth=0;
	imageData.background.R=0;
	imageData.background.G=0;
	imageData.background.B=0;
	imageData.background.A=0;
	imageData.pixelType=GRAY;

	if(GLOP_IMAGE_PNG::validPNG(imageFP)){
		GLOP_IMAGE_PNG::unpackImage(imageFP,&(this->imageData));
		this->frames.push_back(this->imageData);
		currentFrame=0;
	// } else if(GLOP_IMAGE_JPEG::validJPEG(fileBuffer)) {
	// 	GLOP_IMAGE_JPEG::unpackImage(fileBuffer,length,&(this->imageData));
	// 	this->frames.push_back(this->imageData);
	// 	currentFrame=0;
	} else if(GLOP_IMAGE_BMP::validBMP(imageFP)){
		GLOP_IMAGE_BMP::unpackImage(imageFP,&(this->imageData));
		this->frames.push_back(this->imageData);
		currentFrame=0;
	}//GIF or others come here
	else{
		printf("Unknown Filetype\n");
	}

	#ifdef GLOP_IMAGE_DEBUG
		printf("Glop Image Data Debug\n");
		printf("Size (wxh): %d x %d\n",this->imageData.width,this->imageData.height);
		printf("Bit Depth:  %d\n",this->imageData.bitDepth);
		printf("Pixel Type: %d - ",this->imageData.pixelType);
		switch(this->imageData.pixelType){
			case GRAY: printf("GRAY\n");break;
			case GRAY_ALPHA: printf("GRAY_ALPHA\n");break;
			case RGB: printf("RGB\n");break;
			case RGB_ALPHA: printf("RGB_ALPHA\n");break;
		}
		printf("Background: %02X %02X %02X %02X\n\n",this->imageData.background.R,this->imageData.background.G,this->imageData.background.B,this->imageData.background.A);
	#endif

	free(fileBuffer);
	return true; //success
}

//==================================================================================
// PUBLIC METHODS
//==================================================================================

unsigned long Image::width(){
	return this->imageData.width;
}
unsigned long Image::height(){
	return this->imageData.height;
}

Pixel* Image::operator[](unsigned int y){
	return this->imageData.pixels+(y*this->imageData.width);
}

int Image::numberOfFrames(){
	return this->frames.size();
}
void Image::gotoFrame(int frame){
	if(frame < this->frames.size() && frame >= 0){
		this->currentFrame=frame;
		this->imageData=this->frames[frame];
	}
}
int Image::nextFrame(){
	this->gotoFrame(this->currentFrame+1);
	return this->currentFrame;
}
int Image::getCurrentFrame(){
	return this->currentFrame;
}

void Image::saveImage(std::string filename,SAVE_TYPE saveType, uint16_t saveOptions){
	FILE *ff = fopen(filename.c_str(),"wb");
	if(ff==NULL) return; // error

	this->saveImage(ff,saveType,saveOptions); // get the image packed into the buffer
	fclose(ff);
}
void Image::saveImage(FILE* imageFP,SAVE_TYPE saveType, uint16_t saveOptions){

	if(this->imageData.pixels==NULL || width()*height()==0){
		printf("ERROR - attempting to save a zero size image\n");
		return; // nope
	}
	if(saveType==SAVE_TYPE::PNG){
		if(this->imageData.pixelType != RGB && this->imageData.pixelType != RGB_ALPHA){
			printf("Error, only RGB and RGB_ALPHA images are currently supported\n");
			return;
		}
		GLOP_IMAGE_PNG::packImage(imageFP,&(this->imageData), (SAVE_OPTIONS_PNG)saveOptions);
	}else if(saveType==SAVE_TYPE::BMP){
		GLOP_IMAGE_BMP::packImage(imageFP,&(this->imageData), (SAVE_OPTIONS_BMP)saveOptions);
	}
}
int Image::getBitDepth(){
	return this->imageData.bitDepth;
}
void Image::setBitDepth(int val){
	if(this->imageData.pixels==NULL || width()*height()==0) return;
	// We will change every frame to the same bit depth to maintain consistency between things
	//This makes it easier on the end user since they can do the scaling and then not worry about checking again
	for(int frame=0;frame<this->frames.size();frame++){
		if(val==this->frames[frame].bitDepth)continue; // skip since it is already correct
		// double scale = (pow(2,val)-1) / (pow(2,this->frames[frame].bitDepth)-1);
		int shift = this->frames[frame].bitDepth - val;

		#ifdef GLOP_IMAGE_DEBUG
			printf("Scaling Frame %d\n",frame);
			printf("       Factor %f\n",shift);
		#endif

		if(this->frames[frame].bitDepth > val){
			for(unsigned int x=0;x<width()*height();x++){
				this->frames[frame].pixels[x].R = this->frames[frame].pixels[x].R >> shift;
				this->frames[frame].pixels[x].G = this->frames[frame].pixels[x].G >> shift;
				this->frames[frame].pixels[x].B = this->frames[frame].pixels[x].B >> shift;
				this->frames[frame].pixels[x].A = this->frames[frame].pixels[x].A >> shift;
			}
			this->frames[frame].background.R = this->frames[frame].background.R >> shift;
			this->frames[frame].background.G = this->frames[frame].background.G >> shift;
			this->frames[frame].background.B = this->frames[frame].background.B >> shift;
			this->frames[frame].background.A = this->frames[frame].background.A >> shift;
			this->frames[frame].bitDepth=val;
		}else{
			shift = -shift;
			for(unsigned int x=0;x<width()*height();x++){
				this->frames[frame].pixels[x].R = this->frames[frame].pixels[x].R << shift;
				this->frames[frame].pixels[x].G = this->frames[frame].pixels[x].G << shift;
				this->frames[frame].pixels[x].B = this->frames[frame].pixels[x].B << shift;
				this->frames[frame].pixels[x].A = this->frames[frame].pixels[x].A << shift;
			}
			this->frames[frame].background.R = this->frames[frame].background.R << shift;
			this->frames[frame].background.G = this->frames[frame].background.G << shift;
			this->frames[frame].background.B = this->frames[frame].background.B << shift;
			this->frames[frame].background.A = this->frames[frame].background.A << shift;
			this->frames[frame].bitDepth=val;
		}
	}
	this->imageData=this->frames[this->currentFrame]; // update our current frame data

	#ifdef GLOP_IMAGE_DEBUG
		//printf("%x\n%x",this->imageData.pixels,this->frames[0].pixels);
		printf("\n");
	#endif
}

std::map<std::string,std::vector<std::string> >& Image::getTextMappings(){
	return imageData.textPairs;
}

void printPixel(Pixel p){
	printf("R%d G%d B%d A%d \n",p.R,p.G,p.B,p.A);
}