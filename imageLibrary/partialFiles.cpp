#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* args[]){
	if(argc!=2){
		printf("usage: %s <filename in>",args[0]);
		return 0;
	}
	FILE *fp = fopen(args[1],"rb");
	if(fp==NULL){
		printf("Unable to open %s\n",args[1]);
		return 1;
	}

	//lets see how large the file is
	unsigned char *fileBuffer=(unsigned char*)malloc(10000);
	while(!feof(fp)){
		int written = fread(fileBuffer,1,9999,fp); //read until the end of the file - ussually only 1 or two of these operations
	}
	free(fileBuffer);
	int fileSize = ftell(fp);

	//fill the buffer with only half of the file
	fseek(fp,0,SEEK_SET); //reset to the beginning
	fileBuffer=(unsigned char*)malloc(fileSize/2);
	fread(fileBuffer,1,fileSize/2,fp);

	FILE* fp2 = fopen("temp.png","wb");
	if(fp2 != NULL)fwrite(fileBuffer,fileSize/2,1,fp2);

	free(fileBuffer);
	fclose(fp);
	return 0;
}