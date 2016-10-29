#include "png.h"

namespace GLOP_IMAGE_PNG{

/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];
/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

//=========================================================================
// GENERAL FUNCTIONS
//=========================================================================
bool validPNG(FILE* imageFP){
	//PNG validation segment
	//the first 8 bytes are very specific and required to specify a valid PNG
	//we check here that it is valid
	unsigned char temp[]={0,0,0,0,0,0,0,0};
	unsigned char header[]={0x89,'P','N','G','\r','\n',0x1A,'\n'};
	fseek(imageFP,0,SEEK_SET); //reset to the beginning
	fread(temp,1,8,imageFP);

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
	fseek(imageFP,8,SEEK_SET); //reset to the beginning
	//basic info for how we will handle the pixel data stream
	bool usingPalette=false; //will we use a pallete to do color lookups?
	uint8_t palletebackgroundIndex=0; //what index in the pallete will be used as the background color
	bool adam7Interlace=false; //is the data stream interlaced or is it just pixels listed out the simple way
	bool handledIDAT=false; //flag for if we already handled all the IDAT chunks - true if they have been handled
	std::vector<Pixel> transparencyList; //list of what colors are transparent (0% alpha)
	std::vector<Pixel> palleteList;

	while(!feof(imageFP)){
		ChunkData chunk=getNextChunk(imageFP);

		if(strcmp(chunk.type,"IHDR")==0){
			#ifdef PNG_DEBUG
				printf("IHDR Bytes:  ");
				for(int x=0;x<13;x++) printf("%02X  ",chunk.data[x]);
				printf("\n\n");
			#endif
			data->width=0;
			data->height=0;
			for(int x=0;x<4;x++){
				data->width |=  chunk.data[x] <<((3-x)*8);
				data->height|= chunk.data[x+4]<<((3-x)*8);
			}
			data->bitDepth=chunk.data[8];
			switch(chunk.data[9]){
				case 0: data->pixelType=GRAY; break;
				case 3: usingPalette=true; //no break to make RGB
				case 2: data->pixelType=RGB; break;
				case 4: data->pixelType=GRAY_ALPHA; break;
				case 6: data->pixelType=RGB_ALPHA; break;
			}
			// == below are just reference place holders. The standard only allows a single value right now so we can probably ignore it
			//if(chunk.data[10]) some other compression
			//if(chunk.data[11]) some other filter method
			if(chunk.data[12]==1)adam7Interlace=true;
			#ifdef PNG_DEBUG
				if(adam7Interlace)printf("Interlaced\n");
				if(usingPalette)printf("Using Pallete\n");
			#endif
		}else if(strcmp(chunk.type,"PLTE")==0){
			if(!usingPalette)continue; // we can ignore a pallete since we are not color type 3 - it is an extra "just in case" thing added in
			palleteList.reserve(chunk.length/3);
			for(int x=0;x<chunk.length;x+=3){
				Pixel temp;
				temp.R=chunk.data[x];  //R
				temp.G=chunk.data[x+1];//G
				temp.B=chunk.data[x+2];//B
				palleteList.push_back(temp);
				#ifdef PNG_DEBUG
					printf("%d  %02X %02X %02X\n",x/3,palleteList[x/3].R,palleteList[x/3].G,palleteList[x/3].B);
				#endif
			}
		}else if(strcmp(chunk.type,"IDAT")==0){
			//This is a little special in that this should only ever get used once
			//There may be several IDAT chunks, but we need to ignore the ones after the first one
			//This is because in the function we call, we go ahead and read ALL the IDAT chunks since we need them all
			if(handledIDAT)continue; //SKIP!! - should not be needed anymore since we are changing curtPos in the unpacking
			handledIDAT=true;
			data->pixels = (struct Pixel*)malloc(data->width * data->height * sizeof(Pixel) );
			unsigned char *pixelStream = unpackPixelStream(data,chunk,adam7Interlace,imageFP);
			if(pixelStream==NULL){
				printf("Invalid Stream In Data\n");
				free(data->pixels);
				data->pixels=NULL;
				data->width=0;
				data->height=0;
				data->bitDepth=0;
				continue;
			}if(usingPalette){
				while(transparencyList.size() < palleteList.size()){
					transparencyList.push_back({0,0,0,255}); // make sure all the values left are at full opacity
				}
			}

			if(adam7Interlace){
				unfilterPixelStream_interlaced(data,pixelStream,palleteList);
				decodePixelStream_interlaced(data,pixelStream,palleteList,transparencyList);
			}else{
				unfilterPixelStream(data,pixelStream,palleteList);
				decodePixelStream(data,pixelStream,palleteList,transparencyList);
			}

			free(pixelStream);
			if(usingPalette)
				data->bitDepth=8;
			if(transparencyList.size()!=0){
				if(data->pixelType==GRAY)
					data->pixelType=GRAY_ALPHA;
				if(data->pixelType==RGB && !usingPalette)
					data->pixelType=RGB_ALPHA;
				if(usingPalette && transparencyList[0].A != pow(2,data->bitDepth)-1)
					data->pixelType=RGB_ALPHA;
			}

			break; // the IDAT is guraented to be next to last chunk, with IEND being the only one left
			//for efficency we should probably be fine with just quiting now
		}else if(strcmp(chunk.type,"IEND")==0){
			break; //end of image - and i really hope we got it all
		}else if(strcmp(chunk.type,"tRNS")==0){
			//this is a list of values that we match to pixels
			// -- if it matches the pixel, we make the pixel 100% transparent
			// -- if it doesn't match the pixel is 0% transparent
			//only used for color types 0,2,3 - GRAY, RGB, Pallete
			//only for Pallete images should we ever have more than 1 color in the list
			//for the other ones, we will only ever use the first color
			if(usingPalette){
				data->pixelType=RGB_ALPHA; // let them know we have alpha values
				for(int x=0;x<chunk.length;x++){
					transparencyList.push_back({0,0,0,chunk.data[x]}); //Put the data into the A spot
				}
				while(transparencyList.size() < palleteList.size()){
					transparencyList.push_back({0,0,0,255}); // make sure all the values left are at full opacity
				}
			}else if(data->pixelType==GRAY){
				for(int x=0;x<chunk.length;x+=2){
					Pixel temp;
					temp.G=chunk.data[x]|(chunk.data[x+1]<<8); //2 bytes and we need to put it into little indian for intel
					transparencyList.push_back(temp);
				}
			}else if(data->pixelType==RGB){
				for(int x=0;x<chunk.length;x+=6){
					Pixel temp;
					temp.R=chunk.data[x]|(chunk.data[x+1]<<8);
					temp.G=chunk.data[x+2]|(chunk.data[x+3]<<8);
					temp.B=chunk.data[x+4]|(chunk.data[x+5]<<8);
					transparencyList.push_back(temp);
				}
			}
		}else if(strcmp(chunk.type,"tEXt")==0){
			//key value pair that has text on it
			std::string s1=(const char*)chunk.data.data();
			std::string s2;

			chunk.length-=s1.length()+1; //move to after the null terminator
			chunk.data.erase(chunk.data.begin(),chunk.data.begin()+s1.length()+2);
			//get the rest of the string after the null terminator
			char temp[chunk.length]; //big enough for the rest of the data length and a null terminator
			for(int x=0;x<chunk.length;x++) temp[x]=chunk.data[x]; //copy the data to a place that we can put a null without breaking our data
			temp[chunk.length]=0; //make it null terminated
			s2=temp;

			data->textPairs[s1].push_back(s2);

			#ifdef PNG_DEBUG
				printf("%s\n",s1.c_str());
				printf("%s\n\n",s2.c_str());
			#endif
		}else if(strcmp(chunk.type,"iTXt")==0){
			//key value pair that has text on it
			//this might be compressed or have unicode or something - this may break on some things
			std::string s1=(const char*)chunk.data.data();
			std::string s2;

			chunk.length-=s1.length()+1; //move to after the null terminator
			chunk.data.erase(chunk.data.begin(),chunk.data.begin()+s1.length()+2);

			bool compressed=chunk.data[0];
			bool compressionMethod=chunk.data[1];
			chunk.data.erase(chunk.data.begin(),chunk.data.begin()+2);
			chunk.length-=2;

			//get past the language tag and the translated keyword
			while(chunk.data[0]!=0){chunk.data.erase(chunk.data.begin());chunk.length--;} //move until after the next null terminator
			chunk.data.erase(chunk.data.begin());chunk.length--;
			while(chunk.data[0]!=0){chunk.data.erase(chunk.data.begin());chunk.length--;} //move until after the next null terminator
			chunk.data.erase(chunk.data.begin());chunk.length--;

			//get the rest of the string after the null terminator
			char temp[chunk.length+1]; //big enough for the rest of the data length and a null terminator
			for(int x=0;x<chunk.length;x++) temp[x]=chunk.data[x]; //copy the data to a place that we can put a null without breaking our data
			temp[chunk.length]=0; //make it null terminated
			s2=temp;

			data->textPairs[s1].push_back(s2);

			#ifdef PNG_DEBUG
				printf("%s\n",s1.c_str());
				printf("%s\n\n",s2.c_str());
			#endif
		}else if(strcmp(chunk.type,"bKGD")==0){
			//tell us what the default background color for alpha channels and uncropped regions should be
			#ifdef PNG_DEBUG
				printf("Background Data: ");
				for(int x=0;x<chunk.length;x++)
					printf(" %02X",chunk.data[x]);
				printf("\n");
			#endif
			if(usingPalette){
				data->background.R=palleteList[ chunk.data[0] ].R;
				data->background.G=palleteList[ chunk.data[0] ].G;
				data->background.B=palleteList[ chunk.data[0] ].B;
				continue; //the switch below if for normal pixel data in the data region - not for pallets which show up as RGB in my library
			}
			switch(data->pixelType){
				case GRAY: case GRAY_ALPHA:
					data->background.G=(chunk.data[0]<<8)|(chunk.data[1]);
					break;
				case RGB: case RGB_ALPHA:
					data->background.R=(chunk.data[0]<<8)|(chunk.data[1]);
					data->background.G=(chunk.data[2]<<8)|(chunk.data[3]);
					data->background.B=(chunk.data[4]<<8)|(chunk.data[5]);
					break;
			}
		}
	}
}

