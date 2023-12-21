#ifndef ASSERT_H
#define ASSERT_H

#ifdef	NDEBUG

#define assert(exp)

#else

extern void __assert_fail(const char *file, unsigned int line);

#define assert(exp) do { if (!(exp)) __assert_fail(__FILE__, __LINE__); } while(0)

#endif

#endif
