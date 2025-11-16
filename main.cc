#include <stdio.h>
#include <unistd.h>
//#include <zlib.h>
//#include <cassert>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>


#include "./compression.class.h"

int main(void){
	int totalTests=4;
	int testsSuccessful=0;

	printf("~~Compression Tests~~~\n");
	Compression compress;
	
	printf("[*] Compressing test.txt into compress.z...");
	if(!compress.zlibDeflate("./test.txt", "./compress.z", 9)){
		printf("Deflation failed.\n");
	}else{
		testsSuccessful++;
		printf("success!\n");
	}

	printf("[*] Decompressing compress.z into decompress.txt...");
	if(!compress.zlibInflate("./compress.z", "decompress.txt")){
		printf("Inflation failed.\n");
	}else{
		testsSuccessful++;
		printf("success!\n");
	}

	printf("[*] Testing memory to memory compression...");
	std::string sample = "this is a sample string and we are going to test compression using it.";
	size_t zippedSize = (sample.length()/COMPRESSION_CHUNK) < 1 ? COMPRESSION_CHUNK : (sample.length()/COMPRESSION_CHUNK) * COMPRESSION_CHUNK;
	char *zippedBuffer = new char[zippedSize];
	if(!compress.zlibDeflate((char *)sample.c_str(), sample.length(), zippedBuffer, &zippedSize, 9)){
		printf("Memory to memory deflation failed.\n");
	}else{
		testsSuccessful++;
		printf("success!\nEncoded (%ld): ", zippedSize);
		for(int i=0; i<zippedSize-1; i++) printf("%c", zippedBuffer[i]);
		printf("\n");
	}

	printf("[*] Testing memory to memory decompression...");
	size_t unzippedSize=sample.length();
	char *unzipped = new char[unzippedSize];
	if(!compress.zlibInflate(zippedBuffer, zippedSize, unzipped, unzippedSize)){
		printf("memory inflation failed.\n");
	}else{
		testsSuccessful++;
		printf("Success!\nDecoded : ");
		for(int i=0; i<unzippedSize; i++) printf("%c", unzipped[i]);
		printf("\n");
	}

	printf("\n\n%d tests out of %d were successful!\n", testsSuccessful, totalTests);
	exit(EXIT_SUCCESS);
}
