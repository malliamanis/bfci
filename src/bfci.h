#ifndef BFCI_H
#define BFCI_H

#include <stdbool.h>

enum bfci_option {
	BFCI_INTERPRET,
	BFCI_COMPILE_C,
	BFCI_COMPILE_ASM
};

// frees src and returns a heap allocated string
char *bfci_ci(char *src, enum bfci_option option, bool optimise, bool dynamic);

#endif
