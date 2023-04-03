#ifndef BFCI_H
#define BFCI_H

// both free src

char *bfci_compile(const char *src); // returns a heap allocated string
void bfci_interpret(const char *src);

#endif // BFCI_H
