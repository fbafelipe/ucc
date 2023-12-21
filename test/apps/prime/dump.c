#include "dump.h"

#include <stdio.h>

static FILE *outputFile;

void setDumpFile(const char *f) {
	outputFile = fopen(f, "w");
}

void dumpEnd() {
	fclose(outputFile);
}

void dumpToFile(unsigned int prime) {
	fprintf(outputFile, "%u\n", prime);
}

void dumpToStdio(unsigned int prime) {
	printf("%u\n", prime);
}
