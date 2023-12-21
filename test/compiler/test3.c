#include <stdio.h>

int main() {
	int i;
	int j;
	
	i = 1;
	
	while (i <= 5) {
		printf("while %d\n", i);
		i = i + 1;
	}
	
	j = i;
	do {
		printf("do while %d\n", j);
		j += 1;
	} while (j < 11);
	
	for (i = j; i <= 15; ++i) {
		printf("for %d\n", i);
	}
	
	return 0;
}
