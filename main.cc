#include <stdio.h>
#include <unistd.h>
//#include <zlib.h>
//#include <cassert>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>


#include "./compression.class.h"

int main(void){
	int totalTests=2;
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

	printf("\n\n%d tests out of %d were successful!\n", testsSuccessful, totalTests);
	exit(EXIT_SUCCESS);
}
