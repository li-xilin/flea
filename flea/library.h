#ifndef LIBRARY_H
#define LIBRARY_H
#include <stdint.h>

typedef uintptr_t library;

library library_open(const char* fname);

void *library_symbol(library l, const char *func);

int library_close(library l);

char *library_error(void);

#endif
