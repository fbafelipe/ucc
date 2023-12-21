#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

static void foo();

int main(int argc, char *argv[]) {
	char *file;
	unsigned int line;
	const char *date;
	const char *t;
	
	file = __FILE__;
	line = __LINE__;
	date = __DATE__;
	t = __TIME__;
	
	return 0;
}

// test complex defines
#define INT_X_EQ int x =

#define MUL_FOUR * 4
#define THREE_MUL_FOUR_PLUS 3 MUL_FOUR +

#define DIV_POPEN / (
#define TWO 2
#define THREE_PCLOSE 3)

#define __FIVE__ 5
#define __FIVE_ __FIVE__
#define __FIVE __FIVE_
#define _FIVE __FIVE
#define FIVE _FIVE

#define FIVE_DIV_POPEN_TWO_AND_TRHEE_PCLOSE FIVE DIV_POPEN TWO & THREE_PCLOSE

#define EXP INT_X_EQ THREE_MUL_FOUR_PLUS FIVE_DIV_POPEN_TWO_AND_TRHEE_PCLOSE

static void foo() {
	int x = 3 * 4 + 5 / (2 & 3);
	EXP;
}
