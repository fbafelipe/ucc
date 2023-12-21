#ifndef SEARCH_H
#define SEARCH_H

typedef void (*DumpFunction)(unsigned int);

void searchPrimes(unsigned int start, unsigned int end, DumpFunction dump);

#endif
