static void foo(void);

int main(int argc, char *argv[]) {
	int i;
	void (*bar)() = foo;
	
	while (1) for (;;) for (i = 0; i < 100; ++i) if (bar) bar();
	
	return 0;
}