//fileBuffer will be set to the buffer of chars that need to be written directly out to a file - or a socket - or something
//bufferSize will be set to the size of the fileBuffer
//data is the image we are encoding - only one frame is allowed so that is why we are not taking in a vector of frames
//save options - what it says - can specify different save options for when encoding the file
void packImage(FILE* imageFP,struct ImageData *data, SAVE_OPTIONS_PNG saveOptions){
	std::vector<Pixel> palleteList;
	bool has256orLessColors = __256orLessColors(data);
	if(has256orLessColors && (data->pixelType==GRAY || data->pixelType==GRAY_ALPHA) && !(saveOptions&USE_PALLETE) ) has256orLessColors=false; //do not promote grayscale to pallete unless explicitly asked
	bool hasSingleTransparentColor = __hasSingleTransparentColor(data);
	Pixel singleTransparentColor;
	if(hasSingleTransparentColor){
		singleTransparentColor = __singleTransparentColor(data); // remember what the pixel was that was transparent
		if(data->pixelType==GRAY_ALPHA) data->pixelType=GRAY; // remove the alpha channel to make it compress better
		if(data->pixelType==RGB_ALPHA) data->pixelType=RGB;
		#ifdef PNG_DEBUG
			printf("Single Transparent Color : ");
			printPixel(singleTransparentColor);
			printf("\n");
		#endif
	}

	make_crc_table(); // get the crc stuff ready to go
	{
		unsigned char header[]={0x89,'P','N','G','\r','\n',0x1A,'\n'};
		fwrite(header,8,1,imageFP);
	}

	//we always start with the IHDR chunk
	{ // IHDR Chunk
		unsigned char header[]={0,0,0,13,'I','H','D','R'};
		fwrite(header,8,1,imageFP);

		unsigned char body[13];
		//Width of image - padded out to 4 bytes
		body[0]= (data->width >> (8*3)) & 255;
		body[1]= (data->width >> (8*2)) & 255;
		body[2]= (data->width >> (8*1)) & 255;
		body[3]= (data->width >> (8*0)) & 255;
		//Height of image - padded out to 4 bytes
		body[4]= (data->height >> (8*3)) & 255;
		body[5]= (data->height >> (8*2)) & 255;
		body[6]= (data->height >> (8*1)) & 255;
		body[7]= (data->height >> (8*0)) & 255;
		//bit depth
		body[8]= data->bitDepth;
		//color type
		switch(data->pixelType){
			case GRAY:
				body[9]=0; break;
			case GRAY_ALPHA:
				body[9]=4; break;
			case RGB:
				body[9]=2; break;
			case RGB_ALPHA:
				body[9]=6; break;
		}
		//compression method
		body[10]=0;
		//filter method
		body[11]=0;
		//interlaced
		body[12]= (saveOptions & SAVE_OPTIONS_PNG::INTERLACING) ? 1 : 0;

		if(saveOptions & USE_PALLETE || has256orLessColors){
			body[8]=8; // we will always do 8-bit pallete indexes
			body[9]=3; //tell them we are using a pallete
		}

		fwrite(body,13,1,imageFP);

		unsigned long crcCode = update_crc(0xffffffffL,header+4,4);
		crcCode = update_crc(crcCode,body,13) ^ 0xffffffffL;
		body[0]= (crcCode >> (8*3)) & 255;
		body[1]= (crcCode >> (8*2)) & 255;
		body[2]= (crcCode >> (8*1)) & 255;
		body[3]= (crcCode >> (8*0)) & 255;
		fwrite(body,4,1,imageFP);
	}

	//Lets handle the text that we may have learned about or had edited
	//we keep it all in a std::map<string,vector>
	//we first need to get every catagory/keyword
	//we then write a chunk for every value under that keyword
	for(auto text_list = data->textPairs.begin();text_list != data->textPairs.end(); text_list++)
	{ // iTXt chunk
		for(unsigned int text_value=0; text_value < text_list->second.size(); text_value++){
			unsigned char header[]={0,0,0,0,'i','T','X','t'};
			int msgLength=5; // for the 2 flags and 3 null characters

			msgLength+=text_list->first.length();
			msgLength+=text_list->second[text_value].length();

			// lets put the size of our body into the header
			header[0]= (msgLength >> (8*3)) & 255;
			header[1]= (msgLength >> (8*2)) & 255;
			header[2]= (msgLength >> (8*1)) & 255;
			header[3]= (msgLength >> (8*0)) & 255;

			fwrite(header,8,1,imageFP);
			fwrite(text_list->first.c_str(),text_list->first.length(),1,imageFP);
			fputc(0,imageFP); // null character
			fputc(0,imageFP); // compression flag - currently we do not compress things
			fputc(0,imageFP); // compression method
			fputc(0,imageFP); // null character
			fputc(0,imageFP); // null character
			fwrite(text_list->second[text_value].c_str(),text_list->second[text_value].length(),1,imageFP);

			unsigned long crcCode = update_crc(0xffffffffL,header+4,4);
			crcCode = update_crc(crcCode,(unsigned char*)text_list->first.c_str(),text_list->first.length());
			header[0]=0; header[1]=0; header[2]=0; header[3]=0; header[4]=0; // the 5 zeros noted above
			crcCode = update_crc(crcCode,header,5);
			crcCode = update_crc(crcCode,(unsigned char*)text_list->second[text_value].c_str(),text_list->second[text_value].length());
			crcCode ^= 0xffffffffL;

			header[0]= (crcCode >> (8*3)) & 255;
			header[1]= (crcCode >> (8*2)) & 255;
			header[2]= (crcCode >> (8*1)) & 255;
			header[3]= (crcCode >> (8*0)) & 255;
			fwrite(header,4,1,imageFP);
		}
	}

	//Pallete chunk
	if(saveOptions & SAVE_OPTIONS_PNG::USE_PALLETE || has256orLessColors)
	{ // PLTE
		palleteList = createPallete(data);

		unsigned char header[]={0,0,0,0,'P','L','T','E'};
		unsigned int size = palleteList.size() * 3;
		header[0] = (size >> (8*3)) & 255;
		header[1] = (size >> (8*2)) & 255;
		header[2] = (size >> (8*1)) & 255;
		header[3] = (size >> (8*0)) & 255;
		unsigned char body[size];
		for(int x=0; x<palleteList.size(); x++){
			body[x*3  ]=palleteList[x].R;
			body[x*3+1]=palleteList[x].G;
			body[x*3+2]=palleteList[x].B;
		}
		fwrite(header,8,1,imageFP);
		fwrite(body,size,1,imageFP);

		unsigned long crcCode = update_crc(0xffffffffL,header+4,4);
		crcCode = update_crc(crcCode,body,size);
		crcCode ^= 0xffffffffL;
		header[0]= (crcCode >> (8*3)) & 255;
		header[1]= (crcCode >> (8*2)) & 255;
		header[2]= (crcCode >> (8*1)) & 255;
		header[3]= (crcCode >> (8*0)) & 255;
		fwrite(header,4,1,imageFP);

		//for(int x=0;x<palleteList.size();x++)
		//	printf("Pallete Entry Num %d:  %d %d %d\n",x,palleteList[x].R,palleteList[x].G,palleteList[x].B);
	}

	//Transparency chunk - we only care about it if we are using a pallete - DAMN THE TORPEDOS, FULL SPEED AHEAD
	{ // tRNS
		if(palleteList.size()!=0){
			#ifdef PNG_DEBUG
				printf("Making Transparency chunk\n");
				printf("Pallete Type\n");
			#endif

			//notable information
			//look at the createPallete function
			//the function sorts all pallete entries to have the onces with alphas at the top
			//this means we can look down the list until we read a pixel with full opcaity
			//this means that we can only write that many and on decoding assume the rest are full opacity

			int max = pow(2,data->bitDepth)-1; // max value that the channels can be

			unsigned char header[]={0,0,0,0,'t','R','N','S'};
			unsigned char body[palleteList.size()];
			int entries=0;
			for(int x=0;x<palleteList.size();x++){
				if(palleteList[x].A == max){
					entries=x;
					break;
				}
				body[x]=palleteList[x].A;
				#ifdef PNG_DEBUG
					printf("Entry %d: R%d G%d B%d\n",x,palleteList[x].R,palleteList[x].G,palleteList[x].B);
				#endif
			}

			if(entries!=0){ // if there are some pixels that are transparent
				header[3] = entries & 255; // save the size of the chunk
				fwrite(header,8,1,imageFP);
				fwrite(body,entries,1,imageFP);

				unsigned long crcCode = update_crc(0xffffffffL,header+4,4);
				crcCode = update_crc(crcCode,body,entries);
				crcCode ^= 0xffffffffL;
				header[0]= (crcCode >> (8*3)) & 255;
				header[1]= (crcCode >> (8*2)) & 255;
				header[2]= (crcCode >> (8*1)) & 255;
				header[3]= (crcCode >> (8*0)) & 255;
				fwrite(header,4,1,imageFP);
			}else{
				#ifdef PNG_DEBUG
					printf("Empty chunk - not writting\n");
				#endif
			}
			#ifdef PNG_DEBUG
				printf("\n");
			#endif
		}else if(hasSingleTransparentColor){
			#ifdef PNG_DEBUG
				printf("Making Transparency chunk\n");
				printf("%s Channels\n",data->pixelType==GRAY ? "Gray" : "RGB");
			#endif
			if(data->pixelType == GRAY){
				unsigned char header[]={0,0,0,2,'t','R','N','S',0,0              ,0,0,0,0};
				header[8] = (singleTransparentColor.G >>(8*1)) & 255;
				header[9] = (singleTransparentColor.G >>(8*0)) & 255;
				unsigned long crcCode = update_crc(0xffffffffL,header+4,6);
				crcCode ^= 0xffffffffL;
				header[10]= (crcCode >> (8*3)) & 255;
				header[11]= (crcCode >> (8*2)) & 255;
				header[12]= (crcCode >> (8*1)) & 255;
				header[13]= (crcCode >> (8*0)) & 255;
				fwrite(header,14,1,imageFP);
				#ifdef PNG_DEBUG
					printf("Gray: %d\n",singleTransparentColor.G);
				#endif
			}else if(data->pixelType == RGB){
				unsigned char header[]={0,0,0,6,'t','R','N','S',0,0,0,0,0,0      ,0,0,0,0};
				header[8] = (singleTransparentColor.R >>(8*1)) & 255;
				header[9] = (singleTransparentColor.R >>(8*0)) & 255;
				header[10] = (singleTransparentColor.G >>(8*1)) & 255;
				header[11] = (singleTransparentColor.G >>(8*0)) & 255;
				header[12] = (singleTransparentColor.B >>(8*1)) & 255;
				header[13] = (singleTransparentColor.B >>(8*0)) & 255;
				unsigned long crcCode = update_crc(0xffffffffL,header+4,10);
				crcCode ^= 0xffffffffL;
				header[14]= (crcCode >> (8*3)) & 255;
				header[15]= (crcCode >> (8*2)) & 255;
				header[16]= (crcCode >> (8*1)) & 255;
				header[17]= (crcCode >> (8*0)) & 255;
				fwrite(header,18,1,imageFP);
				#ifdef PNG_DEBUG
					printf("R%d G%d B%d\n",singleTransparentColor.R,singleTransparentColor.G,singleTransparentColor.B);
				#endif
			}
			#ifdef PNG_DEBUG
				printf("\n");
			#endif
		}
	}

	// background
	{ // bKGD chunk
		if(palleteList.size()!=0){
			int index=-1; // default to let us know if we can even find the background
			for(int x=0;x<palleteList.size();x++){
				if(palleteList[x].R != data->background.R) continue;
				if(palleteList[x].G != data->background.G) continue;
				if(palleteList[x].B != data->background.B) continue;
				index=x;
				break;
			}

			if(index!=-1){
				unsigned char header[]={0,0,0,1,'b','K','G','D',0                         ,0,0,0,0};
				header[8]  = (index >> (8*0)) & 255;

				unsigned long crcCode = update_crc(0xffffffffL,header+4,5);
				crcCode ^= 0xffffffffL;
				header[9]= (crcCode >> (8*3)) & 255;
				header[10]= (crcCode >> (8*2)) & 255;
				header[11]= (crcCode >> (8*1)) & 255;
				header[12]= (crcCode >> (8*0)) & 255;

				fwrite(header,13,1,imageFP);
			}
		}else if(data->pixelType == GRAY || data->pixelType == GRAY_ALPHA){
			unsigned char header[]={0,0,0,2,'b','K','G','D',0,0                       ,0,0,0,0};
			header[8]  = (data->background.G >> (8*1)) & 255;
			header[9]  = (data->background.G >> (8*0)) & 255;

			unsigned long crcCode = update_crc(0xffffffffL,header+4,6);
			crcCode ^= 0xffffffffL;
			header[10]= (crcCode >> (8*3)) & 255;
			header[11]= (crcCode >> (8*2)) & 255;
			header[12]= (crcCode >> (8*1)) & 255;
			header[13]= (crcCode >> (8*0)) & 255;

			fwrite(header,14,1,imageFP);
		}else if(data->pixelType == RGB || data->pixelType == RGB_ALPHA){
			unsigned char header[]={0,0,0,6,'b','K','G','D'  ,0,0  ,0,0  ,0,0         ,0,0,0,0};
			header[8]  = (data->background.R >> (8*1)) & 255;
			header[9]  = (data->background.R >> (8*0)) & 255;
			header[10] = (data->background.G >> (8*1)) & 255;
			header[11] = (data->background.G >> (8*0)) & 255;
			header[12] = (data->background.B >> (8*1)) & 255;
			header[13] = (data->background.B >> (8*0)) & 255;

			unsigned long crcCode = update_crc(0xffffffffL,header+4,10);
			crcCode ^= 0xffffffffL;
			header[14]= (crcCode >> (8*3)) & 255;
			header[15]= (crcCode >> (8*2)) & 255;
			header[16]= (crcCode >> (8*1)) & 255;
			header[17]= (crcCode >> (8*0)) & 255;

			fwrite(header,18,1,imageFP);
		}
	}

	// Image Data Itself
	{ // IDAT chunk
		unsigned long bufferSize;
		unsigned char *pixelStream;
		
		if(saveOptions & SAVE_OPTIONS_PNG::INTERLACING){
			bufferSize = getPixelStreamSize_interlaced(data);
			pixelStream = (unsigned char *)malloc(bufferSize);
			encodePixelStream_interlaced(data,pixelStream,palleteList);
			filterPixelStream_interlaced(data,pixelStream,palleteList);
		}else{
			bufferSize = getPixelStreamSize(data);
			pixelStream = (unsigned char *)malloc(bufferSize);
			encodePixelStream(data,pixelStream,palleteList);
			filterPixelStream(data,pixelStream,palleteList);
		}
		if(saveOptions & SAVE_OPTIONS_PNG::NO_COMPRESSION)
			packPixelStream(pixelStream,bufferSize,imageFP,0);
		else if(saveOptions & SAVE_OPTIONS_PNG::MIN_COMPRESSION)
			packPixelStream(pixelStream,bufferSize,imageFP,1);
		else if(saveOptions & SAVE_OPTIONS_PNG::MAX_COMPRESSION)
			packPixelStream(pixelStream,bufferSize,imageFP,9);
		else
			packPixelStream(pixelStream,bufferSize,imageFP,Z_DEFAULT_COMPRESSION);
		free(pixelStream);
	}

	//To end it all is a simple IEND chunk
	{ // IEND Chunk
		//does the length, type, and crc in one go
		unsigned char header[]={0,0,0,0,'I','E','N','D',0xAE,0x42,0x60,0x82};
		fwrite(header,12,1,imageFP);
	}

	#ifdef PNG_DEBUG
		printf("Done Saving Image\n\n");
	#endif
}

