#include <stdio.h>
#include <string.h>
#include "zipFile.h"

void printHelp();

int main(int argc,char* args[]){
	if(argc==1){
		printHelp();
		return 0;
	}
	for(int x=1;x<argc;x++){
		if(strcmp(args[x],"-h")==0 || strcmp(args[x],"--help")==0){
			printHelp();
			return 0;
		}
	}

	ZipFile z;
	for(int x=1;x<argc;x++){
		//ignore the known args
		if(strcmp(args[x],"-h")==0 || strcmp(args[x],"--help")==0) continue;

		z.addFile(args[x]);
	}
	z.saveFile("archive.zip");
	return 0;
}

void printHelp(){
	printf("Simple example code for using the zip library\n");
	printf("usage: cli.out [-h] set_of_files_to_add\n");
	printf("\t-h\tprint this\n");
}