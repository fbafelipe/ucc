#include <stdio.h>

int main() {
	char nome[80];
	
	printf("Digite seu nome:\n");
	scanf("%s", nome);
	
	printf("Ola %s\n", nome);
	
	return 0;
}