//=========================================================================
// SPECIFIC FUNCTIONS
//=========================================================================
struct ChunkData getNextChunk(FILE *imageFP){
	/*
		Given the file buffer, quite simply we read the buffer untill we get to the next chunk
		We then return a struct with the basic info of what the chunk type is
			- Type
			- Size
			- Starting position pointer in the buffer stream
			- CRC code - which I likely will never care about
	*/
	ChunkData chunk;
	chunk.type[4]=0; //null terminate the string
	chunk.length=0;

	//sanity check # 1
	if(feof(imageFP)){ // make sure there is enough space to read for the length, type, and CRC code
		chunk.type[0]='I';
		chunk.type[1]='E';
		chunk.type[2]='N';
		chunk.type[3]='D';
		return chunk;
	}

	unsigned char buff[4];
	//lets get the length of our chunk
	fread(buff,1,4,imageFP);
	for(int x=0;x<4;x++)
		chunk.length|=buff[x]<<((3-x)*8);
	//lets see what the 4 letter identifier is for this chunk
	fread(buff,1,4,imageFP);
	for(int x=0;x<4;x++)
		chunk.type[x]=buff[x];

	#ifdef PNG_DEBUG
		printf("Chunk Name :: %s\nChunk Length :: %d\n\n",chunk.type,chunk.length);
	#endif

	//read in all the chunks data and save it to our vector
	chunk.data.reserve(chunk.length);
	for(unsigned int x=0; x<chunk.length && !feof(imageFP); x++){
		// chunk.data = fileBuffer + startingPos;
		fread(buff,1,1,imageFP);
		chunk.data.push_back(buff[0]);
	}

	//sanity check # 2
	if(chunk.length > chunk.data.size()){ // dont overflow the chunk buffer if the chunk was not completly transmitted
		chunk.length = chunk.data.size();
		#ifdef PNG_DEBUG
			printf("Chunk Not Fully Transmitted\n");
			printf("New Chunk Length : %d\n\n",chunk.length);
		#endif
		return chunk;
	}

	//clear out the 4 byte CRC code
	fread(buff,1,4,imageFP);

	return chunk;
}

inline void __REVERSE_BYTES(unsigned char* data,int length){
	/*
		Reverse the bytes of a memory location
		Useful for converting Big <---> Little endianness
	*/
	for(int x=0;(length/2)>x;x++){
		unsigned char temp=data[x];
		data[x]=data[(length-1)-x];
		data[(length-1)-x]=temp;
	}
}

bool __256orLessColors(ImageData* data){
	std::unordered_set<uint64_t> colors;
	for(unsigned int x; x< data->width * data->height; x++){
		colors.insert(data->pixels[x].hash());
		if(colors.size() == 257) return false; // we just went over so the game is over
	}
	return true;
}
bool __hasSingleTransparentColor(ImageData* data){
	if(data->pixelType != RGB_ALPHA && data->pixelType !=GRAY_ALPHA) return false; // cant have transparency without these types

	bool found=false;
	Pixel single;
	unsigned int max = pow(2,data->bitDepth)-1;
	for(unsigned int x=0; x < data->width*data->height; x++){
		Pixel temp = data->pixels[x];
		if(temp.A==0 && found==false){
			single=temp;
			found=true;
			continue; // next pixel
		}
		if(temp.A != max && temp.A !=0) return false; // we need the full RGBA channels to represent this image
		if(temp.A==0 && !(temp==single)) return false; // found a second transparent color
		if(temp.A==max && temp.R==single.R && temp.G==single.G && temp.B==single.B) return false; //multiple opacities for the same color
	}
	return found; // never found a second color - return if we found a first color even
}
Pixel __singleTransparentColor(ImageData* data){
	for(unsigned int x=0; x < data->width*data->height; x++){
		if(data->pixels[x].A == 0) return data->pixels[x];
	}
	return {0,0,0,0}; // default option
}

