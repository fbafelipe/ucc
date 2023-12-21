#include <stdio.h>

int main(int argc, char *argv[]) {
	int i;
	
	for (i = 0; i < 10; ++i) {
		printf("%d\n", i);
		
		if (i == 5) break;
	}
	
	printf("\n");
	
	for (i = 0; i < 10; ++i) {
		if (i % 2 == 0) continue;
		
		printf("%d\n", i);
	}
	
	return 0;
}
