#ifndef BFCI_H
#define BFCI_H

#include <stdbool.h>

// both return a heap allocated string
char *bfci_compile_c(const char *src, bool dynamic);
char *bfci_compile_asm(const char *src, bool dynamic);

void bfci_interpret(const char *src, bool dynamic);

#endif // BFCI_H
