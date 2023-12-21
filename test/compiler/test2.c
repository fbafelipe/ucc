#include <stdio.h>

int main() {
	int (*x)(const char *, ...);
	int y;
	
	x = printf;
	
	y = x("Hello World!\n");
	
	printf("%d\n", y);
	
	return 0;
}
