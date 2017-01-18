#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <stdint.h>
#include <time.h>
#include <math.h>

//#define PNG_DEBUG
#define JPEG_DEBUG
#define GLOP_IMAGE_DEBUG
#include "image.h"

#define PI 3.14159265358979323846

void bandShift(Image&,int bands, double shiftAmount);
void randomBandShift_channel(Image&,int bands, double shiftAmount,int channel);
void randomBandShift(Image&,int bands, double shiftAmount);
void sinShift(Image&,double bands, double amount);
void sortHorizontal(Image&);

void convertRGBToHSV(Image&);
void convertRGBToIDK(Image&);
void convertRGBToYCbCr(Image&);
void convertRGBToCMY(Image&);

void convertHSVtoRGB(Image&);

void writeImageFile(Image&,const char*);

int main(int argc, char **args){
	if(argc!=2 && argc!=3){
		printf("Below are the two ways to use this program\n");
		printf("Usage: %s <filename in>\n",args[0]);
		printf("Usage: %s <filename in> <filename out>\n",args[0]);
		return 0;
	}

	FILE *imageFP = fopen(args[1],"rb");
	if(imageFP==NULL){
		printf("Unable to open %s\n",args[1]);
		return 1;
	}
	fclose(imageFP);
	srand(time(NULL)); // random seed

	//Image image(imageFP);
	Image image(args[1]);
	printf("decoded image :: %s\n",args[1]);

	//=======================================================
	// randomBandShift(image,16,-0.2);
	// sinShift(image,6,0.25);


	// auto& pairs = image.getTextMappings();
	// pairs["TestComment"].push_back("With Test Text");
	// pairs["TestComment2"].push_back("With Test Text");
	// pairs["TestComment3"].push_back("With Test Text");

	// printf("Saving image..\n");
	if(argc==3)image.saveImage(args[2],PNG,PNG_NOTHING);
	else{
		// image.saveImage("temp.png",PNG,PNG_NOTHING);
		// printf("encoded png\n");
		image.saveImage("temp.jpg",JPG,JPG_NOTHING);
		// printf("encoded jpg\n");
	}

	return 0;
}

