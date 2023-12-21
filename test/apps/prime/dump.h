#ifndef DUMP_H
#define DUMP_H

void setDumpFile(const char *f);
void dumpEnd(void);

void dumpToFile(unsigned int prime);
void dumpToStdio(unsigned int prime);

#endif
