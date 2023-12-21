#include <stdio.h>

int main() {
	int x, y;
	int z;
	int *p;
	
	x = 1;
	y = 3;
	z = 2;
	
	p = &x;
	
	if (y > *p) p = &y;
	if (z > *p) p = &z;
	
	printf("maior: %d\n", *p);
	
	*p = sizeof(int *);
	
	printf("y = %d\n", y);
	
	return 0;
}
