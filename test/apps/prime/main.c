#include <assert.h>
#include <stdio.h>

#include "dump.h"
#include "search.h"

int main() {
	unsigned int start;
	unsigned int end;
	char c;
	char file[256];
	DumpFunction dumpFunction = dumpToStdio;
	
	printf("Start: ");
	fflush(stdout);
	scanf("%u", &start);
	
	printf("End: ");
	fflush(stdout);
	scanf("%u", &end);
	
	assert(start <= end);
	assert(start > 1);
	
	/* remove garbage from stdin */
	getchar();
	
	do {
		printf("Dump to a file? (y/n) ");
		fflush(stdout);
		c = getchar();
	} while (c != 'y' && c != 'n');
	
	if (c == 'y') {
		printf("File: ");
		fflush(stdout);
		scanf("%s", file);
		
		setDumpFile(file);
		
		dumpFunction = dumpToFile;
	}
	
	searchPrimes(start, end, dumpFunction);
	
	if (c == 'y') dumpEnd();
	
	return 0;
}
