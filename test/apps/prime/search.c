#include "search.h"

#include <math.h>

#include <stdio.h>

static int isPrime(unsigned int num);

void searchPrimes(unsigned int start, unsigned int end, DumpFunction dump) {
	unsigned int i;
	
	if (start == 2) dump(start);
	if (start % 2 == 0) ++start;
	
	for (i = start; i <= end; i += 2) {
		if (isPrime(i)) dump(i);
	}
}

static int isPrime(unsigned int num) {
	unsigned int end = (unsigned int)sqrt(num);
	unsigned int i;
	
	if (num % 2 == 0) return 1;
	
	for (i = 3; i <= end; i += 2) {
		if (num % i == 0) return 0;
	}
	
	return 1;
}
