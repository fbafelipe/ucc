#include <math.h>
#include <stdio.h>

static void circunference(void);
static void pythagoras(void);

static double readDouble(const char *msg);

int main(int argc, char *argv[]) {
	char action;
	
	do {
		printf("Calculate circunference: 0\n");
		printf("Calculate pythagoras: 1\n");
		printf("Quit: 2\n");
		
		action = getchar();
		
		switch (action) {
			case '0':
				circunference();
				break;
			case '1':
				pythagoras();
				break;
			case '2':
				// ignore
				break;
			default:
				printf("Invalid option \'%c\'.\n", action);
		}
		
		printf("\n\n");
		
		/* remove the garbage from stdin */
		getchar();
		
	} while (action != '2');
	
	return 0;
}

static void circunference() {
	double r;
	double c;
	
	r = readDouble("Radius: ");
	c = 2.0 * M_PI * r;
	
	printf("Circunference: %lf\n", c);
}

static void pythagoras() {
	double a, b, h;
	
	a = readDouble("Side A: ");
	b = readDouble("Side B: ");
	
	h = sqrt(a*a + b*b);
	
	printf("Hypotenuse: %lf\n", h);
}

static double readDouble(const char *msg) {
	double v;
	
	printf(msg);
	fflush(stdout);
	
	scanf("%lf", &v);
	
	return v;
}