unsigned char* unpackPixelStream(struct ImageData *data,struct ChunkData chunk,bool interlaced,FILE* imageFP){
	#ifdef PNG_DEBUG
		printf("Unpacking Data Buffer\n\n");
	#endif
	//this is the size of the buffer we need to hold all the pixelStream
	//for the pixels we have H * W * Channels * Bytes per pixel
	//we also have a single byte per scanline to tell us how the scanline channels ahve been ordered
	int pixelStreamSize;
	if(interlaced){
		pixelStreamSize = getPixelStreamSize_interlaced(data);
	}else{
		pixelStreamSize = getPixelStreamSize(data);
	}
	unsigned char *out=(unsigned char*)malloc(pixelStreamSize);

	//now we start the decoding of the stream of data
	int ret; // return value for decompression
	unsigned int bufferLeft = pixelStreamSize; // how much of the output buffer is left
	z_stream strm; // object like thing that does the decompression

	// allocate inflate state
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	//initalize the decompressor
	ret = inflateInit(&strm);
	if (ret != Z_OK){
		free(out);
		return NULL;
	}

	//directly decode from the data of each idat chunk
	//this is faster than the previous method of copying all the appropriate chunks to a vector
	//also fixed a buffer overflow vulnerability
	while(strcmp(chunk.type,"IDAT")==0){
		strm.avail_in = chunk.length; //the size of the input buffer
		strm.next_in = chunk.data.data(); //the raw array of our data
		strm.avail_out = bufferLeft; // how much of
		strm.next_out = out + (pixelStreamSize - bufferLeft); // move to the end of the previously decoded data

		ret = inflate(&strm, Z_NO_FLUSH); //do the acctual decompression
		assert(ret != Z_STREAM_ERROR);  //state not clobbered
		if(ret==Z_NEED_DICT || ret==Z_DATA_ERROR || ret==Z_MEM_ERROR){
			#ifdef PNG_DEBUG
				printf("WARNING : data in chunk caused a zlib error\n");
				switch(ret){
					case Z_NEED_DICT: printf("Z_NEED_DICT\n\n"); break;
					case Z_DATA_ERROR: printf("Z_DATA_ERROR\n\n"); break;
					case Z_MEM_ERROR: printf("Z_MEM_ERROR\n\n"); break;
				}
			#endif
			break; //get out of the loop to allow cleanup
		}

		bufferLeft = strm.avail_out; // save how much of the buffer we
		chunk = getNextChunk(imageFP);
	}

	inflateEnd(&strm);
	return out;
}

void decodePixelStream(struct ImageData *data,unsigned char *pixelStream,std::vector<Pixel> &palleteList,std::vector<Pixel> &transparencyList){
	/*
		Takes an pre-filtered pixel stream - aka, already undid the filters
		It then gets the pixels into the memory of the ImageData

		steps
		figure out how large a scanline of the image is in bytes - round up since we can use the partial of the last byte
			num of chanels * num of bytes per channel * imageWidth
		start looping onto each line of the image
			figure out how far down the stream this line is - lineWidth * linNum + lineNum
				this MUST include an extra byte in the lineWidth which is used to say what filter was applied
			figure out how far down the pixel buffer we are - lineNum*imageWidth
				the Pixel array takes care of us having to know the size in bytes of the Pixel struct
			earmark the start of the scanline in the pixel stream to more easily reference to it
				dont forget that the filtertype is the first byte, so after moving dataOffset to get to the start, we need
				to throw away the first byte before the pixel data itself

			start looping to get every pixel of the current scanline
				take into account what kind of pixel we are dealing with - Gray, Gray_Alpha, RGB, RGBA
					this matters as it tells us how many bytes to read from the pixel stream for each pixel
				Read in the pixel data
					we use G for Gray images since it starts with G, no other reason
					we fill out A to full opacity for images that do not normally have an alpha channel
	*/

	#ifdef PNG_DEBUG
		printf("Decoding Pixel Stream\n\n");
	#endif

	int numChannels=data->pixelType;
	if(palleteList.size()!=0) numChannels=1; //only one channel per pixel in a pallete
	Pixel singleTransparentColor;
	if(transparencyList.size()!=0 && (data->pixelType == RGB || data->pixelType == GRAY))
		singleTransparentColor = transparencyList[0];
	int max = pow(2,data->bitDepth)-1;

	for(int line=0;line<data->height;line++){
		int dataOffset = (int)ceil(line*(data->width) * numChannels * data->bitDepth/8.0 ); //what data number we are starting on in the stream
		int pixelOffset= line*data->width; //what number pixel we are on at the start of the line
		unsigned char *lineStart = pixelStream + dataOffset + line;//the start of this scanline
		unsigned char filterType = lineStart[0]; //the type of filter we are dealing with
		lineStart+=1; // remove the filter flag at the beginning

		//printf("%4d Filter Type: %d\t",line,filterType);
		//printf("Decoding Line %4d \n",line);

		for(int pixel=0;pixel<data->width;pixel++){
			if(palleteList.size()!=0){
				data->pixels[pixelOffset+pixel]=palleteList[__getDataFromStream(lineStart,data->bitDepth,pixel)];
				data->pixels[pixelOffset+pixel].A=transparencyList[__getDataFromStream(lineStart,data->bitDepth,pixel)].A;
			}else if(data->pixelType==GRAY){
				data->pixels[pixelOffset+pixel].G=__getDataFromStream(lineStart,data->bitDepth,pixel);
				if(transparencyList.size() != 0 && data->pixels[pixelOffset+pixel].G == singleTransparentColor.G)
					data->pixels[pixelOffset+pixel].A = 0;
				else
					data->pixels[pixelOffset+pixel].A=max; // set to full opacity for the given bitdepth
			}else if(data->pixelType==GRAY_ALPHA){
				data->pixels[pixelOffset+pixel].G=__getDataFromStream(lineStart,data->bitDepth,pixel*2);
				data->pixels[pixelOffset+pixel].A=__getDataFromStream(lineStart,data->bitDepth,pixel*2+1);
			}else if(data->pixelType==RGB){
				data->pixels[pixelOffset+pixel].R=__getDataFromStream(lineStart,data->bitDepth,pixel*3);
				data->pixels[pixelOffset+pixel].G=__getDataFromStream(lineStart,data->bitDepth,pixel*3+1);
				data->pixels[pixelOffset+pixel].B=__getDataFromStream(lineStart,data->bitDepth,pixel*3+2);
				if(transparencyList.size() !=0                                   &&
				   data->pixels[pixelOffset+pixel].R == singleTransparentColor.R &&
				   data->pixels[pixelOffset+pixel].G == singleTransparentColor.G &&
				   data->pixels[pixelOffset+pixel].B == singleTransparentColor.B    )
					data->pixels[pixelOffset+pixel].A = 0;
				else
					data->pixels[pixelOffset+pixel].A=max; // set to full opacity for the given bitdepth
			}else if(data->pixelType==RGB_ALPHA){
				data->pixels[pixelOffset+pixel].R=__getDataFromStream(lineStart,data->bitDepth,pixel*4);
				data->pixels[pixelOffset+pixel].G=__getDataFromStream(lineStart,data->bitDepth,pixel*4+1);
				data->pixels[pixelOffset+pixel].B=__getDataFromStream(lineStart,data->bitDepth,pixel*4+2);
				data->pixels[pixelOffset+pixel].A=__getDataFromStream(lineStart,data->bitDepth,pixel*4+3);
			}		
		}
		//printf("\n");
	}
}
void decodePixelStream_interlaced(struct ImageData *data,unsigned char *pixelStream,std::vector<Pixel> &palleteList,std::vector<Pixel> &transparencyList){
	/*
		Takes an pre-filtered pixel stream - aka, already undid the filters
		It then gets the pixels into the memory of the ImageData

		steps - assuming you fully understand how the non-interlaced function works already as this is annoyingly worse
		start loop how how many passes we have done - that makes 7 reduced images
			figure out how many cols and rows are in the reduced image
				(Height-startingLine) / lineIncrement
				same for cols
			figure out how large a scanline of the reducedImage is in bytes - round up since we can use the partial of the last byte
				num of chanels * num of bytes per channel * numCols in reducedImage
			start looping onto each line of the reducedImage
				figure out how far down the stream this line is - reducedLineWidth * reducedLineNum + reducedLineNum
					this MUST include an extra byte in the lineWidth which is used to say what filter was applied
				figure out how far down the pixel buffer we are - lineNum*imageWidth
					the Pixel array takes care of us having to know the size in bytes of the Pixel struct
					also note that it is the regular image lineNUm and imageWidth - not the reduced image
				earmark the start of the scanline in the pixel stream to more easily reference to it
					dont forget that the filtertype is the first byte, so after moving dataOffset to get to the start, we need
					to throw away the first byte before the pixel data itself

				start looping to get every pixel of the current scanline - incrementing the regular image pixel counter by col_increment
					take into account what kind of pixel we are dealing with - Gray, Gray_Alpha, RGB, RGBA
						this matters as it tells us how many bytes to read from the pixel stream for each pixel
					Read in the pixel data
						we use G for Gray images since it starts with G, no other reason
						we fill out A to full opacity for images that do not normally have an alpha channel
					Incrementing counters
						we increment 2 counters - smallPixel and pixel
						smallPixel is the pixel of the reduced image, so we increment by 1
						pixel if the pixel of the main image, so we increment by col_increment

				Incrementing counters
					we increment 2 counter - smallLine and line
					smallLine is the line of the reduced image, so we increment by 1
					line is the line of the main image, so we increment by row_increment
	*/
	#ifdef PNG_DEBUG
		printf("Decoding Pixel Stream\n\n");
	#endif

	//need to do the filtering too
	int numChannels=data->pixelType;
	if(palleteList.size()!=0) numChannels=1; //only one channel per pixel in a pallete

	int starting_row[7]  = { 0, 0, 4, 0, 2, 0, 1 };
	int starting_col[7]  = { 0, 4, 0, 2, 0, 1, 0 };
	int row_increment[7] = { 8, 8, 8, 4, 4, 2, 2 };
	int col_increment[7] = { 8, 8, 4, 4, 2, 2, 1 };
	int block_height[7]  = { 8, 8, 4, 4, 2, 2, 1 };
	int block_width[7]   = { 8, 4, 4, 2, 2, 1, 1 };

	for(int pass=0;pass<7;pass++){
		int smallImageWidth =(int)ceil((data->width - starting_col[pass]) / (double)col_increment[pass]); // number of columns in the reduced image
		int smallImageHeight=(int)ceil((data->height - starting_row[pass]) / (double)row_increment[pass]); //number of lines in the reduced image

		int smallImageWidthBytes = (int)ceil(smallImageWidth * numChannels * data->bitDepth/8.0); // how many bytes is each line in the pixel stream

		int smallLine=0; // counts the line of the reduced image we are on
		int line=starting_row[pass]; // we start on a particular line of the FULL image
		while(line<data->height){
			int smallPixel=0; // counts the pixel of the reduced image we are on
			int pixel=starting_col[pass]; // we start on a particular pixel of each line of the FULL image

			int dataOffset = (int)(smallLine*smallImageWidthBytes); //what data number we are starting on in the stream
			int pixelOffset= line*data->width; //what number pixel we are on at the start of the line
			unsigned char *lineStart = pixelStream + dataOffset + smallLine;//the start of this scanline
			unsigned char filterType = lineStart[0]; //the type of filter we are dealing with
			lineStart+=1; // remove the filter flag at the beginning

			//if(filterType<0 || filterType>4)printf("%d filter type : %d\n",smallLine,filterType);
			//if(pass!=6)break;

			while(pixel<data->width){
				if(palleteList.size()!=0){
					data->pixels[pixelOffset+pixel]=palleteList[__getDataFromStream(lineStart,data->bitDepth,smallPixel)];
					data->pixels[pixelOffset+pixel].A=transparencyList[__getDataFromStream(lineStart,data->bitDepth,smallPixel)].A;
				}else if(data->pixelType==GRAY){
					data->pixels[pixelOffset+pixel].G=__getDataFromStream(lineStart,data->bitDepth,smallPixel);
					data->pixels[pixelOffset+pixel].A=pow(2,data->bitDepth)-1; // set to full opacity for the given bitdepth
				}else if(data->pixelType==GRAY_ALPHA){
					data->pixels[pixelOffset+pixel].G=__getDataFromStream(lineStart,data->bitDepth,smallPixel*2);
					data->pixels[pixelOffset+pixel].A=__getDataFromStream(lineStart,data->bitDepth,smallPixel*2+1);
				}else if(data->pixelType==RGB){
					data->pixels[pixelOffset+pixel].R=__getDataFromStream(lineStart,data->bitDepth,smallPixel*3);
					data->pixels[pixelOffset+pixel].G=__getDataFromStream(lineStart,data->bitDepth,smallPixel*3+1);
					data->pixels[pixelOffset+pixel].B=__getDataFromStream(lineStart,data->bitDepth,smallPixel*3+2);
					data->pixels[pixelOffset+pixel].A=pow(2,data->bitDepth)-1; // set to full opacity for the given bitdepth
				}else if(data->pixelType==RGB_ALPHA){
					data->pixels[pixelOffset+pixel].R=__getDataFromStream(lineStart,data->bitDepth,smallPixel*4);
					data->pixels[pixelOffset+pixel].G=__getDataFromStream(lineStart,data->bitDepth,smallPixel*4+1);
					data->pixels[pixelOffset+pixel].B=__getDataFromStream(lineStart,data->bitDepth,smallPixel*4+2);
					data->pixels[pixelOffset+pixel].A=__getDataFromStream(lineStart,data->bitDepth,smallPixel*4+3);
				}

				pixel+=col_increment[pass];
				smallPixel++;
			}
			line+=row_increment[pass];
			smallLine++;
		}
		pixelStream += ((smallImageWidthBytes*smallImageHeight) + smallImageHeight); //simply the image size plus the filters before each line to get to the next reduced image
	}
}