void bandShift(Image& image,int bands, double shiftAmount){
	int bandHeight=image.height()/bands;
	int bandShift =image.width()/bands;
	bandShift*=shiftAmount;

	printf("\nBand Shift\n");
	printf("Band Height: %d\n",bandHeight);
	printf("Band Shift : %d\n",bandShift);

	Pixel temp;
	for(int y=0;y<image.height();y++){
		for(int x=0;x<image.width();x++){
			temp=image[y][x];
			image[y][x] = image[y][( x + (bandShift * (y/bandHeight)) )%image.width()];
			image[y][( x + (bandShift * (y/bandHeight)) )%image.width()]=temp;
		}
	}
}
void randomBandShift(Image& image,int bands, double shiftAmount){	
	int bandHeight=rand()%(image.height()/bands)+1; //choose a random height for the first band;
	int bandShift =image.width()/bands;
	bandShift*=shiftAmount; //amount to shift each band by

	printf("\nRandom Band Shift\n");
	printf("Band Height: %d\n",bandHeight);
	printf("Band Shift : %d\n",bandShift);

	int randGot=rand()%bands; //pick a random bandShift amount to shift by;
	Pixel temp;
	for(int y=0;y<image.height();y++){
		if(y%bandHeight==0){
			randGot=rand()%bands; //pick a random bandShift amount to shift by
			bandHeight = rand()%(image.height()/bands)+1; //choose a random height for the next band
		}
		for(int x=0;x<image.width();x++){
			temp=image[y][x];
			image[y][x] = image[y][( x + (bandShift * (y/bandHeight)) )%image.width()];
			image[y][( x + (bandShift * (y/bandHeight)) )%image.width()]=temp;
		}
	}
}
void randomBandShift_channel(Image& image,int bands, double shiftAmount,int channel){
	int bandHeight=rand()%(image.height()/bands)+1; //choose a random height for the first band;
	int bandShift =image.width()/bands;
	bandShift*=shiftAmount; //amount to shift each band by

	printf("\nRandom Band Shift\n");
	printf("Band Height: %d\n",bandHeight);
	printf("Band Shift : %d\n",bandShift);
	if(channel & 0b0001){
		printf("Shifting Red\n");
	}if(channel & 0b0010){
		printf("Shifting Green\n");
	}if(channel & 0b0100){
		printf("Shifting Blue\n");
	}if(channel & 0b1000){
		printf("Shifting Alpha\n");
	}

	int randGot=rand()%bands; //pick a random bandShift amount to shift by;
	unsigned int temp;
	for(int y=0;y<image.height();y++){
		if(y%bandHeight==0){
			randGot=rand()%bands; //pick a random bandShift amount to shift by
			bandHeight = rand()%(image.height()/bands)+1; //choose a random height for the next band
		}
		for(int x=0;x<image.width();x++){
			if(channel & 0b0001){
				temp=image[y][x].R;
				image[y][x].R = image[y][( x + (bandShift * (y/bandHeight)) )%image.width()].R;
				image[y][( x + (bandShift * (y/bandHeight)) )%image.width()].R=temp;
				//image[y][x].R = 0;
			}if(channel & 0b0010){
				temp=image[y][x].G;
				image[y][x].G = image[y][( x + (bandShift * (y/bandHeight)) )%image.width()].G;
				image[y][( x + (bandShift * (y/bandHeight)) )%image.width()].G=temp;
				//image[y][x].G = 0;
			}if(channel & 0b0100){
				temp=image[y][x].B;
				image[y][x].B = image[y][( x + (bandShift * (y/bandHeight)) )%image.width()].B;
				image[y][( x + (bandShift * (y/bandHeight)) )%image.width()].B=temp;
				//image[y][x].B = 0;
			}if(channel & 0b1000){
				temp=image[y][x].A;
				image[y][x].A = image[y][( x + (bandShift * (y/bandHeight)) )%image.width()].A;
				image[y][( x + (bandShift * (y/bandHeight)) )%image.width()].A=temp;
			}
		}
	}
}
void sinShift(Image& image,double bands, double shiftAmount){
	int bandHeight=image.height()/(2*PI*bands);
	int bandShift =image.width()/bands;
	bandShift*=shiftAmount;

	printf("\nSine Shift\n");
	printf("Band Height: %d\n",bandHeight);
	printf("Band Shift : %d\n",bandShift);

	Pixel temp;
	for(int y=0;y<image.height();y++){
		for(int x=0;x<image.width();x++){
			temp=image[y][x];
			image[y][x] = image[y][((long)( x + (sin(y/(double)bandHeight)*bandShift) ))%image.width()];
			image[y][((long)( x + (sin(y/(double)bandHeight)*bandShift) ))%image.width()]=temp;
		}
	}
}
double min(double f,double s){
	if(f<s) return f;
	return s;
}
double max(double f,double s){
	if(f>s)return f;
	return s;
}
void convertRGBToIDK(Image& image){
	printf("\nIDK Color Space Conversion\n");
	//http://www.javascripter.net/faq/rgb2hsv.htm
	//not sure this is correct
	double scale = pow(2,image.getBitDepth())-1;
	for(int line=0;line<image.height();line++){
		for(int pixel=0;pixel<image.width();pixel++){
			Pixel pix = image[line][pixel];
			double r,g,b;
			r = pix.R / scale;
			g = pix.G / scale;
			b = pix.B / scale;

			double maxRGB,minRGB;
			maxRGB=max(r,max(g,b));
			minRGB=min(r,min(g,b));
			double d = (r==minRGB) ? g-b : ((b==minRGB) ? r-g : b-r);
			double h = (r==minRGB) ? 3 : ((b==minRGB) ? 1 : 5);

			double H = 60.0 * ((h-d) / (maxRGB-minRGB));
			double S = (maxRGB-minRGB)/maxRGB;
			double V = maxRGB;

			image[line][pixel].R = H*scale;
			image[line][pixel].G = S*scale;
			image[line][pixel].B = V*scale;
		}
	}
}
double __modulo(double f,double s){
	while(f<0)f+=s;
	while(f>s)f-=s;
	return f;
}
void convertRGBToHSV(Image& image){
	printf("\nHSV Color Space Conversion\n");
	//http://www.rapidtables.com/convert/color/rgb-to-hsv.htm
	double scale = pow(2,image.getBitDepth())-1;
	for(int line=0;line<image.height();line++){
		for(int pixel=0;pixel<image.width();pixel++){
			Pixel pix = image[line][pixel];
			double r,g,b;
			r = pix.R / scale; // make the rgb values a percentage
			g = pix.G / scale;
			b = pix.B / scale;

			double maxRGB,minRGB;
			maxRGB=max(r,max(g,b));
			minRGB=min(r,min(g,b));
			double delta=maxRGB-minRGB;

			double H,S,V;
			if(delta==0)
				H = 0;
			else if(maxRGB==r){
				H = 60.0 * __modulo(((g-b)/delta),6);
			}else if(maxRGB==g){
				H = 60.0 * (((b-r)/delta) +2);
			}else if(maxRGB==b){
				H = 60.0 * (((r-g)/delta) +4);
			}
			H /= 360.0; //turn back from degrees to percentage

			if(maxRGB==0) S = 0;
			else          S = delta/maxRGB;

			V=maxRGB;

			image[line][pixel].R = H*scale;
			image[line][pixel].G = S*scale;
			image[line][pixel].B = V*scale;
		}
	}
}
void convertRGBToCMY(Image& image){
	printf("\nCMY Color Space Conversion\n");
	double scale = pow(2,image.getBitDepth())-1;
	for(unsigned int line=0; line<image.height(); line++){
		for(unsigned int pixel=0; pixel<image.width(); pixel++){
			double r,g,b;
			r=image[line][pixel].R / scale;
			g=image[line][pixel].G / scale;
			b=image[line][pixel].B / scale;

			double c,m,y,k;
			k = 1 - max(r,max(g,b));
			c = (1-r-k) / (1-k);
			m = (1-g-k) / (1-k);
			y = (1-b-k) / (1-k);

			image[line][pixel].R = c * scale;
			image[line][pixel].G = m * scale;
			image[line][pixel].B = y * scale;
		}
	}
}
void convertHSVtoRGB(Image& image){
	printf("\nRGB Color Space Conversion\n");
	double scale = pow(2,image.getBitDepth())-1;
	for(unsigned int line=0; line<image.height(); line++){
		for(unsigned int pixel=0; pixel<image.width(); pixel++){
			Pixel pix = image[line][pixel];
			double h,s,v;
			h = pix.R / scale;
			s = pix.G / scale;
			v = pix.B / scale;
			h *= 360.0; // change to degrees

			double c,x,m;
			c = s * v;
			x = c * (1 - fabs(__modulo(h/60.0, 2.0) - 1));
			m = v - c;

			double r,g,b;
			if(h<60){
				r=c;
				g=x;
				b=0;
			}else if(h<120){
				r=x;
				g=c;
				b=0;
			}else if(h<180){
				r=0;
				g=c;
				b=x;
			}else if(h<240){
				r=0;
				g=x;
				b=c;
			}else if(h<300){
				r=x;
				g=0;
				b=c;
			}else if(h<360){
				r=c;
				g=0;
				b=x;
			}

			image[line][pixel].R = (r+m) * scale;
			image[line][pixel].G = (g+m) * scale;
			image[line][pixel].B = (b+m) * scale;
		}
	}
}
void convertRGBToYCbCr(Image& image){
	//https://msdn.microsoft.com/en-us/library/ff635643.aspx
	for(int line=0;line<image.height();line++){
		for(int pixel=0;pixel<image.width();pixel++){
			Pixel p = image[line][pixel];
			int y=0,cb=0,cr=0;
			//the conversion maps y to 0 -> 255  cb to -128 -> 127  cr to -126 -> 127

			y = (p.R*0.299) + (p.G*0.587) + (p.B*0.114);
			cb= (p.R*-0.168935) + (p.G*-0.331665) + (p.B*0.50059);
			cr= (p.R*0.499813)  + (p.G*-0.418531) + (p.B*-0.081282);

			image[line][pixel].R = y;
			image[line][pixel].G = cb+128;
			image[line][pixel].B = cr+128;
		}
	}
}

void sortHorizontal(Image& image){
	printf("\nSorting\n");
	for(unsigned int line=0; line<image.height(); line++){
		for(unsigned int pixel=image.width(); pixel>0; pixel--){
			for(unsigned int p2=0; p2<pixel; p2++){
				if(image[line][p2].R > image[line][p2+1].R){
					Pixel temp = image[line][p2+1];
					image[line][p2+1] = image[line][p2];
					image[line][p2] = temp;
				}
			}
		}
	}
}