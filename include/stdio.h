#ifndef STDIO_H
#define STDIO_H

typedef int FILE;

extern int printf(const char *format, ...);
extern int scanf(const char *format, ...);

extern int fprintf(FILE *fp, const char *format, ...);
extern int fscanf(FILE *fp, const char *format, ...);

extern int fgetc (FILE *fp);
extern int getc (FILE *fp);

extern int getchar(void);

extern FILE *fopen(const char *path, const char *mode);
extern int fclose(FILE *fp);
extern int fflush(FILE *fp);

// stdin, stdout and stderr must be accessed by a function call
extern FILE *__get_stdin(void);
extern FILE *__get_stdout(void);
extern FILE *__get_stderr(void);

#define stdin __get_stdin()
#define stdout __get_stdout()
#define stderr __get_stderr()

#endif