uint16_t __getDataFromStream(unsigned char* stream,int bitDepth,int dataNum){
	/*
		This gets the data from the stream for every bitdepth
		This is a base utility for reading the pixel stream when putting numbers to the RGB values of the image
		YOu give it the stream that you want to get data from, what the bitdepth of the data is that you are
		looking for, and also which data down the line you want.
			You want the 83rd piece of data in a line of 4Bit data? will return the number from the right place (values 0-15)

		BitDepth types - reference when working on the function
		if 8 bits, then you get back the simple byte that is dataNum from the beginning of stream
		if 16 bit, then you get back the double byte that is dataNum*2 and dataNum*2+1 from the beginning
		if 4 bits, then the halfbyte of dataNum/2 from the beginning - etc
		namely this makes it a one stop shop for getting the right value from the stream
		this has no concept of channels - you must take that into account when you pass in dataNum
	*/
	stream+=(int)(bitDepth/8.0 * dataNum);
	if(bitDepth==16){
		return *stream | (*(stream+1))<<8;
	}if(bitDepth==8){
		return *stream;
	}if(bitDepth==4){
		if(dataNum%2 == 0) //evens - 0,2,4,6,8 etc are for the left half of the byte
			return (*stream)>>4;
		else
			return (*stream) & 0x0F;
	}if(bitDepth==2){
		if(dataNum%4 == 0){ // 0,4,8 etc are for the left left
			return (*stream)>>6;
		}if(dataNum%4 == 1){ // 1,5,9 etc are for the left middle
			return ((*stream)>>4) & 0x03;
		}if(dataNum%4 == 2){ // 2,6,10 etc are for the right middle
			return ((*stream)>>2) & 0x03;
		}if(dataNum%4 == 3){ // 3,7,11 etc are for the right right
			return (*stream) & 0x03;
		}
	}if(bitDepth==1){
		return ((*stream)>>(8-(dataNum%8))) & 0x01;
	}
	return 0;
}
void __setDataIntoStream(uint16_t data,unsigned char* stream,int bitDepth,int dataNum){
	//opposite to the __getDataFromStream function
	stream+=(int)(bitDepth/8.0 * dataNum);
	if(bitDepth==16){
		stream[0]=data>>8;
		stream[1]=data & 255;
	}if(bitDepth==8){
		stream[0]=data & 255;
	}if(bitDepth==4){
		if(dataNum%2 == 0) //evens - 0,2,4,6,8 etc are for the left half of the byte
			//return (*stream)>>4;
			stream[0]= ((data<<4) & 0xF0) | (stream[0] & 0x0F);
		else
			stream[0] = (stream[0] & 0xF0) | (data & 0x0F);
	}if(bitDepth==2){
		if(dataNum%4 == 0){ // 0,4,8 etc are for the left left
			stream[0] = ((data & 0x03) <<6) | (stream[0] & 0x3F); // upper two bits
		}if(dataNum%4 == 1){ // 1,5,9 etc are for the left middle
			stream[0] = ((data & 0x03) <<4) | (stream[0] & 0xCF);
		}if(dataNum%4 == 2){ // 2,6,10 etc are for the right middle
			stream[0] = ((data & 0x03) <<2) | (stream[0] & 0xF3);
		}if(dataNum%4 == 3){ // 3,7,11 etc are for the right right
			stream[0] = ((data & 0x03) <<0) | (stream[0] & 0xFC);
		}
	}if(bitDepth==1){
		stream[0] &= 1<<(8-dataNum%8); //clear bin number blah
		stream[0] |= (data&1) << (8-dataNum%8);
	}
}

