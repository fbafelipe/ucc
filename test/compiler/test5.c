#include <stdio.h>

int main() {
	int x = 32;
	int a[32];
	int i;
	
	for (i = 0; i < x; ++i) {
		a[i] = i + 1;
	}
	
	for (i = 0; i < x; ++i) printf("%d\n", a[i]);
	
	return 0;
}
