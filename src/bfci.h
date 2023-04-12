#ifndef BFCI_H
#define BFCI_H

// both return a heap allocated string
char *bfci_compile_c(const char *src);
char *bfci_compile_asm(const char *src);

void bfci_interpret(const char *src);

#endif // BFCI_H