void unfilterPixelStream(struct ImageData *data,unsigned char* pixelStream,std::vector<Pixel> &palleteList){
	/*
		Goes throught the pixel stream and removes the filter of each scanline to allow the decode functions
			to not worry about the filtering with the raw values right there ready to go
		It steps through every single line of the image, gets the filter type, and then for every byte does
			the undo operation for each filter

		steps
		figure out how many bytes each pixel is - minimum of 1 byte - so 4bit grayscale we still say we are 1 byte wide
		start looping for everyline - same as the decode functions above
			earmark the start of the scanline in the pixelStream
			get the first byte of the scanline as that tells us what the filter is
			start looping through the pixels
				based on the filter type, do a certain undo operation
	*/
	int numChannels=data->pixelType;
	if(palleteList.size()!=0) numChannels=1; //only one channel per pixel in a pallete
	int byteStride=numChannels;//lets see how far we will move for doing comparisons
	if(data->bitDepth==16)byteStride*=2;
	int width = (int)ceil(data->width * numChannels * data->bitDepth/8.0); //how long is each scanline of pixels in bytes

	#ifdef PNG_DEBUG
		printf("Unfiltering\n");
		printf("Pixel Size : %d\n",numChannels);
		printf("Byte Stride: %d\n",byteStride);
		printf("Stream Width %d\n",width);
		printf("\n");
	#endif

	for(int line=0;line<data->height;line++){
		int dataOffset = ((int)line*width)+ line; //what data number we are starting on in the stream
		unsigned char *lineStart = pixelStream + dataOffset;//the start of this scanline
		unsigned char filterType = lineStart[0]; //the type of filter we are dealing with
		lineStart+=1; // remove the filter flag at the beginning

		for(int pixel=0;pixel<width;pixel++){
			if(filterType==0)break; //no need to do anything
			switch(filterType){
				case 1: //sub
					if(pixel-byteStride<0)break; //nothing to do
					lineStart[pixel]+=lineStart[pixel-byteStride];
					break;
				case 2: //up
					if(line==0)break; //nothing to do
					lineStart[pixel]+=(lineStart-(width + 1))[pixel];
					break;
				case 3:{ //average
					int a,b;
					if(line==0)b=0;
					else b=(lineStart-(width+1))[pixel];
					if(pixel-byteStride<0)a=0;
					else a=lineStart[pixel-byteStride];

					lineStart[pixel]+=(unsigned char)floor(( a+b )/2 );
					break;
				} case 4:{ //Paeth
					int a,b,c,p;
					if(pixel-byteStride<0)a=0;
					else a=lineStart[pixel-byteStride];
					if(line==0)b=0;
					else b=(lineStart-(width+1))[pixel];
					if(line==0 || pixel-byteStride<0)c=0;
					else c=(lineStart-(width+1))[pixel-byteStride];

					p=a+b-c;
					if(abs(p-a)<=abs(p-b) && abs(p-a)<=abs(p-c))lineStart[pixel]+=a;
					else if(abs(p-b)<=abs(p-c))lineStart[pixel]+=b;
					else lineStart[pixel]+=c;

					break;
				} default:
					printf("%d Unkown Filter:  %d\n",line,filterType);
					pixel=width;
					break;
			}
		}
	}
}
void unfilterPixelStream_interlaced(struct ImageData *data,unsigned char* pixelStream,std::vector<Pixel> &palleteList){
	/*
		Goes throught the pixel stream and removes the filter of each scanline to allow the decode functions
			to not worry about the filtering with the raw values right there ready to go
		It steps through every single line of the image, gets the filter type, and then for every byte does
			the undo operation for each filter

		steps - interlaced version
		figure out how many bytes each pixel is - minimum of 1 byte - so 4bit grayscale we still say we are 1 byte wide
		start looping for the 7 passes
			figure out the new width and height of the reduced image
			start looping for everyline of the reduced image - same as the decode functions above
				earmark the start of the scanline in the pixelStream
				get the first byte of the scanline as that tells us what the filter is
				start looping through the pixels
					based on the filter type, do a certain undo operation
	*/
	int starting_row[7]  = { 0, 0, 4, 0, 2, 0, 1 };
	int starting_col[7]  = { 0, 4, 0, 2, 0, 1, 0 };
	int row_increment[7] = { 8, 8, 8, 4, 4, 2, 2 };
	int col_increment[7] = { 8, 8, 4, 4, 2, 2, 1 };

	int numChannels=data->pixelType;
	if(palleteList.size()!=0) numChannels=1; //only one channel per pixel in a pallete
	int byteStride=numChannels;//lets see how far we will move for doing comparisons
	if(data->bitDepth==16)byteStride*=2;
	unsigned char *lineStart;

	#ifdef PNG_DEBUG
		printf("Unfiltering\n");
	#endif

	for(int pass=0;pass<7;pass++){
		int smallImageWidth =(int)ceil((data->width - starting_col[pass]) / (double)col_increment[pass]); // number of columns in the reduced image
		int smallImageHeight=(int)ceil((data->height - starting_row[pass]) / (double)row_increment[pass]); //number of lines in the reduced image

		int smallImageWidthBytes = (int)ceil(smallImageWidth * numChannels * data->bitDepth/8.0); // how many bytes is each line in the pixel stream
		int width = (int)ceil(data->width * numChannels * data->bitDepth/8.0); //how long is each scanline of pixels

		#ifdef PNG_DEBUG
			printf("Pass %d: %dx%d\n",pass,smallImageWidth,smallImageHeight);
			printf("Pixel Size : %d\n",numChannels);
			printf("Byte Stride: %d\n",byteStride);
			printf("Stream Width %d\n",smallImageWidthBytes);
			printf("\n");
		#endif

		int smallLine=0;
		while(smallLine<smallImageHeight){
			int dataOffset = (smallLine*smallImageWidthBytes) + smallLine; //what data number we are starting on in the stream
			lineStart = pixelStream + dataOffset;//the start of this scanline
			unsigned char filterType = lineStart[0]; //the type of filter we are dealing with
			lineStart+=1; // remove the filter flag at the beginning

			int smallPixel=0;
			while(smallPixel<smallImageWidthBytes){
				if(filterType==0)break; //no need to do anything
				switch(filterType){
					case 1: //sub
						if(smallPixel-byteStride<0)break; //nothing to do
						lineStart[smallPixel]+=lineStart[smallPixel-byteStride];
						break;
					case 2: //up
						if(smallLine==0)break; //nothing to do
						lineStart[smallPixel]+=(lineStart-(smallImageWidthBytes + 1))[smallPixel];
						break;
					case 3:{ //average
						int a,b;
						if(smallLine==0)b=0;
						else b=(lineStart-(smallImageWidthBytes+1))[smallPixel];
						if(smallPixel-byteStride<0)a=0;
						else a=lineStart[smallPixel-byteStride];

						lineStart[smallPixel]+=(unsigned char)floor(( a+b )/2 );
						break;
					} case 4:{ //Paeth
						int a,b,c,p;
						if(smallPixel-byteStride<0)a=0;
						else a=lineStart[smallPixel-byteStride];
						if(smallLine==0)b=0;
						else b=(lineStart-(smallImageWidthBytes+1))[smallPixel];
						if(smallLine==0 || smallPixel-byteStride<0)c=0;
						else c=(lineStart-(smallImageWidthBytes+1))[smallPixel-byteStride];

						p=a+b-c;
						if(abs(p-a)<=abs(p-b) && abs(p-a)<=abs(p-c))lineStart[smallPixel]+=a;
						else if(abs(p-b)<=abs(p-c))lineStart[smallPixel]+=b;
						else lineStart[smallPixel]+=c;

						break;
					} default:
						printf("%d Unkown Filter:  %d\n",smallLine,filterType);
						smallPixel=smallImageWidth; // get out of the while loop since we dont care to just idle on this one line
						break;
				}
				smallPixel++;
			}
			smallLine++;
		}
		pixelStream+=(smallImageHeight*smallImageWidthBytes)+smallImageHeight;
	}
}

/* Make the table for a fast CRC. */
void make_crc_table(void)
{
 unsigned long c;
 int n, k;

 for (n = 0; n < 256; n++) {
   c = (unsigned long) n;
   for (k = 0; k < 8; k++) {
     if (c & 1)
       c = 0xedb88320L ^ (c >> 1);
     else
       c = c >> 1;
   }
   crc_table[n] = c;
 }
 crc_table_computed = 1;
}


/* Update a running CRC with the bytes buf[0..len-1]--the CRC
  should be initialized to all 1's, and the transmitted value
  is the 1's complement of the final running CRC (see the
  crc() routine below). */

