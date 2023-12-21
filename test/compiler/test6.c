#include <stdio.h>

static void foo(int x, int y, int z);
static void bar(int x, int y, int z);

static void doCall(void (*f)(int, int, int), int x, int y, int z);

int main() {
	doCall(foo, 1, 2, 3);
	doCall(bar, 4, 5, 6);
	
	return 0;
}

static void foo(int x, int y, int z) {
	printf("foo %d %d %d\n", x, y, z);
}

static void bar(int x, int y, int z) {
	printf("bar %d %d %d\n", x, y, z);
}

static void doCall(void (*f)(int, int, int), int x, int y, int z) {
	f(x, y, z);
}
