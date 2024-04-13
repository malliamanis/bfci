#ifndef BFCI_H
#define BFCI_H

#include <stdbool.h>

// all free src

// both return a heap allocated string
char *bfci_compile_c(char *src, bool dynamic);
char *bfci_compile_asm(char *src, bool dynamic);

void bfci_interpret(char *src, bool dynamic);

#endif // BFCI_H