unsigned long update_crc(unsigned long crc, unsigned char *buf, int len)
{
 unsigned long c = crc;
 int n;

 if (!crc_table_computed)
   make_crc_table();
 for (n = 0; n < len; n++) {
   c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
 }
 return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long crc(unsigned char *buf, int len)
{
 return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}

unsigned long getPixelStreamSize(struct ImageData *data){
	return (unsigned long)ceil(data->width*data->height * data->pixelType * data->bitDepth/8.0) + data->height;
}
unsigned long getPixelStreamSize_interlaced(struct ImageData *data){
	int starting_row[7]  = { 0, 0, 4, 0, 2, 0, 1 };
	int starting_col[7]  = { 0, 4, 0, 2, 0, 1, 0 };
	int row_increment[7] = { 8, 8, 8, 4, 4, 2, 2 };
	int col_increment[7] = { 8, 8, 4, 4, 2, 2, 1 };
	int totalSize=0;

	for(int pass=0;pass<7;pass++){
		int smallImageWidth =(int)ceil((data->width - starting_col[pass]) / (double)col_increment[pass]); // number of columns in the reduced image
		int smallImageHeight=(int)ceil((data->height - starting_row[pass]) / (double)row_increment[pass]); //number of lines in the reduced image
		int smallImageWidthBytes = (int)ceil(smallImageWidth * data->pixelType * data->bitDepth/8.0); // how many bytes is each line in the pixel stream

		totalSize += (smallImageHeight*smallImageWidthBytes)+smallImageHeight;
	}
	return totalSize;
}

void encodePixelStream(struct ImageData *data,unsigned char *pixelStream,std::vector<Pixel> &palleteList){
	#ifdef PNG_DEBUG
		printf("Encoding Pixel Stream\n\n");
	#endif

	int numChannels=data->pixelType;
	if(palleteList.size()!=0) numChannels=1; //only one channel per pixel in a pallete

	std::unordered_map<Pixel,unsigned char,PixelHash> pl2; // super fast lookup for entries of pixels
	if(palleteList.size()!=0){
		for(int x=0;x<palleteList.size();x++){
			pl2[palleteList[x]]=x;
		}
	}

	for(int line=data->height-1; line>=0; line--){
		int dataOffset = (int)ceil(line*(data->width) * numChannels * data->bitDepth/8.0 ); //what data number we are starting on in the stream
		int pixelOffset= line*data->width; //what number pixel we are on at the start of the line
		unsigned char *lineStart = pixelStream + dataOffset + line;//the start of this scanline
		lineStart[0] = 0; //the type of filter we are dealing with
		lineStart+=1; // remove the filter flag at the beginning

		for(int pixel=data->width-1; pixel>=0; pixel--){
			if(palleteList.size()!=0){
				// find index of pallete
				int index = pl2[data->pixels[pixelOffset+pixel]];
				__setDataIntoStream(index,lineStart,8,pixel);
			}else if(data->pixelType==GRAY){
				__setDataIntoStream(data->pixels[pixelOffset+pixel].G,lineStart,data->bitDepth,pixel);
			}else if(data->pixelType==GRAY_ALPHA){
				__setDataIntoStream(data->pixels[pixelOffset+pixel].G,lineStart,data->bitDepth,pixel*2  );
				__setDataIntoStream(data->pixels[pixelOffset+pixel].A,lineStart,data->bitDepth,pixel*2+1);
			}else if(data->pixelType==RGB){
				__setDataIntoStream(data->pixels[pixelOffset+pixel].R,lineStart,data->bitDepth,pixel*3  );
				__setDataIntoStream(data->pixels[pixelOffset+pixel].G,lineStart,data->bitDepth,pixel*3+1);
				__setDataIntoStream(data->pixels[pixelOffset+pixel].B,lineStart,data->bitDepth,pixel*3+2);
			}else if(data->pixelType==RGB_ALPHA){
				__setDataIntoStream(data->pixels[pixelOffset+pixel].R,lineStart,data->bitDepth,pixel*4  );
				__setDataIntoStream(data->pixels[pixelOffset+pixel].G,lineStart,data->bitDepth,pixel*4+1);
				__setDataIntoStream(data->pixels[pixelOffset+pixel].B,lineStart,data->bitDepth,pixel*4+2);
				__setDataIntoStream(data->pixels[pixelOffset+pixel].A,lineStart,data->bitDepth,pixel*4+3);
			}
		}
	}
}
void encodePixelStream_interlaced(struct ImageData *data,unsigned char *pixelStream,std::vector<Pixel> &palleteList){
	#ifdef PNG_DEBUG
		printf("Encoding Pixel Stream\n\n");
	#endif

	//need to do the filtering too
	int numChannels=data->pixelType;
	if(palleteList.size()!=0) numChannels=1; //only one channel per pixel in a pallete
	std::unordered_map<Pixel,unsigned char,PixelHash> pl2; // super fast lookup for entries of pixels
	if(palleteList.size()!=0){
		for(int x=0;x<palleteList.size();x++){
			pl2[palleteList[x]]=x;
		}
	}

	int starting_row[7]  = { 0, 0, 4, 0, 2, 0, 1 };
	int starting_col[7]  = { 0, 4, 0, 2, 0, 1, 0 };
	int row_increment[7] = { 8, 8, 8, 4, 4, 2, 2 };
	int col_increment[7] = { 8, 8, 4, 4, 2, 2, 1 };
	int block_height[7]  = { 8, 8, 4, 4, 2, 2, 1 };
	int block_width[7]   = { 8, 4, 4, 2, 2, 1, 1 };

	for(int pass=0;pass<7;pass++){
		int smallImageWidth =(int)ceil((data->width - starting_col[pass]) / (double)col_increment[pass]); // number of columns in the reduced image
		int smallImageHeight=(int)ceil((data->height - starting_row[pass]) / (double)row_increment[pass]); //number of lines in the reduced image

		int smallImageWidthBytes = (int)ceil(smallImageWidth * numChannels * data->bitDepth/8.0); // how many bytes is each line in the pixel stream

		int smallLine=0; // counts the line of the reduced image we are on
		int line=starting_row[pass]; // we start on a particular line of the FULL image
		while(line<data->height){
			int smallPixel=0; // counts the pixel of the reduced image we are on
			int pixel=starting_col[pass]; // we start on a particular pixel of each line of the FULL image

			int dataOffset = (int)(smallLine*smallImageWidthBytes); //what data number we are starting on in the stream
			int pixelOffset= line*data->width; //what number pixel we are on at the start of the line
			unsigned char *lineStart = pixelStream + dataOffset + smallLine;//the start of this scanline
			lineStart[0] = 0; //the type of filter we are dealing with
			lineStart+=1; // remove the filter flag at the beginning

			while(pixel<data->width){
				if(palleteList.size()!=0){
					int index = pl2[data->pixels[pixelOffset+pixel]];
					__setDataIntoStream(index,lineStart,8,smallPixel);
				}else if(data->pixelType==GRAY){
					__setDataIntoStream(data->pixels[pixelOffset+pixel].G,lineStart,data->bitDepth,smallPixel);
				}else if(data->pixelType==GRAY_ALPHA){
					__setDataIntoStream(data->pixels[pixelOffset+pixel].G,lineStart,data->bitDepth,smallPixel*2  );
					__setDataIntoStream(data->pixels[pixelOffset+pixel].A,lineStart,data->bitDepth,smallPixel*2+1);
				}else if(data->pixelType==RGB){
					__setDataIntoStream(data->pixels[pixelOffset+pixel].R,lineStart,data->bitDepth,smallPixel*3  );
					__setDataIntoStream(data->pixels[pixelOffset+pixel].G,lineStart,data->bitDepth,smallPixel*3+1);
					__setDataIntoStream(data->pixels[pixelOffset+pixel].B,lineStart,data->bitDepth,smallPixel*3+2);
				}else if(data->pixelType==RGB_ALPHA){
					__setDataIntoStream(data->pixels[pixelOffset+pixel].R,lineStart,data->bitDepth,smallPixel*4  );
					__setDataIntoStream(data->pixels[pixelOffset+pixel].G,lineStart,data->bitDepth,smallPixel*4+1);
					__setDataIntoStream(data->pixels[pixelOffset+pixel].B,lineStart,data->bitDepth,smallPixel*4+2);
					__setDataIntoStream(data->pixels[pixelOffset+pixel].A,lineStart,data->bitDepth,smallPixel*4+3);
				}

				pixel+=col_increment[pass];
				smallPixel++;
			}
			line+=row_increment[pass];
			smallLine++;
		}
		pixelStream += ((smallImageWidthBytes*smallImageHeight) + smallImageHeight); //simply the image size plus the filters before each line to get to the next reduced image
	}
}

void filterPixelStream(struct ImageData *data,unsigned char* pixelStream,std::vector<Pixel> &palleteList){
	int numChannels=data->pixelType;
	if(palleteList.size()!=0) numChannels=1; //only one channel per pixel in a pallete
	int byteStride=numChannels;//lets see how far we will move for doing comparisons
	if(data->bitDepth==16)byteStride*=2;
	int width = (int)ceil(data->width * numChannels * data->bitDepth/8.0); //how long is each scanline of pixels in bytes

	#ifdef PNG_DEBUG
		printf("Filtering\n");
		printf("Pixel Size : %d\n",numChannels);
		printf("Byte Stride: %d\n",byteStride);
		printf("Stream Width %d\n",width);
		printf("\n");
	#endif

	for(int line=data->height-1;line>=0;line--){
		int dataOffset = ((int)line*width)+ line; //what data number we are starting on in the stream
		unsigned char *lineStart = pixelStream + dataOffset;//the start of this scanline
		unsigned char filterType = lineStart[0]; //the type of filter we are dealing with
		lineStart+=1; // remove the filter flag at the beginning
		int bestCaseFilter=0;

		//Start by figuring out the best filter to use on this line
		int f0=0,f1=0,f2=0,f3=0,f4=0; // these are the totals the we will use
		for(int pixel=width-1;pixel>=0;pixel--){
			f0+=(signed char)lineStart[pixel];

			if(pixel-byteStride>=0)
				f1+=(signed char) (lineStart[pixel]-lineStart[pixel-byteStride]);
			else
				f1+=(signed char) lineStart[pixel];

			if(line==0)
				f2+=(signed char) lineStart[pixel];
			else
				f2+=(signed char) (lineStart[pixel]-(lineStart-(width + 1))[pixel]);

			{
				int a,b;
				if(line==0)b=0;
				else b=(lineStart-(width+1))[pixel];
				if(pixel-byteStride<0)a=0;
				else a=lineStart[pixel-byteStride];

				f3+=(signed char) (lineStart[pixel]-floor(( a+b )/2 ));
			}

			{
				int a,b,c,p;
				if(pixel-byteStride<0)a=0;
				else a=lineStart[pixel-byteStride];
				if(line==0)b=0;
				else b=(lineStart-(width+1))[pixel];
				if(line==0 || pixel-byteStride<0)c=0;
				else c=(lineStart-(width+1))[pixel-byteStride];

				p=a+b-c;
				if(abs(p-a)<=abs(p-b) && abs(p-a)<=abs(p-c))
					f4+=(signed char) lineStart[pixel]-a;
				else if(abs(p-b)<=abs(p-c))
					f4+=(signed char) lineStart[pixel]-b;
				else
					f4+=(signed char) lineStart[pixel]-c;
			}
		}
		if(f0<=f1 && f0<=f2 && f0<=f3 && f0<=f4)
			bestCaseFilter = 0;
		else if(f1<=f2 && f1<=f3 && f1<=f4)
			bestCaseFilter = 1;
		else if(f2<=f3 && f2<=f4)
			bestCaseFilter = 2;
		else if(f3<=f4)
			bestCaseFilter = 3;
		else
			bestCaseFilter = 4;
		*(lineStart-1) = bestCaseFilter; // let the decoder know what filter we used

		//lets apply this filter that we found to be best
		for(int pixel=width-1;pixel>=0;pixel--){
			if(bestCaseFilter==0)break; //no need to do anything
			switch(bestCaseFilter){
				case 1: //sub
					if(pixel-byteStride<0)break; //nothing to do
					lineStart[pixel]-=lineStart[pixel-byteStride];
					break;
				case 2: //up
					if(line==0)break; //nothing to do
					lineStart[pixel]-=(lineStart-(width + 1))[pixel];
					break;
				case 3:{ //average
					int a,b;
					if(line==0)b=0;
					else b=(lineStart-(width+1))[pixel];
					if(pixel-byteStride<0)a=0;
					else a=lineStart[pixel-byteStride];

					lineStart[pixel]-=(unsigned char)floor(( a+b )/2 );
					break;
				} case 4:{ //Paeth
					int a,b,c,p;
					if(pixel-byteStride<0)a=0;
					else a=lineStart[pixel-byteStride];
					if(line==0)b=0;
					else b=(lineStart-(width+1))[pixel];
					if(line==0 || pixel-byteStride<0)c=0;
					else c=(lineStart-(width+1))[pixel-byteStride];

					p=a+b-c;
					if(abs(p-a)<=abs(p-b) && abs(p-a)<=abs(p-c))
						lineStart[pixel]-=a;
					else if(abs(p-b)<=abs(p-c))
						lineStart[pixel]-=b;
					else
						lineStart[pixel]-=c;

					break;
				} default:
					printf("%d Unkown Filter:  %d\n",line,filterType);
					pixel=width;
					break;
			}
		}
	}
}
void filterPixelStream_interlaced(struct ImageData *data,unsigned char* pixelStream,std::vector<Pixel> &palleteList){
	int starting_row[7]  = { 0, 0, 4, 0, 2, 0, 1 };
	int starting_col[7]  = { 0, 4, 0, 2, 0, 1, 0 };
	int row_increment[7] = { 8, 8, 8, 4, 4, 2, 2 };
	int col_increment[7] = { 8, 8, 4, 4, 2, 2, 1 };

	int numChannels=data->pixelType;
	if(palleteList.size()!=0) numChannels=1; //only one channel per pixel in a pallete
	int byteStride=numChannels;//lets see how far we will move for doing comparisons
	if(data->bitDepth==16)byteStride*=2;
	unsigned char *lineStart;

	#ifdef PNG_DEBUG
		printf("Filtering\n");
	#endif

	for(int pass=0;pass<7;pass++){
		int smallImageWidth =(int)ceil((data->width - starting_col[pass]) / (double)col_increment[pass]); // number of columns in the reduced image
		int smallImageHeight=(int)ceil((data->height - starting_row[pass]) / (double)row_increment[pass]); //number of lines in the reduced image

		int smallImageWidthBytes = (int)ceil(smallImageWidth * numChannels * data->bitDepth/8.0); // how many bytes is each line in the pixel stream
		int width = (int)ceil(data->width * numChannels * data->bitDepth/8.0); //how long is each scanline of pixels

		#ifdef PNG_DEBUG
			printf("Pass %d: %dx%d\n",pass,smallImageWidth,smallImageHeight);
			printf("Pixel Size : %d\n",numChannels);
			printf("Byte Stride: %d\n",byteStride);
			printf("Stream Width %d\n",smallImageWidthBytes);
			printf("\n");
		#endif

		int smallLine=smallImageHeight-1;
		while(smallLine>=0){
			int dataOffset = (smallLine*smallImageWidthBytes) + smallLine; //what data number we are starting on in the stream
			lineStart = pixelStream + dataOffset;//the start of this scanline
			unsigned char filterType = lineStart[0]; //the type of filter we are dealing with
			lineStart+=1; // remove the filter flag at the beginning
			int bestCaseFilter=0;

			//Start by figuring out the best filter to use on this line
			int f0=0,f1=0,f2=0,f3=0,f4=0; // these are the totals the we will use
			for(int smallPixel=smallImageWidthBytes-1;smallPixel>=0;smallPixel--){
				f0+=(signed char)lineStart[smallPixel];

				if(smallPixel-byteStride>=0)
					f1+=(signed char) (lineStart[smallPixel]-lineStart[smallPixel-byteStride]);
				else
					f1+=(signed char) lineStart[smallPixel];

				if(smallLine==0)
					f2+=(signed char) lineStart[smallPixel];
				else
					f2+=(signed char) (lineStart[smallPixel]-(lineStart-(smallImageWidthBytes + 1))[smallPixel]);

				{
					int a,b;
					if(smallLine==0)b=0;
					else b=(lineStart-(smallImageWidthBytes+1))[smallPixel];
					if(smallPixel-byteStride<0)a=0;
					else a=lineStart[smallPixel-byteStride];

					f3+=(signed char) (lineStart[smallPixel]-floor(( a+b )/2 ));
				}

				{
					int a,b,c,p;
					if(smallPixel-byteStride<0)a=0;
					else a=lineStart[smallPixel-byteStride];
					if(smallLine==0)b=0;
					else b=(lineStart-(smallImageWidthBytes+1))[smallPixel];
					if(smallLine==0 || smallPixel-byteStride<0)c=0;
					else c=(lineStart-(smallImageWidthBytes+1))[smallPixel-byteStride];

					p=a+b-c;
					if(abs(p-a)<=abs(p-b) && abs(p-a)<=abs(p-c))
						f4+=(signed char) lineStart[smallPixel]-a;
					else if(abs(p-b)<=abs(p-c))
						f4+=(signed char) lineStart[smallPixel]-b;
					else
						f4+=(signed char) lineStart[smallPixel]-c;
				}
			}
			if(f0<=f1 && f0<=f2 && f0<=f3 && f0<=f4)
				bestCaseFilter = 0;
			else if(f1<=f2 && f1<=f3 && f1<=f4)
				bestCaseFilter = 1;
			else if(f2<=f3 && f2<=f4)
				bestCaseFilter = 2;
			else if(f3<=f4)
				bestCaseFilter = 3;
			else
				bestCaseFilter = 4;
			*(lineStart-1) = bestCaseFilter; // let the decoder know what filter we used

			//now lets apply the filter that we found to be best
			int smallPixel=smallImageWidthBytes-1;
			while(smallPixel>=0){
				if(bestCaseFilter==0)break; //no need to do anything
				switch(bestCaseFilter){
					case 1: //sub
						if(smallPixel-byteStride<0)break; //nothing to do
						lineStart[smallPixel]-=lineStart[smallPixel-byteStride];
						break;
					case 2: //up
						if(smallLine==0)break; //nothing to do
						lineStart[smallPixel]-=(lineStart-(smallImageWidthBytes + 1))[smallPixel];
						break;
					case 3:{ //average
						int a,b;
						if(smallLine==0)b=0;
						else b=(lineStart-(smallImageWidthBytes+1))[smallPixel];
						if(smallPixel-byteStride<0)a=0;
						else a=lineStart[smallPixel-byteStride];

						lineStart[smallPixel]-=(unsigned char)floor(( a+b )/2 );
						break;
					} case 4:{ //Paeth
						int a,b,c,p;
						if(smallPixel-byteStride<0)a=0;
						else a=lineStart[smallPixel-byteStride];
						if(smallLine==0)b=0;
						else b=(lineStart-(smallImageWidthBytes+1))[smallPixel];
						if(smallLine==0 || smallPixel-byteStride<0)c=0;
						else c=(lineStart-(smallImageWidthBytes+1))[smallPixel-byteStride];

						p=a+b-c;
						if(abs(p-a)<=abs(p-b) && abs(p-a)<=abs(p-c))
							lineStart[smallPixel]-=a;
						else if(abs(p-b)<=abs(p-c))
							lineStart[smallPixel]-=b;
						else
							lineStart[smallPixel]-=c;

						break;
					} default:
						printf("%d Unkown Filter:  %d\n",smallLine,filterType);
						smallPixel=smallImageWidthBytes;
						break;
				}
				smallPixel--;
			}
			smallLine--;
		}
		pixelStream+=(smallImageHeight*smallImageWidthBytes)+smallImageHeight;
	}
}

std::vector<Pixel> createPallete(struct ImageData *data){
	//currently this makes a pallete of up to 256 colors
	//this returns a list of colors that are guarenteed to be sorted with transparent colors at the top
	//
	//Things this will do
	// - quantize an image
	if(!__256orLessColors(data)){ // this is so that if the user wanted a pallete but it has too many colors
		#ifdef PNG_DEBUG
			printf("Quantizing Image\n\n");
		#endif
	}
	#ifdef PNG_DEBUG
		printf("Creating Pallete\n\n");
	#endif

	//assuming that we have 256 or fewer colors
	std::set<Pixel> pl; // stands for pallete list
	for(unsigned int x; x < data->width*data->height; x++){
		//if we are grayscale, we must convert pixels over to RGB
		if(data->pixelType == GRAY || data->pixelType == GRAY_ALPHA){
			data->pixels[x].R = data->pixels[x].G;
			data->pixels[x].B = data->pixels[x].G;
		}
		pl.insert(data->pixels[x]);
	}

	std::vector<Pixel> palleteList;
	palleteList.reserve(256);
	for(auto x=pl.begin(); x!=pl.end(); x++){ // put them into our vector
		Pixel temp = *x;
		palleteList.push_back(temp);
	}
	int val=pow(2,data->bitDepth)-1; // the max value the things can be at this images bit depth
	for(int x=0,lastKnown=-1;x<palleteList.size();x++){ // move all transparent pixels to the top
		if(palleteList[x].A==val)continue;//not transparent so ignore this
		lastKnown+=1;//move to the next pixel, which is guarenteed to be alpha free
		#ifdef PNG_DEBUG
			printf("Swapping %d and %d\n",lastKnown,x);
			printf("R%03d G%03d B%03d\n",palleteList[lastKnown].R,palleteList[lastKnown].G,palleteList[lastKnown].B);
			printf("R%03d G%03d B%03d\n\n",palleteList[x].R,palleteList[x].G,palleteList[x].B);
		#endif
		Pixel temp=palleteList[lastKnown];
		palleteList[lastKnown]=palleteList[x];
		palleteList[x]=temp;
	}
	return palleteList;
}

void packPixelStream(unsigned char *pixelStream,int bufferSize,FILE* imageFP,int compressionLevel){

	#ifdef PNG_DEBUG
		printf("Packing Pixel Stream \n\n");
	#endif

	int ret; // return value for decompression
	z_stream strm; // object like thing that does the decompression

	// allocate inflate state
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	//initalize the decompressor
	//ret = deflateInit(&strm,9); // max compression at the cost of time
	ret = deflateInit(&strm,compressionLevel); // assume default compression settings
	if (ret != Z_OK){
		return;
	}

	int outSize = 10000;
	unsigned char* out = (unsigned char*)malloc(outSize);
	strm.avail_in = bufferSize;
	strm.next_in  = pixelStream;
	do{
		//reset the compression settings
		strm.avail_out= outSize;
		strm.next_out = out;

		ret = deflate(&strm, Z_FINISH); //do the acctual compression

		//lets write our compressed data out to file
		unsigned char header[]={0,0,0,0,'I','D','A','T'};
		header[0] = ((outSize-strm.avail_out) >> (8*3)) & 255; // figure out the size of our chunk
		header[1] = ((outSize-strm.avail_out) >> (8*2)) & 255;
		header[2] = ((outSize-strm.avail_out) >> (8*1)) & 255;
		header[3] = ((outSize-strm.avail_out) >> (8*0)) & 255;
		fwrite(header,8,1,imageFP);
		fwrite(out,outSize-strm.avail_out,1,imageFP);

		//make the CRC code
		unsigned long crcCode = update_crc(0xffffffffL,header+4,4);
		crcCode = update_crc(crcCode,out,outSize-strm.avail_out);
		crcCode ^= 0xffffffffL;

		header[0]= (crcCode >> (8*3)) & 255;
		header[1]= (crcCode >> (8*2)) & 255;
		header[2]= (crcCode >> (8*1)) & 255;
		header[3]= (crcCode >> (8*0)) & 255;
		fwrite(header,4,1,imageFP);
	}while(ret != Z_STREAM_END); // while we are still trying to compress

	ret = deflateEnd(&strm);
	if(ret != Z_OK){
		printf("PNG Compression Error\n");
	}
	free(out);
}

} // GLOP_IMAGE_PNG